#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, anyptr)
{
	Pods pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	AnyPtr orignal = { { &pods, Pods::TYPE_ID }, { &pods, Pods::TYPE_ID } };
	AnyPtr DL_ALIGN( 8 ) loaded[64];

	this->do_the_round_about( AnyPtr::TYPE_ID, &orignal, loaded, sizeof( loaded ) );

	EXPECT_NE(orignal.Ptr1.ptr,      loaded[0].Ptr1.ptr);
	EXPECT_EQ(orignal.Ptr1.tid,      loaded[0].Ptr1.tid);
	EXPECT_EQ(loaded[0].Ptr1.ptr,    loaded[0].Ptr2.ptr);
	EXPECT_EQ(loaded[0].Ptr1.tid,    loaded[0].Ptr2.tid);

	const Pods* loaded_pods = static_cast<Pods*>(loaded[0].Ptr1);
	EXPECT_EQ(pods.i8,  loaded_pods->i8);
	EXPECT_EQ(pods.i16, loaded_pods->i16);
	EXPECT_EQ(pods.i32, loaded_pods->i32);
	EXPECT_EQ(pods.i64, loaded_pods->i64);
	EXPECT_EQ(pods.u8,  loaded_pods->u8);
	EXPECT_EQ(pods.u16, loaded_pods->u16);
	EXPECT_EQ(pods.u32, loaded_pods->u32);
	EXPECT_EQ(pods.u64, loaded_pods->u64);
	EXPECT_EQ(pods.f32, loaded_pods->f32);
	EXPECT_EQ(pods.f64, loaded_pods->f64);
}

TYPED_TEST(DLBase, anyptr_null)
{
	Pods pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	AnyPtr orignal = { { nullptr, 0 }, { &pods, Pods::TYPE_ID } };
	AnyPtr DL_ALIGN( 8 ) loaded[64];

	this->do_the_round_about( AnyPtr::TYPE_ID, &orignal, loaded, sizeof( loaded ) );

	EXPECT_EQ(orignal.Ptr1.ptr,      loaded[0].Ptr1.ptr); // nullptr
	EXPECT_EQ(orignal.Ptr2.tid,      loaded[0].Ptr2.tid);
	EXPECT_NE(orignal.Ptr2.ptr,      loaded[0].Ptr2.ptr); 

	const Pods* loaded_pods = static_cast<Pods*>(loaded[0].Ptr2);
	EXPECT_EQ(pods.i8,  loaded_pods->i8);
	EXPECT_EQ(pods.i16, loaded_pods->i16);
	EXPECT_EQ(pods.i32, loaded_pods->i32);
	EXPECT_EQ(pods.i64, loaded_pods->i64);
	EXPECT_EQ(pods.u8,  loaded_pods->u8);
	EXPECT_EQ(pods.u16, loaded_pods->u16);
	EXPECT_EQ(pods.u32, loaded_pods->u32);
	EXPECT_EQ(pods.u64, loaded_pods->u64);
	EXPECT_EQ(pods.f32, loaded_pods->f32);
	EXPECT_EQ(pods.f64, loaded_pods->f64);
}

TYPED_TEST(DLBase, anyptr_with_pointers)
{
	Pods2 pods = { 1, 2 };
	PodPtr ptr = { &pods };
	AnyPtr orignal = { { &ptr, ptr.TYPE_ID }, { &ptr, ptr.TYPE_ID } };
	AnyPtr DL_ALIGN( 8 ) loaded[64];

	this->do_the_round_about( AnyPtr::TYPE_ID, &orignal, loaded, sizeof( loaded ) );

	EXPECT_NE(orignal.Ptr1.ptr,      loaded[0].Ptr1.ptr);
	EXPECT_EQ(orignal.Ptr1.tid,      loaded[0].Ptr1.tid);
	EXPECT_EQ(loaded[0].Ptr1.ptr,    loaded[0].Ptr2.ptr);
	EXPECT_EQ(loaded[0].Ptr1.tid,    loaded[0].Ptr2.tid);

	const PodPtr* loaded_pods = static_cast<PodPtr*>(loaded[0].Ptr1);
	EXPECT_EQ(pods.Int1, loaded_pods->PodPtr1->Int1);
	EXPECT_EQ(pods.Int2, loaded_pods->PodPtr1->Int2);
}

