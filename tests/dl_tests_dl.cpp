/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include "dl_test_common.h"

#include <float.h>

#define EXPECT_INSTANCE_INFO( inst, size, exp_ptr_size, exp_endian, exp_type ) \
{ \
	dl_instance_info_t inst_info; \
	EXPECT_DL_ERR_OK( dl_instance_get_info( inst, size, &inst_info ) ); \
	EXPECT_EQ( exp_ptr_size, inst_info.ptrsize ); \
	EXPECT_EQ( exp_endian,   inst_info.endian ); \
	EXPECT_EQ( exp_type,     inst_info.root_type ); \
}

const unsigned int OUT_BUFFER_SIZE = 2048;
const unsigned int TEST_REPS       = 1; // number of times to repeate tests internally, use for profiling etc.

struct pack_text_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
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
};

struct inplace_load_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
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
};

template<unsigned int conv_ptr_size, dl_endian_t conv_endian>
struct convert_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char* out_buffer,   size_t*     out_size )
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
};

template<unsigned int conv_ptr_size, dl_endian_t conv_endian>
struct convert_inplace_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
			           unsigned char* store_buffer, size_t      store_size,
			           unsigned char* out_buffer,   size_t*     out_size )
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
};

template <typename T>
struct DLBase : public DL
{
	virtual ~DLBase() {}

	void do_the_round_about( dl_typeid_t type, void* pack_me, void* unpack_me, size_t unpack_me_size )
	{
		dl_ctx_t dl_ctx = this->Ctx;

		for( unsigned int i = 0; i < TEST_REPS; ++i )
		{
			unsigned char store_buffer[1024];
			memset(store_buffer, 0xFE, sizeof(store_buffer));

			// calc size of stored instance
			size_t store_size = 0;
			EXPECT_DL_ERR_OK( dl_instance_calc_size( dl_ctx, type, pack_me, &store_size ) );

			// store instance to binary
			EXPECT_DL_ERR_OK( dl_instance_store( dl_ctx, type, pack_me, store_buffer, store_size, 0x0 ) );
			EXPECT_INSTANCE_INFO( store_buffer, store_size, sizeof(void*), DL_ENDIAN_HOST, type );
			EXPECT_EQ( 0xFE, store_buffer[store_size] ); // no overwrite on the calculated size plox!

			unsigned char out_buffer[OUT_BUFFER_SIZE];
			memset( out_buffer, 0xFE, sizeof(out_buffer) );
			size_t out_size;

			T::do_it( dl_ctx, type, store_buffer, store_size, out_buffer, &out_size );

			EXPECT_EQ( (unsigned char)0xFE, out_buffer[out_size] ); // no overwrite when packing text plox!

			// out instance should have correct format
			EXPECT_INSTANCE_INFO( out_buffer, out_size, sizeof(void*), DL_ENDIAN_HOST, type );

			size_t consumed = 0;
			// load binary
			EXPECT_DL_ERR_OK( dl_instance_load( dl_ctx, type, unpack_me, unpack_me_size, out_buffer, out_size, &consumed ) );

			EXPECT_EQ( out_size, consumed );
		}
	}
};

// tests to run!
typedef ::testing::Types<
	 pack_text_test
	,inplace_load_test
	,convert_test<4, DL_ENDIAN_LITTLE>
	,convert_test<8, DL_ENDIAN_LITTLE>
	,convert_test<4, DL_ENDIAN_BIG>
	,convert_test<8, DL_ENDIAN_BIG>
	,convert_inplace_test<4, DL_ENDIAN_LITTLE>
	,convert_inplace_test<8, DL_ENDIAN_LITTLE>
	,convert_inplace_test<4, DL_ENDIAN_BIG>
	,convert_inplace_test<8, DL_ENDIAN_BIG>
> DLBaseTypes;
TYPED_TEST_CASE(DLBase, DLBaseTypes);

TYPED_TEST(DLBase, pods)
{
	Pods P1Original = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,  P1.i8);
	EXPECT_EQ(P1Original.i16, P1.i16);
	EXPECT_EQ(P1Original.i32, P1.i32);
	EXPECT_EQ(P1Original.i64, P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	EXPECT_EQ(P1Original.f32, P1.f32);
	EXPECT_EQ(P1Original.f64, P1.f64);
}

TYPED_TEST(DLBase, pods_max)
{
	Pods P1Original = { INT8_MAX, INT16_MAX, INT32_MAX, INT64_MAX, UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX, FLT_MAX, DBL_MAX };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,  P1.i8);
	EXPECT_EQ(P1Original.i16, P1.i16);
	EXPECT_EQ(P1Original.i32, P1.i32);
	EXPECT_EQ(P1Original.i64, P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	// need to disable these tests for floating-point-errors =/
	// EXPECT_FLOAT_EQ(P1Original.f32,   P1.f32);
	// EXPECT_DOUBLE_EQ(P1Original.f64,   P1.f64);
}

TYPED_TEST(DLBase, pods_min)
{
	Pods P1Original = { INT8_MIN, INT16_MIN, INT32_MIN, INT64_MIN, 0, 0, 0, 0, FLT_MIN, DBL_MIN };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,    P1.i8);
	EXPECT_EQ(P1Original.i16,   P1.i16);
	EXPECT_EQ(P1Original.i32,   P1.i32);
	EXPECT_EQ(P1Original.i64,   P1.i64);
	EXPECT_EQ(P1Original.u8,    P1.u8);
	EXPECT_EQ(P1Original.u16,   P1.u16);
	EXPECT_EQ(P1Original.u32,   P1.u32);
	EXPECT_EQ(P1Original.u64,   P1.u64);
	EXPECT_NEAR(P1Original.f32, P1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.f64, P1.f64, 0.0000001f);
}

