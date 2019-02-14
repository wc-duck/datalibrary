/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>

#include "dl_test_common.h"

#include <math.h> // isnan
#include <limits>

#if defined(_MSC_VER)
#  include <float.h> // isnan
#  define isnan _isnan
#endif

// TODO: add test for default values for uint*[] with true/false.

class DLText : public DL {};

TEST_F(DLText, member_order)
{
	// test to pack a txt-instance that is not in order!
	const char* text_data = STRINGIFY(
		{
			"Pods" : {
			"i16" : 2,
			"u32" : 7,
			"u8"  : 5,
			"i32" : 3,
			"f64" : 4.1234,
			"i64" : 4,
			"u16" : 6,
			"u64" : 8,
			"i8"  : 1,
			"f32" : 3.14
			}
		}
	);

	Pods p1;

 	unsigned char out_data_text[1024];

	ASSERT_DL_ERR_OK(dl_txt_pack( Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	ASSERT_DL_ERR_OK(dl_instance_load( Ctx, Pods::TYPE_ID, &p1, sizeof(Pods), out_data_text, sizeof(out_data_text), 0x0 ));

	EXPECT_EQ(1,      p1.i8);
 	EXPECT_EQ(2,      p1.i16);
	EXPECT_EQ(3,      p1.i32);
	EXPECT_EQ(4,      p1.i64);
	EXPECT_EQ(5u,     p1.u8);
	EXPECT_EQ(6u,     p1.u16);
	EXPECT_EQ(7u,     p1.u32);
	EXPECT_EQ(8u,     p1.u64);
	EXPECT_EQ(3.14f,  p1.f32);
	EXPECT_EQ(4.1234, p1.f64);
}

TEST_F(DLText, set_member_twice)
{
	// test to pack a txt-instance that is not in order!
	const char* text_data = STRINGIFY(
		{
			"Pods" : {
				"i16" : 2,
				"u32" : 7,
				"u8"  : 5,
				"i32" : 3,
				"i32" : 7,
				"f64" : 4.1234,
				"i64" : 4,
				"u16" : 6,
				"u64" : 8,
				"i8"  : 1,
				"f32" : 3.14
			}
		}
	);

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MEMBER_SET_TWICE, dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
}

TEST_F(DLText, tight_text)
{
	const char* text_data[] = {
		STRINGIFY({"Pods":{"i16":2,"u32":7,"u8":5,"i32":3,"f64":4.1234,"i64":4,"u16":6,"u64":8,"i8":1,"f32":3.14}}),
		STRINGIFY({Pods:{i16:2,u32:7,u8:5,i32:3,f64:4.1234,i64:4,u16:6,u64:8,i8:1,f32:3.14}})
	};

	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(text_data); ++test)
	{
		unsigned char unpack_buffer[1024];
		Pods* p1 = dl_txt_test_pack_text<Pods>(Ctx, text_data[test], unpack_buffer, sizeof(unpack_buffer));

		EXPECT_EQ(1,      p1->i8);
		EXPECT_EQ(2,      p1->i16);
		EXPECT_EQ(3,      p1->i32);
		EXPECT_EQ(4,      p1->i64);
		EXPECT_EQ(5u,     p1->u8);
		EXPECT_EQ(6u,     p1->u16);
		EXPECT_EQ(7u,     p1->u32);
		EXPECT_EQ(8u,     p1->u64);
		EXPECT_EQ(3.14f,  p1->f32);
		EXPECT_EQ(4.1234, p1->f64);
	}
}


TEST_F(DLText, member_not_exist)
{
	// test to pack a txt-instance that is not in order!
	const char* text_data = STRINGIFY( { "Pods2" : { "Int1" : 1337, "Int2" : 1337, "IDoNotExist" : 1337 } } );

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_INVALID_MEMBER, dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
}

TEST_F(DLText, type_missing)
{
	const char* text_data = STRINGIFY( { { "Int1" : 1337 } } );

	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ(DL_ERROR_MALFORMED_DATA, dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
}

TEST_F(DLText, member_missing)
{
	// error should be cast if member is not set and has no default value!
	// in this case Int2 is not set!

	const char* text_data = STRINGIFY( { "Pods2" : { "Int1" : 1337 } } );

	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MISSING_MEMBER, dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
}

TEST_F(DLText, default_value_pod)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "PodsDefaults" : {} } );

	unsigned char out_data_text[1024];
	PodsDefaults loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, PodsDefaults::TYPE_ID, &loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(2,     loaded.i8);
	EXPECT_EQ(3,     loaded.i16);
	EXPECT_EQ(4,     loaded.i32);
	EXPECT_EQ(5,     loaded.i64);
	EXPECT_EQ(6u,    loaded.u8);
	EXPECT_EQ(7u,    loaded.u16);
	EXPECT_EQ(8u,    loaded.u32);
	EXPECT_EQ(9u,    loaded.u64);
	EXPECT_EQ(10.0f, loaded.f32);
	EXPECT_EQ(11.0,  loaded.f64);
}

