#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, union_simple)
{
	// simple union test
	test_union_simple original;
	original.type = test_union_simple_type_item1;
	original.value.item1 = 1337;

	test_union_simple loaded;

	this->do_the_round_about( test_union_simple::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded.type, original.type );
	EXPECT_EQ( loaded.value.item1, original.value.item1 );

	original.type = test_union_simple_type_item2;
	original.value.item2 = 13.37f;

	this->do_the_round_about( test_union_simple::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded.type, original.type );
	EXPECT_EQ( loaded.value.item1, original.value.item1 );
}

TYPED_TEST(DLBase, union_with_struct)
{
	// simple union test
	test_union_simple original;
	original.type = test_union_simple_type_item3;
    original.value.item3.i8  = 0;
    original.value.item3.i16 = 1;
    original.value.item3.i32 = 2;
    original.value.item3.i64 = 3;
    original.value.item3.u8  = 4;
    original.value.item3.u16 = 5;
    original.value.item3.u32 = 6;
    original.value.item3.u64 = 7;
    original.value.item3.f32 = 8.0f;
    original.value.item3.f64 = 9.0;

	test_union_simple loaded;

	this->do_the_round_about( test_union_simple::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded.type, original.type );
	EXPECT_EQ( loaded.value.item3.i8,  original.value.item3.i8  );
	EXPECT_EQ( loaded.value.item3.i16, original.value.item3.i16 );
	EXPECT_EQ( loaded.value.item3.i32, original.value.item3.i32 );
	EXPECT_EQ( loaded.value.item3.i64, original.value.item3.i64 );
	EXPECT_EQ( loaded.value.item3.u8,  original.value.item3.u8  );
	EXPECT_EQ( loaded.value.item3.u16, original.value.item3.u16 );
	EXPECT_EQ( loaded.value.item3.u32, original.value.item3.u32 );
	EXPECT_EQ( loaded.value.item3.u64, original.value.item3.u64 );
	EXPECT_EQ( loaded.value.item3.f32, original.value.item3.f32 );
	EXPECT_EQ( loaded.value.item3.f64, original.value.item3.f64 );
}

TYPED_TEST(DLBase, union_with_inline_array)
{
	test_union_inline_array original;
	original.type = test_union_inline_array_type_i32;
	original.value.i32 = 4567;

	test_union_inline_array loaded;

	this->do_the_round_about( test_union_inline_array::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded.type, original.type );
	EXPECT_EQ( loaded.value.i32, original.value.i32 );

	original.type = test_union_inline_array_type_inlarr;
	original.value.inlarr[0] = 1;
	original.value.inlarr[1] = 2;
	original.value.inlarr[2] = 3;
	original.value.inlarr[3] = 4;
	original.value.inlarr[4] = 5;
	original.value.inlarr[5] = 6;

	this->do_the_round_about( test_union_inline_array::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded.type, original.type );
	EXPECT_EQ( loaded.value.inlarr[0], original.value.inlarr[0] );
	EXPECT_EQ( loaded.value.inlarr[1], original.value.inlarr[1] );
	EXPECT_EQ( loaded.value.inlarr[2], original.value.inlarr[2] );
	EXPECT_EQ( loaded.value.inlarr[3], original.value.inlarr[3] );
	EXPECT_EQ( loaded.value.inlarr[4], original.value.inlarr[4] );
	EXPECT_EQ( loaded.value.inlarr[5], original.value.inlarr[5] );
}

