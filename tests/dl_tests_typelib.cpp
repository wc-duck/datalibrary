#include <gtest/gtest.h>

#include "dl_test_common.h"
#include <dl/dl_typelib.h>
#include <dl/dl_txt.h>
#include <dl/dl_reflect.h>

#define STRINGIFY( ... ) #__VA_ARGS__

#include <stdio.h>
static void error_msg_handler( const char* msg, void* )
{
    static bool print_error_msg = false;
    if(print_error_msg)
        printf("%s\n", msg);
}

static void test_log_error( const char* msg, void* )
{
	printf( "%s\n", msg );
}

struct DLTypeLib : public ::testing::Test
{
	dl_ctx_t ctx;

	void SetUp()
	{
		dl_create_params_t p;
		DL_CREATE_PARAMS_SET_DEFAULT(p);
        p.error_msg_func = error_msg_handler;
		EXPECT_DL_ERR_OK( dl_context_create( &ctx, &p ) );
	}

	void TearDown()
	{
		EXPECT_DL_ERR_OK( dl_context_destroy( ctx ) );
	}
};

struct DLTypeLibTxt : public DLTypeLib {};
struct DLTypeLibUnpackTxt : public DLTypeLib {};

TEST_F( DLTypeLib, simple_read_write )
{
	static const unsigned char small_tl[] =
	{
		#include "generated/small.bin.h"
	};

	// ... load typelib ...
	EXPECT_DL_ERR_OK( dl_context_load_type_library( ctx, small_tl, sizeof(small_tl) ) );

	// ... get size ...
	size_t tl_size = 0;
	EXPECT_DL_ERR_OK( dl_context_write_type_library( ctx, 0x0, 0, &tl_size ) );

	// ... same size before and after write ...
	EXPECT_EQ( sizeof( small_tl ), tl_size );

	unsigned char* written = (unsigned char*)malloc( tl_size + 4 );
	written[ tl_size + 0 ] = 0xFE;
	written[ tl_size + 1 ] = 0xFE;
	written[ tl_size + 2 ] = 0xFE;
	written[ tl_size + 3 ] = 0xFE;

	EXPECT_DL_ERR_OK( dl_context_write_type_library( ctx, written, tl_size, &tl_size ) );

	EXPECT_EQ( sizeof( small_tl ), tl_size );
	EXPECT_EQ( 0xFE, written[ tl_size + 0 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 1 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 2 ] );
	EXPECT_EQ( 0xFE, written[ tl_size + 3 ] );

	// ... ensure equal ...
	EXPECT_EQ( 0, memcmp( written, small_tl, sizeof( small_tl ) ) );

	free( written );
}

TEST_F( DLTypeLibTxt, simple_read_write )
{
	const char single_member_typelib[] = STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "name" : "member", "type" : "uint32" } ] }
		}
	});

	// ... load typelib ...
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1, 0 ) );
}

static void typelibtxt_expect_error( dl_ctx_t ctx, dl_error_t expect, const char* libstr )
{
	EXPECT_DL_ERR_EQ( expect, dl_context_load_txt_type_library( ctx, libstr, strlen(libstr), 0 ) );
}

TEST_F( DLTypeLibTxt, member_missing_type )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TYPE_NOT_FOUND, STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "name" : "member", "type" : "i_do_not_exist" } ] }
		}
	}));
}

TEST_F( DLTypeLibTxt, member_missing_type_2 )
{
	typelibtxt_expect_error( ctx, DL_ERROR_MALFORMED_DATA, STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "name" : "member" } ] }
		}
	}));
}

TEST_F( DLTypeLibTxt, member_missing_name )
{
	typelibtxt_expect_error( ctx, DL_ERROR_MALFORMED_DATA, STRINGIFY({
		"module" : "small",
		"types" : {
			"single_int" : { "members" : [ { "type" : "uint32" } ] }
		}
	}));
}


TEST_F( DLTypeLibTxt, missing_comma )
{
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
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1, 0 ) );
}

