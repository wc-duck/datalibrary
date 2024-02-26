#include <dl/dl_typelib.h>
#include "dl_internal_util.h"
#include "dl_types.h"

static void dl_internal_load_type_library_defaults( dl_ctx_t       dl_ctx,
														  const uint8_t* default_data,
														  unsigned int   default_data_size )
{
	if( default_data_size == 0 )
		return;

	dl_ctx->default_data = (uint8_t*)dl_realloc( &dl_ctx->alloc, dl_ctx->default_data, default_data_size + dl_ctx->default_data_size, dl_ctx->default_data_size );
	memcpy( dl_ctx->default_data + dl_ctx->default_data_size, default_data, default_data_size );
	dl_ctx->default_data_size += default_data_size;
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
		header->enum_alias_count = dl_swap_endian_uint32( header->enum_alias_count );
		
		header->default_value_size    = dl_swap_endian_uint32( header->default_value_size );
		header->typeinfo_strings_size = dl_swap_endian_uint32( header->typeinfo_strings_size );
		header->c_includes_size       = dl_swap_endian_uint32( header->c_includes_size );
		header->metadatas_count       = dl_swap_endian_uint32( header->metadatas_count );
	}
}

static void dl_endian_swap_type_desc( dl_type_desc* desc )
{
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->member_count                 = dl_swap_endian_uint32( desc->member_count );
	desc->metadata_count               = dl_swap_endian_uint32( desc->metadata_count );
	desc->metadata_start               = dl_swap_endian_uint32( desc->metadata_start );
}

static void dl_endian_swap_enum_desc( dl_enum_desc* desc )
{
	desc->value_count    = dl_swap_endian_uint32( desc->value_count );
	desc->value_start    = dl_swap_endian_uint32( desc->value_start );
	desc->metadata_count = dl_swap_endian_uint32( desc->metadata_count );
	desc->metadata_start = dl_swap_endian_uint32( desc->metadata_start );
}

static void dl_endian_swap_member_desc( dl_member_desc* desc )
{
	desc->type                         = (dl_type_t)dl_swap_endian_uint32( (uint32_t)desc->type );
	desc->type_id	                   = dl_swap_endian_uint32( desc->type_id );
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->offset[DL_PTR_SIZE_32BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_32BIT] );
	desc->offset[DL_PTR_SIZE_64BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->default_value_offset         = dl_swap_endian_uint32( desc->default_value_offset );
	desc->metadata_count               = dl_swap_endian_uint32( desc->metadata_count );
	desc->metadata_start               = dl_swap_endian_uint32( desc->metadata_start );
}

static void dl_endian_swap_enum_value_desc( dl_enum_value_desc* desc )
{
	desc->value          = dl_swap_endian_uint64( desc->value );
	desc->metadata_count = dl_swap_endian_uint32( desc->metadata_count );
	desc->metadata_start = dl_swap_endian_uint32( desc->metadata_start );
}

template <typename T>
static inline T* dl_realloc_array(dl_allocator* alloc, T* ptr, size_t new_size, size_t old_size)
{
	return (T*)dl_realloc(alloc, ptr, new_size * sizeof(T), old_size * sizeof(T));
}

dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, size_t lib_data_size )
{
	if(lib_data_size < sizeof(dl_typelib_header))
		return DL_ERROR_MALFORMED_DATA;

	dl_typelib_header header;
	dl_internal_read_typelibrary_header(&header, lib_data);

	if( header.id      != DL_TYPELIB_ID )      return DL_ERROR_MALFORMED_DATA;
	if( header.version != DL_TYPELIB_VERSION ) return DL_ERROR_VERSION_MISMATCH;

	size_t types_lookup_offset     = sizeof(dl_typelib_header);
	size_t enums_lookup_offset     = types_lookup_offset + sizeof( dl_typeid_t ) * header.type_count;
	size_t types_offset            = enums_lookup_offset + sizeof( dl_typeid_t ) * header.enum_count;
	size_t enums_offset            = types_offset        + sizeof( dl_type_desc ) * header.type_count;
	size_t members_offset          = enums_offset        + sizeof( dl_enum_desc ) * header.enum_count;
	size_t enum_values_offset      = members_offset      + sizeof( dl_member_desc ) * header.member_count;
	size_t enum_aliases_offset     = enum_values_offset  + sizeof( dl_enum_value_desc ) * header.enum_value_count;
	size_t defaults_offset         = enum_aliases_offset + sizeof( dl_enum_alias_desc ) * header.enum_alias_count;
	size_t typedata_strings_offset = defaults_offset + header.default_value_size;
	size_t c_includes_offset       = typedata_strings_offset + header.typeinfo_strings_size;
	size_t metadatas_offset        = c_includes_offset + header.c_includes_size;

	dl_ctx->type_ids         = dl_realloc_array( &dl_ctx->alloc, dl_ctx->type_ids,         dl_ctx->type_count + header.type_count,                       dl_ctx->type_count );
	dl_ctx->type_descs       = dl_realloc_array( &dl_ctx->alloc, dl_ctx->type_descs,       dl_ctx->type_count + header.type_count,                       dl_ctx->type_count );
	dl_ctx->enum_ids         = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_ids,         dl_ctx->enum_count + header.enum_count,                       dl_ctx->enum_count );
	dl_ctx->enum_descs       = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_descs,       dl_ctx->enum_count + header.enum_count,                       dl_ctx->enum_count );
	dl_ctx->member_descs     = dl_realloc_array( &dl_ctx->alloc, dl_ctx->member_descs,     dl_ctx->member_count + header.member_count,                   dl_ctx->member_count );
	dl_ctx->enum_value_descs = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_value_descs, dl_ctx->enum_value_count + header.enum_value_count,           dl_ctx->enum_value_count );
	dl_ctx->enum_alias_descs = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_alias_descs, dl_ctx->enum_alias_count + header.enum_alias_count,           dl_ctx->enum_alias_count );
	dl_ctx->typedata_strings = dl_realloc_array( &dl_ctx->alloc, dl_ctx->typedata_strings, dl_ctx->typedata_strings_size + header.typeinfo_strings_size, dl_ctx->typedata_strings_size );
	if(header.c_includes_size)
		dl_ctx->c_includes   = dl_realloc_array( &dl_ctx->alloc, dl_ctx->c_includes,       dl_ctx->c_includes_size + header.c_includes_size,             dl_ctx->c_includes_size );
	if(header.metadatas_count)
	{
		dl_ctx->metadatas    = dl_realloc_array( &dl_ctx->alloc, dl_ctx->metadatas,        dl_ctx->metadatas_count + header.metadatas_count,             dl_ctx->metadatas_count );
		dl_ctx->metadata_infos = dl_realloc_array( &dl_ctx->alloc, dl_ctx->metadata_infos, dl_ctx->metadatas_count + header.metadatas_count,             dl_ctx->metadatas_count );
		dl_ctx->metadata_typeinfos = dl_realloc_array( &dl_ctx->alloc, dl_ctx->metadata_typeinfos, dl_ctx->metadatas_count + header.metadatas_count,     dl_ctx->metadatas_count );
		for( unsigned int i = 0; i < header.metadatas_count; ++i )
		{
			uint32_t metadata_offset              = *reinterpret_cast<const uint32_t*>( lib_data + metadatas_offset + i * sizeof( uint32_t ) );
			metadata_offset                       = ( DL_ENDIAN_HOST == DL_ENDIAN_BIG ) ? dl_swap_endian_uint32( metadata_offset ) : metadata_offset;
			const dl_data_header* metadata_header = reinterpret_cast<const dl_data_header*>( lib_data + metadatas_offset + header.metadatas_count * sizeof( uint32_t ) + metadata_offset );
			size_t instance_size                  = ( DL_ENDIAN_HOST == DL_ENDIAN_BIG ) ? dl_swap_endian_uint32( metadata_header->instance_size ) : metadata_header->instance_size;
			dl_ctx->metadatas[dl_ctx->metadatas_count + i] = dl_alloc( &dl_ctx->alloc, instance_size + sizeof( dl_data_header ) );
		}
	}

	memcpy( dl_ctx->type_ids         + dl_ctx->type_count,            lib_data + types_lookup_offset, sizeof( dl_typeid_t ) * header.type_count );
	memcpy( dl_ctx->enum_ids         + dl_ctx->enum_count,            lib_data + enums_lookup_offset, sizeof( dl_typeid_t ) * header.enum_count );
	memcpy( dl_ctx->type_descs       + dl_ctx->type_count,            lib_data + types_offset,        sizeof( dl_type_desc ) * header.type_count );
	memcpy( dl_ctx->enum_descs       + dl_ctx->enum_count,            lib_data + enums_offset,        sizeof( dl_enum_desc ) * header.enum_count );
	memcpy( dl_ctx->member_descs     + dl_ctx->member_count,          lib_data + members_offset,      sizeof( dl_member_desc ) * header.member_count );
	memcpy( dl_ctx->enum_value_descs + dl_ctx->enum_value_count,      lib_data + enum_values_offset,  sizeof( dl_enum_value_desc ) * header.enum_value_count );
	memcpy( dl_ctx->enum_alias_descs + dl_ctx->enum_alias_count,      lib_data + enum_aliases_offset, sizeof( dl_enum_alias_desc ) * header.enum_alias_count );
	memcpy( dl_ctx->typedata_strings + dl_ctx->typedata_strings_size, lib_data + typedata_strings_offset, header.typeinfo_strings_size );
	if(header.c_includes_size)
		memcpy( dl_ctx->c_includes   + dl_ctx->c_includes_size,       lib_data + c_includes_offset,       header.c_includes_size );

	if( DL_ENDIAN_HOST == DL_ENDIAN_BIG )
	{
		for( unsigned int i = 0; i < header.type_count; ++i ) dl_ctx->type_ids[ dl_ctx->type_count + i ] = dl_swap_endian_uint32( dl_ctx->type_ids[ dl_ctx->type_count + i ] );
		for( unsigned int i = 0; i < header.enum_count; ++i ) dl_ctx->enum_ids[ dl_ctx->enum_count + i ] = dl_swap_endian_uint32( dl_ctx->enum_ids[ dl_ctx->enum_count + i ] );
		for( unsigned int i = 0; i < header.type_count; ++i )       dl_endian_swap_type_desc( dl_ctx->type_descs + dl_ctx->type_count + i );
		for( unsigned int i = 0; i < header.enum_count; ++i )       dl_endian_swap_enum_desc( dl_ctx->enum_descs + dl_ctx->enum_count + i );
		for( unsigned int i = 0; i < header.member_count; ++i )     dl_endian_swap_member_desc( dl_ctx->member_descs + dl_ctx->member_count + i );
		for( unsigned int i = 0; i < header.enum_value_count; ++i ) dl_endian_swap_enum_value_desc( dl_ctx->enum_value_descs + dl_ctx->enum_value_count + i );
	}

	uint32_t td_str_offset = (uint32_t)dl_ctx->typedata_strings_size;
	for( unsigned int i = 0; i < header.type_count; ++i )
	{
		dl_ctx->type_descs[ dl_ctx->type_count + i ].name += td_str_offset;
		if(dl_ctx->type_descs[ dl_ctx->type_count + i ].comment != UINT32_MAX)
			dl_ctx->type_descs[ dl_ctx->type_count + i ].comment += td_str_offset;
		dl_ctx->type_descs[ dl_ctx->type_count + i ].member_start += dl_ctx->member_count;
		dl_ctx->type_descs[ dl_ctx->type_count + i ].metadata_start += dl_ctx->metadatas_count;

		if( dl_ctx->performing_include )
		{
			dl_ctx->type_descs[ dl_ctx->type_count + i ].flags |= (uint32_t)DL_TYPE_FLAG_IS_EXTERNAL;
			dl_ctx->type_descs[ dl_ctx->type_count + i ].flags &= ~(uint32_t)DL_TYPE_FLAG_VERIFY_EXTERNAL_SIZE_ALIGN;
		}
	}

	for( unsigned int i = 0; i < header.member_count; ++i )
	{
		dl_ctx->member_descs[ dl_ctx->member_count + i ].name += td_str_offset;
		if( dl_ctx->member_descs[ dl_ctx->member_count + i ].metadata_start )
			dl_ctx->member_descs[ dl_ctx->member_count + i ].metadata_start += dl_ctx->metadatas_count;

		if( dl_ctx->member_descs[ dl_ctx->member_count + i ].default_value_offset != UINT32_MAX )
			dl_ctx->member_descs[ dl_ctx->member_count + i ].default_value_offset += (uint32_t)dl_ctx->default_data_size;

		if( dl_ctx->performing_include )
			dl_ctx->member_descs[ dl_ctx->member_count + i ].flags &= ~(uint32_t)DL_MEMBER_FLAG_VERIFY_EXTERNAL_SIZE_OFFSET;
	}

	for( unsigned int i = 0; i < header.enum_count; ++i )
	{
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].name += td_str_offset;
		if(dl_ctx->enum_descs[ dl_ctx->enum_count + i ].comment != UINT32_MAX)
			dl_ctx->enum_descs[ dl_ctx->enum_count+ i ].comment += td_str_offset;
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].value_start += dl_ctx->enum_value_count;
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].alias_start += dl_ctx->enum_alias_count;
		if( dl_ctx->enum_descs[ dl_ctx->enum_count + i ].metadata_start )
			dl_ctx->enum_descs[ dl_ctx->enum_count + i ].metadata_start += dl_ctx->metadatas_count;
		if( dl_ctx->performing_include )
		{
			dl_ctx->enum_descs[ dl_ctx->enum_count + i ].flags |= (uint32_t)DL_TYPE_FLAG_IS_EXTERNAL;
			dl_ctx->enum_descs[ dl_ctx->enum_count + i ].flags &= ~(uint32_t)DL_TYPE_FLAG_VERIFY_EXTERNAL_SIZE_ALIGN;
		}
	}

	for( unsigned int i = 0; i < header.enum_alias_count; ++i )
	{
		dl_ctx->enum_alias_descs[ dl_ctx->enum_alias_count + i ].name += td_str_offset;
		dl_ctx->enum_alias_descs[ dl_ctx->enum_alias_count + i ].value_index += dl_ctx->enum_value_count;
	}

	for( unsigned int i = 0; i < header.enum_value_count; ++i )
	{
		dl_ctx->enum_value_descs[ dl_ctx->enum_value_count + i ].main_alias += dl_ctx->enum_alias_count;
		if( dl_ctx->enum_value_descs[ dl_ctx->enum_value_count + i ].metadata_start )
			dl_ctx->enum_value_descs[ dl_ctx->enum_value_count + i ].metadata_start += dl_ctx->metadatas_count;
	}

	dl_ctx->type_count            += header.type_count;
	dl_ctx->enum_count            += header.enum_count;
	dl_ctx->member_count          += header.member_count;
	dl_ctx->enum_value_count      += header.enum_value_count;
	dl_ctx->enum_alias_count      += header.enum_alias_count;
	dl_ctx->typedata_strings_size += header.typeinfo_strings_size;
	dl_ctx->c_includes_size       += header.c_includes_size;

	// we still need to keep the capacity around here, even as they are the same as the type-counts in
	// the case where we were to read a typelib from text into this ctx as that would do an incremental
	// grow of the capacity.
	// One day we might get rid of that by working with temp-buffers when reading text?
	dl_ctx->type_capacity         = dl_ctx->type_count;
	dl_ctx->enum_capacity         = dl_ctx->enum_count;
	dl_ctx->member_capacity       = dl_ctx->member_count;
	dl_ctx->enum_value_capacity   = dl_ctx->enum_value_count;
	dl_ctx->enum_alias_capacity   = dl_ctx->enum_alias_count;
	dl_ctx->typedata_strings_cap  = dl_ctx->typedata_strings_size;
	dl_ctx->c_includes_cap        = dl_ctx->c_includes_size;

	// The types used for meta data must be known to be able to patch the data with dl_instance_load_inplace
	for( unsigned int i = 0; i < header.metadatas_count; ++i )
	{
		uint32_t metadata_offset              = *reinterpret_cast<const uint32_t*>( lib_data + metadatas_offset + i * sizeof( uint32_t ) );
		metadata_offset                       = ( DL_ENDIAN_HOST == DL_ENDIAN_BIG ) ? dl_swap_endian_uint32( metadata_offset ) : metadata_offset;
		const dl_data_header* metadata_header = reinterpret_cast<const dl_data_header*>( lib_data + metadatas_offset + header.metadatas_count * sizeof( uint32_t ) + metadata_offset );
		size_t instance_size                  = ( DL_ENDIAN_HOST == DL_ENDIAN_BIG ) ? dl_swap_endian_uint32( metadata_header->instance_size ) : metadata_header->instance_size;
		memcpy( dl_ctx->metadatas[dl_ctx->metadatas_count + i], metadata_header, instance_size + sizeof( dl_data_header ) );
		dl_typeid_t type_id = ( DL_ENDIAN_HOST == DL_ENDIAN_BIG ) ? dl_swap_endian_uint32( metadata_header->root_instance_type ) : metadata_header->root_instance_type;
		void* loaded_instance;
		size_t consumed;
		dl_error_t err = dl_instance_load_inplace( dl_ctx, type_id, (uint8_t*)dl_ctx->metadatas[dl_ctx->metadatas_count + i], instance_size + sizeof( dl_data_header ), &loaded_instance, &consumed );
		if( err != DL_ERROR_OK )
		{
			return err;
		}
		DL_ASSERT( instance_size + sizeof( dl_data_header ) == consumed );
		dl_ctx->metadata_infos[dl_ctx->metadatas_count + i] = loaded_instance;
		dl_ctx->metadata_typeinfos[dl_ctx->metadatas_count + i] = type_id;
	}

	dl_ctx->metadatas_count       += header.metadatas_count;
	dl_ctx->metadatas_cap         = dl_ctx->metadatas_count;

	dl_internal_load_type_library_defaults( dl_ctx, lib_data + defaults_offset, header.default_value_size );
	return DL_ERROR_OK;
}
