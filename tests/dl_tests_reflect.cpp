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
	dl_type_info_t   Info = { 0 };
	dl_member_info_t Members[128];

	EXPECT_DL_ERR_OK(dl_reflect_get_type_info( Ctx, Pods::TYPE_ID, &Info ));
	EXPECT_DL_ERR_OK(dl_reflect_get_type_members( Ctx, Pods::TYPE_ID, Members, DL_ARRAY_LENGTH(Members) ));

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
	dl_type_info_t   ti;

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, Pods2::TYPE_ID, &ti));
	EXPECT_EQ(sizeof(Pods2),                               ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(Pods2),                       ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, PtrChain::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(PtrChain),                            ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(PtrChain),                    ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, Pods::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(Pods),                                ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(Pods),                        ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, MoreBits::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(MoreBits),                            ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(MoreBits),                    ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, MorePods::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(MorePods),                            ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(MorePods),                    ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, Strings::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(Strings),                             ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(Strings),                     ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, TestBits::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(TestBits),                            ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(TestBits),                    ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, PodArray1::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(PodArray1),                           ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(PodArray1),                   ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, PodArray2::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(PodArray2),                           ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(PodArray2),                   ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, SimplePtr::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(SimplePtr),                           ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(SimplePtr),                   ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, StringArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(StringArray),                         ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(StringArray),                 ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, StructArray1::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(StructArray1),                        ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(StructArray1),                ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, Pod2InStruct::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(Pod2InStruct),                        ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(Pod2InStruct),                ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, WithInlineArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(WithInlineArray),                     ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineArray),             ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, StringInlineArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(StringInlineArray),                   ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(StringInlineArray),           ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, Pod2InStructInStruct::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(Pod2InStructInStruct),                ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(Pod2InStructInStruct),        ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, WithInlineStructArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(WithInlineStructArray),               ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineStructArray),       ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, WithInlineStructStructArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(WithInlineStructStructArray),         ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(WithInlineStructStructArray), ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, DoublePtrChain::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(DoublePtrChain),                      ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(DoublePtrChain),              ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, A128BitAlignedType::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(A128BitAlignedType),                  ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(A128BitAlignedType),          ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, BugTest1::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(BugTest1),                            ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(BugTest1),                    ti.alignment);

	EXPECT_DL_ERR_OK(                                      dl_reflect_get_type_info(Ctx, BugTest1_InArray::TYPE_ID, &ti ));
	EXPECT_EQ(sizeof(BugTest1_InArray),                    ti.size);
	EXPECT_EQ(DL_ALIGNMENTOF(BugTest1_InArray),            ti.alignment);
}

TEST_F(DL, TypeLookup)
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