TEST_F(DLText, default_value_string)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultStr" : {} } );

	unsigned char out_data_text[1024];
	DefaultStr loaded[10]; // this is so ugly!

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultStr::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_STREQ("cowbells ftw!", loaded[0].Str);
}

TEST_F(DLText, default_value_with_data_in_struct)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultWithOtherDataBefore" : { "t1" : "apa" } } );

	unsigned char out_data_text[1024];
	DefaultWithOtherDataBefore loaded[10]; // this is so ugly!

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultWithOtherDataBefore::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_STREQ("apa", loaded[0].t1);
	EXPECT_STREQ("who", loaded[0].Str);
}

TEST_F(DLText, default_value_ptr)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultPtr" : {} } );

	unsigned char out_data_text[1024];
	DefaultPtr P1 = { 0 }; // this is so ugly!

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultPtr::TYPE_ID, &P1, sizeof(DefaultPtr), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(0x0, P1.Ptr);
}

TEST_F(DLText, zero_as_ptr_fail)
{
	// when referring to the root item, item 0, the types need to match.

	const char* text_data = STRINGIFY( { "DefaultPtr" : { "Ptr" : 0 } } );
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_INVALID_MEMBER_TYPE, dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
}

TEST_F(DLText, default_value_struct)
{
	const char* text_data = STRINGIFY( { "DefaultStruct" : {} } );

	unsigned char out_data_text[1024];
	DefaultStruct loaded; // this is so ugly!

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultStruct::TYPE_ID, &loaded, sizeof(DefaultStruct), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(13u, loaded.Struct.Int1);
	EXPECT_EQ(37u, loaded.Struct.Int2);
}

TEST_F(DLText, default_value_bitfield)
{
	const char* text_data = STRINGIFY( { "BitfieldDefaultsMulti" : {} } );

	unsigned char out_data_text[1024];
	BitfieldDefaultsMulti loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, BitfieldDefaultsMulti::TYPE_ID, &loaded, sizeof(BitfieldDefaultsMulti), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(0, loaded.f1);
	EXPECT_EQ(1, loaded.f2);
	EXPECT_EQ(0, loaded.f3);
	EXPECT_EQ(1, loaded.f4);
	EXPECT_EQ(2, loaded.f5);
	EXPECT_EQ(3, loaded.f6);
}

TEST_F(DLText, default_value_bitfield_bool)
{
	const char* text_data = STRINGIFY( { "BitfieldDefaultsMulti" : { "f1" : true, "f5" : true } } );

	unsigned char out_data_text[1024];
	BitfieldDefaultsMulti loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, BitfieldDefaultsMulti::TYPE_ID, &loaded, sizeof(BitfieldDefaultsMulti), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(1, loaded.f1);
	EXPECT_EQ(1, loaded.f2);
	EXPECT_EQ(0, loaded.f3);
	EXPECT_EQ(1, loaded.f4);
	EXPECT_EQ(1, loaded.f5);
	EXPECT_EQ(3, loaded.f6);
}

TEST_F(DLText, default_value_enum)
{
	const char* text_data = STRINGIFY( { "DefaultEnum" : {} } );

	unsigned char out_data_text[1024];
	DefaultEnum loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultEnum::TYPE_ID, &loaded, sizeof(DefaultEnum), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(TESTENUM1_VALUE3, loaded.Enum);
}

TEST_F(DLText, default_value_inline_array_pod)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultInlArrayPod" : {} } );

	unsigned char out_data_text[1024];
	DefaultInlArrayPod loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultInlArrayPod::TYPE_ID, &loaded, sizeof(DefaultInlArrayPod), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(1u, loaded.Arr[0]);
	EXPECT_EQ(3u, loaded.Arr[1]);
	EXPECT_EQ(3u, loaded.Arr[2]);
	EXPECT_EQ(7u, loaded.Arr[3]);
}

TEST_F(DLText, default_value_inline_array_enum)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultInlArrayEnum" : {} } );

	unsigned char out_data_text[1024];
	DefaultInlArrayEnum loaded;

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultInlArrayEnum::TYPE_ID, &loaded, sizeof(DefaultInlArrayEnum), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(TESTENUM1_VALUE3, loaded.Arr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, loaded.Arr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, loaded.Arr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, loaded.Arr[3]);
}

TEST_F(DLText, default_value_inline_array_string)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultInlArrayStr" : {} } );

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));

	DefaultInlArrayStr loaded[10];

	// load binary
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultInlArrayStr::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_STREQ("cow",   loaded[0].Arr[0]);
	EXPECT_STREQ("bells", loaded[0].Arr[1]);
	EXPECT_STREQ("are",   loaded[0].Arr[2]);
	EXPECT_STREQ("cool",  loaded[0].Arr[3]);
}