TYPED_TEST(DLBase, struct_in_struct)
{
	MorePods P1Original = { { 1, 2, 3, 4, 5, 6, 7, 8, 0.0f, 0}, { 9, 10, 11, 12, 13, 14, 15, 16, 0.0f, 0} };
	MorePods P1;
	memset( &P1, 0x0, sizeof(MorePods) );

	this->do_the_round_about( MorePods::TYPE_ID, &P1Original, &P1, sizeof(MorePods) );

	EXPECT_EQ(P1Original.Pods1.i8,    P1.Pods1.i8);
	EXPECT_EQ(P1Original.Pods1.i16,   P1.Pods1.i16);
	EXPECT_EQ(P1Original.Pods1.i32,   P1.Pods1.i32);
	EXPECT_EQ(P1Original.Pods1.i64,   P1.Pods1.i64);
	EXPECT_EQ(P1Original.Pods1.u8,    P1.Pods1.u8);
	EXPECT_EQ(P1Original.Pods1.u16,   P1.Pods1.u16);
	EXPECT_EQ(P1Original.Pods1.u32,   P1.Pods1.u32);
	EXPECT_EQ(P1Original.Pods1.u64,   P1.Pods1.u64);
	EXPECT_NEAR(P1Original.Pods1.f32, P1.Pods1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods1.f64, P1.Pods1.f64, 0.0000001f);

	EXPECT_EQ(P1Original.Pods2.i8,    P1.Pods2.i8);
	EXPECT_EQ(P1Original.Pods2.i16,   P1.Pods2.i16);
	EXPECT_EQ(P1Original.Pods2.i32,   P1.Pods2.i32);
	EXPECT_EQ(P1Original.Pods2.i64,   P1.Pods2.i64);
	EXPECT_EQ(P1Original.Pods2.u8,    P1.Pods2.u8);
	EXPECT_EQ(P1Original.Pods2.u16,   P1.Pods2.u16);
	EXPECT_EQ(P1Original.Pods2.u32,   P1.Pods2.u32);
	EXPECT_EQ(P1Original.Pods2.u64,   P1.Pods2.u64);
	EXPECT_NEAR(P1Original.Pods2.f32, P1.Pods2.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods2.f64, P1.Pods2.f64, 0.0000001f);
}

TYPED_TEST(DLBase, struct_in_struct_in_struct)
{
	Pod2InStructInStruct original;
	Pod2InStructInStruct loaded;

	original.p2struct.Pod1.Int1 = 1337;
	original.p2struct.Pod1.Int2 = 7331;
	original.p2struct.Pod2.Int1 = 1234;
	original.p2struct.Pod2.Int2 = 4321;

	this->do_the_round_about( Pod2InStructInStruct::TYPE_ID, &original, &loaded, sizeof(Pod2InStructInStruct) );

	EXPECT_EQ(original.p2struct.Pod1.Int1, loaded.p2struct.Pod1.Int1);
	EXPECT_EQ(original.p2struct.Pod1.Int2, loaded.p2struct.Pod1.Int2);
	EXPECT_EQ(original.p2struct.Pod2.Int1, loaded.p2struct.Pod2.Int1);
	EXPECT_EQ(original.p2struct.Pod2.Int2, loaded.p2struct.Pod2.Int2);
}

