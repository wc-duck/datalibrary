#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, ptr)
{
	Pods pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SimplePtr  orignal = { &pods, &pods };
	SimplePtr DL_ALIGN(8) loaded[64];

	this->do_the_round_about( SimplePtr::TYPE_ID, &orignal, loaded, sizeof(loaded) );

	EXPECT_NE(orignal.Ptr1,      loaded[0].Ptr1);
	EXPECT_EQ(loaded->Ptr1,      loaded[0].Ptr2);
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

TYPED_TEST(DLBase, array_struct_circular_ptr_holder_array)
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

TYPED_TEST(DLBase, inline_ptr_array)
{
	Pods2 p1 = { 1, 2 };
	Pods2 p2 = { 3, 4 };
	inline_ptr_array ptr_array;
	ptr_array.arr[0] = &p1;
	ptr_array.arr[1] = &p2;
	ptr_array.arr[2] = &p1;

	inline_ptr_array loaded[32];

	this->do_the_round_about( inline_ptr_array::TYPE_ID, &ptr_array, &loaded, sizeof(loaded) );

	EXPECT_EQ( p1.Int1, loaded[0].arr[0]->Int1 );
	EXPECT_EQ( p1.Int2, loaded[0].arr[0]->Int2 );
	EXPECT_EQ( p2.Int1, loaded[0].arr[1]->Int1 );
	EXPECT_EQ( p2.Int2, loaded[0].arr[1]->Int2 );
	EXPECT_EQ( p1.Int1, loaded[0].arr[2]->Int1 );
	EXPECT_EQ( p1.Int2, loaded[0].arr[2]->Int2 );

	EXPECT_NE( loaded[0].arr[0], loaded[0].arr[1] );
	EXPECT_EQ( loaded[0].arr[0], loaded[0].arr[2] );
}

TYPED_TEST(DLBase, ptr_array)
{
	Pods2 p1 = { 1, 2 };
	Pods2 p2 = { 3, 4 };
	Pods2* arr[] = { &p1, &p2, &p1 };
	ptr_array original;
	original.arr.data = arr;
	original.arr.count = DL_ARRAY_LENGTH(arr);

	ptr_array loaded[32];

	this->do_the_round_about( ptr_array::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ( original.arr.count, loaded[0].arr.count );
	EXPECT_EQ( p1.Int1, loaded[0].arr[0]->Int1 );
	EXPECT_EQ( p1.Int2, loaded[0].arr[0]->Int2 );
	EXPECT_EQ( p2.Int1, loaded[0].arr[1]->Int1 );
	EXPECT_EQ( p2.Int2, loaded[0].arr[1]->Int2 );
	EXPECT_EQ( p1.Int1, loaded[0].arr[2]->Int1 );
	EXPECT_EQ( p1.Int2, loaded[0].arr[2]->Int2 );

	EXPECT_NE( loaded[0].arr[0], loaded[0].arr[1] );
	EXPECT_EQ( loaded[0].arr[0], loaded[0].arr[2] );
}