TEST_F(DLText, default_value_inline_array_array)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultInlArrayArray" : {} } );

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));

	DefaultInlArrayArray loaded[10];

	// load binary
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultInlArrayArray::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ( 2u, loaded[0].Arr[0].u32_arr.count );
	EXPECT_EQ( 2u, loaded[0].Arr[1].u32_arr.count );

	EXPECT_EQ( 1u, loaded[0].Arr[0].u32_arr[0] );
	EXPECT_EQ( 3u, loaded[0].Arr[0].u32_arr[1] );
	EXPECT_EQ( 3u, loaded[0].Arr[1].u32_arr[0] );
	EXPECT_EQ( 7u, loaded[0].Arr[1].u32_arr[1] );
}

TEST_F(DLText, default_value_array_pod)
{
	const char* text_data = STRINGIFY( { "DefaultArrayPod" : {} } );

	unsigned char out_data_text[1024];
	DefaultArrayPod loaded[10];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultArrayPod::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(4u, loaded[0].Arr.count);

	EXPECT_EQ(1u, loaded[0].Arr[0]);
	EXPECT_EQ(3u, loaded[0].Arr[1]);
	EXPECT_EQ(3u, loaded[0].Arr[2]);
	EXPECT_EQ(7u, loaded[0].Arr[3]);
}

TEST_F(DLText, default_value_array_enum)
{
	const char* text_data = STRINGIFY( { "DefaultArrayEnum" : {} } );

	unsigned char out_data_text[1024];
	DefaultArrayEnum loaded[10];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultArrayEnum::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(4u, loaded[0].Arr.count);

	EXPECT_EQ(TESTENUM1_VALUE3, loaded[0].Arr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, loaded[0].Arr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, loaded[0].Arr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, loaded[0].Arr[3]);
}

TEST_F(DLText, default_value_array_string)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultArrayStr" : {} } );

	unsigned char out_data_text[1024];
	DefaultArrayStr loaded[10];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultArrayStr::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ(4u, loaded[0].Arr.count);

	EXPECT_STREQ("cow",   loaded[0].Arr[0]);
	EXPECT_STREQ("bells", loaded[0].Arr[1]);
	EXPECT_STREQ("are",   loaded[0].Arr[2]);
	EXPECT_STREQ("cool",  loaded[0].Arr[3]);
}

TEST_F(DLText, default_value_array_array)
{
	// default-values should be set correctly!

	const char* text_data = STRINGIFY( { "DefaultArrayArray" : {} } );

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, out_data_text, sizeof(out_data_text), 0x0));

	DefaultArrayArray loaded[10];

	// load binary
	EXPECT_DL_ERR_OK(dl_instance_load(Ctx, DefaultArrayArray::TYPE_ID, loaded, sizeof(loaded), out_data_text, sizeof(out_data_text), 0x0));

	EXPECT_EQ( 2u, loaded[0].Arr.count );
	EXPECT_EQ( 2u, loaded[0].Arr[0].u32_arr.count );
	EXPECT_EQ( 2u, loaded[0].Arr[1].u32_arr.count );

	EXPECT_EQ( 1u, loaded[0].Arr[0].u32_arr[0] );
	EXPECT_EQ( 3u, loaded[0].Arr[0].u32_arr[1] );
	EXPECT_EQ( 3u, loaded[0].Arr[1].u32_arr[0] );
	EXPECT_EQ( 7u, loaded[0].Arr[1].u32_arr[1] );
}

TEST_F( DLText, array_struct )
{
	const char* text_data = STRINGIFY( { "Pods" : [ -1, -2, -3, -4, 1, 2, 3, 4, 2.3, 3.4 ] } );

	Pods loaded;
	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, Pods::TYPE_ID, &loaded, sizeof(loaded), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );

	EXPECT_EQ(  -1,  loaded.i8  );
	EXPECT_EQ(  -2,  loaded.i16 );
	EXPECT_EQ(  -3,  loaded.i32 );
	EXPECT_EQ(  -4,  loaded.i64 );
	EXPECT_EQ(  1u,  loaded.u8 );
	EXPECT_EQ(  2u,  loaded.u16 );
	EXPECT_EQ(  3u,  loaded.u32 );
	EXPECT_EQ(  4u,  loaded.u64 );
	EXPECT_EQ( 2.3f, loaded.f32 );
	EXPECT_EQ( 3.4,  loaded.f64 );
}

TEST_F( DLText, array_struct_in_struct )
{
	const char* text_data = STRINGIFY(
		{
			"Pod2InStruct" : {
				"Pod1" : [1,2],
				"Pod2" : [3,4]
			}
		}
	);

	Pod2InStruct loaded;
	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, Pod2InStruct::TYPE_ID, &loaded, sizeof(loaded), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );

	EXPECT_EQ( 1u, loaded.Pod1.Int1 );
	EXPECT_EQ( 2u, loaded.Pod1.Int2 );
	EXPECT_EQ( 3u, loaded.Pod2.Int1 );
	EXPECT_EQ( 4u, loaded.Pod2.Int2 );
}

TEST_F( DLText, array_struct_in_array_struct )
{
	const char* text_data = STRINGIFY( { "Pod2InStruct" : [ [1,2], [3,4] ] } );

	Pod2InStruct loaded;
	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, Pod2InStruct::TYPE_ID, &loaded, sizeof(loaded), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );

	EXPECT_EQ( 1u, loaded.Pod1.Int1 );
	EXPECT_EQ( 2u, loaded.Pod1.Int2 );
	EXPECT_EQ( 3u, loaded.Pod2.Int1 );
	EXPECT_EQ( 4u, loaded.Pod2.Int2 );
}

