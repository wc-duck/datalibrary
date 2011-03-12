/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include "dl_test_common.h"

#include <float.h>

// TODO: Check these versus standard defines!
static const int8_t  DL_INT8_MAX  = 0x7F;
static const int16_t DL_INT16_MAX = 0x7FFF;
static const int32_t DL_INT32_MAX = 0x7FFFFFFFL;
static const int64_t DL_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const int8_t  DL_INT8_MIN  = (-DL_INT8_MAX  - 1);
static const int16_t DL_INT16_MIN = (-DL_INT16_MAX - 1);
static const int32_t DL_INT32_MIN = (-DL_INT32_MAX - 1);
static const int64_t DL_INT64_MIN = (-DL_INT64_MAX - 1);

static const uint8_t  DL_UINT8_MAX  = 0xFFU;
static const uint16_t DL_UINT16_MAX = 0xFFFFU;
static const uint32_t DL_UINT32_MAX = 0xFFFFFFFFUL;
static const uint64_t DL_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
static const uint8_t  DL_UINT8_MIN  = 0x00U;
static const uint16_t DL_UINT16_MIN = 0x0000U;
static const uint32_t DL_UINT32_MIN = 0x00000000UL;
static const uint64_t DL_UINT64_MIN = 0x0000000000000000ULL;

#define EXPECT_INSTANCE_INFO( inst, size, exp_ptr_size, exp_endian, exp_type ) \
{ \
	dl_instance_info_t inst_info; \
	EXPECT_DL_ERR_OK( dl_instance_get_info( inst, size, &inst_info ) ); \
	EXPECT_EQ( exp_ptr_size, inst_info.ptrsize ); \
	EXPECT_EQ( exp_endian,   inst_info.endian ); \
	EXPECT_EQ( exp_type,     inst_info.root_type ); \
}

const unsigned int OUT_BUFFER_SIZE = 2048;

struct pack_text_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t   type,
					   unsigned char* store_buffer, unsigned int  store_size,
					   unsigned char* out_buffer,   unsigned int* out_size )
	{
		// unpack binary to txt
		char text_buffer[2048];
		memset(text_buffer, 0xFE, sizeof(text_buffer));

		unsigned int text_size = 0;
		EXPECT_DL_ERR_OK( dl_txt_unpack_calc_size( dl_ctx, type, store_buffer, store_size, &text_size ) );
		EXPECT_DL_ERR_OK( dl_txt_unpack( dl_ctx, type, store_buffer, store_size, text_buffer, text_size, 0x0 ) );
		EXPECT_EQ( (unsigned char)0xFE, (unsigned char)text_buffer[text_size] ); // no overwrite on the generated text plox!

		// printf("%s\n", text_buffer);

		// pack txt to binary
		EXPECT_DL_ERR_OK( dl_txt_pack_calc_size( dl_ctx, text_buffer, out_size ) );
		EXPECT_DL_ERR_OK( dl_txt_pack( dl_ctx, text_buffer, out_buffer, *out_size, 0x0 ) );
	}
};

