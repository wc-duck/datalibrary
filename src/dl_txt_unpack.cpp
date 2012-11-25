/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_txt.h>

#include "dl_types.h"
#include "dl_swap.h"
#include "container/dl_array.h"

#include <stdarg.h> // for va_list
#include <stdio.h>  // for vsnprintf

#include <yajl/yajl_gen.h>

static inline int dl_internal_str_format(char* DL_RESTRICT buf, pint buf_size, const char* DL_RESTRICT fmt, ...)
{
	va_list args;
	va_start( args, fmt );
	int res = vsnprintf( buf, buf_size, fmt, args );
	buf[buf_size - 1] = '\0';
	va_end( args );
	return res;
}

struct SDLUnpackContext
{
public:
	SDLUnpackContext(dl_ctx_t _Ctx, yajl_gen _Gen) 
		: m_Ctx(_Ctx)
		, m_JsonGen(_Gen) { }

	dl_ctx_t m_Ctx;
	yajl_gen m_JsonGen;

 	unsigned int AddSubDataMember(const SDLMember* _pMember, const uint8* _pData)
	{
		m_lSubdataMembers.Add(SSubDataMember(_pMember, _pData));
		return (unsigned int)(m_lSubdataMembers.Len() - 1);
	}

	unsigned int FindSubDataMember(const uint8* _pData)
	{
		for (unsigned int iSubdata = 0; iSubdata < m_lSubdataMembers.Len(); ++iSubdata)
			if (m_lSubdataMembers[iSubdata].m_pData == _pData)
				return iSubdata;

		return (unsigned int)(-1);
	}

	// subdata!
	struct SSubDataMember
	{
		SSubDataMember() {}
		SSubDataMember(const SDLMember* _pMember, const uint8* _pData)
			: m_pMember(_pMember)
			, m_pData(_pData) {}
		const SDLMember* m_pMember;
		const uint8*     m_pData;
	};

	CArrayStatic<SSubDataMember, 128> m_lSubdataMembers;
};

static void dl_internal_write_pod_member( yajl_gen gen, dl_type_t pod_type, const uint8* data )
{
	switch( pod_type & DL_TYPE_STORAGE_MASK )
	{
		case DL_TYPE_STORAGE_FP32: yajl_gen_double( gen, *(float*)data); return;
		case DL_TYPE_STORAGE_FP64: yajl_gen_double( gen, *(double*)data); return;
		default: /*ignore*/ break;
	}

	const char* fmt = "";
	union { int64 sign; uint64 unsign; } value;

	switch( pod_type & DL_TYPE_STORAGE_MASK )
	{
		case DL_TYPE_STORAGE_INT8:   fmt = "%lld"; value.sign = int64(*(int8*)data); break;
		case DL_TYPE_STORAGE_INT16:  fmt = "%lld"; value.sign = int64(*(int16*)data); break;
		case DL_TYPE_STORAGE_INT32:  fmt = "%lld"; value.sign = int64(*(int32*)data); break;
		case DL_TYPE_STORAGE_INT64:  fmt = "%lld"; value.sign = int64(*(int64*)data); break;

		case DL_TYPE_STORAGE_UINT8:  fmt = "%llu"; value.unsign = uint64(*(uint8*)data); break;
		case DL_TYPE_STORAGE_UINT16: fmt = "%llu"; value.unsign = uint64(*(uint16*)data); break;
		case DL_TYPE_STORAGE_UINT32: fmt = "%llu"; value.unsign = uint64(*(uint32*)data); break;
		case DL_TYPE_STORAGE_UINT64: fmt = "%llu"; value.unsign = uint64(*(uint64*)data); break;

		default: DL_ASSERT(false && "This should not happen!"); value.unsign = 0; break;
	}

	char buffer64[128];
	int  chars = dl_internal_str_format( buffer64, 128, fmt, value.unsign );
	yajl_gen_number( gen, buffer64, chars );
}

