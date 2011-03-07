#include <dl/dl.h>

#include "dl_types.h"
#include "dl_swap.h"
#include "dl_binary_writer.h"

#include "container/dl_array.h"

#include <stdio.h> // printf
#include <stdlib.h> // malloc, free

// TODO: bug! dl_internal_default_alloc do not follow alignment.
static void* dl_internal_default_alloc( unsigned int size, unsigned int alignment, void* alloc_ctx ) { DL_UNUSED(alignment); DL_UNUSED(alloc_ctx); return malloc(size); }
static void  dl_internal_default_free ( void* ptr, void* alloc_ctx ) {  DL_UNUSED(alloc_ctx); free(ptr); }

static void* dl_internal_realloc( dl_ctx_t dl_ctx, void* ptr, unsigned int old_size, unsigned int new_size, unsigned int alignment )
{
	if( old_size == new_size )
		return ptr;

	void* new_ptr = dl_ctx->alloc_func( new_size, alignment, dl_ctx->alloc_ctx );

	if( ptr != 0x0 )
	{
		memmove( new_ptr, ptr, old_size );
		dl_ctx->free_func( ptr, dl_ctx->alloc_ctx );
	}

	return new_ptr;
}

static void* dl_internal_append_data( dl_ctx_t dl_ctx, void* old_data, unsigned int old_data_size, const void* new_data, unsigned int new_data_size )
{
	uint8* data = (uint8*)dl_internal_realloc( dl_ctx, old_data, old_data_size, old_data_size + new_data_size, sizeof(void*) );
	memcpy( data + old_data_size, new_data, new_data_size );
	return data;
}

dl_error_t dl_context_create( dl_ctx_t* dl_ctx, dl_create_params_t* create_params )
{
	dl_alloc_func the_alloc = create_params->alloc_func != 0x0 ? create_params->alloc_func : dl_internal_default_alloc;
	dl_free_func  the_free  = create_params->free_func  != 0x0 ? create_params->free_func  : dl_internal_default_free;
	dl_context*   ctx       = (dl_context*)the_alloc( sizeof(dl_context), sizeof(void*), create_params->alloc_ctx );

	if(ctx == 0x0)
		return DL_ERROR_OUT_OF_LIBRARY_MEMORY;

	memset(ctx, 0x0, sizeof(dl_context));

	ctx->alloc_func     = the_alloc;
	ctx->free_func      = the_free;
	ctx->alloc_ctx      = create_params->alloc_ctx;
	ctx->error_msg_func = create_params->error_msg_func;
	ctx->error_msg_ctx  = create_params->error_msg_ctx;

	*dl_ctx = ctx;

	return DL_ERROR_OK;
}

dl_error_t dl_context_destroy(dl_ctx_t dl_ctx)
{
	dl_ctx->free_func( dl_ctx->type_info_data, dl_ctx->alloc_ctx );
	dl_ctx->free_func( dl_ctx->enum_info_data, dl_ctx->alloc_ctx );
	dl_ctx->free_func( dl_ctx->default_data, dl_ctx->alloc_ctx );
	dl_ctx->free_func( dl_ctx, dl_ctx->alloc_ctx );
	return DL_ERROR_OK;
}

// implemented in dl_convert.cpp
dl_error_t dl_internal_convert_no_header( dl_ctx_t       dl_ctx,
                                          unsigned char* packed_instance, unsigned char* packed_instance_base,
                                          unsigned char* out_instance,    unsigned int   out_instance_size,
                                          unsigned int*  needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const SDLType* root_type,       unsigned int   base_offset );

struct SPatchedInstances
{
	CArrayStatic<const uint8*, 1024> m_lpPatched;

	bool IsPatched(const uint8* _pInstance)
	{
		for (unsigned int iPatch = 0; iPatch < m_lpPatched.Len(); ++iPatch)
			if(m_lpPatched[iPatch] == _pInstance)
				return true;
		return false;
	}
};