struct inplace_load_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t   type,
					   unsigned char* store_buffer, unsigned int  store_size,
					   unsigned char* out_buffer,   unsigned int* out_size )
	{
		// copy stored instance into temp-buffer
		unsigned char inplace_buffer[4096];
		memset( inplace_buffer, 0xFE, DL_ARRAY_LENGTH(inplace_buffer) );
		memcpy( inplace_buffer, store_buffer, store_size );

		// load inplace
		void* loaded_instance = 0x0;
		unsigned int consumed = 0;
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
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t   type,
					   unsigned char* store_buffer, unsigned int  store_size,
					   unsigned char* out_buffer,   unsigned int* out_size )
	{
		// calc size to convert
		unsigned char convert_buffer[2048];
		memset(convert_buffer, 0xFE, sizeof(convert_buffer));
		unsigned int convert_size = 0;
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
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t   type,
			           unsigned char* store_buffer, unsigned int  store_size,
			           unsigned char* out_buffer,   unsigned int* out_size )
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
		unsigned int convert_size = 0;

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
	void do_the_round_about( dl_typeid_t type, void* pack_me, void* unpack_me, unsigned int unpack_me_size )
	{
		dl_ctx_t dl_ctx = this->Ctx;

		unsigned char store_buffer[1024];
		memset(store_buffer, 0xFE, sizeof(store_buffer));

		// calc size of stored instance
		unsigned int store_size = 0;
		EXPECT_DL_ERR_OK( dl_instance_calc_size( dl_ctx, type, pack_me, &store_size ) );

		// store instance to binary
		EXPECT_DL_ERR_OK( dl_instance_store( dl_ctx, type, pack_me, store_buffer, store_size, 0x0 ) );
		EXPECT_INSTANCE_INFO( store_buffer, store_size, sizeof(void*), DL_ENDIAN_HOST, type );
		EXPECT_EQ( 0xFE, store_buffer[store_size] ); // no overwrite on the calculated size plox!

		unsigned char out_buffer[OUT_BUFFER_SIZE];
		memset( out_buffer, 0xFE, sizeof(out_buffer) );
		unsigned int out_size;

		T::do_it( dl_ctx, type, store_buffer, store_size, out_buffer, &out_size );

		EXPECT_EQ( (unsigned char)0xFE, out_buffer[out_size] ); // no overwrite when packing text plox!

		// out instance should have correct format
		EXPECT_INSTANCE_INFO( out_buffer, out_size, sizeof(void*), DL_ENDIAN_HOST, type );

		unsigned int consumed = 0;
		// load binary
		EXPECT_DL_ERR_OK( dl_instance_load( dl_ctx, type, unpack_me, unpack_me_size, out_buffer, out_size, &consumed ) );

		EXPECT_EQ( out_size, consumed );
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

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, pods)
{
	Pods P1Original = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	Pods P1         = { 0 };

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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, pods_max)
{
	Pods P1Original = { DL_INT8_MAX, DL_INT16_MAX, DL_INT32_MAX, DL_INT64_MAX, DL_UINT8_MAX, DL_UINT16_MAX, DL_UINT32_MAX, DL_UINT64_MAX, FLT_MAX, DBL_MAX };
	Pods P1         = { 0 };

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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, pods_min)
{
	Pods P1Original = { DL_INT8_MIN, DL_INT16_MIN, DL_INT32_MIN, DL_INT64_MIN, DL_UINT8_MIN, DL_UINT16_MIN, DL_UINT32_MIN, DL_UINT64_MIN, FLT_MIN, DBL_MIN };
	Pods P1         = { 0 };

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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, struct_in_struct)
{
	MorePods P1Original = { { 1, 2, 3, 4, 5, 6, 7, 8, 0.0f, 0}, { 9, 10, 11, 12, 13, 14, 15, 16, 0.0f, 0} };
	MorePods P1         = { { 0 }, { 0 } };

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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, struct_in_struct_in_struct)
{
	Pod2InStructInStruct Orig;
	Pod2InStructInStruct New;

	Orig.p2struct.Pod1.Int1 = 1337;
	Orig.p2struct.Pod1.Int2 = 7331;
	Orig.p2struct.Pod2.Int1 = 1234;
	Orig.p2struct.Pod2.Int2 = 4321;

	this->do_the_round_about( Pod2InStructInStruct::TYPE_ID, &Orig, &New, sizeof(Pod2InStructInStruct) );

	EXPECT_EQ(Orig.p2struct.Pod1.Int1, New.p2struct.Pod1.Int1);
	EXPECT_EQ(Orig.p2struct.Pod1.Int2, New.p2struct.Pod1.Int2);
	EXPECT_EQ(Orig.p2struct.Pod2.Int1, New.p2struct.Pod2.Int1);
	EXPECT_EQ(Orig.p2struct.Pod2.Int2, New.p2struct.Pod2.Int2);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, string)
{
	Strings Orig = { "cow", "bell" } ;
	Strings* New;

	Strings Loaded[5]; // this is so ugly!

	this->do_the_round_about( Strings::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	New = Loaded;

	EXPECT_STREQ(Orig.Str1, New->Str1);
	EXPECT_STREQ(Orig.Str2, New->Str2);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, enum)
{
	EXPECT_EQ(TESTENUM2_VALUE2 + 1, TESTENUM2_VALUE3); // value3 is after value2 but has no value. It sohuld automticallay be one bigger!

	TestingEnum Inst;
	Inst.TheEnum = TESTENUM1_VALUE3;

	TestingEnum Loaded;

	this->do_the_round_about( TestingEnum::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_EQ(Inst.TheEnum, Loaded.TheEnum);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, inline_array_pod)
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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
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
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, inline_array_string)
{
	StringInlineArray Orig = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray* New;

	StringInlineArray Loaded[5]; // this is so ugly!

	this->do_the_round_about( StringInlineArray::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	New = Loaded;

	EXPECT_STREQ(Orig.Strings[0], New->Strings[0]);
	EXPECT_STREQ(Orig.Strings[1], New->Strings[1]);
	EXPECT_STREQ(Orig.Strings[2], New->Strings[2]);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, inline_array_enum)
{
	InlineArrayEnum Inst = { { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4 } };
	InlineArrayEnum Loaded;

	this->do_the_round_about( InlineArrayEnum::TYPE_ID, &Inst, &Loaded, sizeof(InlineArrayEnum) );

	EXPECT_EQ(Inst.EnumArr[0], Loaded.EnumArr[0]);
	EXPECT_EQ(Inst.EnumArr[1], Loaded.EnumArr[1]);
	EXPECT_EQ(Inst.EnumArr[2], Loaded.EnumArr[2]);
	EXPECT_EQ(Inst.EnumArr[3], Loaded.EnumArr[3]);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_pod1)
{
	uint32_t Data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	PodArray1 Orig = { { Data, 8 } };
	union
	{
		PodArray1 New;
		uint32_t  Loaded[1024]; // this is so ugly!
	} load;

	this->do_the_round_about( PodArray1::TYPE_ID, &Orig, load.Loaded, sizeof(load.Loaded) );

	EXPECT_EQ(Orig.u32_arr.count, load.New.u32_arr.count);
	EXPECT_ARRAY_EQ(Orig.u32_arr.count, Orig.u32_arr.data, load.New.u32_arr.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_with_sub_array)
{
	uint32_t Data[] = { 1337, 7331 } ;

	PodArray1 OrigArray[1];
	OrigArray[0].u32_arr.data  = Data;
	OrigArray[0].u32_arr.count = DL_ARRAY_LENGTH(Data);
	PodArray2 Orig;
	Orig.sub_arr.data  = OrigArray;
	Orig.sub_arr.count = DL_ARRAY_LENGTH(OrigArray);

	union
	{
		PodArray2 New;
		uint32_t  Loaded[1024]; // this is so ugly!
	} load;

	this->do_the_round_about( PodArray2::TYPE_ID, &Orig, load.Loaded, sizeof(load.Loaded) );

	EXPECT_EQ(Orig.sub_arr.count, load.New.sub_arr.count);
	EXPECT_EQ(Orig.sub_arr[0].u32_arr.count, load.New.sub_arr[0].u32_arr.count);
	EXPECT_ARRAY_EQ(Orig.sub_arr[0].u32_arr.count, Orig.sub_arr[0].u32_arr.data, load.New.sub_arr[0].u32_arr.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_with_sub_array2)
{
	uint32_t Data1[] = { 1337, 7331,  13, 37, 133 } ;
	uint32_t Data2[] = {    7,    1, 337 } ;

	PodArray1 OrigArray[] = { { Data1, DL_ARRAY_LENGTH(Data1) }, { Data2, DL_ARRAY_LENGTH(Data2) } } ;
	PodArray2 Orig = { { OrigArray, DL_ARRAY_LENGTH(OrigArray) } };

	union
	{
		PodArray2 New;
		uint32_t  Loaded[1024]; // this is so ugly!
	} load;

	this->do_the_round_about( PodArray2::TYPE_ID, &Orig, load.Loaded, sizeof(load.Loaded) );

	EXPECT_EQ(Orig.sub_arr.count, load.New.sub_arr.count);
	EXPECT_EQ(Orig.sub_arr[0].u32_arr.count, load.New.sub_arr[0].u32_arr.count);
	EXPECT_EQ(Orig.sub_arr[1].u32_arr.count, load.New.sub_arr[1].u32_arr.count);
	EXPECT_ARRAY_EQ(Orig.sub_arr[0].u32_arr.count, Orig.sub_arr[0].u32_arr.data, load.New.sub_arr[0].u32_arr.data);
	EXPECT_ARRAY_EQ(Orig.sub_arr[1].u32_arr.count, Orig.sub_arr[1].u32_arr.data, load.New.sub_arr[1].u32_arr.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_string)
{
	const char* TheStringArray[] = { "I like", "the", "1337 ", "cowbells of doom!" };
	StringArray Orig = { { TheStringArray, 4 } };
	StringArray Loaded[10]; // this is so ugly!

	this->do_the_round_about( StringArray::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	EXPECT_STREQ(Orig.Strings[0], Loaded[0].Strings[0]);
	EXPECT_STREQ(Orig.Strings[1], Loaded[0].Strings[1]);
	EXPECT_STREQ(Orig.Strings[2], Loaded[0].Strings[2]);
	EXPECT_STREQ(Orig.Strings[3], Loaded[0].Strings[3]);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_struct)
{
	Pods2 Data[4] = { { 1, 2}, { 3, 4 }, { 5, 6 }, { 7, 8 } } ;
	StructArray1 Inst = { { Data, 4 } };

	union
	{
		StructArray1 New;
		uint32_t Loaded[1024]; // this is so ugly!
	} load;

	this->do_the_round_about( StructArray1::TYPE_ID, &Inst, load.Loaded, sizeof(load.Loaded) );

	EXPECT_EQ(Inst.Array.count,   load.New.Array.count);
	EXPECT_EQ(Inst.Array[0].Int1, load.New.Array[0].Int1);
	EXPECT_EQ(Inst.Array[0].Int2, load.New.Array[0].Int2);
	EXPECT_EQ(Inst.Array[1].Int1, load.New.Array[1].Int1);
	EXPECT_EQ(Inst.Array[1].Int2, load.New.Array[1].Int2);
	EXPECT_EQ(Inst.Array[2].Int1, load.New.Array[2].Int1);
	EXPECT_EQ(Inst.Array[2].Int2, load.New.Array[2].Int2);
	EXPECT_EQ(Inst.Array[3].Int1, load.New.Array[3].Int1);
	EXPECT_EQ(Inst.Array[3].Int2, load.New.Array[3].Int2);

}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_enum)
{
	TestEnum2 Data[8] = { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4, TESTENUM2_VALUE4, TESTENUM2_VALUE3, TESTENUM2_VALUE2, TESTENUM2_VALUE1 } ;
	ArrayEnum Inst = { { Data, 8 } };

	union
	{
		ArrayEnum New;
		uint32_t Loaded[1024]; // this is so ugly!
	} load;

	this->do_the_round_about( ArrayEnum::TYPE_ID, &Inst, load.Loaded, sizeof(load.Loaded) );

	EXPECT_EQ(Inst.EnumArr.count, load.New.EnumArr.count);
	EXPECT_EQ(Inst.EnumArr[0],    load.New.EnumArr[0]);
	EXPECT_EQ(Inst.EnumArr[1],    load.New.EnumArr[1]);
	EXPECT_EQ(Inst.EnumArr[2],    load.New.EnumArr[2]);
	EXPECT_EQ(Inst.EnumArr[3],    load.New.EnumArr[3]);
	EXPECT_EQ(Inst.EnumArr[4],    load.New.EnumArr[4]);
	EXPECT_EQ(Inst.EnumArr[5],    load.New.EnumArr[5]);
	EXPECT_EQ(Inst.EnumArr[6],    load.New.EnumArr[6]);
	EXPECT_EQ(Inst.EnumArr[7],    load.New.EnumArr[7]);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bitfield)
{
	TestBits Orig;
	TestBits New;

	Orig.Bit1 = 0;
	Orig.Bit2 = 2;
	Orig.Bit3 = 4;
	Orig.make_it_uneven = 17;
	Orig.Bit4 = 1;
	Orig.Bit5 = 0;
	Orig.Bit6 = 5;

	this->do_the_round_about( TestBits::TYPE_ID, &Orig, &New, sizeof(TestBits) );

	EXPECT_EQ(Orig.Bit1, New.Bit1);
	EXPECT_EQ(Orig.Bit2, New.Bit2);
	EXPECT_EQ(Orig.Bit3, New.Bit3);
	EXPECT_EQ(Orig.make_it_uneven, New.make_it_uneven);
	EXPECT_EQ(Orig.Bit4, New.Bit4);
	EXPECT_EQ(Orig.Bit5, New.Bit5);
	EXPECT_EQ(Orig.Bit6, New.Bit6);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bitfield2)
{
	MoreBits Orig;
	MoreBits New;

	Orig.Bit1 = 512;
	Orig.Bit2 = 1;

	this->do_the_round_about( MoreBits::TYPE_ID, &Orig, &New, sizeof(MoreBits) );

	EXPECT_EQ(Orig.Bit1, New.Bit1);
	EXPECT_EQ(Orig.Bit2, New.Bit2);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bitfield_64bit)
{
	BitBitfield64 Orig;
	BitBitfield64 New;

	Orig.Package  = 2;
	Orig.FileType = 13;
	Orig.PathHash = 1337;
	Orig.FileHash = 0xDEADBEEF;

	this->do_the_round_about( BitBitfield64::TYPE_ID, &Orig, &New, sizeof(BitBitfield64) );

	EXPECT_EQ(Orig.Package,  New.Package);
	EXPECT_EQ(Orig.FileType, New.FileType);
	EXPECT_EQ(Orig.PathHash, New.PathHash);
	EXPECT_EQ(Orig.FileHash, New.FileHash);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, ptr)
{
	Pods Pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SimplePtr  Orig = { &Pods, &Pods };
	SimplePtr* New;

	SimplePtr Loaded[64]; // this is so ugly!

	this->do_the_round_about( SimplePtr::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	New = Loaded;
	EXPECT_NE(Orig.Ptr1, New->Ptr1);
	EXPECT_EQ(New->Ptr1, New->Ptr2);

	EXPECT_EQ(Orig.Ptr1->i8,  New->Ptr1->i8);
	EXPECT_EQ(Orig.Ptr1->i16, New->Ptr1->i16);
	EXPECT_EQ(Orig.Ptr1->i32, New->Ptr1->i32);
	EXPECT_EQ(Orig.Ptr1->i64, New->Ptr1->i64);
	EXPECT_EQ(Orig.Ptr1->u8,  New->Ptr1->u8);
	EXPECT_EQ(Orig.Ptr1->u16, New->Ptr1->u16);
	EXPECT_EQ(Orig.Ptr1->u32, New->Ptr1->u32);
	EXPECT_EQ(Orig.Ptr1->u64, New->Ptr1->u64);
	EXPECT_EQ(Orig.Ptr1->f32, New->Ptr1->f32);
	EXPECT_EQ(Orig.Ptr1->f64, New->Ptr1->f64);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, ptr_chain)
{
	PtrChain Ptr1 = { 1337,   0x0 };
	PtrChain Ptr2 = { 7331, &Ptr1 };
	PtrChain Ptr3 = { 13,   &Ptr2 };
	PtrChain Ptr4 = { 37,   &Ptr3 };

	PtrChain* New;

	PtrChain Loaded[10]; // this is so ugly!

	this->do_the_round_about( PtrChain::TYPE_ID, &Ptr4, Loaded, sizeof(Loaded) );
	New = Loaded;

	EXPECT_NE(Ptr4.Next, New->Next);
	EXPECT_NE(Ptr3.Next, New->Next->Next);
	EXPECT_NE(Ptr2.Next, New->Next->Next->Next);
	EXPECT_EQ(Ptr1.Next, New->Next->Next->Next->Next); // should be equal, 0x0

	EXPECT_EQ(Ptr4.Int, New->Int);
	EXPECT_EQ(Ptr3.Int, New->Next->Int);
	EXPECT_EQ(Ptr2.Int, New->Next->Next->Int);
	EXPECT_EQ(Ptr1.Int, New->Next->Next->Next->Int);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, ptr_chain_circle)
{
	// tests both circualar ptrs and reference to root-node!

	DoublePtrChain Ptr1 = { 1337, 0x0,   0x0 };
	DoublePtrChain Ptr2 = { 7331, &Ptr1, 0x0 };
	DoublePtrChain Ptr3 = { 13,   &Ptr2, 0x0 };
	DoublePtrChain Ptr4 = { 37,   &Ptr3, 0x0 };

	Ptr1.Prev = &Ptr2;
	Ptr2.Prev = &Ptr3;
	Ptr3.Prev = &Ptr4;

	DoublePtrChain* New;

	DoublePtrChain Loaded[10]; // this is so ugly!

	this->do_the_round_about( DoublePtrChain::TYPE_ID, &Ptr4, Loaded, sizeof(Loaded) );
	New = Loaded;

	// Ptr4
	EXPECT_NE(Ptr4.Next, New->Next);
	EXPECT_EQ(Ptr4.Prev, New->Prev); // is a null-ptr
	EXPECT_EQ(Ptr4.Int,  New->Int);

	// Ptr3
	EXPECT_NE(Ptr4.Next->Next, New->Next->Next);
	EXPECT_NE(Ptr4.Next->Prev, New->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Int,  New->Next->Int);
	EXPECT_EQ(New,             New->Next->Prev);

	// Ptr2
	EXPECT_NE(Ptr4.Next->Next->Next, New->Next->Next->Next);
	EXPECT_NE(Ptr4.Next->Next->Prev, New->Next->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Next->Int,  New->Next->Next->Int);
	EXPECT_EQ(New->Next,             New->Next->Next->Prev);

	// Ptr1
	EXPECT_EQ(Ptr4.Next->Next->Next->Next, New->Next->Next->Next->Next); // is null
	EXPECT_NE(Ptr4.Next->Next->Next->Prev, New->Next->Next->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Next->Next->Int,  New->Next->Next->Next->Int);
	EXPECT_EQ(New->Next->Next,             New->Next->Next->Next->Prev);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_pod_empty)
{
	PodArray1 Inst = { { NULL, 0 } };
	PodArray1 Loaded;

	this->do_the_round_about( PodArray1::TYPE_ID, &Inst, &Loaded, sizeof(PodArray1) );

	EXPECT_EQ(0u,  Loaded.u32_arr.count);
	EXPECT_EQ(0x0, Loaded.u32_arr.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_struct_empty)
{
	StructArray1 Inst = { { NULL, 0 } };
	StructArray1 Loaded;

	this->do_the_round_about( StructArray1::TYPE_ID, &Inst, &Loaded, sizeof(StructArray1) );

	EXPECT_EQ(0u,  Loaded.Array.count);
	EXPECT_EQ(0x0, Loaded.Array.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, array_string_empty)
{
	StringArray Inst = { { NULL, 0 } };
	StringArray Loaded;

	this->do_the_round_about( StringArray::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_EQ(0u,  Loaded.Strings.count);
	EXPECT_EQ(0x0, Loaded.Strings.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bug1)
{
	// There was some error packing arrays ;)

	BugTest1_InArray Arr[3] = { { 1337, 1338, 18 }, { 7331, 8331, 19 }, { 31337, 73313, 20 } } ;
	BugTest1 Inst;
	Inst.Arr.data  = Arr;
	Inst.Arr.count = DL_ARRAY_LENGTH(Arr);

	BugTest1 Loaded[10];

	this->do_the_round_about( BugTest1::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_EQ(Arr[0].u64_1, Loaded[0].Arr[0].u64_1);
	EXPECT_EQ(Arr[0].u64_2, Loaded[0].Arr[0].u64_2);
	EXPECT_EQ(Arr[0].u16,   Loaded[0].Arr[0].u16);
	EXPECT_EQ(Arr[1].u64_1, Loaded[0].Arr[1].u64_1);
	EXPECT_EQ(Arr[1].u64_2, Loaded[0].Arr[1].u64_2);
	EXPECT_EQ(Arr[1].u16,   Loaded[0].Arr[1].u16);
	EXPECT_EQ(Arr[2].u64_1, Loaded[0].Arr[2].u64_1);
	EXPECT_EQ(Arr[2].u64_2, Loaded[0].Arr[2].u64_2);
	EXPECT_EQ(Arr[2].u16,   Loaded[0].Arr[2].u16);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bug2)
{
	// some error converting from 32-bit-data to 64-bit.

	BugTest2_WithMat Arr[2] = { { 1337, { 1.0f, 0.0f, 0.0f, 0.0f,
										  0.0f, 1.0f, 0.0f, 0.0f,
										  0.0f, 0.0f, 1.0f, 0.0f,
										  0.0f, 0.0f, 1.0f, 0.0f } },

								{ 7331, { 0.0f, 0.0f, 0.0f, 1.0f,
										  0.0f, 0.0f, 1.0f, 0.0f,
										  0.0f, 1.0f, 0.0f, 0.0f,
										  1.0f, 0.0f, 0.0f, 0.0f } } };
	BugTest2 Inst;
	Inst.Instances.data  = Arr;
	Inst.Instances.count = 2;

	BugTest2 Loaded[40];

	this->do_the_round_about( BugTest2::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

 	EXPECT_EQ(Arr[0].iSubModel, Loaded[0].Instances[0].iSubModel);
 	EXPECT_EQ(Arr[1].iSubModel, Loaded[0].Instances[1].iSubModel);
 	EXPECT_ARRAY_EQ(16, Arr[0].Transform, Loaded[0].Instances[0].Transform);
 	EXPECT_ARRAY_EQ(16, Arr[1].Transform, Loaded[0].Instances[1].Transform);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
TYPED_TEST(DLBase, bug3)
{
	// testing bug where struct first in struct with ptr in substruct will not get patched on load.

	uint32_t arr[] = { 1337, 7331, 13, 37 };
	BugTest3 Inst;
	Inst.sub.arr.data = arr;
	Inst.sub.arr.count = DL_ARRAY_LENGTH( arr );

	BugTest3 Loaded[4];

	this->do_the_round_about( BugTest3::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_ARRAY_EQ( Inst.sub.arr.count,  Inst.sub.arr.data, Loaded[0].sub.arr.data);
}
#endif // DL_UNITTEST_ALL

#ifdef DL_UNITTEST_ALL
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
#endif // DL_UNITTEST_ALL

TEST(DLMisc, built_in_tl_eq_bin_file)
{
	const unsigned char built_in_tl[] =
	{
		#include "generated/unittest.bin.h"
	};
	const char* test_file = "local/generated/unittest.bin";
	
	FILE* f = fopen( test_file, "rb" );
	fseek( f, 0, SEEK_END );
	unsigned int read_tl_size = ftell(f);
	fseek( f, 0, SEEK_SET );
	
	unsigned char* read_tl = (unsigned char*)malloc(read_tl_size);
	
	fread( read_tl, read_tl_size, 1, f );
	
	fclose( f );
	
	EXPECT_EQ( DL_ARRAY_LENGTH(built_in_tl), read_tl_size );
	EXPECT_EQ( 0, memcmp( read_tl, built_in_tl, read_tl_size ) );
	
	free(read_tl);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