static void dl_internal_write_instance( SDLUnpackContext* _Ctx, const SDLType* type, const uint8* data, const uint8* data_base )
{
	yajl_gen_map_open(_Ctx->m_JsonGen);

	for( uint32 member_index = 0; member_index < type->member_count; ++member_index )
	{
		const SDLMember* member = type->members + member_index;

		yajl_gen_string( _Ctx->m_JsonGen, (const unsigned char*)member->name, (unsigned int)strlen( member->name ) );

		const uint8* member_data = data + member->offset[DL_PTR_SIZE_HOST];

		dl_type_t AtomType    = member->AtomType();
		dl_type_t StorageType = member->StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pStructType = dl_internal_find_type( _Ctx->m_Ctx, member->type_id );
						if(pStructType == 0x0) { dl_log_error( _Ctx->m_Ctx, "Subtype for member %s not found!", member->name ); return; }

						dl_internal_write_instance(_Ctx, pStructType, member_data, data_base);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
	 					pint  offset     = *(pint*)member_data;
	 					char* the_string =  (char*)(data_base + offset);
	 					yajl_gen_string(_Ctx->m_JsonGen, (unsigned char*)the_string, (unsigned int)strlen(the_string));
	 				}
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						pint Offset = *(pint*)member_data;

						if(Offset == pint(-1))
							yajl_gen_null(_Ctx->m_JsonGen);
						else
						{
							unsigned int ID = _Ctx->FindSubDataMember( data_base + Offset );
							if(ID == (unsigned int)(-1))
								ID = _Ctx->AddSubDataMember( member, data_base + Offset );
							yajl_gen_integer(_Ctx->m_JsonGen, long(ID));
						}
					}
					break;
					case DL_TYPE_STORAGE_ENUM:
					{
						const char* enum_name = dl_internal_find_enum_name( _Ctx->m_Ctx, member->type_id, *(uint32*)member_data );
						yajl_gen_string( _Ctx->m_JsonGen, (unsigned char*)enum_name, (unsigned int)strlen(enum_name) );
					}
					break;
					default: // default is a standard pod-type
						DL_ASSERT( member->IsSimplePod() );
						dl_internal_write_pod_member( _Ctx->m_JsonGen, member->type, member_data );
						break;
				}
			}
			break;

			case DL_TYPE_ATOM_ARRAY:
			case DL_TYPE_ATOM_INLINE_ARRAY:
			yajl_gen_array_open(_Ctx->m_JsonGen);

			if(AtomType == DL_TYPE_ATOM_INLINE_ARRAY)
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* sub_type = dl_internal_find_type( _Ctx->m_Ctx, member->type_id );
						if( sub_type == 0x0 ) { dl_log_error( _Ctx->m_Ctx, "Type for inline-array member %s not found!", member->name ); return; }

						pint Size  = sub_type->size[DL_PTR_SIZE_HOST];
						pint Count = member->size[DL_PTR_SIZE_HOST] / Size;

						for( pint elem_index = 0; elem_index < Count; ++elem_index )
							dl_internal_write_instance(_Ctx, sub_type, member_data + (elem_index * Size), data_base);
					}
					break;

					case DL_TYPE_STORAGE_STR:
					{
						pint Size  = sizeof(char*);
						pint Count = member->size[DL_PTR_SIZE_HOST] / Size;

						for( pint elem_index = 0; elem_index < Count; ++elem_index )
						{
							pint  offset = *(pint*)( member_data + (elem_index * Size) );
							char* str    = (char*)( data_base + offset );
							yajl_gen_string( _Ctx->m_JsonGen, (unsigned char*)str, (unsigned int)strlen(str) );
						}
					}
					break;

					case DL_TYPE_STORAGE_ENUM:
					{
						pint Size  = sizeof(uint32);
						pint Count = member->size[DL_PTR_SIZE_HOST] / Size;

						for( pint elem_index = 0; elem_index < Count; ++elem_index )
						{
							uint32* enum_data = (uint32*)member_data;

							const char* enum_name = dl_internal_find_enum_name( _Ctx->m_Ctx, member->type_id, enum_data[elem_index] );
							yajl_gen_string(_Ctx->m_JsonGen, (unsigned char*)enum_name, (unsigned int)strlen(enum_name));
						}
					}
					break;

					default: // default is a standard pod-type
					{
						DL_ASSERT( member->IsSimplePod() );

						pint Size  = DLPodSize( member->type ); // the size of the inline-array could be saved in the aux-data only used by bit-field!
						pint Count = member->size[DL_PTR_SIZE_HOST] / Size;

						for(pint elem_index = 0; elem_index < Count; ++elem_index)
							dl_internal_write_pod_member( _Ctx->m_JsonGen, member->type, member_data + (elem_index * Size) );
					}
					break;
				}
			}
			else
			{
				pint   Offset = *(pint*)  member_data;
				uint32 Count  = *(uint32*)(member_data + sizeof(void*));

				const uint8* array_data = data_base + Offset;

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* sub_type = dl_internal_find_type(_Ctx->m_Ctx, member->type_id);
						if( sub_type == 0x0 ) { dl_log_error( _Ctx->m_Ctx, "Type for inline-array member %s not found!", member->name ); return; }

						pint Size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
						for(pint elem_index = 0; elem_index < Count; ++elem_index)
							dl_internal_write_instance(_Ctx, sub_type, array_data + (elem_index * Size), data_base);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						for( pint elem = 0; elem < Count; ++elem )
						{
							union { const uint8* u8p; const pint* pp; } conv;
							conv.u8p = array_data + ( elem * sizeof(char*) );
							char* str =  (char*)( data_base + *conv.pp );
							yajl_gen_string( _Ctx->m_JsonGen, (unsigned char*)str, (unsigned int)strlen(str) );
						}
					}
					break;
					case DL_TYPE_STORAGE_ENUM:
					{
						uint32* enum_data = (uint32*)array_data;

						for(pint elem_index = 0; elem_index < Count; ++elem_index)
						{
							const char* enum_name = dl_internal_find_enum_name(_Ctx->m_Ctx, member->type_id, enum_data[elem_index]);
							yajl_gen_string( _Ctx->m_JsonGen, (unsigned char*)enum_name, (unsigned int)strlen(enum_name) );
						}
					}
					break;
					default: // default is a standard pod-type
					{
						pint Size = DLPodSize( member->type );
						for(pint elem_index = 0; elem_index < Count; ++elem_index)
							dl_internal_write_pod_member( _Ctx->m_JsonGen, member->type, array_data + (elem_index * Size) );
					}
				}
			}

			yajl_gen_array_close( _Ctx->m_JsonGen );
			break;

			case DL_TYPE_ATOM_BITFIELD:
			{
				uint64 write_me  = 0;
 				uint32 bf_bits   = member->BitFieldBits();
 				uint32 bf_offset = dl_bf_offset( DL_ENDIAN_HOST, member->size[DL_PTR_SIZE_HOST], member->BitFieldOffset(), bf_bits );
 
 				switch( member->size[DL_PTR_SIZE_HOST] )
 				{
					case 1: write_me = DL_EXTRACT_BITS( uint64( *(uint8*)member_data), uint64(bf_offset), uint64(bf_bits) ); break; 
 					case 2: write_me = DL_EXTRACT_BITS( uint64(*(uint16*)member_data), uint64(bf_offset), uint64(bf_bits) ); break; 
 					case 4: write_me = DL_EXTRACT_BITS( uint64(*(uint32*)member_data), uint64(bf_offset), uint64(bf_bits) ); break; 
 					case 8: write_me = DL_EXTRACT_BITS( uint64(*(uint64*)member_data), uint64(bf_offset), uint64(bf_bits) ); break; 
 					default: 
 						DL_ASSERT(false && "This should not happen!"); 
 						break;
 				}
 
				char buffer[128];
				yajl_gen_number( _Ctx->m_JsonGen, buffer, dl_internal_str_format( buffer, DL_ARRAY_LENGTH(buffer), "%llu", write_me ) );
			}
			break;

			default:
				DL_ASSERT(false && "Invalid ATOM-type!");
				break;
		}
	}

	yajl_gen_map_close(_Ctx->m_JsonGen);
}

