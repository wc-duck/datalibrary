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

TEST_F(DL, ReflectPods)
{
	dl_type_info_t   Info;
	dl_member_info_t Members[128];

	dl_error_t err = dl_reflect_get_type_info(Ctx, Pods::TYPE_ID, &Info, Members, DL_ARRAY_LENGTH(Members));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

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

TEST_F(DL, SizeAndAlignment)
{
	EXPECT_EQ(sizeof(Pods2),                               dl_size_of_type(Ctx, Pods2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(Pods2),                       dl_alignment_of_type(Ctx, Pods2::TYPE_ID));
	EXPECT_EQ(sizeof(PtrChain),                            dl_size_of_type(Ctx, PtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(PtrChain),                    dl_alignment_of_type(Ctx, PtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(Pods),                                dl_size_of_type(Ctx, Pods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(Pods),                        dl_alignment_of_type(Ctx, Pods::TYPE_ID));
	EXPECT_EQ(sizeof(MoreBits),                            dl_size_of_type(Ctx, MoreBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(MoreBits),                    dl_alignment_of_type(Ctx, MoreBits::TYPE_ID));
	EXPECT_EQ(sizeof(MorePods),                            dl_size_of_type(Ctx, MorePods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(MorePods),                    dl_alignment_of_type(Ctx, MorePods::TYPE_ID));
	EXPECT_EQ(sizeof(Strings),                             dl_size_of_type(Ctx, Strings::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(Strings),                     dl_alignment_of_type(Ctx, Strings::TYPE_ID));
	EXPECT_EQ(sizeof(TestBits),                            dl_size_of_type(Ctx, TestBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(TestBits),                    dl_alignment_of_type(Ctx, TestBits::TYPE_ID));
	EXPECT_EQ(sizeof(PodArray1),                           dl_size_of_type(Ctx, PodArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(PodArray1),                   dl_alignment_of_type(Ctx, PodArray1::TYPE_ID));
	EXPECT_EQ(sizeof(PodArray2),                           dl_size_of_type(Ctx, PodArray2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(PodArray2),                   dl_alignment_of_type(Ctx, PodArray2::TYPE_ID));
	EXPECT_EQ(sizeof(SimplePtr),                           dl_size_of_type(Ctx, SimplePtr::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SimplePtr),                   dl_alignment_of_type(Ctx, SimplePtr::TYPE_ID));
	EXPECT_EQ(sizeof(StringArray),                         dl_size_of_type(Ctx, StringArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(StringArray),                 dl_alignment_of_type(Ctx, StringArray::TYPE_ID));
	EXPECT_EQ(sizeof(StructArray1),                        dl_size_of_type(Ctx, StructArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(StructArray1),                dl_alignment_of_type(Ctx, StructArray1::TYPE_ID));
	EXPECT_EQ(sizeof(Pod2InStruct),                        dl_size_of_type(Ctx, Pod2InStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(Pod2InStruct),                dl_alignment_of_type(Ctx, Pod2InStruct::TYPE_ID));
	EXPECT_EQ(sizeof(WithInlineArray),                     dl_size_of_type(Ctx, WithInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineArray),             dl_alignment_of_type(Ctx, WithInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(StringInlineArray),                   dl_size_of_type(Ctx, StringInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(StringInlineArray),           dl_alignment_of_type(Ctx, StringInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(Pod2InStructInStruct),                dl_size_of_type(Ctx, Pod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(Pod2InStructInStruct),        dl_alignment_of_type(Ctx, Pod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(sizeof(WithInlineStructArray),               dl_size_of_type(Ctx, WithInlineStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineStructArray),       dl_alignment_of_type(Ctx, WithInlineStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(WithInlineStructStructArray),         dl_size_of_type(Ctx, WithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineStructStructArray), dl_alignment_of_type(Ctx, WithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(DoublePtrChain),                      dl_size_of_type(Ctx, DoublePtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(DoublePtrChain),              dl_alignment_of_type(Ctx, DoublePtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(A128BitAlignedType),                  dl_size_of_type(Ctx, A128BitAlignedType::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(A128BitAlignedType),          dl_alignment_of_type(Ctx, A128BitAlignedType::TYPE_ID));

	EXPECT_EQ(sizeof(BugTest1),                 dl_size_of_type(Ctx, BugTest1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(BugTest1),         dl_alignment_of_type(Ctx, BugTest1::TYPE_ID));
	EXPECT_EQ(sizeof(BugTest1_InArray),         dl_size_of_type(Ctx, BugTest1_InArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(BugTest1_InArray), dl_alignment_of_type(Ctx, BugTest1_InArray::TYPE_ID));
}

TEST_F(DL, TypeLookup)
{
	dl_typeid_t type_id;
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "Pods2", &type_id) );
	EXPECT_TRUE(Pods2::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "StructArray1", &type_id) );
	EXPECT_TRUE(StructArray1::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "StringInlineArray", &type_id) );
	EXPECT_TRUE(StringInlineArray::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "TestBits", &type_id) );
	EXPECT_TRUE(TestBits::TYPE_ID == type_id);

	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bloobloo", &type_id) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bopp", &type_id) );
}