static void dl_internal_patch_ptr( const uint8* ptr, const uint8* base_data )
{
	union conv_union { pint offset; const uint8* real_data; };
	conv_union* read_me = (conv_union*)ptr;

	if (read_me->offset == DL_NULL_PTR_OFFSET[DL_PTR_SIZE_HOST])
		read_me->real_data = 0x0;
	else
		read_me->real_data = base_data + read_me->offset;
};

static dl_error_t dl_internal_patch_loaded_ptrs( dl_ctx_t           dl_ctx,
												 SPatchedInstances* patched_instances,
												 const uint8*       instance,
												 const SDLType*     type,
												 const uint8*       base_data,
												 bool               is_member_struct )
{
	// TODO: Optimize this please, linear search might not be the best if many subinstances is in file!
	if( !is_member_struct )
	{
		if(patched_instances->IsPatched(instance))
			return DL_ERROR_OK;

		patched_instances->m_lpPatched.Add(instance);
	}

	for(uint32 iMember = 0; iMember < type->member_count; ++iMember)
	{
		const SDLMember& Member = type->members[iMember];
		const uint8* pMemberData = instance + Member.offset[DL_PTR_SIZE_HOST];

		dl_type_t AtomType    = Member.AtomType();
		dl_type_t StorageType = Member.StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STR:    dl_internal_patch_ptr(pMemberData, base_data); break;
					case DL_TYPE_STORAGE_STRUCT: dl_internal_patch_loaded_ptrs(dl_ctx, patched_instances, pMemberData, DLFindType( dl_ctx, Member.type_id), base_data, true ); break;
					case DL_TYPE_STORAGE_PTR:
					{
						uint8** ppPtr = (uint8**)pMemberData;
						dl_internal_patch_ptr(pMemberData, base_data);

						if(*ppPtr != 0x0)
						{
							const SDLType* pSubType = DLFindType(dl_ctx, Member.type_id);
							dl_internal_patch_loaded_ptrs( dl_ctx, patched_instances, *ppPtr, pSubType, base_data, false );
						}
					}
					break;
					default:
						// ignore
						break;
				}
			}
			break;
			case DL_TYPE_ATOM_ARRAY:
			{
				if(StorageType == DL_TYPE_STORAGE_STR || StorageType == DL_TYPE_STORAGE_STRUCT)
				{
					dl_internal_patch_ptr(pMemberData, base_data);
					const uint8* pArrayData = *(const uint8**)pMemberData;

					uint32 Count = *(uint32*)(pMemberData + sizeof(void*));

					if(Count > 0)
					{
						if(StorageType == DL_TYPE_STORAGE_STRUCT)
						{
							// patch sub-ptrs!
							const SDLType* pSubType = DLFindType(dl_ctx, Member.type_id);
							uint32 Size = dl_internal_align_up(pSubType->size[DL_PTR_SIZE_HOST], pSubType->alignment[DL_PTR_SIZE_HOST]);

							for(uint32 iElemOffset = 0; iElemOffset < Count * Size; iElemOffset += Size)
								dl_internal_patch_loaded_ptrs( dl_ctx, patched_instances, pArrayData + iElemOffset, pSubType, base_data, true );
						}
						else
						{
							for(uint32 iElemOffset = 0; iElemOffset < Count * sizeof(char*); iElemOffset += sizeof(char*))
								dl_internal_patch_ptr(pArrayData + iElemOffset, base_data);
						}
					}
				}
				else // pod
					dl_internal_patch_ptr(pMemberData, base_data);
			}
			break;

			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				if(StorageType == DL_TYPE_STORAGE_STR)
				{
					for(pint iElemOffset = 0; iElemOffset < Member.size[DL_PTR_SIZE_HOST]; iElemOffset += sizeof(char*))
						dl_internal_patch_ptr(pMemberData + iElemOffset, base_data);
				}
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
			// ignore
			break;

		default:
			DL_ASSERT(false && "Unknown atom type");
		}
	}
	return DL_ERROR_OK;
}

struct SOneMemberType
{
	SOneMemberType(const SDLMember* _pMember)
	{
		m_Size[0] = _pMember->size[0];
		m_Size[1] = _pMember->size[1];
		m_Alignment[0] = _pMember->alignment[0];
		m_Alignment[1] = _pMember->alignment[1];
		m_nMembers = 1;

		memcpy(&m_Member, _pMember, sizeof(SDLMember));
		m_Member.offset[0] = 0;
		m_Member.offset[0] = 0;
	}