TYPED_TEST(DLBase, anyptr_with_array)
{
	Pods pods    = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	Pods* pod_ptr[3] = { &pods, nullptr, &pods };
	ptrArray ptr   = { { pod_ptr, DL_ARRAY_LENGTH(pod_ptr) } };
	AnyPtr orignal = { { &ptr, ptr.TYPE_ID }, { &ptr, ptr.TYPE_ID } };
	AnyPtr DL_ALIGN( 8 ) loaded[64];

	this->do_the_round_about( AnyPtr::TYPE_ID, &orignal, loaded, sizeof( loaded ) );

	EXPECT_NE(orignal.Ptr1.ptr,      loaded[0].Ptr1.ptr);
	EXPECT_EQ(orignal.Ptr1.tid,      loaded[0].Ptr1.tid);
	EXPECT_EQ(loaded[0].Ptr1.ptr,    loaded[0].Ptr2.ptr);
	EXPECT_EQ(loaded[0].Ptr1.tid,    loaded[0].Ptr2.tid);

	const ptrArray* loaded_pods = static_cast<ptrArray*>(loaded[0].Ptr1);
	EXPECT_EQ(loaded_pods->arr[0], loaded_pods->arr[2]);
	EXPECT_EQ(pods.i8,  loaded_pods->arr[0]->i8);
	EXPECT_EQ(pods.i16, loaded_pods->arr[0]->i16);
	EXPECT_EQ(pods.i32, loaded_pods->arr[0]->i32);
	EXPECT_EQ(pods.i64, loaded_pods->arr[0]->i64);
	EXPECT_EQ(pods.u8,  loaded_pods->arr[0]->u8);
	EXPECT_EQ(pods.u16, loaded_pods->arr[0]->u16);
	EXPECT_EQ(pods.u32, loaded_pods->arr[0]->u32);
	EXPECT_EQ(pods.u64, loaded_pods->arr[0]->u64);
	EXPECT_EQ(pods.f32, loaded_pods->arr[0]->f32);
	EXPECT_EQ(pods.f64, loaded_pods->arr[0]->f64);
}

TYPED_TEST(DLBase, anyptr_chain_circle)
{
	// tests both circualar ptrs and reference to root-node!

	AnyPtr ptr1   = { { 0x0, AnyPtr::TYPE_ID }, { 0x0, AnyPtr::TYPE_ID } };
	AnyPtr ptr2   = { { &ptr1, AnyPtr::TYPE_ID }, { 0x0, AnyPtr::TYPE_ID } };
	AnyPtr ptr3   = { { &ptr2, AnyPtr::TYPE_ID }, { 0x0, AnyPtr::TYPE_ID } };
	AnyPtr ptr4   = { { &ptr3, AnyPtr::TYPE_ID }, { 0x0, AnyPtr::TYPE_ID } };

	ptr1.Ptr2.ptr = &ptr2;
	ptr2.Ptr2.ptr = &ptr3;
	ptr3.Ptr2.ptr = &ptr4;

	AnyPtr loaded[10];

	this->do_the_round_about( AnyPtr::TYPE_ID, &ptr4, loaded, sizeof( loaded ) );

	// Ptr4
	EXPECT_NE(ptr4.Ptr1.ptr, loaded[0].Ptr1.ptr);
	EXPECT_EQ(ptr4.Ptr2.ptr, loaded[0].Ptr2.ptr); // is a null-ptr
	EXPECT_EQ(ptr4.Ptr1.tid, loaded[0].Ptr1.tid);

	// Ptr3
	EXPECT_EQ(&loaded[0], static_cast<AnyPtr*>(loaded[0].Ptr1.ptr)->Ptr2.ptr);

	// Ptr2
	EXPECT_EQ(loaded[0].Ptr1.ptr, static_cast<AnyPtr*>(static_cast<AnyPtr*>(loaded[0].Ptr1.ptr)->Ptr1.ptr)->Ptr2.ptr);

	// Ptr1
	EXPECT_EQ(static_cast<AnyPtr*>(loaded[0].Ptr1.ptr)->Ptr1.ptr, static_cast<AnyPtr*>(static_cast<AnyPtr*>(static_cast<AnyPtr*>(loaded[0].Ptr1.ptr)->Ptr1.ptr)->Ptr1.ptr)->Ptr2.ptr);
	EXPECT_EQ(0, static_cast<AnyPtr*>(static_cast<AnyPtr*>(static_cast<AnyPtr*>(loaded[0].Ptr1.ptr)->Ptr1.ptr)->Ptr1.ptr)->Ptr1.ptr);
}

