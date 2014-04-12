#include <dl/dl_typelib.h>
#include "dl_binary_writer.h"

static dl_error_t dl_context_write_types( dl_ctx_t dl_ctx, dl_binary_writer* writer )
{
	(void)dl_ctx;
	(void)writer;
	// TODO: this basically just re-outputs the read lib... much work left!

	// TODO: handle endianness!

	// ... write typelookup ... // TODO: this can be removed!
//	dl_binary_writer_write( writer, dl_ctx->type_lookup, sizeof( dl_type_lookup_t ) * dl_ctx->type_count );

	// ... write types ...
//	dl_binary_writer_write( writer, dl_ctx->type_info_data, dl_ctx->type_info_data_size );

	return DL_ERROR_OK;
}

static dl_error_t dl_context_write_enums( dl_ctx_t dl_ctx, dl_binary_writer* writer )
{
	// TODO: this basically just re-outputs the read lib... much work left!

	// TODO: handle endianness!

	// ... write typelookup ... // TODO: this can be removed!
	dl_binary_writer_write( writer, dl_ctx->enum_lookup, sizeof( dl_type_lookup_t ) * dl_ctx->enum_count );

	// ... write types ...
	dl_binary_writer_write( writer, dl_ctx->enum_info_data, dl_ctx->enum_info_data_size );

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
	header.types_size = dl_ctx->type_info_data_size;
	header.enum_count = dl_ctx->enum_count;
	header.enums_size = dl_ctx->enum_info_data_size;
	header.default_value_size = 0;
	dl_binary_writer_write( &writer, &header, sizeof( header ) );

	dl_error_t err;
	err = dl_context_write_types( dl_ctx, &writer );
	if( err != DL_ERROR_OK )
		return err;

	err = dl_context_write_enums( dl_ctx, &writer );
	if( err != DL_ERROR_OK )
		return err;

	// ... write default data ...

	if( produced_bytes )
		*produced_bytes = dl_binary_writer_needed_size( &writer );

	// TODO: should write buffer to small on error.
	return DL_ERROR_OK;
}
