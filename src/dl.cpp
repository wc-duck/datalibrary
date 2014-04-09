#include "dl_types.h"
#include "dl_swap.h"
#include "dl_binary_writer.h"

#include "container/dl_array.h"

#include <dl/dl.h>

#include <stdlib.h> // malloc, free

// TODO: bug! dl_internal_default_alloc do not follow alignment.
static void* dl_internal_default_alloc( unsigned int size, unsigned int alignment, void* alloc_ctx ) { (void)alignment; (void)alloc_ctx; return malloc(size); }
static void  dl_internal_default_free ( void* ptr, void* alloc_ctx ) {  (void)alloc_ctx; free(ptr); }

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
	uint8_t* data = (uint8_t*)dl_internal_realloc( dl_ctx, old_data, old_data_size, old_data_size + new_data_size, sizeof(void*) );
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
                                          unsigned char* out_instance,    size_t         out_instance_size,
                                          size_t*        needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const dl_type_desc* root_type,       size_t         base_offset );

struct SPatchedInstances
{
	CArrayStatic<const uint8_t*, 1024> m_lpPatched;

	bool IsPatched(const uint8_t* _pInstance)
	{
		for (unsigned int iPatch = 0; iPatch < m_lpPatched.Len(); ++iPatch)
			if(m_lpPatched[iPatch] == _pInstance)
				return true;
		return false;
	}
};

static void dl_internal_patch_ptr( const uint8_t* ptr, const uint8_t* base_data )
{
	union conv_union { uintptr_t offset; const uint8_t* real_data; };
	conv_union* read_me = (conv_union*)ptr;

	if (read_me->offset == DL_NULL_PTR_OFFSET[DL_PTR_SIZE_HOST])
		read_me->real_data = 0x0;
	else
		read_me->real_data = base_data + read_me->offset;
};

static dl_error_t dl_internal_patch_loaded_ptrs( dl_ctx_t           dl_ctx,
												 SPatchedInstances* patched_instances,
												 const uint8_t*     instance,
												 const dl_type_desc*     type,
												 const uint8_t*     base_data,
												 bool               is_member_struct )
{
	// TODO: Optimize this please, linear search might not be the best if many subinstances is in file!
	if( !is_member_struct )
	{
		if(patched_instances->IsPatched(instance))
			return DL_ERROR_OK;

		patched_instances->m_lpPatched.Add(instance);
	}

	for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
	{
		const dl_member_desc* member      = type->members + member_index;
		const uint8_t*   member_data = instance + member->offset[DL_PTR_SIZE_HOST];

		dl_type_t atom_type    = member->AtomType();
		dl_type_t storage_type = member->StorageType();

		switch( atom_type )
		{
			case DL_TYPE_ATOM_POD:
			{
				switch( storage_type )
				{
					case DL_TYPE_STORAGE_STR:
						dl_internal_patch_ptr( member_data, base_data );
						break;
					case DL_TYPE_STORAGE_STRUCT:
						dl_internal_patch_loaded_ptrs( dl_ctx,
													   patched_instances,
													   member_data,
													   dl_internal_find_type( dl_ctx, member->type_id ),
													   base_data,
													   true );
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						uint8_t** ptr = (uint8_t**)member_data;
						dl_internal_patch_ptr(member_data, base_data);

						if(*ptr != 0x0)
							dl_internal_patch_loaded_ptrs( dl_ctx,
														   patched_instances,
														   *ptr,
														   dl_internal_find_type( dl_ctx, member->type_id ),
														   base_data,
														   false );
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
				if( storage_type == DL_TYPE_STORAGE_STR || storage_type == DL_TYPE_STORAGE_STRUCT )
				{
					dl_internal_patch_ptr( member_data, base_data );
					const uint8_t* array_data = *(const uint8_t**)member_data;

					uint32_t count = *(uint32_t*)( member_data + sizeof(void*) );

					if( count > 0 )
					{
						if(storage_type == DL_TYPE_STORAGE_STRUCT)
						{
							// patch sub-ptrs!
							const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
							uint32_t size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );

							for( uint32_t elem_offset = 0; elem_offset < count * size; elem_offset += size )
								dl_internal_patch_loaded_ptrs( dl_ctx, patched_instances, array_data + elem_offset, sub_type, base_data, true );
						}
						else
						{
							for( size_t elem_offset = 0; elem_offset < count * sizeof(char*); elem_offset += sizeof(char*) )
								dl_internal_patch_ptr( array_data + elem_offset, base_data );
						}
					}
				}
				else // pod
					dl_internal_patch_ptr( member_data, base_data );
			}
			break;

			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				if( storage_type == DL_TYPE_STORAGE_STR )
				{
					for( size_t elem_offset = 0; elem_offset < member->size[DL_PTR_SIZE_HOST]; elem_offset += sizeof(char*) )
						dl_internal_patch_ptr( member_data + elem_offset, base_data );
				}
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
			// ignore
			break;

		default:
			DL_ASSERT(false && "Unknown atom type");
			break;
		}
	}
	return DL_ERROR_OK;
}