TEST_F( DLTypeLibTxt, crash1 )
{
	const char single_member_typelib[] = STRINGIFY({
		"enums": {
			"anim_spline_type": { "values" : { "ANIM_SPLINE_TYPE_ONCE": 0, "ANIM_SPLINE_TYPE_PING_PONG": 1 } },
		    "timer_type": { "values" : { "LOOP": 1, "ONCE": 0 } }
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
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx, single_member_typelib, sizeof(single_member_typelib)-1, 0 ) );
}

TEST_F( DLTypeLibTxt, nonexisting_type )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TYPE_NOT_FOUND, STRINGIFY({
		"types" : {
			"note_t"  : { "members" : [ { "name" : "points", "type" : "int[]" } ] },
			"track_t" : { "members" : [ { "name" : "notes", "type" : "notes_t[]" } ] }
		}
	}));
}

TEST_F( DLTypeLibTxt, empty_types )
{
	typelibtxt_expect_error( ctx, DL_ERROR_OK, STRINGIFY({ "enums" : { "e" : { "values" : { "a" : 1 } } }, "types" : {} }) );
}

TEST_F( DLTypeLibTxt, empty_enums )
{
	typelibtxt_expect_error( ctx, DL_ERROR_OK, STRINGIFY({ "enums" : {}, "types" : { "t" : { "members" : [ { "name" : "ok", "type" : "int32" } ] } } }) );
}

TEST_F( DLTypeLibTxt, struct_with_no_members1 )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, STRINGIFY({ "types" : { "no_members"  : {} } }) );
}

TEST_F( DLTypeLibTxt, struct_with_no_members2 )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, STRINGIFY({ "types" : { "no_members"  : { "members" : [] } } }) );
}

TEST_F( DLTypeLibTxt, invalid_default_value_array )
{
	typelibtxt_expect_error( ctx, DL_ERROR_INVALID_DEFAULT_VALUE, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int32", "default" : [] } ] } } }));
}

TEST_F( DLTypeLibTxt, invalid_alias_array )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, 
		STRINGIFY( { 
			"enums" : {
				"e" : {
					"values" : {
						 "v" : { "value" : 1, "aliases" : [ "apa" }
					}
				} 
			}
		})
	);

	typelibtxt_expect_error( ctx, DL_ERROR_MALFORMED_DATA,
		STRINGIFY( { 
			"enums" : {
				"e" : {
					"values" : {
						"v" : { "aliases" : [ "apa" ] } 
					}
				} 
			}
		} )
	);
}

TEST_F( DLTypeLibTxt, invalid_type_fmt_array )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "single_int *" } ] } } }) ); // extra space!
    typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "single_int#"  } ] } } }) ); // bad char!
}

TEST_F( DLTypeLibTxt, invalid_type_fmt_inline_array )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_INVALID_ENUM_VALUE, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int32[a]" } ] } } }) );
}

TEST_F( DLTypeLibTxt, invalid_type_fmt_bitfield )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "bitfield:"   } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "bitfield:a"  } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "bitfield<1"  } ] } } }) );
    typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "bitfield:1[]"  } ] } } }) );
    typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "bitfield:1[123]"  } ] } } }) );
    typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "single_int:" } ] } } }) ); // ':' found when type is not bitfield
}

TEST_F( DLTypeLibTxt, empty_typelib )
{
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_context_load_txt_type_library( ctx, 0x0, 0, 0 ) );
}

TEST_F( DLTypeLibTxt, invalid_type_fmt_pointer_to_pod )
{
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int8*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int16*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int32*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "int64*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "uint8*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "uint16*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "uint32*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "uint64*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "fp32*" } ] } } }) );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, STRINGIFY({ "types" : { "t"  : { "members" : [ { "name" : "m", "type" : "fp64*" } ] } } }) );
}