static void dl_internal_write_root( SDLUnpackContext*  unpack_ctx, const SDLType* type, const unsigned char* data )
{
	unpack_ctx->AddSubDataMember(0x0, data); // add root-node as a subdata to be able to look it up as id 0!

	yajl_gen_map_open( unpack_ctx->m_JsonGen);

		yajl_gen_string( unpack_ctx->m_JsonGen, (const unsigned char*)"type", 4 );
		yajl_gen_string( unpack_ctx->m_JsonGen, (const unsigned char*)type->name, (unsigned int)strlen(type->name) );

		yajl_gen_string( unpack_ctx->m_JsonGen, (const unsigned char*)"data", 4 );
		dl_internal_write_instance( unpack_ctx, type, data, data );

		if( unpack_ctx->m_lSubdataMembers.Len() != 1 )
		{
			yajl_gen_string( unpack_ctx->m_JsonGen, (const unsigned char*)"subdata", 7 );
			yajl_gen_map_open( unpack_ctx->m_JsonGen );

			// start at 1 to skip root-node!
			for( unsigned int subdata_index = 1; subdata_index <  unpack_ctx->m_lSubdataMembers.Len(); ++subdata_index )
			{
				unsigned char num_buffer[16];
				int len = dl_internal_str_format( (char*)num_buffer, 16, "%u", subdata_index );
				yajl_gen_string( unpack_ctx->m_JsonGen, num_buffer, len );

				const SDLMember* member  = unpack_ctx->m_lSubdataMembers[subdata_index].m_pMember;
				const uint8* member_data = unpack_ctx->m_lSubdataMembers[subdata_index].m_pData;

				DL_ASSERT( member->AtomType()    == DL_TYPE_ATOM_POD );
				DL_ASSERT( member->StorageType() == DL_TYPE_STORAGE_PTR );

				const SDLType* sub_type = dl_internal_find_type( unpack_ctx->m_Ctx, member->type_id );
				if( sub_type == 0x0 )
				{
					dl_log_error(  unpack_ctx->m_Ctx, "Type for inline-array member %s not found!", member->name );
					return; // TODO: need to report error some how!
				}

				dl_internal_write_instance( unpack_ctx, sub_type, member_data, data );
			}

			yajl_gen_map_close( unpack_ctx->m_JsonGen );
		}

	yajl_gen_map_close( unpack_ctx->m_JsonGen );
}

