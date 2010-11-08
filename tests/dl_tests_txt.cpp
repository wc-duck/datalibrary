/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>

#include "dl_test_common.h"

TEST_F(DL, TextMemberOrder)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData =
	"{"
		"\"root\" : {"
			"\"type\" : \"Pods\","
			"\"data\" : {"
				"\"int16\"  : 2,"
				"\"uint32\" : 7,"
				"\"uint8\"  : 5,"
				"\"int32\"  : 3,"
				"\"fp64\"   : 4.1234,"
				"\"int64\"  : 4,"
				"\"uint16\" : 6,"
				"\"uint64\" : 8,"
				"\"int8\"   : 1,"
				"\"fp32\"   : 3.14"
			"}"
		"}"
	"}";

	SPods P1;

 	uint8 OutDataText[1024];

	// pack txt to binary
	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(1,      P1.m_int8);
 	EXPECT_EQ(2,      P1.m_int16);
	EXPECT_EQ(3,      P1.m_int32);
	EXPECT_EQ(4,      P1.m_int64);
	EXPECT_EQ(5u,     P1.m_uint8);
	EXPECT_EQ(6u,     P1.m_uint16);
	EXPECT_EQ(7u,     P1.m_uint32);
	EXPECT_EQ(8u,     P1.m_uint64);
	EXPECT_EQ(3.14f,  P1.m_fp32);
	EXPECT_EQ(4.1234, P1.m_fp64);
}

TEST_F(DL, TextSetMemberTwice)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData = 
		"{"
			"\"root\" : {"
				"\"type\" : \"Pods\","
				"\"data\" : {"
					"\"int16\"  : 2,"
					"\"uint32\" : 7,"
					"\"uint8\"  : 5,"
					"\"int32\"  : 3,"
					"\"int32\"  : 7,"
					"\"fp64\"   : 4.1234,"
					"\"int64\"  : 4,"
					"\"uint16\" : 6,"
					"\"uint64\" : 8,"
					"\"int8\"   : 1,"
					"\"fp32\"   : 3.14"
				"}"
			"}"
		"}";

	uint8 OutDataText[1024];

	// pack txt to binary
	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MEMBER_SET_TWICE, err);
}

TEST_F(DL, TextNonExistMember)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData = 
		"{"
			"\"root\" : {"
				"\"type\" : \"Pods2\","
				"\"data\" : { \"Int1\" : 1337, \"Int2\" : 1337, \"IDoNotExist\" : 1337 }"
			"}"
		"}";

	uint8 OutDataText[1024];

	// pack txt to binary
	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_MEMBER_NOT_FOUND, err);
}

TEST_F(DL, TextErrorMissingMember)
{
	// error should be cast if member is not set and has no default value!
	// in this case Int2 is not set!

	const char* TextData = "{ \"root\" : { \"type\" : \"Pods2\", \"data\" : { \"Int1\" : 1337 } } }";

	uint8 OutDataText[1024];
	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MEMBER_MISSING, err);
}

TEST_F(DL, TextPodDefaults)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"PodsDefaults\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SPodsDefaults P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(2,     P1.m_int8);
	EXPECT_EQ(3,     P1.m_int16);
	EXPECT_EQ(4,     P1.m_int32);
	EXPECT_EQ(5,     P1.m_int64);
	EXPECT_EQ(6u,    P1.m_uint8);
	EXPECT_EQ(7u,    P1.m_uint16);
	EXPECT_EQ(8u,    P1.m_uint32);
	EXPECT_EQ(9u,    P1.m_uint64);
	EXPECT_EQ(10.0f, P1.m_fp32);
	EXPECT_EQ(11.0,  P1.m_fp64);
}

TEST_F(DL, TextDefaultStr)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultStr P1[10]; // this is so ugly!
	
	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_STREQ("cowbells ftw!", P1[0].m_pStr);
}

TEST_F(DL, TextDefaultPtr)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultPtr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultPtr P1; // this is so ugly!

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(0x0, P1.m_pPtr);
}

TEST_F(DL, TextDefaultStruct)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultStruct\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultStruct P1; // this is so ugly!

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(13u, P1.m_Struct.m_Int1);
	EXPECT_EQ(37u, P1.m_Struct.m_Int2);
}

TEST_F(DL, TextDefaultEnum)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultEnum P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(TESTENUM1_VALUE3, P1.m_Enum);
}

TEST_F(DL, TextDefaultInlineArrayPod)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayPod\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultInlArrayPod P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(1u, P1.m_lArr[0]);
	EXPECT_EQ(3u, P1.m_lArr[1]);
	EXPECT_EQ(3u, P1.m_lArr[2]);
	EXPECT_EQ(7u, P1.m_lArr[3]);
}

TEST_F(DL, TextDefaultInlineArrayEnum)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultInlArrayEnum P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(TESTENUM1_VALUE3, P1.m_lArr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, P1.m_lArr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, P1.m_lArr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, P1.m_lArr[3]);
}

TEST_F(DL, TextDefaultInlineArrayString)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultInlArrayStr P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_STREQ("cow",   P1[0].m_lpArr[0]);
	EXPECT_STREQ("bells", P1[0].m_lpArr[1]);
	EXPECT_STREQ("are",   P1[0].m_lpArr[2]);
	EXPECT_STREQ("cool",  P1[0].m_lpArr[3]);
}

TEST_F(DL, TextDefaultArrayPod)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayPod\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultArrayPod P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].m_lArr.m_nCount);
	
	EXPECT_EQ(1u, P1[0].m_lArr[0]);
	EXPECT_EQ(3u, P1[0].m_lArr[1]);
	EXPECT_EQ(3u, P1[0].m_lArr[2]);
	EXPECT_EQ(7u, P1[0].m_lArr[3]);
}

TEST_F(DL, TextDefaultArrayEnum)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultArrayEnum P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].m_lArr.m_nCount);

	EXPECT_EQ(TESTENUM1_VALUE3, P1[0].m_lArr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, P1[0].m_lArr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, P1[0].m_lArr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, P1[0].m_lArr[3]);
}

TEST_F(DL, TextDefaultArrayString)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	EDLError err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	SDefaultArrayStr P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].m_lpArr.m_nCount);

	EXPECT_STREQ("cow",   P1[0].m_lpArr[0]);
	EXPECT_STREQ("bells", P1[0].m_lpArr[1]);
	EXPECT_STREQ("are",   P1[0].m_lpArr[2]);
	EXPECT_STREQ("cool",  P1[0].m_lpArr[3]);
}