TYPED_TEST(DLBase, inline_anyptr_array)
{
	Pods2 p1 = { 1, 2 };
	Pods2 p2 = { 3, 4 };
	WithInlineAnyptrArray ptr_array = { { { &p1, Pods2::TYPE_ID }, { &p2, Pods2::TYPE_ID }, { nullptr, 0 }, { &p1, Pods2::TYPE_ID } } };

	WithInlineAnyptrArray loaded[32];

	this->do_the_round_about( WithInlineAnyptrArray::TYPE_ID, &ptr_array, &loaded, sizeof( loaded ) );

	EXPECT_EQ( p1.Int1, static_cast<Pods2*>(loaded[0].Array[0].ptr)->Int1 );
	EXPECT_EQ( p1.Int2, static_cast<Pods2*>(loaded[0].Array[0].ptr)->Int2 );
	EXPECT_EQ( p2.Int1, static_cast<Pods2*>(loaded[0].Array[1].ptr)->Int1 );
	EXPECT_EQ( p2.Int2, static_cast<Pods2*>(loaded[0].Array[1].ptr)->Int2 );
	EXPECT_EQ( nullptr, loaded[0].Array[2].ptr );
	EXPECT_EQ( p1.Int1, static_cast<Pods2*>(loaded[0].Array[3].ptr)->Int1 );
	EXPECT_EQ( p1.Int2, static_cast<Pods2*>(loaded[0].Array[3].ptr)->Int2 );

	EXPECT_NE( loaded[0].Array[0].ptr, loaded[0].Array[1].ptr );
	EXPECT_EQ( loaded[0].Array[0].ptr, loaded[0].Array[3].ptr );
}

TYPED_TEST(DLBase, anyptr_array)
{
	Pods2 p1 = { 1, 2 };
	Pods2 p2 = { 3, 4 };
	dl_anyptr arr[] = { { &p1, Pods2::TYPE_ID }, { &p2, Pods2::TYPE_ID }, { nullptr, 0 }, { &p1, Pods2::TYPE_ID } };
	anyptrArray original;
	original.arr.data  = arr;
	original.arr.count = DL_ARRAY_LENGTH(arr);

	anyptrArray loaded[32];

	this->do_the_round_about( anyptrArray::TYPE_ID, &original, &loaded, sizeof( loaded ) );

	EXPECT_EQ( original.arr.count, loaded[0].arr.count );
	EXPECT_EQ( p1.Int1, static_cast<Pods2*>(loaded[0].arr[0].ptr)->Int1 );
	EXPECT_EQ( p1.Int2, static_cast<Pods2*>(loaded[0].arr[0].ptr)->Int2 );
	EXPECT_EQ( p2.Int1, static_cast<Pods2*>(loaded[0].arr[1].ptr)->Int1 );
	EXPECT_EQ( p2.Int2, static_cast<Pods2*>(loaded[0].arr[1].ptr)->Int2 );
	EXPECT_EQ( nullptr, loaded[0].arr[2].ptr );
	EXPECT_EQ( p1.Int1, static_cast<Pods2*>(loaded[0].arr[3].ptr)->Int1 );
	EXPECT_EQ( p1.Int2, static_cast<Pods2*>(loaded[0].arr[3].ptr)->Int2 );

	EXPECT_NE( &p2, loaded[0].arr[1].ptr );
	EXPECT_NE( loaded[0].arr[0].ptr, loaded[0].arr[1].ptr );
	EXPECT_EQ( loaded[0].arr[0].ptr, loaded[0].arr[3].ptr );
}
