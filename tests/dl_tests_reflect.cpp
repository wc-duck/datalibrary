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
	SDLTypeInfo   Info;
	SDLMemberInfo Members[128];

	EDLError err = dl_reflect_get_type_info(Ctx, SPods::TYPE_ID, &Info, Members, DL_ARRAY_LENGTH(Members));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_STREQ("Pods", Info.m_Name);
	EXPECT_EQ   (10u,    Info.m_nMembers);

	EXPECT_STREQ("int8",                 Members[0].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[0].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT8,   Members[0].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("int16",                Members[1].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[1].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT16,  Members[1].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("int32",                Members[2].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[2].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT32,  Members[2].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("int64",                Members[3].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[3].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT64,  Members[3].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("uint8",                Members[4].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[4].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT8,  Members[4].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("uint16",               Members[5].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[5].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT16, Members[5].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("uint32",               Members[6].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[6].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT32, Members[6].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("uint64",               Members[7].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[7].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT64, Members[7].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("fp32",                 Members[8].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[8].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP32,   Members[8].m_Type & DL_TYPE_STORAGE_MASK);

	EXPECT_STREQ("fp64",                 Members[9].m_Name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[9].m_Type & DL_TYPE_ATOM_MASK);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP64,   Members[9].m_Type & DL_TYPE_STORAGE_MASK);
}

TEST_F(DL, SizeAndAlignment)
{
	EXPECT_EQ(sizeof(SPods2),                               dl_size_of_type(Ctx, SPods2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPods2),                       dl_alignment_of_type(Ctx, SPods2::TYPE_ID));
	EXPECT_EQ(sizeof(SPtrChain),                            dl_size_of_type(Ctx, SPtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPtrChain),                    dl_alignment_of_type(Ctx, SPtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(SPods),                                dl_size_of_type(Ctx, SPods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPods),                        dl_alignment_of_type(Ctx, SPods::TYPE_ID));
	EXPECT_EQ(sizeof(SMoreBits),                            dl_size_of_type(Ctx, SMoreBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SMoreBits),                    dl_alignment_of_type(Ctx, SMoreBits::TYPE_ID));
	EXPECT_EQ(sizeof(SMorePods),                            dl_size_of_type(Ctx, SMorePods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SMorePods),                    dl_alignment_of_type(Ctx, SMorePods::TYPE_ID));
	EXPECT_EQ(sizeof(SStrings),                             dl_size_of_type(Ctx, SStrings::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStrings),                     dl_alignment_of_type(Ctx, SStrings::TYPE_ID));
	EXPECT_EQ(sizeof(STestBits),                            dl_size_of_type(Ctx, STestBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(STestBits),                    dl_alignment_of_type(Ctx, STestBits::TYPE_ID));
	EXPECT_EQ(sizeof(SPodArray1),                           dl_size_of_type(Ctx, SPodArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPodArray1),                   dl_alignment_of_type(Ctx, SPodArray1::TYPE_ID));
	EXPECT_EQ(sizeof(SPodArray2),                           dl_size_of_type(Ctx, SPodArray2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPodArray2),                   dl_alignment_of_type(Ctx, SPodArray2::TYPE_ID));
	EXPECT_EQ(sizeof(SSimplePtr),                           dl_size_of_type(Ctx, SSimplePtr::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SSimplePtr),                   dl_alignment_of_type(Ctx, SSimplePtr::TYPE_ID));
	EXPECT_EQ(sizeof(SStringArray),                         dl_size_of_type(Ctx, SStringArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStringArray),                 dl_alignment_of_type(Ctx, SStringArray::TYPE_ID));
	EXPECT_EQ(sizeof(SStructArray1),                        dl_size_of_type(Ctx, SStructArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStructArray1),                dl_alignment_of_type(Ctx, SStructArray1::TYPE_ID));
	EXPECT_EQ(sizeof(SPod2InStruct),                        dl_size_of_type(Ctx, SPod2InStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPod2InStruct),                dl_alignment_of_type(Ctx, SPod2InStruct::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineArray),                     dl_size_of_type(Ctx, SWithInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineArray),             dl_alignment_of_type(Ctx, SWithInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(SStringInlineArray),                   dl_size_of_type(Ctx, SStringInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStringInlineArray),           dl_alignment_of_type(Ctx, SStringInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(SPod2InStructInStruct),                dl_size_of_type(Ctx, SPod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPod2InStructInStruct),        dl_alignment_of_type(Ctx, SPod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineStructArray),               dl_size_of_type(Ctx, SWithInlineStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineStructArray),       dl_alignment_of_type(Ctx, SWithInlineStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineStructStructArray),         dl_size_of_type(Ctx, SWithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineStructStructArray), dl_alignment_of_type(Ctx, SWithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(SDoublePtrChain),                      dl_size_of_type(Ctx, SDoublePtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SDoublePtrChain),              dl_alignment_of_type(Ctx, SDoublePtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(SA128BitAlignedType),                  dl_size_of_type(Ctx, SA128BitAlignedType::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SA128BitAlignedType),          dl_alignment_of_type(Ctx, SA128BitAlignedType::TYPE_ID));

	EXPECT_EQ(sizeof(SBugTest1),                 dl_size_of_type(Ctx, SBugTest1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SBugTest1),         dl_alignment_of_type(Ctx, SBugTest1::TYPE_ID));
	EXPECT_EQ(sizeof(SBugTest1_InArray),         dl_size_of_type(Ctx, SBugTest1_InArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SBugTest1_InArray), dl_alignment_of_type(Ctx, SBugTest1_InArray::TYPE_ID));
}

TEST_F(DL, TypeLookup)
{
	StrHash type_id;
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "Pods2", &type_id) );
	EXPECT_TRUE(SPods2::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "StructArray1", &type_id) );
	EXPECT_TRUE(SStructArray1::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "StringInlineArray", &type_id) );
	EXPECT_TRUE(SStringInlineArray::TYPE_ID == type_id);
	EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_reflect_get_type_id(Ctx, "TestBits", &type_id) );
	EXPECT_TRUE(STestBits::TYPE_ID == type_id);

	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bloobloo", &type_id) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bopp", &type_id) );
}