TYPED_TEST(DLBase, union_with_array)
{
	test_union_array original;
	original.type = test_union_array_type_i32;
	original.value.i32 = 4567;

	test_union_array loaded[10];

	this->do_the_round_about( test_union_array::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ( loaded[0].type, original.type );
	EXPECT_EQ( loaded[0].value.i32, original.value.i32 );

	int32_t arrdata[] = {1,2,3,4,5,6,7};

	original.type = test_union_array_type_arr;
	original.value.arr.data = arrdata;
	original.value.arr.count = DL_ARRAY_LENGTH(arrdata);

	this->do_the_round_about( test_union_array::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( loaded[0].type, original.type );
	EXPECT_EQ( loaded[0].value.arr.count, original.value.arr.count );
	for( uint32_t i = 0; i < loaded->value.arr.count; ++i )
		EXPECT_EQ( loaded[0].value.arr[i], original.value.arr[i] );
}

TYPED_TEST(DLBase, union_with_ptr)
{
	// simple union test
	Pods p;
	p.i8  = 1;
	p.i16 = 2;
	p.i32 = 3;
	p.i64 = 4;
	p.u8  = 5;
	p.u16 = 6;
	p.u32 = 7;
	p.u64 = 8;
	p.f32 = 9.0f;
	p.f64 = 10.0f;
	test_union_ptr original;
	original.value.p1 = &p;
	original.type = test_union_ptr_type_p1;

	test_union_ptr loaded[10];

	this->do_the_round_about( test_union_ptr::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ( original.type, loaded[0].type );
	EXPECT_EQ( original.value.p1->i8,  loaded[0].value.p1->i8 );
	EXPECT_EQ( original.value.p1->i16, loaded[0].value.p1->i16 );
	EXPECT_EQ( original.value.p1->i32, loaded[0].value.p1->i32 );
	EXPECT_EQ( original.value.p1->i64, loaded[0].value.p1->i64 );
	EXPECT_EQ( original.value.p1->u8,  loaded[0].value.p1->u8 );
	EXPECT_EQ( original.value.p1->u16, loaded[0].value.p1->u16 );
	EXPECT_EQ( original.value.p1->u32, loaded[0].value.p1->u32 );
	EXPECT_EQ( original.value.p1->u64, loaded[0].value.p1->u64 );
	EXPECT_EQ( original.value.p1->f32, loaded[0].value.p1->f32 );
	EXPECT_EQ( original.value.p1->f64, loaded[0].value.p1->f64 );
}

// TODO: add test in dl_tests_txt.cpp of setting more than one member in txt data that it is an error.

TYPED_TEST(DLBase, union_in_inline_array)
{
	test_inline_array_of_unions original;
	original.arr[0].type = test_union_simple_type_item1;
	original.arr[1].type = test_union_simple_type_item2;
	original.arr[2].type = test_union_simple_type_item3;

	original.arr[0].value.item1 = 1;
	original.arr[1].value.item2 = 2.0f;

	original.arr[2].value.item3.i8  = 3;
	original.arr[2].value.item3.i16 = 4;
	original.arr[2].value.item3.i32 = 5;
	original.arr[2].value.item3.i64 = 6;
	original.arr[2].value.item3.u8  = 7;
	original.arr[2].value.item3.u16 = 8;
	original.arr[2].value.item3.u32 = 9;
	original.arr[2].value.item3.u64 = 10;
	original.arr[2].value.item3.f32 = 11.0f;
	original.arr[2].value.item3.f64 = 12.0;

	test_inline_array_of_unions loaded[10];

	this->do_the_round_about( test_inline_array_of_unions::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ( original.arr[0].type, loaded[0].arr[0].type );
	EXPECT_EQ( original.arr[1].type, loaded[0].arr[1].type );
	EXPECT_EQ( original.arr[2].type, loaded[0].arr[2].type );

	EXPECT_EQ( original.arr[0].value.item1, loaded[0].arr[0].value.item1 );

	EXPECT_EQ( original.arr[1].value.item2, loaded[0].arr[1].value.item2 );

	EXPECT_EQ( original.arr[2].value.item3.i8,  loaded[0].arr[2].value.item3.i8  );
	EXPECT_EQ( original.arr[2].value.item3.i16, loaded[0].arr[2].value.item3.i16 );
	EXPECT_EQ( original.arr[2].value.item3.i32, loaded[0].arr[2].value.item3.i32 );
	EXPECT_EQ( original.arr[2].value.item3.i64, loaded[0].arr[2].value.item3.i64 );
	EXPECT_EQ( original.arr[2].value.item3.u8,  loaded[0].arr[2].value.item3.u8  );
	EXPECT_EQ( original.arr[2].value.item3.u16, loaded[0].arr[2].value.item3.u16 );
	EXPECT_EQ( original.arr[2].value.item3.u32, loaded[0].arr[2].value.item3.u32 );
	EXPECT_EQ( original.arr[2].value.item3.u64, loaded[0].arr[2].value.item3.u64 );
	EXPECT_EQ( original.arr[2].value.item3.f32, loaded[0].arr[2].value.item3.f32 );
	EXPECT_EQ( original.arr[2].value.item3.f64, loaded[0].arr[2].value.item3.f64 );
}

TYPED_TEST(DLBase, union_in_array)
{
}

TYPED_TEST(DLBase, ptr_to_union)
{
}