struct dl_one_member_type
{
	dl_one_member_type( const dl_member_desc* member )
	{
		size[0]      = member->size[0];
		size[1]      = member->size[1];
		alignment[0] = member->alignment[0];
		alignment[1] = member->alignment[1];
		member_count = 1;

		memcpy(&members, member, sizeof(dl_member_desc));
		members.offset[0] = 0;
		members.offset[0] = 0;
	}

	char      name[DL_TYPE_NAME_MAX_LEN];
	uint32_t  size[2];
	uint32_t  alignment[2];
	uint32_t  member_count;
	dl_member_desc members;
};

DL_STATIC_ASSERT( sizeof(dl_one_member_type) - sizeof(dl_member_desc) == sizeof(dl_type_desc), these_need_same_size );

static dl_error_t dl_internal_load_type_library_defaults( dl_ctx_t       dl_ctx,
														  unsigned int   first_new_type,
														  const uint8_t* default_data,
														  unsigned int   default_data_size )
{
	if( default_data_size == 0 ) return DL_ERROR_OK;

	if( dl_ctx->default_data != 0x0 )
		return DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS;

	dl_ctx->default_data = (uint8_t*)dl_ctx->alloc_func( default_data_size * 2, sizeof(void*), dl_ctx->alloc_ctx ); // TODO: times 2 here need to be fixed!

	uint8_t* dst = dl_ctx->default_data;

	// ptr-patch and convert to native
	for( unsigned int type_index = first_new_type; type_index < dl_ctx->type_count; ++type_index )
	{
		union
		{
			const uint8_t* data_ptr;
			const dl_type_desc* type_ptr;
		} ptr_conv;
		ptr_conv.data_ptr = dl_ctx->type_info_data + dl_ctx->type_lookup[type_index].offset;

		for( unsigned int member_index = 0; member_index < ptr_conv.type_ptr->member_count; ++member_index )
		{
			dl_member_desc* member = (dl_member_desc*)ptr_conv.type_ptr->members + member_index;
			if( member->default_value_offset == UINT32_MAX )
				continue;

			dst                          = dl_internal_align_up( dst, member->alignment[DL_PTR_SIZE_HOST] );
			uint8_t* src                 = (uint8_t*)default_data + member->default_value_offset;
			uintptr_t base_offset        = (uintptr_t)dst - (uintptr_t)dl_ctx->default_data;
			member->default_value_offset = (uint32_t)base_offset;

			dl_one_member_type dummy( member );
			size_t needed_size;

			union
			{
				dl_one_member_type* one_mem_ptr;
				dl_type_desc*        type_ptr;
			} conv;
			conv.one_mem_ptr = &dummy;

			dl_internal_convert_no_header( dl_ctx,
										   src, (unsigned char*)default_data,
										   dst, 1337, // need to check this size ;) Should be the remainder of space in m_pDefaultInstances.
										   &needed_size,
										   DL_ENDIAN_LITTLE,  DL_ENDIAN_HOST,
										   DL_PTR_SIZE_32BIT, DL_PTR_SIZE_HOST,
										   conv.type_ptr, base_offset );

			SPatchedInstances patch_instances;
			dl_internal_patch_loaded_ptrs( dl_ctx, &patch_instances, dst, conv.type_ptr, dl_ctx->default_data, false );

			dst += needed_size;
		}
	}

	return DL_ERROR_OK;
}