static char* dl_test_build_many_members(int member_cnt)
{
	char* typelib = (char*)malloc(100 * 1024); // should be enough for anyone!?! (I trust valgrind to help me out here!)
	typelib[0] = '\0';
	strcat(typelib, STRINGIFY( { "types" : { 
									"t" : { 
										"members" : [));

	for(int i = 0; i < member_cnt; ++i)
	{
		char mem[256];
		snprintf(mem, sizeof(mem), "\n\t{ \"name\" : \"m%d\", \"type\" : \"int8\" }", i);
		strcat(typelib, mem);
		if(i != member_cnt -1)
		strcat(typelib, ",");
	}
	strcat(typelib, "] } } }");
	return typelib;
}

TEST_F( DLTypeLibTxt, invalid_typelib_too_many_members)
{
	char* typelib = dl_test_build_many_members(DL_MEMBERS_IN_TYPE_MAX);

	typelibtxt_expect_error( ctx, DL_ERROR_OK, typelib );
	free(typelib);

	typelib = dl_test_build_many_members(DL_MEMBERS_IN_TYPE_MAX + 1);
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, typelib );
	free(typelib);
}

TEST_F( DLTypeLibTxt, inline_array_too_big)
{
	char typelib[2048];
	snprintf(typelib, sizeof(typelib), STRINGIFY({
		"types" : { "t" : { "members" : [{ "name" : "a", "type" : "int8[%d]" }]}} }),
		 DL_INLINE_ARRAY_LENGTH_MAX-1 );
	typelibtxt_expect_error( ctx, DL_ERROR_OK, typelib );

	snprintf(typelib, sizeof(typelib), STRINGIFY({
		"types" : { "t" : { "members" : [{ "name" : "a", "type" : "int8[%d]" }]}} }),
		 DL_INLINE_ARRAY_LENGTH_MAX );
	typelibtxt_expect_error( ctx, DL_ERROR_TXT_PARSE_ERROR, typelib );
}

TEST_F( DLTypeLibUnpackTxt, round_about )
{
	const char* testlib1 = STRINGIFY({
		"enums" : {
			"e1" : { 
				"values" : { "val1" : 1, "val2" : 2 }
			},
			"e2" : { 
				"values" : { "val3" : 3, "val2" : 4 }
			}
		},
		"types" : {
			"t1" : {
				"members" : [
					 { "name" : "m1", "type" : "int8" },
					 { "name" : "m2", "type" : "int8" }
				]
			},
			"t2" : {
				"members" : [
					 { "name" : "m3", "type" : "int8" },
					 { "name" : "m4", "type" : "int8" }
				]
			}
		}
	});

	// ... pack from text ...
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx, testlib1, sizeof(testlib1)-1, 0 ) );

	size_t txt_size = 0;
	char testlib_txt_buffer[2048];
	EXPECT_DL_ERR_OK( dl_context_write_txt_type_library( ctx, testlib_txt_buffer, sizeof(testlib_txt_buffer), &txt_size ) );

	dl_ctx_t ctx2;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = test_log_error;
	EXPECT_DL_ERR_OK( dl_context_create( &ctx2, &p ) );
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx2, testlib_txt_buffer, txt_size, 0 ) );

	// ... check that typelib contain the enum and the type ...
	dl_type_context_info_t info;
	EXPECT_DL_ERR_OK( dl_reflect_context_info( ctx2, &info ) );
	EXPECT_EQ( 2u, info.num_types );
	EXPECT_EQ( 2u, info.num_enums );

	EXPECT_DL_ERR_OK(dl_context_destroy( ctx2 ));
}

TEST_F( DLTypeLibUnpackTxt, round_about_big )
{
	const char testlib1[] = {
		#include "generated/unittest.txt.h"
	};
	dl_include_handler include_handler = []( void* handler_ctx, dl_ctx_t dl_ctx, const char* file_to_include ) -> dl_error_t {
		const char testlib2[] = {
			#include "generated/to_include.txt.h"
		};
		EXPECT_STREQ( "../local/generated/to_include.bin", file_to_include );
		EXPECT_EQ( (void*)1, handler_ctx );
		return dl_context_load_txt_type_library( dl_ctx, testlib2, sizeof(testlib2) - 1, 0 );
	};

	// ... pack from text ...
	dl_load_type_library_params_t load_params{ include_handler, (void*)1 };
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx, testlib1, sizeof(testlib1) - 1, &load_params ) );

	size_t txt_buffer_size = 4096*8;
	size_t txt_size = 0;
	char* testlib_txt_buffer = (char*)malloc(txt_buffer_size);
	EXPECT_DL_ERR_OK( dl_context_write_txt_type_library( ctx, testlib_txt_buffer, txt_buffer_size, &txt_size ) );

	dl_ctx_t ctx2;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = test_log_error;
	EXPECT_DL_ERR_OK( dl_context_create( &ctx2, &p ) );
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx2, testlib_txt_buffer, txt_size, 0 ) );

	free((void*)testlib_txt_buffer);

	EXPECT_DL_ERR_OK(dl_context_destroy( ctx2 ));
}


