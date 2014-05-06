#include "dl_types.h"
#include "dl_swap.h"
#include "dl_binary_writer.h"
#include "dl_patch_ptr.h"

#include "container/dl_array.h"

#include <dl/dl.h>

dl_error_t dl_context_create( dl_ctx_t* dl_ctx, dl_create_params_t* create_params )
{
	dl_allocator alloc;
	dl_allocator_initialize( &alloc, create_params->alloc_func, create_params->realloc_func, create_params->free_func, create_params->alloc_ctx );

	dl_context* ctx = (dl_context*)dl_alloc( &alloc, sizeof( dl_context ) );

	if(ctx == 0x0)
		return DL_ERROR_OUT_OF_LIBRARY_MEMORY;

	memset(ctx, 0x0, sizeof(dl_context));
	memcpy(&ctx->alloc, &alloc, sizeof( dl_allocator ) );

	ctx->error_msg_func = create_params->error_msg_func;
	ctx->error_msg_ctx  = create_params->error_msg_ctx;

	ctx->type_count        = 0;
	ctx->member_count      = 0;
	ctx->enum_count        = 0;
	ctx->enum_value_count  = 0;
	ctx->default_data      = 0;
	ctx->default_data_size = 0;

	*dl_ctx = ctx;

	return DL_ERROR_OK;
}

dl_error_t dl_context_destroy(dl_ctx_t dl_ctx)
{
	dl_free( &dl_ctx->alloc, dl_ctx->default_data );
	dl_free( &dl_ctx->alloc, dl_ctx );
	return DL_ERROR_OK;
}

// implemented in dl_convert.cpp
dl_error_t dl_internal_convert_no_header( dl_ctx_t       dl_ctx,
                                          unsigned char* packed_instance, unsigned char* packed_instance_base,
                                          unsigned char* out_instance,    size_t         out_instance_size,
                                          size_t*        needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const dl_type_desc* root_type,       size_t    base_offset );

static dl_error_t dl_internal_load_type_library_defaults( dl_ctx_t       dl_ctx,
														  const uint8_t* default_data,
														  unsigned int   default_data_size )
{
	if( default_data_size == 0 ) return DL_ERROR_OK;

	if( dl_ctx->default_data != 0x0 )
		return DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS;

	// TODO: times 2 here need to be fixed! Can be removed now if we store instances as 64bit.
	dl_ctx->default_data = (uint8_t*)dl_alloc( &dl_ctx->alloc, default_data_size * 2 );
	dl_ctx->default_data_size = default_data_size;

	memcpy( dl_ctx->default_data, default_data, default_data_size );

	return DL_ERROR_OK;
}

static void dl_internal_read_typelibrary_header( dl_typelib_header* header, const uint8_t* data )
{
	memcpy(header, data, sizeof(dl_typelib_header));

	if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
	{
		header->id           = dl_swap_endian_uint32( header->id );
		header->version      = dl_swap_endian_uint32( header->version );

		header->type_count       = dl_swap_endian_uint32( header->type_count );
		header->enum_count       = dl_swap_endian_uint32( header->enum_count );
		header->member_count     = dl_swap_endian_uint32( header->member_count );
		header->enum_value_count = dl_swap_endian_uint32( header->enum_value_count );

		header->default_value_size   = dl_swap_endian_uint32( header->default_value_size );
	}
}

static void dl_endian_swap_type_desc( dl_type_desc* desc )
{
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->member_count                 = dl_swap_endian_uint32( desc->member_count );
}

static void dl_endian_swap_enum_desc( dl_enum_desc* desc )
{
	desc->value_count = dl_swap_endian_uint32( desc->value_count );
	desc->value_start = dl_swap_endian_uint32( desc->value_start );
}

static void dl_endian_swap_member_desc( dl_member_desc* desc )
{
	desc->type                         = dl_swap_endian_dl_type( desc->type );
	desc->type_id	                   = dl_swap_endian_uint32( desc->type_id );
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->offset[DL_PTR_SIZE_32BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_32BIT] );
	desc->offset[DL_PTR_SIZE_64BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->default_value_offset         = dl_swap_endian_uint32( desc->default_value_offset );
}

static void dl_endian_swap_enum_value_desc( dl_enum_value_desc* desc )
{
	desc->value = dl_swap_endian_uint32( desc->value );
}

dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, size_t lib_data_size )
{
	if(lib_data_size < sizeof(dl_typelib_header))
		return DL_ERROR_MALFORMED_DATA;

	dl_typelib_header header;
	dl_internal_read_typelibrary_header(&header, lib_data);

	if( header.id      != DL_TYPELIB_ID )      return DL_ERROR_MALFORMED_DATA;
	if( header.version != DL_TYPELIB_VERSION ) return DL_ERROR_VERSION_MISMATCH;

	size_t types_lookup_offset = sizeof(dl_typelib_header);
	size_t enums_lookup_offset = types_lookup_offset + sizeof( dl_typeid_t ) * header.type_count;
	size_t types_offset        = enums_lookup_offset + sizeof( dl_typeid_t ) * header.enum_count;
	size_t enums_offset        = types_offset        + sizeof( dl_type_desc ) * header.type_count;
	size_t members_offset      = enums_offset        + sizeof( dl_enum_desc ) * header.enum_count;
	size_t enum_values_offset  = members_offset      + sizeof( dl_member_desc ) * header.member_count;
	size_t defaults_offset     = enum_values_offset  + sizeof( dl_enum_value_desc ) * header.enum_value_count;

	memcpy( dl_ctx->type_ids         + dl_ctx->type_count,         lib_data + types_lookup_offset, sizeof( dl_typeid_t ) * header.type_count );
	memcpy( dl_ctx->enum_ids         + dl_ctx->enum_count,         lib_data + enums_lookup_offset, sizeof( dl_typeid_t ) * header.enum_count );
	memcpy( dl_ctx->type_descs       + dl_ctx->type_count,         lib_data + types_offset,        sizeof( dl_type_desc ) * header.type_count );
	memcpy( dl_ctx->enum_descs       + dl_ctx->enum_count,         lib_data + enums_offset,        sizeof( dl_enum_desc ) * header.enum_count );
	memcpy( dl_ctx->member_descs     + dl_ctx->member_count,       lib_data + members_offset,      sizeof( dl_member_desc ) * header.member_count );
	memcpy( dl_ctx->enum_value_descs + dl_ctx->enum_value_count,   lib_data + enum_values_offset,  sizeof( dl_enum_value_desc ) * header.enum_value_count );

	if( DL_ENDIAN_HOST == DL_ENDIAN_BIG )
	{
		for( unsigned int i = 0; i < header.type_count; ++i ) dl_ctx->type_ids[ dl_ctx->type_count + i ] = dl_swap_endian_uint32( dl_ctx->type_ids[ dl_ctx->type_count + i ] );
		for( unsigned int i = 0; i < header.enum_count; ++i ) dl_ctx->enum_ids[ dl_ctx->enum_count + i ] = dl_swap_endian_uint32( dl_ctx->enum_ids[ dl_ctx->enum_count + i ] );
		for( unsigned int i = 0; i < header.type_count; ++i )       dl_endian_swap_type_desc( dl_ctx->type_descs + dl_ctx->type_count + i );
		for( unsigned int i = 0; i < header.enum_count; ++i )       dl_endian_swap_enum_desc( dl_ctx->enum_descs + dl_ctx->enum_count + i );
		for( unsigned int i = 0; i < header.member_count; ++i )     dl_endian_swap_member_desc( dl_ctx->member_descs + dl_ctx->member_count + i );
		for( unsigned int i = 0; i < header.enum_value_count; ++i ) dl_endian_swap_enum_value_desc( dl_ctx->enum_value_descs + dl_ctx->enum_value_count + i );
	}

	for( unsigned int i = 0; i < header.type_count; ++i )
		dl_ctx->type_descs[ dl_ctx->type_count + i ].member_start += dl_ctx->member_count;

	for( unsigned int i = 0; i < header.enum_count; ++i )
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].value_start += dl_ctx->enum_value_count;

	dl_ctx->type_count += header.type_count;
	dl_ctx->enum_count += header.enum_count;
	dl_ctx->member_count += header.member_count;
	dl_ctx->enum_value_count += header.enum_value_count;

	return dl_internal_load_type_library_defaults( dl_ctx, lib_data + defaults_offset, header.default_value_size );
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

	dl_internal_patch_instance( dl_ctx, root_type, (uint8_t*)instance, 0x0, (uintptr_t)instance );

	if( consumed )
		*consumed = (size_t)header->instance_size + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_instance_load_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type_id,
												   unsigned char* packed_instance, size_t      packed_instance_size,
												   void**         loaded_instance, size_t*     consumed)
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )           return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                  return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )        return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type_id )         return DL_ERROR_TYPE_MISMATCH;

	const dl_type_desc* type = dl_internal_find_type(dl_ctx, header->root_instance_type);
	if( type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	uint8_t* instance_ptr = packed_instance + sizeof(dl_data_header);
	dl_internal_patch_instance( dl_ctx, type, instance_ptr, 0x0, (uintptr_t)instance_ptr );

	*loaded_instance = instance_ptr;

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
					for( uint32_t elem = 0; elem < member->inline_array_cnt(); ++elem )
						dl_internal_store_string( instance + (elem * sizeof(char*)), store_ctx );
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
						size = dl_pod_size( member->type );
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
		const dl_member_desc* member = dl_get_type_member( dl_ctx, dl_type, member_index );

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