struct dl_write_text_context
{
	char* buffer;
	pint  buffer_size;
	pint  write_pos;
};

static void dl_internal_write_text_callback( void* ctx, const char *str, unsigned int len )
{
	dl_write_text_context* write_ctx = (dl_write_text_context*)ctx;

	if( write_ctx->write_pos + len <= write_ctx->buffer_size )
		memcpy( write_ctx->buffer + write_ctx->write_pos, str, len );

	write_ctx->write_pos += len;
}

static const unsigned int UNPACK_STORAGE_SIZE = 1024;

static void* dl_internal_unpack_malloc( void* ctx, unsigned int size )
{
	(void)size;
	DL_ASSERT( size <= UNPACK_STORAGE_SIZE );
	return (void*) ctx;
}
static void dl_internal_unpack_free( void* ctx, void* ptr )
{
	(void)ctx; (void)ptr;
	// just ignore free, the data should be allocated on the stack!
}

static void* dl_internal_unpack_realloc_func( void* ctx, void* ptr, unsigned int size ) 
{
	(void)ctx; (void)ptr; (void)size;
	DL_ASSERT(false && "yajl should never need to realloc");
	return 0x0;
}

dl_error_t dl_txt_unpack( dl_ctx_t dl_ctx,                       dl_typeid_t type,
                          const unsigned char* packed_instance,  size_t      packed_instance_size,
                          char*                out_txt_instance, size_t      out_txt_instance_size,
                          size_t*              produced_bytes )
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )          return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                 return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION)        return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type )           return DL_ERROR_TYPE_MISMATCH;

	const SDLType* pType = dl_internal_find_type(dl_ctx, header->root_instance_type);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND; // could not find root-type!

	static const int BEAUTIFY_OUTPUT = 1;
	yajl_gen_config Conf = { BEAUTIFY_OUTPUT, "  " };

	dl_write_text_context write_ctx = { out_txt_instance, out_txt_instance_size, 0 };
	uint8 storage[ UNPACK_STORAGE_SIZE ];
	yajl_alloc_funcs AllocCatch = { dl_internal_unpack_malloc, dl_internal_unpack_realloc_func, dl_internal_unpack_free, storage };

	yajl_gen generator = yajl_gen_alloc2( dl_internal_write_text_callback, &Conf, &AllocCatch, &write_ctx );

	SDLUnpackContext PackCtx(dl_ctx, generator );

	dl_internal_write_root(&PackCtx, pType, packed_instance + sizeof(SDLDataHeader));

	yajl_gen_free( generator );

	if(out_txt_instance != 0x0)
		out_txt_instance[ write_ctx.write_pos - 1 ] = 0;

	if( produced_bytes )
		*produced_bytes = write_ctx.write_pos;

	if( out_txt_instance_size > 0 && write_ctx.write_pos > write_ctx.buffer_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	return DL_ERROR_OK;
};

dl_error_t dl_txt_unpack_calc_size( dl_ctx_t dl_ctx,                           dl_typeid_t type,
                                    const unsigned char* packed_instance,      size_t      packed_instance_size,
                                    size_t*              out_txt_instance_size )
{
	return dl_txt_unpack( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, out_txt_instance_size );
}
