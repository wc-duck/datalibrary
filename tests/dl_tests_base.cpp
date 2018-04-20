#include <gtest/gtest.h>
#include "dl_tests_base.h"
#include "dl_test_common.h"

#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

void pack_text_test::do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char** out_buffer,   size_t*     out_size )
{
	// unpack binary to txt
	size_t text_size = 0;
	EXPECT_DL_ERR_OK( dl_txt_unpack_calc_size( dl_ctx, type, store_buffer, store_size, &text_size ) );
	char *text_buffer = (char*)malloc(text_size+1);
	memset(text_buffer, 0xFE, text_size+1);

	EXPECT_DL_ERR_OK( dl_txt_unpack( dl_ctx, type, store_buffer, store_size, text_buffer, text_size, 0x0 ) );
	EXPECT_EQ( (unsigned char)0xFE, (unsigned char)text_buffer[text_size] ); // no overwrite on the generated text plox!

//	printf("%s\n", text_buffer);

	// pack txt to binary
	EXPECT_DL_ERR_OK( dl_txt_pack_calc_size( dl_ctx, text_buffer, out_size ) );
	*out_buffer = (unsigned char*)malloc(*out_size+1);
	memset(*out_buffer, 0xFE, *out_size+1);

	EXPECT_DL_ERR_OK( dl_txt_pack( dl_ctx, text_buffer, *out_buffer, *out_size, 0x0 ) );

	free(text_buffer);
}

void inplace_load_test::do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   	   	   unsigned char* store_buffer, size_t      store_size,
							   unsigned char** out_buffer,   size_t*     out_size )
{
	// copy stored instance into temp-buffer
	unsigned char *inplace_buffer = (unsigned char*)malloc(store_size+1);
	memset( inplace_buffer, 0xFE, store_size+1 );
	memcpy( inplace_buffer, store_buffer, store_size );

	// load inplace
	void* loaded_instance = 0x0;
	size_t consumed = 0;
	EXPECT_DL_ERR_OK( dl_instance_load_inplace( dl_ctx, type, inplace_buffer, store_size, &loaded_instance, &consumed ));

	EXPECT_EQ( store_size, consumed );
	EXPECT_EQ( (unsigned char)0xFE, (unsigned char)inplace_buffer[store_size] ); // no overwrite by inplace load!

	// store to out-buffer
	EXPECT_DL_ERR_OK( dl_instance_calc_size( dl_ctx, type, loaded_instance, out_size ) );
	*out_buffer = (unsigned char*)malloc(*out_size + 1);
	memset(*out_buffer, 0xFE, *out_size + 1);

	EXPECT_DL_ERR_OK( dl_instance_store( dl_ctx, type, loaded_instance, *out_buffer, *out_size, 0x0 ) );
	free(inplace_buffer);
}

void convert_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
						 unsigned char* store_buffer,  size_t      store_size,
						 unsigned char** out_buffer,    size_t*     out_size,
						 unsigned int   conv_ptr_size, dl_endian_t conv_endian )
{
	// calc size to convert
	size_t convert_size = 0;
	EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, store_buffer, store_size, conv_ptr_size, &convert_size ) );
	unsigned char *convert_buffer = (unsigned char*)malloc(convert_size+1);
	memset(convert_buffer, 0xFE, convert_size+1);

	// convert to other pointer size
	EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, store_buffer, store_size, convert_buffer, convert_size, conv_endian, conv_ptr_size, 0x0 ) );
	EXPECT_EQ( (unsigned char)0xFE, convert_buffer[convert_size] ); // no overwrite on the generated text plox!
	EXPECT_INSTANCE_INFO( convert_buffer, convert_size, conv_ptr_size, conv_endian, type );

	// calc size to re-convert to host pointer size
	EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, convert_buffer, convert_size, sizeof(void*), out_size ) );
	*out_buffer = (unsigned char*)malloc(*out_size + 1);
	memset(*out_buffer, 0xFE, *out_size + 1);

	// re-convert to host pointer size
	EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, convert_buffer, convert_size, *out_buffer, *out_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );
	free(convert_buffer);
}

void convert_inplace_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
        						 unsigned char* store_buffer,  size_t      store_size,
								 unsigned char** out_buffer,    size_t*     out_size,
								 unsigned int   conv_ptr_size, dl_endian_t conv_endian )
{
	/*
		About test
			since converting pointersizes from smaller to bigger can't be done inplace this test differs
			depending on conversion.
			tests will use inplace convert if possible, otherwise it checks that the operation returns
			DL_ERROR_UNSUPORTED_OPERATION as it should.

			ie. if our test is only converting endian we do both conversions inplace, otherwise we
			do the supported operation inplace.
	*/

	size_t convert_size = 0;
	EXPECT_DL_ERR_OK(dl_convert_calc_size(dl_ctx, type, store_buffer, store_size, conv_ptr_size, &convert_size));
	size_t convert_buffer_size = store_size > convert_size ? store_size + 1 : convert_size + 1;
	unsigned char *convert_buffer = (unsigned char*)malloc(convert_buffer_size);
	memset( convert_buffer, 0xFE, convert_buffer_size);

	if( conv_ptr_size <= sizeof(void*) )
	{
		memcpy( convert_buffer, store_buffer, store_size );
		EXPECT_DL_ERR_OK( dl_convert_inplace( dl_ctx, type, convert_buffer, store_size, conv_endian, conv_ptr_size, &convert_size ) );
		memset( convert_buffer + convert_size, 0xFE, store_size - convert_size);
	}
	else
	{
		// check that error is correct
		EXPECT_DL_ERR_EQ( DL_ERROR_UNSUPPORTED_OPERATION, dl_convert_inplace( dl_ctx, type, store_buffer, store_size, conv_endian, conv_ptr_size, &convert_size ) );

		// convert with ordinary convert
		EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, store_buffer, store_size, conv_ptr_size, &convert_size ) );
		EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, store_buffer, store_size, convert_buffer, convert_size, conv_endian, conv_ptr_size, 0x0 ) );
	}

	EXPECT_EQ( (unsigned char)0xFE, convert_buffer[convert_size] ); // no overwrite on the generated text plox!
	EXPECT_INSTANCE_INFO( convert_buffer, convert_size, conv_ptr_size, conv_endian, type );


	// convert back!
	if( conv_ptr_size < sizeof(void*))
	{
		// check that error is correct
		EXPECT_DL_ERR_EQ( DL_ERROR_UNSUPPORTED_OPERATION, dl_convert_inplace( dl_ctx, type, convert_buffer, convert_size, DL_ENDIAN_HOST, sizeof(void*), out_size ) );

		// convert with ordinary convert
		EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, convert_buffer, convert_size, sizeof(void*), out_size ) );
		*out_buffer = (unsigned char*)malloc(*out_size + 1);
		memset(*out_buffer, 0xFE, *out_size + 1);

		EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, convert_buffer, convert_size, *out_buffer, *out_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );
	}
	else
	{
		EXPECT_DL_ERR_OK(dl_convert_calc_size(dl_ctx, type, convert_buffer, convert_size, sizeof(void*), out_size));
		*out_buffer = (unsigned char*)malloc(*out_size + 1);
		memset(*out_buffer, 0xFE, *out_size + 1);

		EXPECT_DL_ERR_OK( dl_convert_inplace( dl_ctx, type, convert_buffer, convert_size, DL_ENDIAN_HOST, sizeof(void*), out_size ) );
		memcpy(*out_buffer, convert_buffer, *out_size);
	}

	free(convert_buffer);
}
