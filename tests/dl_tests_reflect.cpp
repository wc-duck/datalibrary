/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl_reflect.h>

#include "dl_test_common.h"

TEST_F(DL, ReflectPods)
{
	SDLTypeInfo   Info;
	SDLMemberInfo Members[128];

	EDLError err = DLReflectGetTypeInfo(Ctx, SPods::TYPE_ID, &Info, Members, DL_ARRAY_LENGTH(Members));
	M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

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
	EXPECT_EQ(sizeof(SPods2),                               DLSizeOfType(Ctx, SPods2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPods2),                       DLAlignmentOfType(Ctx, SPods2::TYPE_ID));
	EXPECT_EQ(sizeof(SPtrChain),                            DLSizeOfType(Ctx, SPtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPtrChain),                    DLAlignmentOfType(Ctx, SPtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(SPods),                                DLSizeOfType(Ctx, SPods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPods),                        DLAlignmentOfType(Ctx, SPods::TYPE_ID));
	EXPECT_EQ(sizeof(SMoreBits),                            DLSizeOfType(Ctx, SMoreBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SMoreBits),                    DLAlignmentOfType(Ctx, SMoreBits::TYPE_ID));
	EXPECT_EQ(sizeof(SMorePods),                            DLSizeOfType(Ctx, SMorePods::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SMorePods),                    DLAlignmentOfType(Ctx, SMorePods::TYPE_ID));
	EXPECT_EQ(sizeof(SStrings),                             DLSizeOfType(Ctx, SStrings::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStrings),                     DLAlignmentOfType(Ctx, SStrings::TYPE_ID));
	EXPECT_EQ(sizeof(STestBits),                            DLSizeOfType(Ctx, STestBits::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(STestBits),                    DLAlignmentOfType(Ctx, STestBits::TYPE_ID));
	EXPECT_EQ(sizeof(SPodArray1),                           DLSizeOfType(Ctx, SPodArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPodArray1),                   DLAlignmentOfType(Ctx, SPodArray1::TYPE_ID));
	EXPECT_EQ(sizeof(SPodArray2),                           DLSizeOfType(Ctx, SPodArray2::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPodArray2),                   DLAlignmentOfType(Ctx, SPodArray2::TYPE_ID));
	EXPECT_EQ(sizeof(SSimplePtr),                           DLSizeOfType(Ctx, SSimplePtr::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SSimplePtr),                   DLAlignmentOfType(Ctx, SSimplePtr::TYPE_ID));
	EXPECT_EQ(sizeof(SStringArray),                         DLSizeOfType(Ctx, SStringArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStringArray),                 DLAlignmentOfType(Ctx, SStringArray::TYPE_ID));
	EXPECT_EQ(sizeof(SStructArray1),                        DLSizeOfType(Ctx, SStructArray1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStructArray1),                DLAlignmentOfType(Ctx, SStructArray1::TYPE_ID));
	EXPECT_EQ(sizeof(SPod2InStruct),                        DLSizeOfType(Ctx, SPod2InStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPod2InStruct),                DLAlignmentOfType(Ctx, SPod2InStruct::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineArray),                     DLSizeOfType(Ctx, SWithInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineArray),             DLAlignmentOfType(Ctx, SWithInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(SStringInlineArray),                   DLSizeOfType(Ctx, SStringInlineArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SStringInlineArray),           DLAlignmentOfType(Ctx, SStringInlineArray::TYPE_ID));
	EXPECT_EQ(sizeof(SPod2InStructInStruct),                DLSizeOfType(Ctx, SPod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SPod2InStructInStruct),        DLAlignmentOfType(Ctx, SPod2InStructInStruct::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineStructArray),               DLSizeOfType(Ctx, SWithInlineStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineStructArray),       DLAlignmentOfType(Ctx, SWithInlineStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(SWithInlineStructStructArray),         DLSizeOfType(Ctx, SWithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SWithInlineStructStructArray), DLAlignmentOfType(Ctx, SWithInlineStructStructArray::TYPE_ID));
	EXPECT_EQ(sizeof(SDoublePtrChain),                      DLSizeOfType(Ctx, SDoublePtrChain::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SDoublePtrChain),              DLAlignmentOfType(Ctx, SDoublePtrChain::TYPE_ID));
	EXPECT_EQ(sizeof(SA128BitAlignedType),                  DLSizeOfType(Ctx, SA128BitAlignedType::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SA128BitAlignedType),          DLAlignmentOfType(Ctx, SA128BitAlignedType::TYPE_ID));

	EXPECT_EQ(sizeof(SBugTest1),                 DLSizeOfType(Ctx, SBugTest1::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SBugTest1),         DLAlignmentOfType(Ctx, SBugTest1::TYPE_ID));
	EXPECT_EQ(sizeof(SBugTest1_InArray),         DLSizeOfType(Ctx, SBugTest1_InArray::TYPE_ID));
	EXPECT_EQ(DL_ALIGNMENTOF(SBugTest1_InArray), DLAlignmentOfType(Ctx, SBugTest1_InArray::TYPE_ID));
}
