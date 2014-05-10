#include <gtest/gtest.h>

#include "dl_test_common.h"
#include <dl/dl_typelib.h>

#include <stdio.h>

#define STRINGIFY( ... ) #__VA_ARGS__

TEST( DLTypeLib, simple_read_write )
{
	static const unsigned char small_tl[] =
	{
		#include "generated/small.bin.h"
	};

	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_type_library( ctx, small_tl, sizeof(small_tl) ) );

	// ... get size ...
	size_t tl_size = 0;
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_write_type_library( ctx, 0x0, 0, &tl_size ) );

	// ... same size before and after write ...
	EXPECT_EQ( sizeof( small_tl ), tl_size );

	unsigned char* written = (unsigned char*)malloc( tl_size + 4 );
	written[ tl_size + 0 ] = 0xFE;
	written[ tl_size + 1 ] = 0xFE;
	written[ tl_size + 2 ] = 0xFE;
	written[ tl_size + 3 ] = 0xFE;

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_write_type_library( ctx, written, tl_size, &tl_size ) );

	EXPECT_EQ( sizeof( small_tl ), tl_size );
	EXPECT_EQ( 0xFE, written[ tl_size + 0 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 1 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 2 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 3 ] );

	// ... ensure equal ...
	EXPECT_EQ( 0, memcmp( written, small_tl, sizeof( small_tl ) ) );

	free( written );

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, simple_read_write )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "name" : "member", "type" : "uint32" } ] }
		}
	});

	printf("%s\n", single_member_typelib);

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}