static void dl_internal_read_typelibrary_header( dl_typelib_header* header, const uint8_t* data )
{
	memcpy(header, data, sizeof(dl_typelib_header));

	if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
	{
		header->id           = dl_swap_endian_uint32( header->id );
		header->version      = dl_swap_endian_uint32( header->version );

		header->type_count   = dl_swap_endian_uint32( header->type_count );
		header->types_size   = dl_swap_endian_uint32( header->types_size );

		header->enum_count   = dl_swap_endian_uint32( header->enum_count );
		header->enums_size   = dl_swap_endian_uint32( header->enums_size );

		header->default_value_size   = dl_swap_endian_uint32( header->default_value_size );
	}
}

dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, size_t lib_data_size )
{
	if(lib_data_size < sizeof(dl_typelib_header))
		return DL_ERROR_MALFORMED_DATA;

	dl_typelib_header header;
	dl_internal_read_typelibrary_header(&header, lib_data);

	if( header.id      != DL_TYPELIB_ID )      return DL_ERROR_MALFORMED_DATA;
	if( header.version != DL_TYPELIB_VERSION ) return DL_ERROR_VERSION_MISMATCH;

	size_t types_offset    = sizeof(dl_typelib_header) + header.type_count * sizeof(dl_type_lookup_t);
	size_t enums_offset    = types_offset + header.types_size + header.enum_count * sizeof(dl_type_lookup_t);
	size_t defaults_offset = enums_offset + header.enums_size;

	const unsigned char* types_data    = lib_data + types_offset;
	const unsigned char* enums_data    = lib_data + enums_offset;
	const unsigned char* defaults_data = lib_data + defaults_offset;

	// store type-info data.
	dl_ctx->type_info_data = (uint8_t*)dl_internal_append_data( dl_ctx, dl_ctx->type_info_data, dl_ctx->type_info_data_size, types_data, header.types_size );

	// read type-lookup table
	union
	{
		const uint8_t* data_ptr;
		dl_type_lookup_t* lookup;
	} cast;
	cast.data_ptr = lib_data + sizeof(dl_typelib_header);
	dl_type_lookup_t* from_data = cast.lookup;
	for( uint32_t i = dl_ctx->type_count; i < dl_ctx->type_count + header.type_count; ++i )
	{
		dl_type_lookup_t* look = dl_ctx->type_lookup + i;

		if( DL_ENDIAN_HOST == DL_ENDIAN_BIG )
		{
			look->type_id  = dl_swap_endian_uint32( from_data->type_id );
			look->offset   = dl_ctx->type_info_data_size + dl_swap_endian_uint32( from_data->offset );
			union
			{
				const uint8_t* data_ptr;
				dl_type_desc*  type_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->type_info_data + look->offset;
			dl_type_desc* type = ptr_conv.type_ptr;

			type->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( type->size[DL_PTR_SIZE_32BIT] );
			type->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( type->size[DL_PTR_SIZE_64BIT] );
			type->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( type->alignment[DL_PTR_SIZE_32BIT] );
			type->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( type->alignment[DL_PTR_SIZE_64BIT] );
			type->member_count                 = dl_swap_endian_uint32( type->member_count );

			for( uint32_t i = 0; i < type->member_count; ++i )
			{
				dl_member_desc* member = type->members + i;

				member->type                         = dl_swap_endian_dl_type( member->type );
				member->type_id	                     = dl_swap_endian_uint32( member->type_id );
				member->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( member->size[DL_PTR_SIZE_32BIT] );
				member->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( member->size[DL_PTR_SIZE_64BIT] );
				member->offset[DL_PTR_SIZE_32BIT]    = dl_swap_endian_uint32( member->offset[DL_PTR_SIZE_32BIT] );
				member->offset[DL_PTR_SIZE_64BIT]    = dl_swap_endian_uint32( member->offset[DL_PTR_SIZE_64BIT] );
				member->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( member->alignment[DL_PTR_SIZE_32BIT] );
				member->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( member->alignment[DL_PTR_SIZE_64BIT] );
				member->default_value_offset         = dl_swap_endian_uint32( member->default_value_offset );
			}
		}
		else
		{
			look->type_id = from_data->type_id;
			look->offset  = dl_ctx->type_info_data_size + from_data->offset;
		}
		++from_data;
	}

	dl_ctx->type_count          += header.type_count;
	dl_ctx->type_info_data_size += header.types_size;

	// store enum-info data.
	dl_ctx->enum_info_data = (uint8_t*)dl_internal_append_data( dl_ctx, dl_ctx->enum_info_data, dl_ctx->enum_info_data_size, enums_data, header.enums_size );

	// read enum-lookup table
	dl_type_lookup_t* enum_from_data = (dl_type_lookup_t*)( lib_data + types_offset + header.types_size );
	for( uint32_t i = dl_ctx->enum_count; i < dl_ctx->enum_count + header.enum_count; ++i )
	{
		dl_type_lookup_t* look = dl_ctx->enum_lookup + i;

		if( DL_ENDIAN_HOST == DL_ENDIAN_BIG )
		{
			look->type_id  = dl_swap_endian_uint32( enum_from_data->type_id );
			look->offset   = dl_ctx->enum_info_data_size + dl_swap_endian_uint32( enum_from_data->offset );
			union
			{
				const uint8_t* data_ptr;
				dl_enum_desc*  enum_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->enum_info_data + look->offset;
			dl_enum_desc* e = ptr_conv.enum_ptr;

			e->value_count = dl_swap_endian_uint32( e->value_count );
			for( uint32_t i = 0; i < e->value_count; ++i )
				e->values[i].value = dl_swap_endian_uint32( e->values[i].value );
		}
		else
		{
			look->type_id = enum_from_data->type_id;
			look->offset  = dl_ctx->enum_info_data_size + enum_from_data->offset;
		}

		++enum_from_data;
	}

	dl_ctx->enum_count          += header.enum_count;
	dl_ctx->enum_info_data_size += header.enums_size;

	return dl_internal_load_type_library_defaults( dl_ctx, dl_ctx->type_count - header.type_count, defaults_data, header.default_value_size );
}

dl_error_t dl_instance_load( dl_ctx_t             dl_ctx,          dl_typeid_t  type_id,
                             void*                instance,        size_t instance_size,
                             const unsigned char* packed_instance, size_t packed_instance_size,
                             size_t*              consumed )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )          return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                 return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )       return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type_id )        return DL_ERROR_TYPE_MISMATCH;
	if( header->instance_size > instance_size )        return DL_ERROR_BUFFER_TO_SMALL;

	const dl_type_desc* root_type = dl_internal_find_type( dl_ctx, header->root_instance_type );
	if( root_type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	// TODO: Temporary disabled due to CL doing some magic stuff!!! 
	// Structs allocated on qstack seems to be unaligned!!!
	// if( !dl_internal_is_align( instance, pType->m_Alignment[DL_PTR_SIZE_HOST] ) )
	//	return DL_ERROR_BAD_ALIGNMENT;

	// TODO: memmove here is a hack, should only need memcpy but due to abuse of dl_instance_load in dl_util.cpp
	// memmove is needed!
	memmove( instance, packed_instance + sizeof(dl_data_header), header->instance_size );

	SPatchedInstances patch_instances;
	dl_internal_patch_loaded_ptrs( dl_ctx, &patch_instances, (uint8_t*)instance, root_type, (uint8_t*)instance, false );

	if( consumed )
		*consumed = (size_t)header->instance_size + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_instance_load_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type,
												   unsigned char* packed_instance, size_t      packed_instance_size,
												   void**         loaded_instance, size_t*     consumed)
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )          return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                 return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )       return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type )           return DL_ERROR_TYPE_MISMATCH;

	const dl_type_desc* pType = dl_internal_find_type(dl_ctx, header->root_instance_type);
	if( pType == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	uint8_t* intstance_ptr = packed_instance + sizeof(dl_data_header);

	SPatchedInstances patched_instances;
	dl_internal_patch_loaded_ptrs( dl_ctx, &patched_instances, intstance_ptr, pType, intstance_ptr, false );

	*loaded_instance = intstance_ptr;

	if( consumed )
		*consumed = header->instance_size + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

struct CDLBinStoreContext
{
	CDLBinStoreContext( uint8_t* out_data, size_t out_data_size, bool is_dummy )
	{
		dl_binary_writer_init( &writer, out_data, out_data_size, is_dummy, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );
		num_written_ptrs = 0;
	}

	uintptr_t FindWrittenPtr( void* ptr )
	{
		for( int i = 0; i < num_written_ptrs; ++i )
			if( written_ptrs[i].ptr == ptr )
				return written_ptrs[i].pos;

		return (uintptr_t)-1;
	}

	void AddWrittenPtr( const void* ptr, uintptr_t pos )
	{
		DL_ASSERT( num_written_ptrs < (int)DL_ARRAY_LENGTH(written_ptrs) );
		written_ptrs[num_written_ptrs].ptr = ptr;
		written_ptrs[num_written_ptrs].pos = pos;
		++num_written_ptrs;
	}

	dl_binary_writer writer;

	struct
	{
		uintptr_t   pos;
		const void* ptr;
	} written_ptrs[128];
	int num_written_ptrs;
};

static void dl_internal_store_string( const uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	char* str = *(char**)instance;
	uintptr_t pos = dl_binary_writer_tell( &store_ctx->writer );
	dl_binary_writer_seek_end( &store_ctx->writer );
	uintptr_t offset = dl_binary_writer_tell( &store_ctx->writer );
	dl_binary_writer_write( &store_ctx->writer, str, strlen(str) + 1 );
	dl_binary_writer_seek_set( &store_ctx->writer, pos );
	dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );
}

