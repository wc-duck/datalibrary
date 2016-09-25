#include <gtest/gtest.h>
#include "dl_tests_base.h"
#include "dl_test_common.h"

#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

void pack_text_test::do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char* out_buffer,   size_t*     out_size )
{
	// unpack binary to txt
	char text_buffer[2048];
	memset(text_buffer, 0xFE, sizeof(text_buffer));

	size_t text_size = 0;
	EXPECT_DL_ERR_OK( dl_txt_unpack_calc_size( dl_ctx, type, store_buffer, store_size, &text_size ) );
	EXPECT_DL_ERR_OK( dl_txt_unpack( dl_ctx, type, store_buffer, store_size, text_buffer, text_size, 0x0 ) );
	EXPECT_EQ( (unsigned char)0xFE, (unsigned char)text_buffer[text_size] ); // no overwrite on the generated text plox!

//		printf("%s\n", text_buffer);

	// pack txt to binary
	EXPECT_DL_ERR_OK( dl_txt_pack_calc_size( dl_ctx, text_buffer, out_size ) );
	EXPECT_DL_ERR_OK( dl_txt_pack( dl_ctx, text_buffer, out_buffer, *out_size, 0x0 ) );
}

void inplace_load_test::do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   	   	   unsigned char* store_buffer, size_t      store_size,
							   unsigned char* out_buffer,   size_t*     out_size )
{
	// copy stored instance into temp-buffer
	unsigned char inplace_buffer[4096];
	memset( inplace_buffer, 0xFE, DL_ARRAY_LENGTH(inplace_buffer) );
	memcpy( inplace_buffer, store_buffer, store_size );

	// load inplace
	void* loaded_instance = 0x0;
	size_t consumed = 0;
	EXPECT_DL_ERR_OK( dl_instance_load_inplace( dl_ctx, type, inplace_buffer, store_size, &loaded_instance, &consumed ));

	EXPECT_EQ( store_size, consumed );
	EXPECT_EQ( (unsigned char)0xFE, (unsigned char)inplace_buffer[store_size + 1] ); // no overwrite by inplace load!

	// store to out-buffer
	EXPECT_DL_ERR_OK( dl_instance_calc_size( dl_ctx, type, loaded_instance, out_size ) );
	EXPECT_DL_ERR_OK( dl_instance_store( dl_ctx, type, loaded_instance, out_buffer, *out_size, 0x0 ) );
}

void convert_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
						 unsigned char* store_buffer,  size_t      store_size,
						 unsigned char* out_buffer,    size_t*     out_size,
						 unsigned int   conv_ptr_size, dl_endian_t conv_endian )
{
	// calc size to convert
	unsigned char convert_buffer[2048];
	memset(convert_buffer, 0xFE, sizeof(convert_buffer));
	size_t convert_size = 0;
	EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, store_buffer, store_size, conv_ptr_size, &convert_size ) );

	// convert to other pointer size
	EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, store_buffer, store_size, convert_buffer, convert_size, conv_endian, conv_ptr_size, 0x0 ) );
	EXPECT_EQ( (unsigned char)0xFE, convert_buffer[convert_size] ); // no overwrite on the generated text plox!
	EXPECT_INSTANCE_INFO( convert_buffer, convert_size, conv_ptr_size, conv_endian, type );

	// calc size to re-convert to host pointer size
	EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, convert_buffer, convert_size, sizeof(void*), out_size ) );

	// re-convert to host pointer size
	EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, convert_buffer, convert_size, out_buffer, *out_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );
}

void convert_inplace_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
        						 unsigned char* store_buffer,  size_t      store_size,
								 unsigned char* out_buffer,    size_t*     out_size,
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

	unsigned char convert_buffer[2048];
	memset( convert_buffer, 0xFE, sizeof(convert_buffer) );
	size_t convert_size = 0;

	if( conv_ptr_size <= sizeof(void*) )
	{
		memcpy( convert_buffer, store_buffer, store_size );
		EXPECT_DL_ERR_OK( dl_convert_inplace( dl_ctx, type, convert_buffer, store_size, conv_endian, conv_ptr_size, &convert_size ) );
		memset( convert_buffer + convert_size, 0xFE, sizeof(convert_buffer) - convert_size );
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
		EXPECT_DL_ERR_EQ( DL_ERROR_UNSUPPORTED_OPERATION, dl_convert_inplace( dl_ctx, type, convert_buffer, store_size, DL_ENDIAN_HOST, sizeof(void*), out_size ) );

		// convert with ordinary convert
		EXPECT_DL_ERR_OK( dl_convert_calc_size( dl_ctx, type, convert_buffer, convert_size, sizeof(void*), out_size ) );
		EXPECT_DL_ERR_OK( dl_convert( dl_ctx, type, convert_buffer, convert_size, out_buffer, *out_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );
	}
	else
	{
		memcpy( out_buffer, convert_buffer, convert_size );
		EXPECT_DL_ERR_OK( dl_convert_inplace( dl_ctx, type, out_buffer, convert_size, DL_ENDIAN_HOST, sizeof(void*), out_size ) );
		memset( out_buffer + *out_size, 0xFE, OUT_BUFFER_SIZE - *out_size );
	}
}