TEST_F( DLText, array_struct_missing_member )
{
	const char* text_data = STRINGIFY( { "Pods" : [ -1, -2, -3, -4, 1, 2, 3, 4 ] } );
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_MISSING_MEMBER, dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, array_struct_wrong_type )
{
	const char* text_data = STRINGIFY( { "Pods" : [ -1, -2, -3, -4, 1, 2, 3, 4, "whooo" ] } );
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_MALFORMED_DATA, dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}


TEST_F( DLText, test_alias )
{
	const char* text_data = STRINGIFY(
		{
			"with_alias_enum" : {
				"e1" : "MULTI_ALIAS1",
				"e2" : "alias1",
				"e3" : "alias2",
				"e4" : "alias4"
			}
		}
	);
	with_alias_enum loaded;
	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, with_alias_enum::TYPE_ID, &loaded, sizeof(loaded), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );

	EXPECT_EQ( MULTI_ALIAS1, loaded.e1 );
	EXPECT_EQ( MULTI_ALIAS1, loaded.e2 );
	EXPECT_EQ( MULTI_ALIAS1, loaded.e3 );
	EXPECT_EQ( MULTI_ALIAS2, loaded.e4 );
}

TEST_F( DLText, invalid_enum_value )
{
	const char* text_data = STRINGIFY(
		{
			"with_alias_enum" : {
				"e1" : "MULTI_ALIAS1",
				"e2" : "I do not exist",
				"e3" : "alias2",
				"e4" : "alias4"
			}
		}
	);
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_INVALID_ENUM_VALUE, dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, invalid_data_format )
{
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, STRINGIFY( [] ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, STRINGIFY( { "Pods" : 1 }    ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, STRINGIFY( { "Pods" : true } ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, multiple_union_members_set )
{
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_MULTIPLE_MEMBERS_IN_UNION_SET, dl_txt_pack( Ctx, STRINGIFY( { "test_union_simple" : { "item1" : 1, "item2" : 2 } } ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, invalid_underscore_member )
{
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_INVALID_MEMBER, dl_txt_pack( Ctx, STRINGIFY( { "Pods" : { "__whooo" : 1 } } ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, bad_root_type )
{
	unsigned char out_data_text[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_txt_pack( Ctx, STRINGIFY( { "PodsWHOOO" : { "test" : 1 } } ), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
}

TEST_F( DLText, basic_union_type_assignment )
{
	const char* text_data = STRINGIFY(
	{
		"struct_with_array_of_weird_unions" : {
			"event_array" : [
				{
					"delay" : 0,
					"effect" : {
						"hide_all_meshes" : {
						}
					}
				},
				{
					"delay" : 0
					"effect" : {
						"spawn_particles" : {
							"my_variable_1" : 12345678910,
							"my_variable_3" : 1
						}
					}
				}
			]
		}
	}
	);
	uint64_t data_buffer[128];

	unsigned char out_data_text[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, text_data, out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, struct_with_array_of_weird_unions::TYPE_ID, data_buffer, DL_ARRAY_LENGTH(data_buffer), out_data_text, DL_ARRAY_LENGTH(out_data_text), 0x0 ) );
	void* temp_ptr = data_buffer;
	struct_with_array_of_weird_unions *t1 = (struct_with_array_of_weird_unions *)temp_ptr;
	EXPECT_EQ(union_with_weird_members_type_hide_all_meshes, t1->event_array[0].effect.type);
}

TEST_F( DLText, basic_bool_all_true )
{
	const char* all_true_text = STRINGIFY(
		{
			"Pods" : {
				"u8"  : true,
				"u16" : true,
				"u32" : true,
				"u64" : true,
				"i8"  : true,
				"i16" : true,
				"i32" : true,
				"i64" : true,
				"f32" : 0.0,
				"f64" : 0.0
			}
		}
	);

	Pods p1;

	unsigned char out_text_data[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, all_true_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, Pods::TYPE_ID, &p1, sizeof(Pods), out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );

	EXPECT_EQ(1,    p1.i8);
	EXPECT_EQ(1,    p1.i16);
	EXPECT_EQ(1,    p1.i32);
	EXPECT_EQ(1,    p1.i64);
	EXPECT_EQ(1u,   p1.u8);
	EXPECT_EQ(1u,   p1.u16);
	EXPECT_EQ(1u,   p1.u32);
	EXPECT_EQ(1u,   p1.u64);
	EXPECT_EQ(0.0f, p1.f32);
	EXPECT_EQ(0.0,  p1.f64);
}

TEST_F( DLText, basic_bool_all_false )
{
	const char* all_false_text = STRINGIFY(
		{
			"Pods" : {
			"u8"  : false,
			"u16" : false,
			"u32" : false,
			"u64" : false,
			"i8"  : false,
			"i16" : false,
			"i32" : false,
			"i64" : false,
			"f32" : 0.0,
			"f64" : 0.0
			}
		}
	);

	Pods p1;

	unsigned char out_text_data[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, all_false_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, Pods::TYPE_ID, &p1, sizeof(Pods), out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );

	EXPECT_EQ(0,    p1.i8);
	EXPECT_EQ(0,    p1.i16);
	EXPECT_EQ(0,    p1.i32);
	EXPECT_EQ(0,    p1.i64);
	EXPECT_EQ(0u,   p1.u8);
	EXPECT_EQ(0u,   p1.u16);
	EXPECT_EQ(0u,   p1.u32);
	EXPECT_EQ(0u,   p1.u64);
	EXPECT_EQ(0.0f, p1.f32);
	EXPECT_EQ(0.0,  p1.f64);
}

TEST_F( DLText, basic_bool_in_bitfield )
{
	const char* test_bits_text = STRINGIFY(
		{
			"TestBits" : {
			"Bit1" : false,
			"Bit2" : true,
			"Bit3" : false,
			"Bit4" : true,
			"Bit5" : false,
			"Bit6" : true,
			"make_it_uneven" : true
			}
		}
	);

	TestBits p1;
	memset( &p1, 0x0, sizeof(TestBits) );

	unsigned char out_text_data[1024];

	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_bits_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_instance_load( Ctx, TestBits::TYPE_ID, &p1, sizeof(TestBits), out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );

	EXPECT_EQ(0, p1.Bit1);
	EXPECT_EQ(1, p1.Bit2);
	EXPECT_EQ(0, p1.Bit3);
	EXPECT_EQ(1, p1.Bit4);
	EXPECT_EQ(0, p1.Bit5);
	EXPECT_EQ(1, p1.Bit6);
	EXPECT_EQ(1, p1.make_it_uneven);
}

TEST_F( DLText, missing_field_data )
{
	const char* test_text = STRINGIFY( { "Pods" } );

	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, error_bad_type_name )
{
	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, "{ \"single_int : {\"val\" : 1} }", out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, "{ \"single_?int\" : {\"val\" : 1} }", out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, "{ \"single int\" : {\"val\" : 1} }", out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, error_bad_member_name )
{
	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, "{ \"single_int\" : {\"val : \"1\"} }", out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, single_quoted_object_keys )
{
	unsigned char out_text_data[1024];
	{
		const char* test_text = STRINGIFY( { 'single_int' : { "val" : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}

	{
		const char* test_text = STRINGIFY( { "single_int" : { 'val' : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}

	{
		const char* test_text = STRINGIFY( { 'single_int' : { 'val' : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}
}

TEST_F( DLText, unquoted_object_keys )
{
	unsigned char out_text_data[1024];
	{
		const char* test_text = STRINGIFY( { single_int : { "val" : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}

	{
		const char* test_text = STRINGIFY( { "single_int" : { val : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}

	{
		const char* test_text = STRINGIFY( { single_int : { val : 1 } } );
		EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
	}
}


TEST_F( DLText, single_line_comments )
{
	const char* test_text =
		"{\n"
			"\"PodsDefaults\" : {\n"
			"\"u8\" : 5 // whoooo\n"
			"}\n"
		"}";

	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, multi_line_comments )
{
	const char* test_text =
		"{\n"
			"\"PodsDefaults\" : {\n"
			"\"u8\" : 5 /* whoooo\n"
			"whaaa */\n"
			"}\n"
		"}";

	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, leading_decimal_point )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : .5, f64 : .5 } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_FLOAT_EQ (.5f, pods->f32 );
    EXPECT_DOUBLE_EQ(.5,  pods->f64 );
}

TEST_F( DLText, trailing_decimal_point )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : 5., f64 : 5. } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_FLOAT_EQ (5.f, pods->f32 );
    EXPECT_DOUBLE_EQ(5.,  pods->f64 );
}

TEST_F( DLText, float_infinity )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : Infinity, f64 : Infinity } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_FLOAT_EQ (std::numeric_limits<float>::infinity(),  pods->f32 );
    EXPECT_DOUBLE_EQ(std::numeric_limits<double>::infinity(), pods->f64 );
}

TEST_F( DLText, float_infinity_neg )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : -Infinity, f64 : -Infinity } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_FLOAT_EQ (-std::numeric_limits<float>::infinity(),  pods->f32 );
    EXPECT_DOUBLE_EQ(-std::numeric_limits<double>::infinity(), pods->f64 );
}

TEST_F( DLText, float_nan )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : NaN, f64 : NaN } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_TRUE(isnan(pods->f32) == 1);
    EXPECT_TRUE(isnan(pods->f64) == 1);
}

TEST_F( DLText, float_nan_neg )
{
    uint64_t unpack_buffer[128];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { f32 : -NaN, f64 : -NaN } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_TRUE(isnan(pods->f32) == 1);
    EXPECT_TRUE(isnan(pods->f64) == 1);
}

TEST_F( DLText, intXX_min )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { i8 : min, u8 : min, i16 : min, u16 : min, i32 : min, u32 : min, i64 : min, u64 : min } } ),
		STRINGIFY( { PodsDefaults : { i8 : Min, u8 : Min, i16 : Min, u16 : Min, i32 : Min, u32 : Min, i64 : Min, u64 : Min } } ),
		STRINGIFY( { PodsDefaults : { i8 : MIN, u8 : MIN, i16 : MIN, u16 : MIN, i32 : MIN, u32 : MIN, i64 : MIN, u64 : MIN } } ),
		STRINGIFY( { PodsDefaults : {i8:min,u8:min,i16:min,u16:min,i32:min,u32:min,i64:min,u64:min} } )
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(INT8_MIN,  pods->i8);
		EXPECT_EQ(0,         pods->u8);
		EXPECT_EQ(INT16_MIN, pods->i16);
		EXPECT_EQ(0,         pods->u16);
		EXPECT_EQ(INT32_MIN, pods->i32);
		EXPECT_EQ(0u,        pods->u32);
		EXPECT_EQ(INT64_MIN, pods->i64);
		EXPECT_EQ(0u,        pods->u64);
	}
}

TEST_F( DLText, intXX_max )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { i8 : max, u8 : max, i16 : max, u16 : max, i32 : max, u32 : max, i64 : max, u64 : max } } ),
		STRINGIFY( { PodsDefaults : { i8 : Max, u8 : Max, i16 : Max, u16 : Max, i32 : Max, u32 : Max, i64 : Max, u64 : Max } } ),
		STRINGIFY( { PodsDefaults : { i8 : MAX, u8 : MAX, i16 : MAX, u16 : MAX, i32 : MAX, u32 : MAX, i64 : MAX, u64 : MAX } } ),
		STRINGIFY( { PodsDefaults : {i8:max,u8:max,i16:max,u16:max,i32:max,u32:max,i64:max,u64:max} } ),
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(INT8_MAX,   pods->i8);
		EXPECT_EQ(UINT8_MAX,  pods->u8);
		EXPECT_EQ(INT16_MAX,  pods->i16);
		EXPECT_EQ(UINT16_MAX, pods->u16);
		EXPECT_EQ(INT32_MAX,  pods->i32);
		EXPECT_EQ(UINT32_MAX, pods->u32);
		EXPECT_EQ(INT64_MAX,  pods->i64);
		EXPECT_EQ(UINT64_MAX, pods->u64);
	}
}

TEST_F( DLText, fpXX_min )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { f32 : min, f64 : min } } ),
		STRINGIFY( { PodsDefaults : { f32 : Min, f64 : Min } } ),
		STRINGIFY( { PodsDefaults : { f32 : MIN, f64 : MIN } } ),
		STRINGIFY( { PodsDefaults : {f32:min,f64:min} } )
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(FLT_MIN, pods->f32);
		EXPECT_EQ(DBL_MIN, pods->f64);
	}
}

TEST_F( DLText, fpXX_max )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { f32 : max, f64 : max } } ),
		STRINGIFY( { PodsDefaults : { f32 : Max, f64 : Max } } ),
		STRINGIFY( { PodsDefaults : { f32 : MAX, f64 : MAX } } ),
		STRINGIFY( { PodsDefaults : {f32:max,f64:max} } )
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(FLT_MAX, pods->f32);
		EXPECT_EQ(DBL_MAX, pods->f64);
	}
}