	char      m_Name[DL_TYPE_NAME_MAX_LEN];
	uint32    m_Size[2];
	uint32    m_Alignment[2];
	uint32    m_nMembers;
	SDLMember m_Member;
};

DL_STATIC_ASSERT(sizeof(SOneMemberType) - sizeof(SDLMember) == sizeof(SDLType), these_need_same_size);

static dl_error_t dl_internal_load_type_library_defaults(dl_ctx_t dl_ctx, unsigned int first_new_type, const uint8* default_data, unsigned int default_data_size)
{
	if( default_data_size == 0 ) return DL_ERROR_OK;

	if( dl_ctx->default_data != 0x0 )
		return DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS;

	dl_ctx->default_data = (uint8*)dl_ctx->alloc_func( default_data_size * 2, sizeof(void*), dl_ctx->alloc_ctx ); // TODO: times 2 here need to be fixed!

	uint8* pDst = dl_ctx->default_data;

	// ptr-patch and convert to native
	for(uint32 iType = first_new_type; iType < dl_ctx->type_count; ++iType)
	{
		const SDLType* pType = (SDLType*)(dl_ctx->type_info_data + dl_ctx->type_lookup[iType].offset);
		for(uint32 iMember = 0; iMember < pType->member_count; ++iMember)
		{
			SDLMember* pMember = (SDLMember*)pType->members + iMember;
			if(pMember->default_value_offset == DL_UINT32_MAX)
				continue;

			pDst                          = dl_internal_align_up( pDst, pMember->alignment[DL_PTR_SIZE_HOST] );
			uint8* pSrc                   = (uint8*)default_data + pMember->default_value_offset;
			pint BaseOffset               = pint( pDst ) - pint( dl_ctx->default_data );
			pMember->default_value_offset = uint32( BaseOffset );

			SOneMemberType Dummy(pMember);
			unsigned int NeededSize;
			dl_internal_convert_no_header( dl_ctx,
										   pSrc, (unsigned char*)default_data,
										   pDst, 1337, // need to check this size ;) Should be the remainder of space in m_pDefaultInstances.
										   &NeededSize,
										   DL_ENDIAN_LITTLE,  DL_ENDIAN_HOST,
										   DL_PTR_SIZE_32BIT, DL_PTR_SIZE_HOST,
										   (const SDLType*)&Dummy, (unsigned int)BaseOffset ); // TODO: Ugly cast, remove plox!

			SPatchedInstances PI;
			dl_internal_patch_loaded_ptrs( dl_ctx, &PI, pDst, (const SDLType*)&Dummy, dl_ctx->default_data, false );

			pDst += NeededSize;
		}
	}

	return DL_ERROR_OK;
}

static void dl_internal_read_typelibrary_header( SDLTypeLibraryHeader* header, const uint8* data )
{
	memcpy(header, data, sizeof(SDLTypeLibraryHeader));

	if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
	{
		header->id           = DLSwapEndian(header->id);
		header->version      = DLSwapEndian(header->version);

		header->type_count   = DLSwapEndian(header->type_count);
		header->types_offset = DLSwapEndian(header->types_offset);
		header->types_size   = DLSwapEndian(header->types_size);

		header->enum_count   = DLSwapEndian(header->enum_count);
		header->enums_offset = DLSwapEndian(header->enums_offset);
		header->enums_size   = DLSwapEndian(header->enums_size);

		header->default_value_offset = DLSwapEndian(header->default_value_offset);
		header->default_value_size   = DLSwapEndian(header->default_value_size);
	}
}

dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, unsigned int lib_data_size )
{
	if(lib_data_size < sizeof(SDLTypeLibraryHeader))
		return DL_ERROR_MALFORMED_DATA;

	SDLTypeLibraryHeader Header;
	dl_internal_read_typelibrary_header(&Header, lib_data);

	if( Header.id      != DL_TYPELIB_ID )      return DL_ERROR_MALFORMED_DATA;
	if( Header.version != DL_TYPELIB_VERSION ) return DL_ERROR_VERSION_MISMATCH;

	// store type-info data.
	dl_ctx->type_info_data = (uint8*)dl_internal_append_data( dl_ctx, dl_ctx->type_info_data, dl_ctx->type_info_data_size, lib_data + Header.types_offset, Header.types_size );

	// read type-lookup table
	dl_type_lookup_t* _pFromData = (dl_type_lookup_t*)(lib_data + sizeof(SDLTypeLibraryHeader));
	for(uint32 i = dl_ctx->type_count; i < dl_ctx->type_count + Header.type_count; ++i)
	{
		dl_type_lookup_t* look = dl_ctx->type_lookup + i;

		if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
		{
			look->type_id  = DLSwapEndian(_pFromData->type_id);
			look->offset   = dl_ctx->type_info_data_size + DLSwapEndian(_pFromData->offset);
			SDLType* pType = (SDLType*)(dl_ctx->type_info_data + look->offset );

			pType->size[DL_PTR_SIZE_32BIT]      = DLSwapEndian(pType->size[DL_PTR_SIZE_32BIT]);
			pType->size[DL_PTR_SIZE_64BIT]      = DLSwapEndian(pType->size[DL_PTR_SIZE_64BIT]);
			pType->alignment[DL_PTR_SIZE_32BIT] = DLSwapEndian(pType->alignment[DL_PTR_SIZE_32BIT]);
			pType->alignment[DL_PTR_SIZE_64BIT] = DLSwapEndian(pType->alignment[DL_PTR_SIZE_64BIT]);
			pType->member_count                 = DLSwapEndian(pType->member_count);

			for(uint32 i = 0; i < pType->member_count; ++i)
			{
				SDLMember* pMember = pType->members + i;

				pMember->type                         = DLSwapEndian(pMember->type);
				pMember->type_id	                  = DLSwapEndian(pMember->type_id);
				pMember->size[DL_PTR_SIZE_32BIT]      = DLSwapEndian(pMember->size[DL_PTR_SIZE_32BIT]);
				pMember->size[DL_PTR_SIZE_64BIT]      = DLSwapEndian(pMember->size[DL_PTR_SIZE_64BIT]);
				pMember->offset[DL_PTR_SIZE_32BIT]    = DLSwapEndian(pMember->offset[DL_PTR_SIZE_32BIT]);
				pMember->offset[DL_PTR_SIZE_64BIT]    = DLSwapEndian(pMember->offset[DL_PTR_SIZE_64BIT]);
				pMember->alignment[DL_PTR_SIZE_32BIT] = DLSwapEndian(pMember->alignment[DL_PTR_SIZE_32BIT]);
				pMember->alignment[DL_PTR_SIZE_64BIT] = DLSwapEndian(pMember->alignment[DL_PTR_SIZE_64BIT]);
				pMember->default_value_offset         = DLSwapEndian(pMember->default_value_offset);
			}
		}
		else
		{
			look->type_id = _pFromData->type_id;
			look->offset  = dl_ctx->type_info_data_size + _pFromData->offset;
		}

		++_pFromData;
	}

	dl_ctx->type_count          += Header.type_count;
	dl_ctx->type_info_data_size += Header.types_size;

	// store enum-info data.
	dl_ctx->enum_info_data = (uint8*)dl_internal_append_data( dl_ctx, dl_ctx->enum_info_data, dl_ctx->enum_info_data_size, lib_data + Header.enums_offset, Header.enums_size );

	// read enum-lookup table
	dl_type_lookup_t* _pEnumFromData = (dl_type_lookup_t*)(lib_data + Header.types_offset + Header.types_size);
	for(uint32 i = dl_ctx->enum_count; i < dl_ctx->enum_count + Header.enum_count; ++i)
	{
		dl_type_lookup_t* look = dl_ctx->enum_lookup + i;

		if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
		{
			look->type_id  = DLSwapEndian(_pEnumFromData->type_id);
			look->offset   = dl_ctx->enum_info_data_size + DLSwapEndian(_pEnumFromData->offset);
			SDLEnum* e = (SDLEnum*)(dl_ctx->enum_info_data + look->offset);

			e->type_id     = DLSwapEndian(e->type_id);
			e->value_count = DLSwapEndian(e->value_count);
			for(uint32 i = 0; i < e->value_count; ++i)
				e->values[i].value = DLSwapEndian(e->values[i].value);
		}
		else
		{
			look->type_id = _pEnumFromData->type_id;
			look->offset  = dl_ctx->enum_info_data_size + _pEnumFromData->offset;
		}

		++_pEnumFromData;
	}

	dl_ctx->enum_count          += Header.enum_count;
	dl_ctx->enum_info_data_size += Header.enums_size;

	return dl_internal_load_type_library_defaults( dl_ctx, dl_ctx->type_count - Header.type_count, lib_data + Header.default_value_offset, Header.default_value_size );
}

