/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl_reflect.h>

#include "dl_test_common.h"

template<class T>
struct TAlignmentOf
{
        struct CAlign { ~CAlign() {}; unsigned char m_Dummy; T m_T; };
        enum { ALIGNOF = sizeof(CAlign) - sizeof(T) };
};
#define DL_ALIGNMENTOF(Type) TAlignmentOf<Type>::ALIGNOF

class DLReflect : public DL {};

TEST_F(DLReflect, pods)
{
	dl_type_info_t   Info;
	dl_member_info_t Members[128];
	memset( &Info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK(dl_reflect_get_type_info( Ctx, Pods::TYPE_ID, &Info ));
	EXPECT_DL_ERR_OK(dl_reflect_get_type_members( Ctx, Pods::TYPE_ID, Members, DL_ARRAY_LENGTH(Members) ));

	EXPECT_EQ   ((uint32_t)Pods::TYPE_ID, Info.tid );
	EXPECT_STREQ("Pods", Info.name);
	EXPECT_EQ   (10u,    Info.member_count);

	EXPECT_STREQ("i8",                   Members[0].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[0].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT8,   Members[0].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("i16"  ,                Members[1].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[1].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT16,  Members[1].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("i32",                  Members[2].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[2].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT32,  Members[2].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("i64",                  Members[3].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[3].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT64,  Members[3].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("u8",                   Members[4].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[4].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT8,  Members[4].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("u16",                  Members[5].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[5].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT16, Members[5].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("u32",                  Members[6].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[6].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT32, Members[6].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("u64",                  Members[7].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[7].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT64, Members[7].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("f32",                  Members[8].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[8].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP32,   Members[8].type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("f64",                  Members[9].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[9].type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP64,   Members[9].type & DL_TYPE_STORAGE_MASK);
}

#define CHECK_TYPE_INFO_CORRECT( TYPE_NAME, MEM_COUNT ) { \
	dl_type_info_t ti; \
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, TYPE_NAME::TYPE_ID, &ti ) ); \
	EXPECT_EQ((uint32_t)TYPE_NAME::TYPE_ID,      ti.tid); \
	EXPECT_STREQ( #TYPE_NAME,                    ti.name ); \
	EXPECT_EQ(sizeof(TYPE_NAME),                 ti.size); \
	EXPECT_EQ((size_t)DL_ALIGNMENTOF(TYPE_NAME), ti.alignment); \
	EXPECT_EQ(MEM_COUNT,                         ti.member_count); \
}

TEST_F(DLReflect, get_type_info)
{
	CHECK_TYPE_INFO_CORRECT( Pods, 10u );
	CHECK_TYPE_INFO_CORRECT( Pods2, 2u );
	CHECK_TYPE_INFO_CORRECT( Strings, 2u );
	CHECK_TYPE_INFO_CORRECT( PtrChain, 2u );
	CHECK_TYPE_INFO_CORRECT( MoreBits, 2u );
	CHECK_TYPE_INFO_CORRECT( MorePods, 2u );
	CHECK_TYPE_INFO_CORRECT( TestBits, 7u );
	CHECK_TYPE_INFO_CORRECT( PodArray1, 1u );
	CHECK_TYPE_INFO_CORRECT( PodArray2, 1u );
	CHECK_TYPE_INFO_CORRECT( SimplePtr, 2u );
	CHECK_TYPE_INFO_CORRECT( StringArray, 1u );
	CHECK_TYPE_INFO_CORRECT( StructArray1, 1u );
	CHECK_TYPE_INFO_CORRECT( Pod2InStruct, 2u );
	CHECK_TYPE_INFO_CORRECT( WithInlineArray, 1u );
	CHECK_TYPE_INFO_CORRECT( StringInlineArray, 1u );
	CHECK_TYPE_INFO_CORRECT( ArrayEnum, 1u );
	CHECK_TYPE_INFO_CORRECT( InlineArrayEnum, 1u );
	CHECK_TYPE_INFO_CORRECT( Pod2InStructInStruct, 1u );
	CHECK_TYPE_INFO_CORRECT( WithInlineStructArray, 1u );
	CHECK_TYPE_INFO_CORRECT( WithInlineStructStructArray, 1u );
	CHECK_TYPE_INFO_CORRECT( DoublePtrChain, 3u );
	CHECK_TYPE_INFO_CORRECT( A128BitAlignedType, 1u );
	CHECK_TYPE_INFO_CORRECT( BugTest1, 1u );
	CHECK_TYPE_INFO_CORRECT( BugTest1_InArray, 3u );
	CHECK_TYPE_INFO_CORRECT( circular_array, 2u );
	CHECK_TYPE_INFO_CORRECT( circular_array_ptr_holder, 1u );
	CHECK_TYPE_INFO_CORRECT( BitBitfield64, 4u );
	CHECK_TYPE_INFO_CORRECT( test_union_simple, 3u );
	CHECK_TYPE_INFO_CORRECT( test_union_array, 2u );
}

TEST_F(DLReflect, is_extern_reflected)
{
	dl_type_info_t info;
	memset( &info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods_TYPE_ID, &info ) );
	EXPECT_FALSE( info.is_extern );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, vec3_test_TYPE_ID, &info ) );
	EXPECT_TRUE( info.is_extern );
}

TEST_F(DLReflect, is_union_reflected)
{
	dl_type_info_t info;
	memset( &info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods_TYPE_ID, &info ) );
	EXPECT_FALSE( info.is_union );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, test_union_simple_TYPE_ID, &info ) );
	EXPECT_TRUE( info.is_union );
}

TEST_F(DLReflect, type_lookup)
{
	dl_typeid_t type_id = 0;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "Pods2", &type_id) );
	EXPECT_TRUE(Pods2::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "StructArray1", &type_id) );
	EXPECT_TRUE(StructArray1::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "StringInlineArray", &type_id) );
	EXPECT_TRUE(StringInlineArray::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "TestBits", &type_id) );
	EXPECT_TRUE(TestBits::TYPE_ID == type_id);

	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bloobloo", &type_id) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bopp", &type_id) );
}

TEST_F(DLReflect, type_strings)
{
	// tests that all strings in tlds is reflected as expected... test types/members and enums from both unittest.tld and unittest2.tld that is both loaded in Ctx.
	dl_type_info_t Pods2_type_info;
	dl_type_info_t BugTest1_InArray_type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods2_TYPE_ID, &Pods2_type_info ) );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, BugTest1_InArray_TYPE_ID, &BugTest1_InArray_type_info ) );

	EXPECT_EQ( 2u, Pods2_type_info.member_count );
	EXPECT_EQ( 3u, BugTest1_InArray_type_info.member_count );
	EXPECT_STREQ( "Pods2", Pods2_type_info.name );
	EXPECT_STREQ( "BugTest1_InArray", BugTest1_InArray_type_info.name );

	// ... and members ...
	dl_member_info_t Pods2_members[2];
	dl_member_info_t BugTest1_InArray_members[3];
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, Pods2_TYPE_ID, Pods2_members, 2 ) );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, BugTest1_InArray_TYPE_ID, BugTest1_InArray_members, 3 ) );

	EXPECT_STREQ( "Int1", Pods2_members[0].name );
	EXPECT_STREQ( "Int2", Pods2_members[1].name );

	EXPECT_STREQ( "u64_1", BugTest1_InArray_members[0].name );
	EXPECT_STREQ( "u64_2", BugTest1_InArray_members[1].name );
	EXPECT_STREQ( "u16",   BugTest1_InArray_members[2].name );
}