TEST_F( DLText, fpXX_neg_min )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { f32 : -min, f64 : -min } } ),
		STRINGIFY( { PodsDefaults : { f32 : -Min, f64 : -Min } } ),
		STRINGIFY( { PodsDefaults : { f32 : -MIN, f64 : -MIN } } ),
		STRINGIFY( { PodsDefaults : {f32:-min,f64:-min} } )
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(-FLT_MIN, pods->f32);
		EXPECT_EQ(-DBL_MIN, pods->f64);
	}
}

TEST_F( DLText, fpXX_neg_max )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { f32 : -max, f64 : -max } } ),
		STRINGIFY( { PodsDefaults : { f32 : -Max, f64 : -Max } } ),
		STRINGIFY( { PodsDefaults : { f32 : -MAX, f64 : -MAX } } ),
		STRINGIFY( { PodsDefaults : {f32:-max,f64:-max} } )
	};

    uint64_t unpack_buffer[128];
	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
	{
		PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, test_str[test], unpack_buffer, sizeof(unpack_buffer));
		EXPECT_EQ(-FLT_MAX, pods->f32);
		EXPECT_EQ(-DBL_MAX, pods->f64);
	}
}

template <typename T>
static void dl_txt_test_expect_error(dl_ctx_t Ctx, const char* txt, dl_error_t expected)
{
    unsigned char out_text_data[4096];
    EXPECT_DL_ERR_EQ( expected, dl_txt_pack( Ctx, txt, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, intXX_invalid_const )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { i8 : m } } ),
		STRINGIFY( { PodsDefaults : { i8 : Ma } } ),
		STRINGIFY( { PodsDefaults : { i8 : Mi } } ),
		STRINGIFY( { PodsDefaults : { i8 : MAXi } } ),
		STRINGIFY( { PodsDefaults : { i8 : blo } } )
	};

	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
		dl_txt_test_expect_error<PodsDefaults>(Ctx, test_str[test], DL_ERROR_MALFORMED_DATA);
}