dl_error_t dl_instance_load( dl_ctx_t             dl_ctx,          dl_typeid_t  type,
                             void*                instance,        unsigned int instance_size,
                             const unsigned char* packed_instance, unsigned int packed_instance_size,
                             unsigned int*        consumed )
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )          return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                 return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )       return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type )           return DL_ERROR_TYPE_MISMATCH;
	if( header->instance_size > instance_size )        return DL_ERROR_BUFFER_TO_SMALL;

	const SDLType* pType = DLFindType(dl_ctx, header->root_instance_type);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	// TODO: Temporary disabled due to CL doing some magic stuff!!! 
	// Structs allocated on qstack seems to be unaligned!!!
	// if( !dl_internal_is_align( instance, pType->m_Alignment[DL_PTR_SIZE_HOST] ) )
	//	return DL_ERROR_BAD_ALIGNMENT;

	memcpy(instance, packed_instance + sizeof(SDLDataHeader), header->instance_size);

	SPatchedInstances PI;
	dl_internal_patch_loaded_ptrs( dl_ctx, &PI, (uint8*)instance, pType, (uint8*)instance, false );

	if( consumed )
		*consumed = header->instance_size + sizeof(SDLDataHeader);

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_instance_load_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t   type,
												   unsigned char* packed_instance, unsigned int  packed_instance_size,
												   void**         loaded_instance, unsigned int* consumed)
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )          return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                 return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )       return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type )           return DL_ERROR_TYPE_MISMATCH;

	const SDLType* pType = DLFindType(dl_ctx, header->root_instance_type);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	uint8* intstance_ptr = packed_instance + sizeof(SDLDataHeader);

	SPatchedInstances PI;
	dl_internal_patch_loaded_ptrs( dl_ctx, &PI, intstance_ptr, pType, intstance_ptr, false );

	*loaded_instance = intstance_ptr;

	if( consumed )
		*consumed = header->instance_size + sizeof(SDLDataHeader);

	return DL_ERROR_OK;
}

struct CDLBinStoreContext
{
	CDLBinStoreContext(uint8* _OutData, pint _OutDataSize, bool _Dummy)
		: writer( _OutData, _OutDataSize, _Dummy, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST ) {}

	pint FindWrittenPtr(void* _Ptr)
	{
		for (unsigned int iPtr = 0; iPtr < m_WrittenPtrs.Len(); ++iPtr)
			if(m_WrittenPtrs[iPtr].ptr == _Ptr)
				return m_WrittenPtrs[iPtr].pos;

		return pint(-1);
	}

	void AddWrittenPtr(void* _Ptr, pint _Pos) { m_WrittenPtrs.Add(SWrittenPtr(_Pos, _Ptr)); }

	CDLBinaryWriter writer;

	struct SWrittenPtr
	{
 		SWrittenPtr() {}
 		SWrittenPtr(pint _Pos, void* _pPtr) : pos(_Pos), ptr(_pPtr) {}
		pint  pos;
		void* ptr;
	};

	CArrayStatic<SWrittenPtr, 128> m_WrittenPtrs;
};

