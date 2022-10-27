#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, anyarray)
{
	Pods2 ps[] = { { 1, 2 }, { 3, 4 } };
	anyArray original = { { { ps, DL_ARRAY_LENGTH(ps) }, Pods2::TYPE_ID } };
	anyArray loaded[64];

	this->do_the_round_about( anyArray::TYPE_ID, &original, loaded, sizeof( loaded ) );
	
	EXPECT_EQ( original.arr.array.count, loaded[0].arr.array.count );
	EXPECT_EQ( original.arr.tid, loaded[0].arr.tid );
	dl_array<Pods2>& loaded_array = static_cast<dl_array<Pods2>&>(loaded[0].arr);
	EXPECT_EQ( ps[0].Int1, loaded_array[0].Int1 );
	EXPECT_EQ( ps[0].Int2, loaded_array[0].Int2 );
	EXPECT_EQ( ps[1].Int1, loaded_array[1].Int1 );
	EXPECT_EQ( ps[1].Int2, loaded_array[1].Int2 );
}

TYPED_TEST(DLBase, ArrayOfAnyarray)
{
	Pods2 ps1[] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
	PodPtr ps2[] = { { &ps1[2] }, { &ps1[1] }, { &ps1[0] } };
	dl_anyarray arr[] = { { { ps1, DL_ARRAY_LENGTH(ps1) }, ps1[0].TYPE_ID }, { { ps2, DL_ARRAY_LENGTH(ps2) }, ps2[0].TYPE_ID } };
	anyArrayArray original = { { arr, DL_ARRAY_LENGTH(arr) } };
	anyArrayArray loaded[64];

	this->do_the_round_about( original.TYPE_ID, &original, loaded, sizeof( loaded ) );
	
	EXPECT_EQ( original.arr.count, loaded[0].arr.count );
	EXPECT_EQ( original.arr[0].array.count, loaded[0].arr[0].array.count );
	EXPECT_EQ( original.arr[1].array.count, loaded[0].arr[1].array.count );
	EXPECT_EQ( original.arr[0].tid, loaded[0].arr[0].tid );
	EXPECT_EQ( original.arr[1].tid, loaded[0].arr[1].tid );
	dl_array<Pods2>& loaded_pods2 = static_cast<dl_array<Pods2>&>(loaded[0].arr[0]);
	EXPECT_EQ( ps1[0].Int1, loaded_pods2[0].Int1 );
	EXPECT_EQ( ps1[0].Int2, loaded_pods2[0].Int2 );
	EXPECT_EQ( ps1[1].Int1, loaded_pods2[1].Int1 );
	EXPECT_EQ( ps1[1].Int2, loaded_pods2[1].Int2 );
	EXPECT_EQ( ps1[2].Int1, loaded_pods2[2].Int1 );
	EXPECT_EQ( ps1[2].Int2, loaded_pods2[2].Int2 );
	dl_array<PodPtr>& loaded_podptr = static_cast<dl_array<PodPtr>&>(loaded[0].arr[1]);
	EXPECT_EQ( ps1[0].Int1, loaded_podptr[2].PodPtr1->Int1 );
	EXPECT_EQ( ps1[0].Int2, loaded_podptr[2].PodPtr1->Int2 );
	EXPECT_EQ( ps1[1].Int1, loaded_podptr[1].PodPtr1->Int1 );
	EXPECT_EQ( ps1[1].Int2, loaded_podptr[1].PodPtr1->Int2 );
	EXPECT_EQ( ps1[2].Int1, loaded_podptr[0].PodPtr1->Int1 );
	EXPECT_EQ( ps1[2].Int2, loaded_podptr[0].PodPtr1->Int2 );
}

TYPED_TEST(DLBase, InlineArrayOfAnyarray)
{
	Pods2 ps1[] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
	PodPtr ps2[] = { { &ps1[2] }, { &ps1[1] }, { &ps1[0] } };
	AnyPtr any[] = { { { &ps1[0], ps1[0].TYPE_ID }, { &ps1[1], ps1[1].TYPE_ID } } };
	WithInlineAnyarrayArray original = { { { { ps1, DL_ARRAY_LENGTH(ps1) }, ps1[0].TYPE_ID }, { { ps2, DL_ARRAY_LENGTH(ps2) }, ps2[0].TYPE_ID }, { { any, DL_ARRAY_LENGTH(any) }, any[0].TYPE_ID } } };
	WithInlineAnyarrayArray loaded[64];

	this->do_the_round_about( original.TYPE_ID, &original, loaded, sizeof( loaded ) );
	
	EXPECT_EQ( original.Array[0].array.count, loaded[0].Array[0].array.count );
	EXPECT_EQ( original.Array[1].array.count, loaded[0].Array[1].array.count );
	EXPECT_EQ( original.Array[2].array.count, loaded[0].Array[2].array.count );
	EXPECT_EQ( original.Array[0].tid, loaded[0].Array[0].tid );
	EXPECT_EQ( original.Array[1].tid, loaded[0].Array[1].tid );
	EXPECT_EQ( original.Array[2].tid, loaded[0].Array[2].tid );
	dl_array<Pods2>& loaded_pods2 = static_cast<dl_array<Pods2>&>(loaded[0].Array[0]);
	EXPECT_EQ( ps1[0].Int1, loaded_pods2[0].Int1 );
	EXPECT_EQ( ps1[0].Int2, loaded_pods2[0].Int2 );
	EXPECT_EQ( ps1[1].Int1, loaded_pods2[1].Int1 );
	EXPECT_EQ( ps1[1].Int2, loaded_pods2[1].Int2 );
	EXPECT_EQ( ps1[2].Int1, loaded_pods2[2].Int1 );
	EXPECT_EQ( ps1[2].Int2, loaded_pods2[2].Int2 );
	dl_array<PodPtr>& loaded_podptr = static_cast<dl_array<PodPtr>&>(loaded[0].Array[1]);
	EXPECT_EQ( ps1[0].Int1, loaded_podptr[2].PodPtr1->Int1 );
	EXPECT_EQ( ps1[0].Int2, loaded_podptr[2].PodPtr1->Int2 );
	EXPECT_EQ( ps1[1].Int1, loaded_podptr[1].PodPtr1->Int1 );
	EXPECT_EQ( ps1[1].Int2, loaded_podptr[1].PodPtr1->Int2 );
	EXPECT_EQ( ps1[2].Int1, loaded_podptr[0].PodPtr1->Int1 );
	EXPECT_EQ( ps1[2].Int2, loaded_podptr[0].PodPtr1->Int2 );
	dl_array<AnyPtr>& loaded_any = static_cast<dl_array<AnyPtr>&>(loaded[0].Array[2]);
	EXPECT_EQ( ps1[0].Int1, static_cast<Pods2*>(loaded_any[0].Ptr1)->Int1 );
	EXPECT_EQ( ps1[0].Int2, static_cast<Pods2*>(loaded_any[0].Ptr1)->Int2 );
	EXPECT_EQ( ps1[1].Int1, static_cast<Pods2*>(loaded_any[0].Ptr2)->Int1 );
	EXPECT_EQ( ps1[1].Int2, static_cast<Pods2*>(loaded_any[0].Ptr2)->Int2 );
}
