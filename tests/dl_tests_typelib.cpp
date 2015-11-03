#include <gtest/gtest.h>

#include "dl_test_common.h"
#include <dl/dl_typelib.h>

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

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, missing_type )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "name" : "member", "type" : "i_do_not_exist" } ] }
		}
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, missing_comma )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [
			                   { "name" : "member1", "type" : "uint8" }
			                   { "name" : "member2", "type" : "uint8" }
			                 ] }
		}
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, crash1 )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"enums": {
			"anim_spline_type": { "ANIM_SPLINE_TYPE_ONCE": 0, "ANIM_SPLINE_TYPE_PING_PONG": 1 },
		    "timer_type": { "LOOP": 1, "ONCE": 0 }
		  },
		  "types": {
		    "a": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "b": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "c": { "members": [ { "type": "uint8", "name": "a", "default": true } ] },
		    "d": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "e": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "f": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "g": { "members": [ { "type": "uint8", "name": "a" } ] },
		    "h": { "members": [ { "type": "uint8", "name": "a" } ] }
		  },
		  "module": "component_init_data"
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, nonexisting_type )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"types" : {
			"note_t"  : { "members" : [ { "name" : "points", "type" : "int[]" } ] },
			"track_t" : { "members" : [ { "name" : "notes", "type" : "notes_t[]" } ] }
		}
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, struct_with_no_members1 )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({ "types" : { "no_members"  : {} } });

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, struct_with_no_members2 )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({ "types" : { "no_members"  : { "members" : [] } } });

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}

TEST( DLTypeLibTxt, invalid_default_value_array )
{
	dl_ctx_t ctx;

	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &ctx, &p ) );

	const char single_member_typelib[] = STRINGIFY({
		"types" : {
			"note_t"  : { "members" : [ { "name" : "points", "type" : "int32", "default" : [] } ] }
		}
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_INVALID_DEFAULT_VALUE, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_destroy( ctx ) );
}