static uint8_t* test_pack_txt_type_lib( const char* lib_txt, size_t lib_txt_size, size_t* out_size )
{
	dl_ctx_t ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = test_log_error;
	EXPECT_DL_ERR_OK( dl_context_create( &ctx, &p ) );
	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx, lib_txt, lib_txt_size, 0 ) );

	EXPECT_DL_ERR_OK( dl_context_write_type_library( ctx, 0x0, 0, out_size ) );
	uint8_t* packed = (uint8_t*)malloc(*out_size);
	EXPECT_DL_ERR_OK( dl_context_write_type_library( ctx, packed, *out_size, 0x0 ) );

	EXPECT_DL_ERR_OK(dl_context_destroy( ctx ));
	return packed;
}

TEST_F( DLTypeLib, enum_in_2_tlds )
{
	// capturing bug where enum-values would not be loaded as expected in binary tld.
	// alias-offests were not patched correctly.

	const char typelib1[] = STRINGIFY({ "module" : "tl1", "enums" : { "e1" : { "values" : { "e1_v1" : 1, "e1_v2" : 2 } } }, "types" : { "tl1_type" : { "members" : [ { "name" : "m1", "type" : "e1" } ] } } });
	const char typelib2[] = STRINGIFY({ "module" : "tl2", "enums" : { "e2" : { "values" : { "e2_v1" : 3, "e2_v2" : 4 } } }, "types" : { "tl2_type" : { "members" : [ { "name" : "m2", "type" : "e2" } ] } } });

	size_t tl1_size;
	size_t tl2_size;
	uint8_t* tl1 = test_pack_txt_type_lib( typelib1, sizeof(typelib1)-1, &tl1_size );
	uint8_t* tl2 = test_pack_txt_type_lib( typelib2, sizeof(typelib2)-1, &tl2_size );

	EXPECT_DL_ERR_OK( dl_context_load_type_library( ctx, tl1, tl1_size ) );
	EXPECT_DL_ERR_OK( dl_context_load_type_library( ctx, tl2, tl2_size ) );

	uint8_t outbuf[256];
	const char test1[] = STRINGIFY( { "tl1_type" : { "m1" : "e1_v1" } } );
	const char test2[] = STRINGIFY( { "tl2_type" : { "m2" : "e2_v1" } } );
	EXPECT_DL_ERR_OK( dl_txt_pack( ctx, test1, outbuf, sizeof(outbuf), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_txt_pack( ctx, test2, outbuf, sizeof(outbuf), 0x0 ) );

	free(tl1);
	free(tl2);
}

// test read-errors for enum

// invalid type

// enum that do not fit in type!

TEST_F( DLTypeLib, defaults_in_2_tlds )
{
	// capturing bug where default values in a second tld would generate DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS if the 
	// first tld had default values.	

	const char typelib1[] = STRINGIFY({ "module" : "tl1", "types" : { "tl1_type" : { "members" : [ { "name" : "m1", "type" : "fp32", "default" : 1.0 } ] } } });
	const char typelib2[] = STRINGIFY({ "module" : "tl2", "types" : { "tl2_type" : { "members" : [ { "name" : "m2", "type" : "fp32", "default" : 2.0 } ] } } });

	size_t tl1_size;
	size_t tl2_size;
	uint8_t* tl1 = test_pack_txt_type_lib( typelib1, sizeof(typelib1)-1, &tl1_size );
	uint8_t* tl2 = test_pack_txt_type_lib( typelib2, sizeof(typelib2)-1, &tl2_size );

	EXPECT_DL_ERR_OK( dl_context_load_type_library( ctx, tl1, tl1_size ) );
	EXPECT_DL_ERR_OK( dl_context_load_type_library( ctx, tl2, tl2_size ) );

	free(tl1);
	free(tl2);
}

TEST_F( DLTypeLibTxt, DISABLED_default_inl_arr_of_bits )
{
	// Only one default value for an inline array of two
	const char json_typelib[] = STRINGIFY( {
		"types": {
			"BitField": { "members": [ { "name": "bits", "type": "bitfield:1" } ] },
			"InlineArray": { "members": [ { "name": "seeds", "type": "BitField[2]", "default": [ { "bits": 1 } ] } ] }
		}
	} );

	EXPECT_DL_ERR_OK( dl_context_load_txt_type_library( ctx, json_typelib, sizeof( json_typelib ) - 1, 0 ) );
}