static dl_error_t dl_internal_instance_store( dl_ctx_t dl_ctx, const dl_type_desc* dl_type, uint8_t* instance, CDLBinStoreContext* store_ctx );

static dl_error_t dl_internal_store_member( dl_ctx_t dl_ctx, const dl_member_desc* member, uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	dl_type_t atom_type    = dl_type_t(member->type & DL_TYPE_ATOM_MASK);
	dl_type_t storage_type = dl_type_t(member->type & DL_TYPE_STORAGE_MASK);

	switch ( atom_type )
	{
		case DL_TYPE_ATOM_POD:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
					if( sub_type == 0x0 )
					{
						dl_log_error( dl_ctx, "Could not find subtype for member %s", member->name );
						return DL_ERROR_TYPE_NOT_FOUND;
					}
					dl_internal_instance_store( dl_ctx, sub_type, instance, store_ctx );
				}
				break;
				case DL_TYPE_STORAGE_STR:
					dl_internal_store_string( instance, store_ctx );
					break;
				case DL_TYPE_STORAGE_PTR:
				{
					uint8_t* data = *(uint8_t**)instance;
					uintptr_t offset = store_ctx->FindWrittenPtr( data );

					if( data == 0x0 ) // Null-pointer, store pint(-1) to signal to patching!
					{
						DL_ASSERT(offset == (uintptr_t)-1 && "This pointer should not have been found among the written ptrs!");
						// keep the -1 in Offset and store it to ptr.
					}
					else if( offset == (uintptr_t)-1 ) // has not been written yet!
					{
						uintptr_t pos = dl_binary_writer_tell( &store_ctx->writer );
						dl_binary_writer_seek_end( &store_ctx->writer );

						const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
						uintptr_t size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
						dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );

						offset = dl_binary_writer_tell( &store_ctx->writer );

						// write data!
						dl_binary_writer_reserve( &store_ctx->writer, size ); // reserve space for ptr so subdata is placed correctly

						store_ctx->AddWrittenPtr(data, offset);

						dl_internal_instance_store(dl_ctx, sub_type, data, store_ctx);

						dl_binary_writer_seek_set( &store_ctx->writer, pos );
					}

					dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );
				}
				break;
				default: // default is a standard pod-type
					DL_ASSERT( member->IsSimplePod() || storage_type == DL_TYPE_STORAGE_ENUM );
					dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] );
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STRUCT:
					dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] ); // TODO: I Guess that this is a bug! Will it fix ptrs well?
					break;
				case DL_TYPE_STORAGE_STR:
				{
					uint32_t count = (uint32_t)(member->size[DL_PTR_SIZE_HOST] / sizeof(char*));

					for( uint32_t elem = 0; elem < count; ++elem )
						dl_internal_store_string( instance + (elem * sizeof(char*)), store_ctx );
				}
				break;
				default: // default is a standard pod-type
					DL_ASSERT( member->IsSimplePod() || storage_type == DL_TYPE_STORAGE_ENUM );
					dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] );
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t size = 0;
			const dl_type_desc* sub_type = 0x0;

			uint8_t* data_ptr = instance;
			uint32_t count    = *(uint32_t*)( data_ptr + sizeof(void*) );

			uintptr_t offset = 0;

			if( count == 0 )
				offset = DL_NULL_PTR_OFFSET[ DL_PTR_SIZE_HOST ];
			else
			{
				uintptr_t pos = dl_binary_writer_tell( &store_ctx->writer );
				dl_binary_writer_seek_end( &store_ctx->writer );

				switch(storage_type)
				{
					case DL_TYPE_STORAGE_STRUCT:
						sub_type = dl_internal_find_type( dl_ctx, member->type_id );
						size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
						dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );
						break;
					case DL_TYPE_STORAGE_STR:
						size = sizeof(void*);
						dl_binary_writer_align( &store_ctx->writer, size );
						break;
					default:
						size = DLPodSize( member->type );
						dl_binary_writer_align( &store_ctx->writer, size );
				}

				offset = dl_binary_writer_tell( &store_ctx->writer );

				// write data!
				dl_binary_writer_reserve( &store_ctx->writer, count * size ); // reserve space for array so subdata is placed correctly

				uint8_t* data = *(uint8_t**)data_ptr;

				switch(storage_type)
				{
					case DL_TYPE_STORAGE_STRUCT:
						for ( unsigned int elem = 0; elem < count; ++elem )
							dl_internal_instance_store( dl_ctx, sub_type, data + (elem * size), store_ctx );
						break;
					case DL_TYPE_STORAGE_STR:
						for ( unsigned int elem = 0; elem < count; ++elem )
							dl_internal_store_string( data + (elem * size), store_ctx );
						break;
					default:
						for ( unsigned int elem = 0; elem < count; ++elem )
							dl_binary_writer_write( &store_ctx->writer, data + (elem * size), size ); break;
				}

				dl_binary_writer_seek_set( &store_ctx->writer, pos );
			}

			// make room for ptr
			dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );

			// write count
			dl_binary_writer_write( &store_ctx->writer, &count, sizeof(uint32_t) );
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_BITFIELD:
			dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] );
		break;

		default:
			DL_ASSERT(false && "Invalid ATOM-type!");
			break;
	}

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_instance_store( dl_ctx_t dl_ctx, const dl_type_desc* dl_type, uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	bool last_was_bitfield = false;

	dl_binary_writer_align( &store_ctx->writer, dl_type->alignment[DL_PTR_SIZE_HOST] );

	uintptr_t instance_pos = dl_binary_writer_tell( &store_ctx->writer );
	for( uint32_t member_index = 0; member_index < dl_type->member_count; ++member_index )
	{
		const dl_member_desc* member = dl_type->members + member_index;

		if( !last_was_bitfield || member->AtomType() != DL_TYPE_ATOM_BITFIELD )
		{
			dl_binary_writer_seek_set( &store_ctx->writer, instance_pos + member->offset[DL_PTR_SIZE_HOST] );
			dl_error_t err = dl_internal_store_member( dl_ctx, member, instance + member->offset[DL_PTR_SIZE_HOST], store_ctx );
			if( err != DL_ERROR_OK )
				return err;
		}

		last_was_bitfield = member->AtomType() == DL_TYPE_ATOM_BITFIELD;
	}

	return DL_ERROR_OK;
}