TYPED_TEST(DLBase, string)
{
	Strings original = { "cow", "bell" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, string_null)
{
	Strings original = { 0x0, "bell" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, string_with_commas)
{
	Strings original = { "str,1", "str,2" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, escaped_fnutt_in_string )
{
	SubString original = { "whoo\"whaa" };
	SubString loaded[10];

	this->do_the_round_about( SubString::TYPE_ID, &original, loaded, sizeof(loaded) );
	EXPECT_STREQ( original.Str, loaded[0].Str );
}

TYPED_TEST(DLBase, escaped_val_in_string)
{
	Strings Orig = { "cow\nbell\tbopp", "\\\"\"bopp" } ;
	Strings Loaded[16];

	this->do_the_round_about( Strings::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	EXPECT_STREQ(Orig.Str1, Loaded[0].Str1);
	EXPECT_STREQ(Orig.Str2, Loaded[0].Str2);
}

TYPED_TEST(DLBase, enum)
{
	EXPECT_EQ( 0, TESTENUM1_VALUE1 );
	EXPECT_EQ( 1, TESTENUM1_VALUE2 );
	EXPECT_EQ( 2, TESTENUM1_VALUE3 );
	EXPECT_EQ( 3, TESTENUM1_VALUE4 );

	EXPECT_EQ(TESTENUM2_VALUE2 + 1, TESTENUM2_VALUE3); // value3 is after value2 but has no value. It sohuld automticallay be one bigger!

	TestingEnum Inst;
	Inst.TheEnum = TESTENUM1_VALUE3;

	TestingEnum Loaded;

	this->do_the_round_about( TestingEnum::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_EQ(Inst.TheEnum, Loaded.TheEnum);
}

TYPED_TEST(DLBase, inline_array_pod) // TODO: add tests for all inline_array-types
{
	WithInlineArray Orig;
	WithInlineArray New;

	Orig.Array[0] = 1337;
	Orig.Array[1] = 7331;
	Orig.Array[2] = 1234;

	this->do_the_round_about( WithInlineArray::TYPE_ID, &Orig, &New, sizeof(WithInlineArray) );

	EXPECT_EQ(Orig.Array[0], New.Array[0]);
	EXPECT_EQ(Orig.Array[1], New.Array[1]);
	EXPECT_EQ(Orig.Array[2], New.Array[2]);
}

TYPED_TEST(DLBase, inline_array_struct)
{
	WithInlineStructArray Orig;
	WithInlineStructArray New;

	Orig.Array[0].Int1 = 1337;
	Orig.Array[0].Int2 = 7331;
	Orig.Array[1].Int1 = 1224;
	Orig.Array[1].Int2 = 5678;
	Orig.Array[2].Int1 = 9012;
	Orig.Array[2].Int2 = 3456;

	this->do_the_round_about( WithInlineStructArray::TYPE_ID, &Orig, &New, sizeof(WithInlineStructArray) );

	EXPECT_EQ(Orig.Array[0].Int1, New.Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Int2, New.Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Int1, New.Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Int2, New.Array[1].Int2);
	EXPECT_EQ(Orig.Array[2].Int1, New.Array[2].Int1);
	EXPECT_EQ(Orig.Array[2].Int2, New.Array[2].Int2);
}

TYPED_TEST(DLBase, inline_array_struct_in_struct)
{
	WithInlineStructStructArray Orig;
	WithInlineStructStructArray New;

	Orig.Array[0].Array[0].Int1 = 1;
	Orig.Array[0].Array[0].Int2 = 3;
	Orig.Array[0].Array[1].Int1 = 3;
	Orig.Array[0].Array[1].Int2 = 7;
	Orig.Array[0].Array[2].Int1 = 13;
	Orig.Array[0].Array[2].Int2 = 37;
	Orig.Array[1].Array[0].Int1 = 1337;
	Orig.Array[1].Array[0].Int2 = 7331;
	Orig.Array[1].Array[1].Int1 = 1;
	Orig.Array[1].Array[1].Int2 = 337;
	Orig.Array[1].Array[2].Int1 = 133;
	Orig.Array[1].Array[2].Int2 = 7;

	this->do_the_round_about( WithInlineStructStructArray::TYPE_ID, &Orig, &New, sizeof(WithInlineStructStructArray) );

	EXPECT_EQ(Orig.Array[0].Array[0].Int1, New.Array[0].Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Array[0].Int2, New.Array[0].Array[0].Int2);
	EXPECT_EQ(Orig.Array[0].Array[1].Int1, New.Array[0].Array[1].Int1);
	EXPECT_EQ(Orig.Array[0].Array[1].Int2, New.Array[0].Array[1].Int2);
	EXPECT_EQ(Orig.Array[0].Array[2].Int1, New.Array[0].Array[2].Int1);
	EXPECT_EQ(Orig.Array[0].Array[2].Int2, New.Array[0].Array[2].Int2);
	EXPECT_EQ(Orig.Array[1].Array[0].Int1, New.Array[1].Array[0].Int1);
	EXPECT_EQ(Orig.Array[1].Array[0].Int2, New.Array[1].Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Array[1].Int1, New.Array[1].Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Array[1].Int2, New.Array[1].Array[1].Int2);
	EXPECT_EQ(Orig.Array[1].Array[2].Int1, New.Array[1].Array[2].Int1);
	EXPECT_EQ(Orig.Array[1].Array[2].Int2, New.Array[1].Array[2].Int2);
}

TYPED_TEST(DLBase, inline_array_string)
{
	StringInlineArray original = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_string_with_commas)
{
	StringInlineArray original = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_subptr_patched)
{
	InlineArrayWithSubString original = { { { (char*)"a,pa" }, { (char*)"kos,sa" } } };
	InlineArrayWithSubString loaded[5];

	this->do_the_round_about( InlineArrayWithSubString::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ( original.Array[0].Str, loaded[0].Array[0].Str );
	EXPECT_STREQ( original.Array[1].Str, loaded[0].Array[1].Str );
}

TYPED_TEST(DLBase, inline_array_string_null)
{
	StringInlineArray original = { { (char*)"awsum", 0x0, (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_enum)
{
	InlineArrayEnum original = { { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4 } };
	InlineArrayEnum loaded;

	this->do_the_round_about( InlineArrayEnum::TYPE_ID, &original, &loaded, sizeof(InlineArrayEnum) );

	EXPECT_EQ(original.EnumArr[0], loaded.EnumArr[0]);
	EXPECT_EQ(original.EnumArr[1], loaded.EnumArr[1]);
	EXPECT_EQ(original.EnumArr[2], loaded.EnumArr[2]);
	EXPECT_EQ(original.EnumArr[3], loaded.EnumArr[3]);
}

TYPED_TEST(DLBase, array_pod1)
{
	uint32_t array_data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	PodArray1 original = { { array_data, DL_ARRAY_LENGTH( array_data ) } };
	PodArray1 loaded[16];

	this->do_the_round_about( PodArray1::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.u32_arr.count, loaded[0].u32_arr.count);
	EXPECT_ARRAY_EQ(original.u32_arr.count, original.u32_arr.data, loaded[0].u32_arr.data);
}

TYPED_TEST(DLBase, array_with_sub_array)
{
	uint32_t array_data[] = { 1337, 7331 } ;

	PodArray1 original_array[1];
	original_array[0].u32_arr.data  = array_data;
	original_array[0].u32_arr.count = DL_ARRAY_LENGTH(array_data);
	PodArray2 original;
	original.sub_arr.data  = original_array;
	original.sub_arr.count = DL_ARRAY_LENGTH(original_array);
	PodArray2 loaded[16];

	this->do_the_round_about( PodArray2::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.sub_arr.count, loaded[0].sub_arr.count);
	EXPECT_EQ(original.sub_arr[0].u32_arr.count, loaded[0].sub_arr[0].u32_arr.count);
	EXPECT_ARRAY_EQ(original.sub_arr[0].u32_arr.count, original.sub_arr[0].u32_arr.data, loaded[0].sub_arr[0].u32_arr.data);
}

TYPED_TEST(DLBase, array_with_sub_array2)
{
	uint32_t array_data1[] = { 1337, 7331, 13, 37, 133 } ;
	uint32_t array_data2[] = { 7, 1, 337 } ;

	PodArray1 original_array[] = { { { array_data1, DL_ARRAY_LENGTH(array_data1) } }, { { array_data2, DL_ARRAY_LENGTH(array_data2) } } } ;
	PodArray2 original = { { original_array, DL_ARRAY_LENGTH(original_array) } };

	PodArray2 loaded[16];

	this->do_the_round_about( PodArray2::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.sub_arr.count, loaded[0].sub_arr.count);
	EXPECT_EQ(original.sub_arr[0].u32_arr.count, loaded[0].sub_arr[0].u32_arr.count);
	EXPECT_EQ(original.sub_arr[1].u32_arr.count, loaded[0].sub_arr[1].u32_arr.count);
	EXPECT_ARRAY_EQ(original.sub_arr[0].u32_arr.count, original.sub_arr[0].u32_arr.data, loaded[0].sub_arr[0].u32_arr.data);
	EXPECT_ARRAY_EQ(original.sub_arr[1].u32_arr.count, original.sub_arr[1].u32_arr.data, loaded[0].sub_arr[1].u32_arr.data);
}

TYPED_TEST(DLBase, array_string)
{
	const char* array_data[] = { "I like", "the", "1337 ", "cowbells of doom!" }; // TODO: test string-arrays with , in strings.
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_string_null)
{
	const char* array_data[] = { "I like", "the", 0x0, "cowbells of doom!" };
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_string_with_commas)
{
	const char* array_data[] = { "I li,ke", "the", "13,37 ", "cowbe,lls of doom!" }; // TODO: test string-arrays with , in strings.
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_struct)
{
	Pods2 array_data[4] = { { 1, 2}, { 3, 4 }, { 5, 6 }, { 7, 8 } } ;
	StructArray1 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StructArray1 loaded[16];

	this->do_the_round_about( StructArray1::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.Array.count,   loaded[0].Array.count);
	EXPECT_EQ(original.Array[0].Int1, loaded[0].Array[0].Int1);
	EXPECT_EQ(original.Array[0].Int2, loaded[0].Array[0].Int2);
	EXPECT_EQ(original.Array[1].Int1, loaded[0].Array[1].Int1);
	EXPECT_EQ(original.Array[1].Int2, loaded[0].Array[1].Int2);
	EXPECT_EQ(original.Array[2].Int1, loaded[0].Array[2].Int1);
	EXPECT_EQ(original.Array[2].Int2, loaded[0].Array[2].Int2);
	EXPECT_EQ(original.Array[3].Int1, loaded[0].Array[3].Int1);
	EXPECT_EQ(original.Array[3].Int2, loaded[0].Array[3].Int2);
}

TYPED_TEST(DLBase, array_enum)
{
	TestEnum2 array_data[8] = { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4, TESTENUM2_VALUE4, TESTENUM2_VALUE3, TESTENUM2_VALUE2, TESTENUM2_VALUE1 } ;
	ArrayEnum original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	ArrayEnum loaded[16];

	this->do_the_round_about( ArrayEnum::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.EnumArr.count, loaded[0].EnumArr.count);
	EXPECT_EQ(original.EnumArr[0],    loaded[0].EnumArr[0]);
	EXPECT_EQ(original.EnumArr[1],    loaded[0].EnumArr[1]);
	EXPECT_EQ(original.EnumArr[2],    loaded[0].EnumArr[2]);
	EXPECT_EQ(original.EnumArr[3],    loaded[0].EnumArr[3]);
	EXPECT_EQ(original.EnumArr[4],    loaded[0].EnumArr[4]);
	EXPECT_EQ(original.EnumArr[5],    loaded[0].EnumArr[5]);
	EXPECT_EQ(original.EnumArr[6],    loaded[0].EnumArr[6]);
	EXPECT_EQ(original.EnumArr[7],    loaded[0].EnumArr[7]);
}

TYPED_TEST(DLBase, bitfield)
{
	TestBits original;
	TestBits loaded;

	original.Bit1 = 0;
	original.Bit2 = 2;
	original.Bit3 = 4;
	original.make_it_uneven = 17;
	original.Bit4 = 1;
	original.Bit5 = 0;
	original.Bit6 = 5;

	this->do_the_round_about( TestBits::TYPE_ID, &original, &loaded, sizeof(TestBits) );

	EXPECT_EQ(original.Bit1, loaded.Bit1);
	EXPECT_EQ(original.Bit2, loaded.Bit2);
	EXPECT_EQ(original.Bit3, loaded.Bit3);
	EXPECT_EQ(original.make_it_uneven, loaded.make_it_uneven);
	EXPECT_EQ(original.Bit4, loaded.Bit4);
	EXPECT_EQ(original.Bit5, loaded.Bit5);
	EXPECT_EQ(original.Bit6, loaded.Bit6);
}

TYPED_TEST(DLBase, bitfield2)
{
	MoreBits original;
	MoreBits loaded;

	original.Bit1 = 512;
	original.Bit2 = 1;

	this->do_the_round_about( MoreBits::TYPE_ID, &original, &loaded, sizeof(MoreBits) );

	EXPECT_EQ(original.Bit1, loaded.Bit1);
	EXPECT_EQ(original.Bit2, loaded.Bit2);
}

TYPED_TEST(DLBase, bitfield_64bit)
{
	BitBitfield64 original;
	BitBitfield64 loaded;

	original.Package  = 2;
	original.FileType = 13;
	original.PathHash = 1337;
	original.FileHash = 0xDEADBEEF;

	this->do_the_round_about( BitBitfield64::TYPE_ID, &original, &loaded, sizeof(BitBitfield64) );

	EXPECT_EQ(original.Package,  loaded.Package);
	EXPECT_EQ(original.FileType, loaded.FileType);
	EXPECT_EQ(original.PathHash, loaded.PathHash);
	EXPECT_EQ(original.FileHash, loaded.FileHash);
}

TYPED_TEST(DLBase, ptr)
{
	Pods pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SimplePtr  orignal = { &pods, &pods };
	SimplePtr loaded[64];

	this->do_the_round_about( SimplePtr::TYPE_ID, &orignal, loaded, sizeof(loaded) );

	EXPECT_NE(orignal.Ptr1, loaded[0].Ptr1);
	EXPECT_EQ(loaded[0].Ptr1, loaded[0].Ptr2);

	EXPECT_EQ(orignal.Ptr1->i8,  loaded[0].Ptr1->i8);
	EXPECT_EQ(orignal.Ptr1->i16, loaded[0].Ptr1->i16);
	EXPECT_EQ(orignal.Ptr1->i32, loaded[0].Ptr1->i32);
	EXPECT_EQ(orignal.Ptr1->i64, loaded[0].Ptr1->i64);
	EXPECT_EQ(orignal.Ptr1->u8,  loaded[0].Ptr1->u8);
	EXPECT_EQ(orignal.Ptr1->u16, loaded[0].Ptr1->u16);
	EXPECT_EQ(orignal.Ptr1->u32, loaded[0].Ptr1->u32);
	EXPECT_EQ(orignal.Ptr1->u64, loaded[0].Ptr1->u64);
	EXPECT_EQ(orignal.Ptr1->f32, loaded[0].Ptr1->f32);
	EXPECT_EQ(orignal.Ptr1->f64, loaded[0].Ptr1->f64);
}

TYPED_TEST(DLBase, ptr_chain)
{
	PtrChain ptr1 = { 1337,   0x0 };
	PtrChain ptr2 = { 7331, &ptr1 };
	PtrChain ptr3 = { 13,   &ptr2 };
	PtrChain ptr4 = { 37,   &ptr3 };

	PtrChain loaded[10];

	this->do_the_round_about( PtrChain::TYPE_ID, &ptr4, loaded, sizeof(loaded) );

	EXPECT_NE(ptr4.Next, loaded[0].Next);
	EXPECT_NE(ptr3.Next, loaded[0].Next->Next);
	EXPECT_NE(ptr2.Next, loaded[0].Next->Next->Next);
	EXPECT_EQ(ptr1.Next, loaded[0].Next->Next->Next->Next); // should be equal, 0x0

	EXPECT_EQ(ptr4.Int, loaded[0].Int);
	EXPECT_EQ(ptr3.Int, loaded[0].Next->Int);
	EXPECT_EQ(ptr2.Int, loaded[0].Next->Next->Int);
	EXPECT_EQ(ptr1.Int, loaded[0].Next->Next->Next->Int);
}

TYPED_TEST(DLBase, ptr_chain_circle)
{
	// tests both circualar ptrs and reference to root-node!

	DoublePtrChain ptr1 = { 1337, 0x0,   0x0 };
	DoublePtrChain ptr2 = { 7331, &ptr1, 0x0 };
	DoublePtrChain ptr3 = { 13,   &ptr2, 0x0 };
	DoublePtrChain ptr4 = { 37,   &ptr3, 0x0 };

	ptr1.Prev = &ptr2;
	ptr2.Prev = &ptr3;
	ptr3.Prev = &ptr4;

	DoublePtrChain loaded[10];

	this->do_the_round_about( DoublePtrChain::TYPE_ID, &ptr4, loaded, sizeof(loaded) );

	// Ptr4
	EXPECT_NE(ptr4.Next, loaded[0].Next);
	EXPECT_EQ(ptr4.Prev, loaded[0].Prev); // is a null-ptr
	EXPECT_EQ(ptr4.Int,  loaded[0].Int);

	// Ptr3
	EXPECT_NE(ptr4.Next->Next, loaded[0].Next->Next);
	EXPECT_NE(ptr4.Next->Prev, loaded[0].Next->Prev);
	EXPECT_EQ(ptr4.Next->Int,  loaded[0].Next->Int);
	EXPECT_EQ(&loaded[0],      loaded[0].Next->Prev);

	// Ptr2
	EXPECT_NE(ptr4.Next->Next->Next, loaded[0].Next->Next->Next);
	EXPECT_NE(ptr4.Next->Next->Prev, loaded[0].Next->Next->Prev);
	EXPECT_EQ(ptr4.Next->Next->Int,  loaded[0].Next->Next->Int);
	EXPECT_EQ(loaded[0].Next,        loaded[0].Next->Next->Prev);

	// Ptr1
	EXPECT_EQ(ptr4.Next->Next->Next->Next, loaded[0].Next->Next->Next->Next); // is null
	EXPECT_NE(ptr4.Next->Next->Next->Prev, loaded[0].Next->Next->Next->Prev);
	EXPECT_EQ(ptr4.Next->Next->Next->Int,  loaded[0].Next->Next->Next->Int);
	EXPECT_EQ(loaded[0].Next->Next,        loaded[0].Next->Next->Next->Prev);
}

TYPED_TEST(DLBase, array_struct_with_ptr_holder)
{
	Pods2 p1, p2, p3;
	p1.Int1 = 1; p1.Int2 = 2;
	p2.Int1 = 3; p2.Int2 = 4;
	p3.Int1 = 5; p3.Int2 = 6;

	// testing array of pointer-type
	PtrHolder arr[] = { { &p1 }, { &p2 }, { &p2 }, { &p3 } };
	PtrArray original = { { arr, DL_ARRAY_LENGTH( arr ) } };

	PtrArray loaded[16];

	this->do_the_round_about( PtrArray::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( original.arr.count, loaded[0].arr.count );
	for( uint32_t i = 0; i < loaded[0].arr.count; ++i )
	{
		EXPECT_EQ( original.arr[i].ptr->Int1, loaded[0].arr[i].ptr->Int1 );
		EXPECT_EQ( original.arr[i].ptr->Int2, loaded[0].arr[i].ptr->Int2 );
	}

	EXPECT_NE( loaded[0].arr[0].ptr, loaded[0].arr[1].ptr );
	EXPECT_NE( loaded[0].arr[0].ptr, loaded[0].arr[2].ptr );
	EXPECT_NE( loaded[0].arr[0].ptr, loaded[0].arr[3].ptr );
	EXPECT_NE( loaded[0].arr[0].ptr, loaded[0].arr[4].ptr );

	EXPECT_EQ( loaded[0].arr[1].ptr, loaded[0].arr[2].ptr );
}

TYPED_TEST(DLBase, array_struct_circular_ptr_holder_array )
{
	// long name!
	circular_array p1, p2, p3;
	p1.val = 1337;
	p2.val = 1338;
	p3.val = 1339;

	circular_array_ptr_holder p1_arr[] = { { &p2 }, { &p3 } };
	circular_array_ptr_holder p2_arr[] = { { &p1 }, { &p2 } };
	circular_array_ptr_holder p3_arr[] = { { &p3 }, { &p1 } };

	p1.arr.data  = p1_arr; p1.arr.count = DL_ARRAY_LENGTH( p1_arr );
	p2.arr.data  = p2_arr; p2.arr.count = DL_ARRAY_LENGTH( p2_arr );
	p3.arr.data  = p3_arr; p3.arr.count = DL_ARRAY_LENGTH( p3_arr );

	circular_array loaded[16];

	this->do_the_round_about( circular_array::TYPE_ID, &p1, &loaded, sizeof(loaded) );

	const circular_array* l1 = &loaded[0];
	const circular_array* l2 = loaded[0].arr[0].ptr;
	const circular_array* l3 = loaded[0].arr[1].ptr;

	EXPECT_EQ( p1.val,        l1->val );
	EXPECT_EQ( p1.arr.count,  l1->arr.count );

	EXPECT_EQ( p2.val,        l2->val );
	EXPECT_EQ( p2.arr.count,  l2->arr.count );

	EXPECT_EQ( p3.val,        l3->val );
	EXPECT_EQ( p3.arr.count,  l3->arr.count );

	EXPECT_EQ( l1->arr[0].ptr, l2 );
	EXPECT_EQ( l1->arr[1].ptr, l3 );

	EXPECT_EQ( l2->arr[0].ptr, l1 );
	EXPECT_EQ( l2->arr[1].ptr, l2 );

	EXPECT_EQ( l3->arr[0].ptr, l3 );
	EXPECT_EQ( l3->arr[1].ptr, l1 );
}

TYPED_TEST(DLBase, array_pod_empty)
{
	PodArray1 original = { { NULL, 0 } };
	PodArray1 loaded;

	this->do_the_round_about( PodArray1::TYPE_ID, &original, &loaded, sizeof(PodArray1) );

	EXPECT_EQ(0u,  loaded.u32_arr.count);
	EXPECT_EQ(0x0, loaded.u32_arr.data);
}

TYPED_TEST(DLBase, array_struct_empty)
{
	StructArray1 original = { { NULL, 0 } };
	StructArray1 loaded;

	this->do_the_round_about( StructArray1::TYPE_ID, &original, &loaded, sizeof(StructArray1) );

	EXPECT_EQ(0u,  loaded.Array.count);
	EXPECT_EQ(0x0, loaded.Array.data);
}

TYPED_TEST(DLBase, array_string_empty)
{
	StringArray original = { { NULL, 0 } };
	StringArray loaded;

	this->do_the_round_about( StringArray::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ(0u,  loaded.Strings.count);
	EXPECT_EQ(0x0, loaded.Strings.data);
}

TYPED_TEST(DLBase, bug1)
{
	// There was some error packing arrays ;)

	BugTest1_InArray array_data[3] = { { 1337, 1338, 18 }, { 7331, 8331, 19 }, { 31337, 73313, 20 } } ;
	BugTest1 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };

	BugTest1 loaded[10];

	this->do_the_round_about( BugTest1::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ(array_data[0].u64_1, loaded[0].Arr[0].u64_1);
	EXPECT_EQ(array_data[0].u64_2, loaded[0].Arr[0].u64_2);
	EXPECT_EQ(array_data[0].u16,   loaded[0].Arr[0].u16);
	EXPECT_EQ(array_data[1].u64_1, loaded[0].Arr[1].u64_1);
	EXPECT_EQ(array_data[1].u64_2, loaded[0].Arr[1].u64_2);
	EXPECT_EQ(array_data[1].u16,   loaded[0].Arr[1].u16);
	EXPECT_EQ(array_data[2].u64_1, loaded[0].Arr[2].u64_1);
	EXPECT_EQ(array_data[2].u64_2, loaded[0].Arr[2].u64_2);
	EXPECT_EQ(array_data[2].u16,   loaded[0].Arr[2].u16);
}

TYPED_TEST(DLBase, bug2)
{
	// some error converting from 32-bit-data to 64-bit.

	BugTest2_WithMat array_data[2] = { { 1337, { 1.0f, 0.0f, 0.0f, 0.0f,
										  	  	 0.0f, 1.0f, 0.0f, 0.0f,
												 0.0f, 0.0f, 1.0f, 0.0f,
												 0.0f, 0.0f, 1.0f, 0.0f } },

									   { 7331, { 0.0f, 0.0f, 0.0f, 1.0f,
											     0.0f, 0.0f, 1.0f, 0.0f,
												 0.0f, 1.0f, 0.0f, 0.0f,
												 1.0f, 0.0f, 0.0f, 0.0f } } };
	BugTest2 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };

	BugTest2 loaded[40];

	this->do_the_round_about( BugTest2::TYPE_ID, &original, &loaded, sizeof(loaded) );

 	EXPECT_EQ(array_data[0].iSubModel, loaded[0].Instances[0].iSubModel);
 	EXPECT_EQ(array_data[1].iSubModel, loaded[0].Instances[1].iSubModel);
 	EXPECT_ARRAY_EQ(16, array_data[0].Transform, loaded[0].Instances[0].Transform);
 	EXPECT_ARRAY_EQ(16, array_data[1].Transform, loaded[0].Instances[1].Transform);
}

TYPED_TEST(DLBase, bug3)
{
	// testing bug where struct first in struct with ptr in substruct will not get patched on load.

	uint32_t array_data[] = { 1337, 7331, 13, 37 };
	BugTest3 original = { { array_data, DL_ARRAY_LENGTH( array_data ) } };
	BugTest3 loaded[4];

	this->do_the_round_about( BugTest3::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_ARRAY_EQ( original.sub.arr.count,  original.sub.arr.data, loaded[0].sub.arr.data);
}

TYPED_TEST(DLBase, bug4)
{
	// testing bug where struct first in struct with ptr in substruct will not get patched on load.

	// TODO: Test with only 1
	const char* strings1[] = { "cow",   "bells",   "rule" };
	const char* strings2[] = { "2cow2", "2bells2", "2rule2", "and the last one" };
	StringArray str_arr[2];
	str_arr[0].Strings.data  = strings1;
	str_arr[0].Strings.count = DL_ARRAY_LENGTH( strings1 );
	str_arr[1].Strings.data  = strings2;
	str_arr[1].Strings.count = DL_ARRAY_LENGTH( strings2 );

	BugTest4 original;
	original.struct_with_str_arr.data  = str_arr;
	original.struct_with_str_arr.count = 2;

	BugTest4 loaded[1024];

	this->do_the_round_about( BugTest4::TYPE_ID, &original, &loaded, sizeof(loaded) );

	unsigned int num_struct = original.struct_with_str_arr.count;
	EXPECT_EQ( num_struct, loaded[0].struct_with_str_arr.count );

	for( unsigned int i = 0; i < num_struct; ++i )
	{
		StringArray* orig =      original.struct_with_str_arr.data + i;
		StringArray* load = loaded[0].struct_with_str_arr.data + i;
		EXPECT_EQ( orig->Strings.count, load->Strings.count );

		for( unsigned int j = 0; j < orig->Strings.count; ++j )
			EXPECT_STREQ( orig->Strings[j], load->Strings[j] );
	}
}

TYPED_TEST(DLBase, str_before_array_bug)
{
	// Test for bug #7, where string written before array would lead to mis-alignment of the array.

	str_before_array_bug_arr_type arr_data[] = { { "apan.ymesjh" } };
	str_before_array_bug mod;

	mod.str = "1234";
	mod.arr.data  = arr_data;
	mod.arr.count = DL_ARRAY_LENGTH( arr_data );

	str_before_array_bug loaded[16];

	this->do_the_round_about( str_before_array_bug::TYPE_ID, &mod, &loaded, sizeof(loaded) );

	EXPECT_STREQ( mod.str,        loaded[0].str );
	EXPECT_EQ   ( mod.arr.count,  loaded[0].arr.count );
	EXPECT_STREQ( mod.arr[0].str, loaded[0].arr[0].str );
}

TYPED_TEST(DLBase, bug_first_member_in_struct)
{
	// testing bug where first member of struct do not have the same
	// alignment as parent-struct and this struct is part of an array.

	bug_array_alignment_struct descs[] = {
		{ 1,  2,  3 },
		{ 9, 10, 11 },
	};

	bug_array_alignment desc = { { descs, DL_ARRAY_LENGTH(descs) } };
	bug_array_alignment loaded[128];

	this->do_the_round_about( bug_array_alignment::TYPE_ID, &desc, &loaded, sizeof(loaded) );

	EXPECT_EQ( desc.components[0].type, loaded[0].components[0].type );
	EXPECT_EQ( desc.components[0].ptr,  loaded[0].components[0].ptr );

	EXPECT_EQ( desc.components[1].type, loaded[0].components[1].type );
	EXPECT_EQ( desc.components[1].ptr,  loaded[0].components[1].ptr );
}

TYPED_TEST(DLBase, bug_test_1)
{
	// Testing bug where padding in array-member was missed in some cases.
	uint8_t arr1[] = { 1, 3, 3, 7 };
	uint8_t arr2[] = { 13, 37 };
	test_array_pad_1 descs[] = {
		{ { arr1, DL_ARRAY_LENGTH(arr1) },  9 },
		{ { arr2, DL_ARRAY_LENGTH(arr2) }, 11 }
	};

	test_array_pad desc = { { descs, DL_ARRAY_LENGTH(descs) } };

	test_array_pad loaded[128];
	this->do_the_round_about( test_array_pad::TYPE_ID, &desc, &loaded, sizeof(loaded) );

	EXPECT_EQ( desc.components[0].type,         loaded[0].components[0].type );

	EXPECT_EQ( desc.components[0].ptr.count, loaded[0].components[0].ptr.count );
	EXPECT_EQ( desc.components[0].ptr[0],    loaded[0].components[0].ptr[0] );
	EXPECT_EQ( desc.components[0].ptr[1],    loaded[0].components[0].ptr[1] );
	EXPECT_EQ( desc.components[0].ptr[2],    loaded[0].components[0].ptr[2] );
	EXPECT_EQ( desc.components[0].ptr[3],    loaded[0].components[0].ptr[3] );

	EXPECT_EQ( desc.components[1].ptr.count, loaded[0].components[1].ptr.count );
	EXPECT_EQ( desc.components[1].ptr[0],    loaded[0].components[1].ptr[0] );
	EXPECT_EQ( desc.components[1].ptr[1],    loaded[0].components[1].ptr[1] );
}

TYPED_TEST( DLBase, per_bug_1 )
{
	// testing that struct with string in it can be used in array.
	per_bug_1 mapping[] = { { "1str1" }, { "2str2" }, { "3str3" } };
	per_bug inst = { { mapping, DL_ARRAY_LENGTH(mapping) } };

	per_bug loaded[128];
	this->do_the_round_about( per_bug::TYPE_ID, &inst, &loaded, sizeof(loaded) );

	EXPECT_EQ( inst.arr.count, loaded[0].arr.count );

	for( uint32_t i = 0; i < loaded[0].arr.count; ++i )
		EXPECT_STREQ( inst.arr[i].str, loaded[0].arr[i].str );
}

TEST(DLMisc, endian_is_correct)
{
	// Test that DL_ENDIAN_HOST is set correctly
	union
	{
		uint32_t i;
		unsigned char c[4];
	} test;
	test.i = 0x01020304;

	if( DL_ENDIAN_HOST == DL_ENDIAN_LITTLE )
	{
		EXPECT_EQ(4, test.c[0]);
		EXPECT_EQ(3, test.c[1]);
		EXPECT_EQ(2, test.c[2]);
		EXPECT_EQ(1, test.c[3]);
	}
	else
	{
		EXPECT_EQ(1, test.c[0]);
		EXPECT_EQ(2, test.c[1]);
		EXPECT_EQ(3, test.c[2]);
		EXPECT_EQ(4, test.c[3]);
	}
}

TEST(DLMisc, built_in_tl_eq_bin_file)
{
	const unsigned char built_in_tl[] =
	{
		#include "generated/unittest.bin.h"
	};
	const char* test_file = "local/generated/unittest.bin";
	
	FILE* f = fopen( test_file, "rb" );
	fseek( f, 0, SEEK_END );
	size_t read_tl_size = (size_t)ftell(f);
	fseek( f, 0, SEEK_SET );
	
	unsigned char* read_tl = (unsigned char*)malloc(read_tl_size);
	
	EXPECT_EQ( read_tl_size, fread( read_tl, 1, read_tl_size, f ) );
	
	fclose( f );
	
	EXPECT_EQ( DL_ARRAY_LENGTH(built_in_tl), read_tl_size );
	EXPECT_EQ( 0, memcmp( read_tl, built_in_tl, read_tl_size ) );
	
	free(read_tl);
}

TYPED_TEST( DLBase, simple_alias_test )
{
	// testing usage of externally defined types
	// the members m1 and m2 are of the type vec3_test
	// that is defined outside the .tld
	//
	// specified in the tld is vec3_test marked as extern
	// that make dl_tlc not emit a definition of the
	// struct to the header.

	alias_test at;
	at.m1.x = 1.0f; at.m1.y = 2.0f; at.m1.z = 3.0f;
	at.m2.x = 4.0f; at.m2.y = 5.0f; at.m2.z = 6.0f;

	alias_test loaded;

	this->do_the_round_about( alias_test::TYPE_ID, &at, &loaded, sizeof(loaded) );

	EXPECT_EQ( at.m1.x, loaded.m1.x ); EXPECT_EQ( at.m1.y, loaded.m1.y ); EXPECT_EQ( at.m1.z, loaded.m1.z );
	EXPECT_EQ( at.m2.x, loaded.m2.x ); EXPECT_EQ( at.m2.y, loaded.m2.y ); EXPECT_EQ( at.m2.z, loaded.m2.z );
}

TEST_F( DL, bug_with_substring_and_inplace_load )
{
	bug_with_substr t1 = { "str1", { "str2" } };
	unsigned char packed_instance[256];

	// store instance to binary
	size_t pack_size;
	EXPECT_DL_ERR_OK( dl_instance_store( this->Ctx, bug_with_substr::TYPE_ID, (void**)(void*)&t1, packed_instance, sizeof( packed_instance ), &pack_size ) );

	bug_with_substr* loaded;
	EXPECT_DL_ERR_OK( dl_instance_load_inplace( this->Ctx, bug_with_substr::TYPE_ID, packed_instance, pack_size, (void**)(void*)&loaded, 0x0 ) );

	EXPECT_STREQ( t1.str, loaded->str );
	EXPECT_STREQ( t1.sub.str, loaded->sub.str );
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::GTEST_FLAG(catch_exceptions) = 0;
	return RUN_ALL_TESTS();
}