TEST_F( DLText, fpXX_invalid_const )
{
	const char* test_str[] = {
		STRINGIFY( { PodsDefaults : { f32 : m } } ),
		STRINGIFY( { PodsDefaults : { f32 : Ma } } ),
		STRINGIFY( { PodsDefaults : { f32 : Mi } } ),
		STRINGIFY( { PodsDefaults : { f32 : -Ma } } ),
		STRINGIFY( { PodsDefaults : { f32 : -Mi } } ),
		STRINGIFY( { PodsDefaults : { f32 : MAXi } } ),
		STRINGIFY( { PodsDefaults : { f32 : -MAXi } } ),
		STRINGIFY( { PodsDefaults : { f32 : blo } } ),
	};

	for(uint32_t test = 0; test < DL_ARRAY_LENGTH(test_str); ++test)
		dl_txt_test_expect_error<PodsDefaults>(Ctx, test_str[test], DL_ERROR_MALFORMED_DATA);
}

TEST_F( DLText, numbers_start_with_plus )
{
    unsigned char unpack_buffer[1024];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY(
    	{
    		PodsDefaults : {
    			i8  : +1,
				i16 : +2,
				i32 : +3,
				i64 : +4,
    			u8  : +1,
				u16 : +2,
				u32 : +3,
				u64 : +4,
    			f32 : +1,
				f64 : +1
    		}
    	} ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_EQ(1, pods->i8);
    EXPECT_EQ(2, pods->i16);
    EXPECT_EQ(3, pods->i32);
    EXPECT_EQ(4, pods->i64);
    EXPECT_EQ(1u, pods->u8);
    EXPECT_EQ(2u, pods->u16);
    EXPECT_EQ(3u, pods->u32);
    EXPECT_EQ(4u, pods->u64);
    EXPECT_EQ(1.0f, pods->f32);
    EXPECT_EQ(1.0,  pods->f64);
}

TEST_F( DLText, accept_trailing_comma_array_i8 )
{
    unsigned char unpack_buffer[1024];
    i8Array* arr = dl_txt_test_pack_text<i8Array>(Ctx, STRINGIFY( { i8Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    int8_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_i16 )
{
    unsigned char unpack_buffer[1024];
    i16Array* arr = dl_txt_test_pack_text<i16Array>(Ctx, STRINGIFY( { i16Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    int16_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_i32 )
{
    unsigned char unpack_buffer[1024];
    i32Array* arr = dl_txt_test_pack_text<i32Array>(Ctx, STRINGIFY( { i32Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    int32_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_i64 )
{
    unsigned char unpack_buffer[1024];
    i64Array* arr = dl_txt_test_pack_text<i64Array>(Ctx, STRINGIFY( { i64Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    int64_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_u8 )
{
    unsigned char unpack_buffer[1024];
    u8Array* arr = dl_txt_test_pack_text<u8Array>(Ctx, STRINGIFY( { u8Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    uint8_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_u16 )
{
    unsigned char unpack_buffer[1024];
    u16Array* arr = dl_txt_test_pack_text<u16Array>(Ctx, STRINGIFY( { u16Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    uint16_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_u32 )
{
    unsigned char unpack_buffer[1024];
    u32Array* arr = dl_txt_test_pack_text<u32Array>(Ctx, STRINGIFY( { u32Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    uint32_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_u64 )
{
    unsigned char unpack_buffer[1024];
    u64Array* arr = dl_txt_test_pack_text<u64Array>(Ctx, STRINGIFY( { u64Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    uint64_t expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_fp32 )
{
    unsigned char unpack_buffer[1024];
    fp32Array* arr = dl_txt_test_pack_text<fp32Array>(Ctx, STRINGIFY( { fp32Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    float expect[] = {1.0f,2.0f,3.0f};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_fp64 )
{
    unsigned char unpack_buffer[1024];
    fp64Array* arr = dl_txt_test_pack_text<fp64Array>(Ctx, STRINGIFY( { fp64Array : { arr : [1,2,3,] } } ), unpack_buffer, sizeof(unpack_buffer));
    double expect[] = {1,2,3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_str )
{
    unsigned char unpack_buffer[1024];
    strArray* arr = dl_txt_test_pack_text<strArray>(Ctx, STRINGIFY( { strArray : { arr : ["a", "b", "c",] } } ), unpack_buffer, sizeof(unpack_buffer));
    const char* expect[] = {"a", "b", "c"};
    EXPECT_EQ(3u, arr->arr.count);
    EXPECT_STREQ(expect[0], arr->arr[0]);
    EXPECT_STREQ(expect[1], arr->arr[1]);
    EXPECT_STREQ(expect[2], arr->arr[2]);
}

TEST_F( DLText, accept_trailing_comma_array_ptr )
{
    unsigned char unpack_buffer[1024];
    ptrArray* arr = dl_txt_test_pack_text<ptrArray>(Ctx, STRINGIFY( { ptrArray : { arr : [null, null, null,] } } ), unpack_buffer, sizeof(unpack_buffer));
    Pods* expect[] = {0x0, 0x0, 0x0};
    EXPECT_EQ(3u, arr->arr.count);
    EXPECT_EQ(expect[0], arr->arr[0]);
    EXPECT_EQ(expect[1], arr->arr[1]);
    EXPECT_EQ(expect[2], arr->arr[2]);
}

TEST_F( DLText, accept_trailing_comma_array_enum )
{
    unsigned char unpack_buffer[1024];
    enumArray* arr = dl_txt_test_pack_text<enumArray>(Ctx, STRINGIFY( { enumArray : { arr : ["TESTENUM1_VALUE1", "TESTENUM1_VALUE2", "TESTENUM1_VALUE3",] } } ), unpack_buffer, sizeof(unpack_buffer));
    TestEnum1 expect[] = {TESTENUM1_VALUE1, TESTENUM1_VALUE2, TESTENUM1_VALUE3};
    EXPECT_ARRAY_EQ(3, arr->arr.data, expect);
}

TEST_F( DLText, accept_trailing_comma_array_struct )
{
    unsigned char unpack_buffer[1024];
    StructArray1* arr = dl_txt_test_pack_text<StructArray1>(Ctx, STRINGIFY( { StructArray1 : { Array : [{Int1 : 1, Int2 : 2}, {Int1 : 3, Int2 : 4},] } } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_EQ(2u, arr->Array.count);
    EXPECT_EQ(1u, arr->Array[0].Int1);
    EXPECT_EQ(2u, arr->Array[0].Int2);
    EXPECT_EQ(3u, arr->Array[1].Int1);
    EXPECT_EQ(4u, arr->Array[1].Int2);
}

TEST_F( DLText, accept_trailing_comma_struct)
{
	const char* test_text =
		"{\n"
			"\"PodsDefaults\" : {\n"
			"\"u8\" : 5,\n"
			"}\n"
		"}";

	unsigned char out_text_data[1024];
	EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, missing_struct_end_in_struct_array )
{
	unsigned char out_text_data[1024];
	const char* test_text = STRINGIFY( { StructArray1 : { Array : [{Int1 : 1, Int2 : 2}, {Int1 : 3, Int2 : 4,] } } );
	EXPECT_DL_ERR_EQ( DL_ERROR_TXT_PARSE_ERROR, dl_txt_pack( Ctx, test_text, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );
}

TEST_F( DLText, hex_ints )
{
    unsigned char unpack_buffer[1024];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY(
        {
            PodsDefaults : {
                i8  : 0xA,
                i16 : 0xB,
                i32 : 0xC,
                i64 : 0xD,
                u8  : 0xE,
                u16 : 0xF,
                u32 : 0x1A,
                u64 : 0x1B,
                f32 : +1,
                f64 : +1
            }
        } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_EQ(0xA, pods->i8);
    EXPECT_EQ(0xB, pods->i16);
    EXPECT_EQ(0xC, pods->i32);
    EXPECT_EQ(0xD, pods->i64);
    EXPECT_EQ(0xEu, pods->u8);
    EXPECT_EQ(0xFu, pods->u16);
    EXPECT_EQ(0x1Au, pods->u32);
    EXPECT_EQ(0x1Bu, pods->u64);
}

TEST_F( DLText, hex_ints_neg )
{
    unsigned char unpack_buffer[1024];
    PodsDefaults* pods = dl_txt_test_pack_text<PodsDefaults>(Ctx, STRINGIFY(
        {
            PodsDefaults : {
                i8  : -0xA,
                i16 : -0xB,
                i32 : -0xC,
                i64 : -0xD
            }
        } ), unpack_buffer, sizeof(unpack_buffer));
    EXPECT_EQ(-0xA, pods->i8);
    EXPECT_EQ(-0xB, pods->i16);
    EXPECT_EQ(-0xC, pods->i32);
    EXPECT_EQ(-0xD, pods->i64);
}

TEST_F( DLText, hex_int8_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i8  :  0xFF } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i8  : -0xFF } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_int16_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i16  :  0xFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i16  : -0xFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_int32_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i32  :  0xFFFFFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i32  : -0xFFFFFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_int64_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i64  :  0xFFFFFFFFFFFFFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { i64  : -0xFFFFFFFFFFFFFFFF } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_uint8_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u8  :  0x100 } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u8  : -0x1   } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_uint16_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u16  :  0x10000 } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u16  : -0x1     } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_uint32_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u32  :  0x100000000 } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u32  : -0x1         } } ), DL_ERROR_TXT_RANGE_ERROR);
}

TEST_F( DLText, hex_uint64_invalid_range )
{
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u64  :  0x10000000000000000 } } ), DL_ERROR_TXT_RANGE_ERROR);
    dl_txt_test_expect_error<PodsDefaults>(Ctx, STRINGIFY( { PodsDefaults : { u64  : -0x1                 } } ), DL_ERROR_TXT_RANGE_ERROR);
}