dl_error_t dl_instance_store( dl_ctx_t       dl_ctx,     dl_typeid_t type_id,         const void* instance,
							  unsigned char* out_buffer, size_t      out_buffer_size, size_t*     produced_bytes )
{
	if( out_buffer_size > 0 && out_buffer_size <= sizeof(dl_data_header) )
		return DL_ERROR_BUFFER_TO_SMALL;

	const dl_type_desc* type = dl_internal_find_type( dl_ctx, type_id );
	if( type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	// write header
	dl_data_header header;
	header.id = DL_INSTANCE_ID;
	header.version = DL_INSTANCE_VERSION;
	header.root_instance_type = type_id;
	header.instance_size = 0;
	header.is_64_bit_ptr = sizeof(void*) == 8 ? 1 : 0;
	header.pad[0] = header.pad[1] = header.pad[2] = 0;

	unsigned char* store_ctx_buffer      = 0x0;
	size_t         store_ctx_buffer_size = 0;
	bool           store_ctx_is_dummy    = out_buffer_size == 0;

	if( out_buffer_size > 0 )
	{
		memcpy(out_buffer, &header, sizeof(dl_data_header));
		store_ctx_buffer      = out_buffer + sizeof(dl_data_header);
		store_ctx_buffer_size = out_buffer_size - sizeof(dl_data_header);
	}

	CDLBinStoreContext store_context( store_ctx_buffer, store_ctx_buffer_size, store_ctx_is_dummy );

	dl_binary_writer_reserve( &store_context.writer, type->size[DL_PTR_SIZE_HOST] );
	store_context.AddWrittenPtr(instance, 0); // if pointer refere to root-node, it can be found at offset 0

	dl_error_t err = dl_internal_instance_store( dl_ctx, type, (uint8_t*)instance, &store_context );

	// write instance size!
	dl_data_header* out_header = (dl_data_header*)out_buffer;
	dl_binary_writer_seek_end( &store_context.writer );
	if( out_buffer )
		out_header->instance_size = (uint32_t)dl_binary_writer_tell( &store_context.writer );

	if( produced_bytes )
		*produced_bytes = (uint32_t)dl_binary_writer_tell( &store_context.writer ) + sizeof(dl_data_header);

	if( out_buffer_size > 0 && out_header->instance_size > out_buffer_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	return err;
}

dl_error_t dl_instance_calc_size( dl_ctx_t dl_ctx, dl_typeid_t type, void* instance, size_t* out_size )
{
	return dl_instance_store( dl_ctx, type, instance, 0x0, 0, out_size );
}

const char* dl_error_to_string( dl_error_t error )
{
#define DL_ERR_TO_STR(ERR) case ERR: return #ERR
	switch(error)
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
		DL_ERR_TO_STR(DL_ERROR_TXT_INVALID_ENUM_VALUE);

		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_TYPE_MISMATCH);

		DL_ERR_TO_STR(DL_ERROR_INTERNAL_ERROR);
		default: return "Unknown error!";
	}
#undef DL_ERR_TO_STR
}

dl_error_t dl_instance_get_info( const unsigned char* packed_instance, size_t packed_instance_size, dl_instance_info_t* out_info )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) && header->id != DL_INSTANCE_ID_SWAPED && header->id != DL_INSTANCE_ID )
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
		out_info->load_size = dl_swap_endian_uint32( header->instance_size );
		out_info->endian    = dl_other_endian( DL_ENDIAN_HOST );
		out_info->root_type = dl_swap_endian_uint32( header->root_instance_type );
	}

	return DL_ERROR_OK;
}