static void DLInternalStoreString(const uint8* _pInstance, CDLBinStoreContext* _pStoreContext)
{
	char* pTheString = *(char**)_pInstance;
	pint Pos = _pStoreContext->writer.Tell();
	_pStoreContext->writer.SeekEnd();
	pint Offset = _pStoreContext->writer.Tell();
	_pStoreContext->writer.Write(pTheString, strlen(pTheString) + 1);
	_pStoreContext->writer.SeekSet(Pos);
	_pStoreContext->writer.Write(&Offset, sizeof(pint));
}

static dl_error_t dl_internal_instance_store(dl_ctx_t dl_ctx, const SDLType* dl_type, uint8* instance, CDLBinStoreContext* store_ctx);

static dl_error_t DLInternalStoreMember(dl_ctx_t _Context, const SDLMember* _pMember, uint8* _pInstance, CDLBinStoreContext* _pStoreContext)
{
	_pStoreContext->writer.Align(_pMember->alignment[DL_PTR_SIZE_HOST]);

	dl_type_t AtomType    = dl_type_t(_pMember->type & DL_TYPE_ATOM_MASK);
	dl_type_t StorageType = dl_type_t(_pMember->type & DL_TYPE_STORAGE_MASK);

	switch (AtomType)
	{
		case DL_TYPE_ATOM_POD:
		{
			switch(StorageType)
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const SDLType* pSubType = DLFindType(_Context, _pMember->type_id);
					if(pSubType == 0x0)
					{
						dl_log_error( _Context, "Could not find subtype for member %s", _pMember->name );
						return DL_ERROR_TYPE_NOT_FOUND;
					}
					dl_internal_instance_store(_Context, pSubType, _pInstance, _pStoreContext );
				}
				break;
				case DL_TYPE_STORAGE_STR:
					DLInternalStoreString(_pInstance, _pStoreContext);
					break;
				case DL_TYPE_STORAGE_PTR:
				{
					uint8* pData = *(uint8**)_pInstance;
					pint Offset = _pStoreContext->FindWrittenPtr(pData);

					if(pData == 0x0) // Null-pointer, store pint(-1) to signal to patching!
					{
						DL_ASSERT(Offset == pint(-1) && "This pointer should not have been found among the written ptrs!");
						// keep the -1 in Offset and store it to ptr.
					}
					else if(Offset == pint(-1)) // has not been written yet!
					{
						pint Pos = _pStoreContext->writer.Tell();
						_pStoreContext->writer.SeekEnd();

						const SDLType* pSubType = DLFindType(_Context, _pMember->type_id);
						pint Size = dl_internal_align_up(pSubType->size[DL_PTR_SIZE_HOST], pSubType->alignment[DL_PTR_SIZE_HOST]);
						_pStoreContext->writer.Align(pSubType->alignment[DL_PTR_SIZE_HOST]);

						Offset = _pStoreContext->writer.Tell();

						// write data!
						_pStoreContext->writer.Reserve(Size); // reserve space for ptr so subdata is placed correctly

						_pStoreContext->AddWrittenPtr(pData, Offset);

						dl_internal_instance_store(_Context, pSubType, pData, _pStoreContext);

						_pStoreContext->writer.SeekSet(Pos);
					}

					_pStoreContext->writer.Write(&Offset, sizeof(pint));
				}
				break;
				default: // default is a standard pod-type
					DL_ASSERT(_pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
					_pStoreContext->writer.Write(_pInstance, _pMember->size[DL_PTR_SIZE_HOST]);
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch(StorageType)
			{
				case DL_TYPE_STORAGE_STRUCT:
					_pStoreContext->writer.Write(_pInstance, _pMember->size[DL_PTR_SIZE_HOST]); // TODO: I Guess that this is a bug! Will it fix ptrs well?
					break;
				case DL_TYPE_STORAGE_STR:
				{
					uint32 Count = _pMember->size[DL_PTR_SIZE_HOST] / sizeof(char*);

					for(uint32 iElem = 0; iElem < Count; ++iElem)
						DLInternalStoreString(_pInstance + (iElem * sizeof(char*)), _pStoreContext);
				}
				break;
				default: // default is a standard pod-type
					DL_ASSERT(_pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
					_pStoreContext->writer.Write(_pInstance, _pMember->size[DL_PTR_SIZE_HOST]);
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_ARRAY:
		{
			pint Size = 0;
			const SDLType* pSubType = 0x0;

			uint8* pDataPtr = _pInstance;
			uint32 Count = *(uint32*)(pDataPtr + sizeof(void*));

			pint Offset = 0;

			if( Count == 0 )
				Offset = DL_NULL_PTR_OFFSET[ DL_PTR_SIZE_HOST ];
			else
			{
				pint Pos = _pStoreContext->writer.Tell();
				_pStoreContext->writer.SeekEnd();

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
						pSubType = DLFindType(_Context, _pMember->type_id);
						Size = dl_internal_align_up(pSubType->size[DL_PTR_SIZE_HOST], pSubType->alignment[DL_PTR_SIZE_HOST]);
						_pStoreContext->writer.Align(pSubType->alignment[DL_PTR_SIZE_HOST]);
						break;
					case DL_TYPE_STORAGE_STR:
						Size = sizeof(void*);
						_pStoreContext->writer.Align(Size);
						break;
					default:
						Size = DLPodSize(_pMember->type);
						_pStoreContext->writer.Align(Size);
				}

				Offset = _pStoreContext->writer.Tell();

				// write data!
				_pStoreContext->writer.Reserve(Count * Size); // reserve space for array so subdata is placed correctly

				uint8* pData = *(uint8**)pDataPtr;

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
						for (unsigned int iElem = 0; iElem < Count; ++iElem)
							dl_internal_instance_store(_Context, pSubType, pData + (iElem * Size), _pStoreContext);
						break;
					case DL_TYPE_STORAGE_STR:
						for (unsigned int iElem = 0; iElem < Count; ++iElem)
							DLInternalStoreString(pData + (iElem * Size), _pStoreContext);
						break;
					default:
						for (unsigned int iElem = 0; iElem < Count; ++iElem)
							_pStoreContext->writer.Write(pData + (iElem * Size), Size); break;
				}

				_pStoreContext->writer.SeekSet(Pos);
			}

			// make room for ptr
			_pStoreContext->writer.Write(&Offset, sizeof(pint));

			// write count
			_pStoreContext->writer.Write(&Count, sizeof(uint32));
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_BITFIELD:
			_pStoreContext->writer.Write(_pInstance, _pMember->size[DL_PTR_SIZE_HOST]);
		break;

		default:
			DL_ASSERT(false && "Invalid ATOM-type!");
	}

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_instance_store(dl_ctx_t dl_ctx, const SDLType* dl_type, uint8* instance, CDLBinStoreContext* store_ctx)
{
	bool bLastWasBF = false;

	for(uint32 member = 0; member < dl_type->member_count; ++member)
	{
		const SDLMember& Member = dl_type->members[member];

		if(!bLastWasBF || Member.AtomType() != DL_TYPE_ATOM_BITFIELD)
		{
			dl_error_t Err = DLInternalStoreMember(dl_ctx, &Member, instance + Member.offset[DL_PTR_SIZE_HOST], store_ctx);
			if(Err != DL_ERROR_OK)
				return Err;
		}

		bLastWasBF = Member.AtomType() == DL_TYPE_ATOM_BITFIELD;
	}

	return DL_ERROR_OK;
}

dl_error_t dl_instance_store( dl_ctx_t       dl_ctx,     dl_typeid_t  type,            void*         instance,
							  unsigned char* out_buffer, unsigned int out_buffer_size, unsigned int* produced_bytes )
{
	if( out_buffer_size > 0 && out_buffer_size <= sizeof(SDLDataHeader) )
		return DL_ERROR_BUFFER_TO_SMALL;

	const SDLType* pType = DLFindType(dl_ctx, type);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	// write header
	SDLDataHeader Header;
	Header.id = DL_INSTANCE_ID;
	Header.version = DL_INSTANCE_VERSION;
	Header.root_instance_type = type;
	Header.instance_size = 0;
	Header.is_64_bit_ptr = sizeof(void*) == 8 ? 1 : 0;

	unsigned char* store_ctx_buffer      = 0x0;
	unsigned int   store_ctx_buffer_size = 0;
	bool           store_ctx_is_dummy    = out_buffer_size == 0;

	if( out_buffer_size > 0 )
	{
		memcpy(out_buffer, &Header, sizeof(SDLDataHeader));
		store_ctx_buffer      = out_buffer + sizeof(SDLDataHeader);
		store_ctx_buffer_size = out_buffer_size - sizeof(SDLDataHeader);
	}

	CDLBinStoreContext StoreContext( store_ctx_buffer, store_ctx_buffer_size, store_ctx_is_dummy );

	StoreContext.writer.Reserve(pType->size[DL_PTR_SIZE_HOST]);
	StoreContext.AddWrittenPtr(instance, 0); // if pointer refere to root-node, it can be found at offset 0

	dl_error_t err = dl_internal_instance_store(dl_ctx, pType, (uint8*)instance, &StoreContext);

	// write instance size!
	SDLDataHeader* pHeader = (SDLDataHeader*)out_buffer;
	StoreContext.writer.SeekEnd();
	if( out_buffer )
		pHeader->instance_size = (uint32)StoreContext.writer.Tell();

	if( produced_bytes )
		*produced_bytes = (uint32)StoreContext.writer.Tell() + sizeof(SDLDataHeader);

	if( out_buffer_size > 0 && pHeader->instance_size > out_buffer_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	return err;
}

dl_error_t dl_instance_calc_size(dl_ctx_t dl_ctx, dl_typeid_t type, void* instance, unsigned int* out_size )
{
	return dl_instance_store( dl_ctx, type, instance, 0x0, 0, out_size );
}

const char* dl_error_to_string(dl_error_t _Err)
{
#define DL_ERR_TO_STR(ERR) case ERR: return #ERR
	switch(_Err)
	{
		DL_ERR_TO_STR(DL_ERROR_OK);
		DL_ERR_TO_STR(DL_ERROR_MALFORMED_DATA);
		DL_ERR_TO_STR(DL_ERROR_VERSION_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_LIBRARY_MEMORY);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_INSTANCE_MEMORY);
		DL_ERR_TO_STR(DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS);
		DL_ERR_TO_STR(DL_ERROR_TYPE_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_TYPE_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_MEMBER_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_BUFFER_TO_SMALL);
		DL_ERR_TO_STR(DL_ERROR_ENDIAN_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_BAD_ALIGNMENT);
		DL_ERR_TO_STR(DL_ERROR_INVALID_PARAMETER);
		DL_ERR_TO_STR(DL_ERROR_UNSUPPORTED_OPERATION);

		DL_ERR_TO_STR(DL_ERROR_TXT_PARSE_ERROR);
		DL_ERR_TO_STR(DL_ERROR_TXT_MEMBER_MISSING);
		DL_ERR_TO_STR(DL_ERROR_TXT_MEMBER_SET_TWICE);

		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_TYPE_MISMATCH);

		DL_ERR_TO_STR(DL_ERROR_INTERNAL_ERROR);
		default: return "Unknown error!";
	}
#undef DL_ERR_TO_STR
}

dl_error_t dl_instance_get_info( const unsigned char* packed_instance, unsigned int packed_instance_size, dl_instance_info_t* out_info )
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) && header->id != DL_INSTANCE_ID_SWAPED && header->id != DL_INSTANCE_ID )
		return DL_ERROR_MALFORMED_DATA;
	if(header->version != DL_INSTANCE_VERSION && header->version != DL_INSTANCE_VERSION_SWAPED)
		return DL_ERROR_VERSION_MISMATCH;

	out_info->ptrsize   = header->is_64_bit_ptr ? 8 : 4;
	if( header->id == DL_INSTANCE_ID )
	{
		out_info->load_size = header->instance_size;
		out_info->endian    = DL_ENDIAN_HOST;
		out_info->root_type = header->root_instance_type ;
	}
	else
	{
		out_info->load_size = DLSwapEndian( header->instance_size );
		out_info->endian    = DLOtherEndian( DL_ENDIAN_HOST );
		out_info->root_type = DLSwapEndian( header->root_instance_type );
	}

	return DL_ERROR_OK;
}
