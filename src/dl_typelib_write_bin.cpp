#include <dl/dl_typelib.h>
#include "dl_binary_writer.h"

static dl_error_t dl_context_write_types( dl_ctx_t dl_ctx, dl_binary_writer* writer, uint32_t* type_info_size )
{
	// TODO: this basically just re-outputs the read lib... much work left!

	// TODO: handle endianness!

	dl_type_lookup_t lookup[256]; // haxxy before we fix typelib fmt
	DL_ASSERT( dl_ctx->type_count < 256 );

	size_t lookup_start = dl_binary_writer_tell( writer );
	size_t type_desc_size = dl_ctx->enum_count * sizeof( dl_type_lookup_t );
	dl_binary_writer_reserve( writer, type_desc_size );
	dl_binary_writer_seek_set( writer, lookup_start + type_desc_size );

	size_t start = dl_binary_writer_tell( writer );
	for( unsigned int i = 0; i < dl_ctx->type_count; ++i )
	{
		lookup[i].type_id = dl_ctx->type_ids[i];
		lookup[i].offset  = (uint32_t)(dl_binary_writer_tell( writer ) - start);

		const dl_type_desc* type = dl_ctx->type_descs + i;
		dl_binary_writer_write( writer, dl_ctx->type_descs + i, sizeof( dl_type_desc ) - sizeof( uint32_t ) );

		const dl_member_desc* mem = dl_get_type_member( dl_ctx, type, 0 );
		dl_binary_writer_write( writer, mem, type->member_count * sizeof( dl_member_desc ) );
	}

	dl_binary_writer_seek_set( writer, lookup_start );
	dl_binary_writer_write( writer, lookup, type_desc_size );

	dl_binary_writer_seek_end( writer );

	*type_info_size = (uint32_t)(dl_binary_writer_tell( writer ) - start);

	return DL_ERROR_OK;
}

static dl_error_t dl_context_write_enums( dl_ctx_t dl_ctx, dl_binary_writer* writer, uint32_t* enum_info_size )
{
	// TODO: this basically just re-outputs the read lib... much work left!

	// TODO: handle endianness!

	dl_type_lookup_t lookup[256]; // haxxy before we fix typelib fmt
	DL_ASSERT( dl_ctx->enum_count < 256 );

	size_t lookup_start = dl_binary_writer_tell( writer );
	size_t enum_desc_size = dl_ctx->enum_count * ( sizeof( dl_type_lookup_t ) );
	dl_binary_writer_reserve( writer, enum_desc_size );
	dl_binary_writer_seek_set( writer, lookup_start + enum_desc_size );

	size_t start = dl_binary_writer_tell( writer );
	for( unsigned int i = 0; i < dl_ctx->enum_count; ++i )
	{
		lookup[i].type_id = dl_ctx->enum_ids[i];
		lookup[i].offset  = (uint32_t)(dl_binary_writer_tell( writer ) - start);

		const dl_enum_desc* e = dl_ctx->enum_descs + i;
		dl_binary_writer_write( writer, dl_ctx->enum_descs + i, sizeof( dl_enum_desc ) - sizeof( uint32_t ) );

		const dl_enum_value_desc* val = dl_get_enum_value( dl_ctx, e, 0 );
		dl_binary_writer_write( writer, val, e->value_count * sizeof( dl_enum_value_desc ) );
	}

	dl_binary_writer_seek_set( writer, lookup_start );
	dl_binary_writer_write( writer, lookup, enum_desc_size );

	dl_binary_writer_seek_end( writer );

	*enum_info_size = (uint32_t)(dl_binary_writer_tell( writer ) - start);

	return DL_ERROR_OK;
}

dl_error_t dl_context_write_type_library( dl_ctx_t dl_ctx, unsigned char* out_lib, size_t out_lib_size, size_t* produced_bytes )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_lib, out_lib_size, out_lib == 0x0, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_32BIT );

	// ... write header ...
	// TODO: handle endianness!
	dl_typelib_header header;
	header.id         = DL_TYPELIB_ID;
	header.version    = DL_TYPELIB_VERSION;
	header.type_count = dl_ctx->type_count;
	header.enum_count = dl_ctx->enum_count;
	header.default_value_size = 0;
	dl_binary_writer_reserve( &writer, sizeof( dl_typelib_header ) );
	dl_binary_writer_seek_set( &writer, sizeof( dl_typelib_header ) );


	dl_error_t err;
	err = dl_context_write_types( dl_ctx, &writer, &header.types_size );
	if( err != DL_ERROR_OK )
		return err;

	err = dl_context_write_enums( dl_ctx, &writer, &header.enums_size );
	if( err != DL_ERROR_OK )
		return err;

	dl_binary_writer_seek_set( &writer, 0 );
	dl_binary_writer_write( &writer, &header, sizeof( header ) );

	// ... write default data ...

	if( produced_bytes )
		*produced_bytes = dl_binary_writer_needed_size( &writer );

	// TODO: should write buffer to small on error.
	return DL_ERROR_OK;
}
