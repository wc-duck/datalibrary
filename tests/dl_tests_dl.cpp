/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>

#include "dl_test_common.h"

#include <float.h>

// TODO: Check these versus standard defines!
#define DL_INT8_MAX  (0x7F)
#define DL_INT16_MAX (0x7FFF)
static const int32 DL_INT32_MAX = 0x7FFFFFFFL;
static const int64 DL_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const int8  DL_INT8_MIN  = (-DL_INT8_MAX  - 1);
static const int16 DL_INT16_MIN = (-DL_INT16_MAX - 1);
static const int32 DL_INT32_MIN = (-DL_INT32_MAX - 1);
static const int64 DL_INT64_MIN = (-DL_INT64_MAX - 1);

#define DL_UINT8_MAX  (0xFFU)
#define DL_UINT16_MAX (0xFFFFU)
static const uint32 DL_UINT32_MAX = 0xFFFFFFFFUL;
static const uint64 DL_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
static const uint8  DL_UINT8_MIN  = 0x00U;
static const uint16 DL_UINT16_MIN = 0x0000U;
static const uint32 DL_UINT32_MIN = 0x00000000UL;
static const uint64 DL_UINT64_MIN = 0x0000000000000000ULL;


void DoTheRoundAbout(HDLContext _Ctx, StrHash _TypeHash, void* _pPackMe, void* _pUnpackMe)
{
	unsigned char OutDataInstance[1024];
	char  TxtOut[2048];

	memset(OutDataInstance, 0x0, sizeof(OutDataInstance));
	memset(TxtOut, 0x0,  sizeof(TxtOut));

	// store instance to binary
	EDLError err = dl_store_instace(_Ctx, _TypeHash, _pPackMe, OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance));
	M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
	EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(OutDataInstance,  DL_ARRAY_LENGTH(OutDataInstance)));
	EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(OutDataInstance,   DL_ARRAY_LENGTH(OutDataInstance)));
	EXPECT_EQ(_TypeHash,      dl_instance_root_type(OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance)));

	// unpack binary to txt
	err = dl_unpack(_Ctx, OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance), TxtOut, DL_ARRAY_LENGTH(TxtOut));
	M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	// M_LOG_INFO("%s", TxtOut);

	unsigned char OutDataText[1024];
	memset(OutDataText, 0x0, DL_ARRAY_LENGTH(OutDataText));

	// pack txt to binary
	err = dl_pack_text(_Ctx, TxtOut, OutDataText, DL_ARRAY_LENGTH(OutDataText));
	M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
	EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(OutDataText,  DL_ARRAY_LENGTH(OutDataText)));
	EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(OutDataText,   DL_ARRAY_LENGTH(OutDataText)));
	EXPECT_EQ(_TypeHash,      dl_instance_root_type(OutDataText, DL_ARRAY_LENGTH(OutDataText)));

	// load binary
	err = dl_load_instance_inplace(_Ctx, _pUnpackMe, OutDataText, DL_ARRAY_LENGTH(OutDataText));
	M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EDLCpuEndian   OtherEndian  = DL_ENDIAN_HOST == DL_ENDIAN_BIG ? DL_ENDIAN_LITTLE : DL_ENDIAN_BIG;
	unsigned int OtherPtrSize = sizeof(void*) == 4 ? 8 : 4;

	// check if we can convert only endianness!
	{
		uint8 SwitchedEndian[DL_ARRAY_LENGTH(OutDataText)];
		memcpy(SwitchedEndian, OutDataText, DL_ARRAY_LENGTH(OutDataText));

		err = dl_convert_instance_inplace(_Ctx, SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian), OtherEndian, sizeof(void*));
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
		EXPECT_EQ(sizeof(void*), dl_instance_ptr_size(SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(OtherEndian,   dl_instance_endian(SwitchedEndian,  DL_ARRAY_LENGTH(SwitchedEndian)));

		EXPECT_NE(0, memcmp(OutDataText, SwitchedEndian, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

		err = dl_convert_instance_inplace(_Ctx, SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian), DL_ENDIAN_HOST, sizeof(void*));
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
		EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(SwitchedEndian,  DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(SwitchedEndian,    DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(_TypeHash,      dl_instance_root_type(SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian)));

		EXPECT_EQ(0, memcmp(OutDataText, SwitchedEndian, DL_ARRAY_LENGTH(OutDataText))); // original data should be equal !
	}

	// check problems when only ptr-size!
	{
		unsigned char OriginalData[DL_ARRAY_LENGTH(OutDataText)];
		memcpy(OriginalData, OutDataText, DL_ARRAY_LENGTH(OutDataText));

		unsigned int PackedSize = 0;
		err = dl_instace_size_stored(_Ctx, _TypeHash, _pPackMe, &PackedSize);
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

		unsigned int ConvertedSize = 0;
		err = dl_instance_size_converted(_Ctx, OriginalData, DL_ARRAY_LENGTH(OriginalData), OtherPtrSize, &ConvertedSize);
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

		if(OtherPtrSize <= sizeof(void*)) EXPECT_LE(ConvertedSize, PackedSize);
		else                              EXPECT_GE(ConvertedSize, PackedSize);

		// check inplace conversion.
		if(OtherPtrSize < sizeof(void*))
		{
			// if this is true, the conversion is doable with inplace-version.
			uint8 ConvertedData[DL_ARRAY_LENGTH(OutDataText)];
			memcpy(ConvertedData, OriginalData, DL_ARRAY_LENGTH(OriginalData));

			err = dl_convert_instance_inplace(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), DL_ENDIAN_HOST, OtherPtrSize);
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(OtherPtrSize,   dl_instance_ptr_size(ConvertedData,  DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ConvertedData,    DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ConvertedData, DL_ARRAY_LENGTH(ConvertedData)));

			EXPECT_NE(0, memcmp(OutDataText, ConvertedData, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

			unsigned char OriginalConverted[DL_ARRAY_LENGTH(OutDataText)];
			memcpy(OriginalConverted, ConvertedData, DL_ARRAY_LENGTH(OriginalData));

			unsigned int ReConvertedSize = 0;
			err = dl_instance_size_converted(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), sizeof(void*), &ReConvertedSize);
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(PackedSize, ReConvertedSize); // should be same size as original!

			unsigned char ReConvertedData[DL_ARRAY_LENGTH(OutDataText) * 3];
			ReConvertedData[ReConvertedSize + 1] = 'L';
			ReConvertedData[ReConvertedSize + 2] = 'O';
			ReConvertedData[ReConvertedSize + 3] = 'L';
			ReConvertedData[ReConvertedSize + 4] = '!';

			err = dl_convert_instance(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData), DL_ENDIAN_HOST, sizeof(void*));
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(ReConvertedData,  DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ReConvertedData,    DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData)));

			EXPECT_EQ(0, memcmp(OriginalConverted, ConvertedData, DL_ARRAY_LENGTH(OriginalConverted))); // original data should be equal !

			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 1]);
			EXPECT_EQ('O', ReConvertedData[ReConvertedSize + 2]);
			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 3]);
			EXPECT_EQ('!', ReConvertedData[ReConvertedSize + 4]);
		}

		// check both endian and ptrsize convert!
		{
			unsigned char ConvertedData[DL_ARRAY_LENGTH(OutDataText) * 3];
			ConvertedData[ConvertedSize + 1] = 'L';
			ConvertedData[ConvertedSize + 2] = 'O';
			ConvertedData[ConvertedSize + 3] = 'L';
			ConvertedData[ConvertedSize + 4] = '!';

			err = dl_convert_instance(_Ctx, OriginalData, DL_ARRAY_LENGTH(OriginalData), ConvertedData, DL_ARRAY_LENGTH(ConvertedData), OtherEndian, OtherPtrSize);
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(OtherPtrSize, dl_instance_ptr_size(ConvertedData, DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(OtherEndian,  dl_instance_endian(ConvertedData,  DL_ARRAY_LENGTH(ConvertedData)));

			EXPECT_EQ('L', ConvertedData[ConvertedSize + 1]);
			EXPECT_EQ('O', ConvertedData[ConvertedSize + 2]);
			EXPECT_EQ('L', ConvertedData[ConvertedSize + 3]);
			EXPECT_EQ('!', ConvertedData[ConvertedSize + 4]);

			EXPECT_NE(0, memcmp(OutDataText, ConvertedData, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

			unsigned int ReConvertedSize = 0;
			err = dl_instance_size_converted(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), sizeof(void*), &ReConvertedSize);
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(PackedSize, ReConvertedSize); // should be same size as original!

			uint8 ReConvertedData[DL_ARRAY_LENGTH(OutDataText) * 2];
			ReConvertedData[ReConvertedSize + 1] = 'L';
			ReConvertedData[ReConvertedSize + 2] = 'O';
			ReConvertedData[ReConvertedSize + 3] = 'L';
			ReConvertedData[ReConvertedSize + 4] = '!';

			err = dl_convert_instance(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData), DL_ENDIAN_HOST, sizeof(void*));
			M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(ReConvertedData,  DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ReConvertedData,    DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData)));

			EXPECT_EQ(0, memcmp(OutDataText, ReConvertedData, PackedSize)); // original data should be equal !

			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 1]);
			EXPECT_EQ('O', ReConvertedData[ReConvertedSize + 2]);
			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 3]);
			EXPECT_EQ('!', ReConvertedData[ReConvertedSize + 4]);
		}
	}
}

TEST_F(DL, TestPack)
{
	SPods P1Original = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SPods P1         = { 0 };

	DoTheRoundAbout(Ctx, SPods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.m_int8,   P1.m_int8);
	EXPECT_EQ(P1Original.m_int16,  P1.m_int16);
	EXPECT_EQ(P1Original.m_int32,  P1.m_int32);
	EXPECT_EQ(P1Original.m_int64,  P1.m_int64);
	EXPECT_EQ(P1Original.m_uint8,  P1.m_uint8);
	EXPECT_EQ(P1Original.m_uint16, P1.m_uint16);
	EXPECT_EQ(P1Original.m_uint32, P1.m_uint32);
	EXPECT_EQ(P1Original.m_uint64, P1.m_uint64);
	EXPECT_EQ(P1Original.m_fp32,   P1.m_fp32);
	EXPECT_EQ(P1Original.m_fp64,   P1.m_fp64);
}

TEST_F(DL, MaxPod)
{
	SPods P1Original = { DL_INT8_MAX, DL_INT16_MAX, DL_INT32_MAX, DL_INT64_MAX, DL_UINT8_MAX, DL_UINT16_MAX, DL_UINT32_MAX, DL_UINT64_MAX, FLT_MAX, DBL_MAX };
	SPods P1         = { 0 };

	DoTheRoundAbout(Ctx, SPods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.m_int8,   P1.m_int8);
	EXPECT_EQ(P1Original.m_int16,  P1.m_int16);
	EXPECT_EQ(P1Original.m_int32,  P1.m_int32);
	EXPECT_EQ(P1Original.m_int64,  P1.m_int64);
	EXPECT_EQ(P1Original.m_uint8,  P1.m_uint8);
	EXPECT_EQ(P1Original.m_uint16, P1.m_uint16);
	EXPECT_EQ(P1Original.m_uint32, P1.m_uint32);
	EXPECT_EQ(P1Original.m_uint64, P1.m_uint64);
	// need to disable these tests for floating-point-errors =/
	// EXPECT_FLOAT_EQ(P1Original.m_fp32,   P1.m_fp32);
	// EXPECT_DOUBLE_EQ(P1Original.m_fp64,   P1.m_fp64);
}

TEST_F(DL, MinPod)
{
	SPods P1Original = { DL_INT8_MIN, DL_INT16_MIN, DL_INT32_MIN, DL_INT64_MIN, DL_UINT8_MIN, DL_UINT16_MIN, DL_UINT32_MIN, DL_UINT64_MIN, FLT_MIN, DBL_MIN };
	SPods P1         = { 0 };

	DoTheRoundAbout(Ctx, SPods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.m_int8,   P1.m_int8);
	EXPECT_EQ(P1Original.m_int16,  P1.m_int16);
	EXPECT_EQ(P1Original.m_int32,  P1.m_int32);
	EXPECT_EQ(P1Original.m_int64,  P1.m_int64);
	EXPECT_EQ(P1Original.m_uint8,  P1.m_uint8);
	EXPECT_EQ(P1Original.m_uint16, P1.m_uint16);
	EXPECT_EQ(P1Original.m_uint32, P1.m_uint32);
	EXPECT_EQ(P1Original.m_uint64, P1.m_uint64);
	EXPECT_NEAR(P1Original.m_fp32, P1.m_fp32, 0.0000001f);
	EXPECT_NEAR(P1Original.m_fp64, P1.m_fp64, 0.0000001f);
}

TEST_F(DL, StructInStruct)
{
	SMorePods P1Original = { { 1, 2, 3, 4, 5, 6, 7, 8, 0.0f, 0}, { 9, 10, 11, 12, 13, 14, 15, 16, 0.0f, 0} };
	SMorePods P1         = { { 0 }, { 0 } };

	DoTheRoundAbout(Ctx, SMorePods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.m_Pods1.m_int8,   P1.m_Pods1.m_int8);
	EXPECT_EQ(P1Original.m_Pods1.m_int16,  P1.m_Pods1.m_int16);
	EXPECT_EQ(P1Original.m_Pods1.m_int32,  P1.m_Pods1.m_int32);
	EXPECT_EQ(P1Original.m_Pods1.m_int64,  P1.m_Pods1.m_int64);
	EXPECT_EQ(P1Original.m_Pods1.m_uint8,  P1.m_Pods1.m_uint8);
	EXPECT_EQ(P1Original.m_Pods1.m_uint16, P1.m_Pods1.m_uint16);
	EXPECT_EQ(P1Original.m_Pods1.m_uint32, P1.m_Pods1.m_uint32);
	EXPECT_EQ(P1Original.m_Pods1.m_uint64, P1.m_Pods1.m_uint64);
	EXPECT_NEAR(P1Original.m_Pods1.m_fp32, P1.m_Pods1.m_fp32, 0.0000001f);
	EXPECT_NEAR(P1Original.m_Pods1.m_fp64, P1.m_Pods1.m_fp64, 0.0000001f);

	EXPECT_EQ(P1Original.m_Pods2.m_int8,   P1.m_Pods2.m_int8);
	EXPECT_EQ(P1Original.m_Pods2.m_int16,  P1.m_Pods2.m_int16);
	EXPECT_EQ(P1Original.m_Pods2.m_int32,  P1.m_Pods2.m_int32);
	EXPECT_EQ(P1Original.m_Pods2.m_int64,  P1.m_Pods2.m_int64);
	EXPECT_EQ(P1Original.m_Pods2.m_uint8,  P1.m_Pods2.m_uint8);
	EXPECT_EQ(P1Original.m_Pods2.m_uint16, P1.m_Pods2.m_uint16);
	EXPECT_EQ(P1Original.m_Pods2.m_uint32, P1.m_Pods2.m_uint32);
	EXPECT_EQ(P1Original.m_Pods2.m_uint64, P1.m_Pods2.m_uint64);
	EXPECT_NEAR(P1Original.m_Pods2.m_fp32, P1.m_Pods2.m_fp32, 0.0000001f);
	EXPECT_NEAR(P1Original.m_Pods2.m_fp64, P1.m_Pods2.m_fp64, 0.0000001f);
}

TEST_F(DL, StructInStructInStruct)
{
	SPod2InStructInStruct Orig;
	SPod2InStructInStruct New;

	Orig.m_Pod2InStruct.m_Pod1.m_Int1 = 1337;
	Orig.m_Pod2InStruct.m_Pod1.m_Int2 = 7331;
	Orig.m_Pod2InStruct.m_Pod2.m_Int1 = 1234;
	Orig.m_Pod2InStruct.m_Pod2.m_Int2 = 4321;

	DoTheRoundAbout(Ctx, SPod2InStructInStruct::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_Pod2InStruct.m_Pod1.m_Int1, New.m_Pod2InStruct.m_Pod1.m_Int1);
	EXPECT_EQ(Orig.m_Pod2InStruct.m_Pod1.m_Int2, New.m_Pod2InStruct.m_Pod1.m_Int2);
	EXPECT_EQ(Orig.m_Pod2InStruct.m_Pod2.m_Int1, New.m_Pod2InStruct.m_Pod2.m_Int1);
	EXPECT_EQ(Orig.m_Pod2InStruct.m_Pod2.m_Int2, New.m_Pod2InStruct.m_Pod2.m_Int2);
}

TEST_F(DL, InlineArray)
{
	SWithInlineArray Orig;
	SWithInlineArray New;

	Orig.m_lArray[0] = 1337;
	Orig.m_lArray[1] = 7331;
	Orig.m_lArray[2] = 1234;

	DoTheRoundAbout(Ctx, SWithInlineArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_lArray[0], New.m_lArray[0]);
	EXPECT_EQ(Orig.m_lArray[1], New.m_lArray[1]);
	EXPECT_EQ(Orig.m_lArray[2], New.m_lArray[2]);
}

TEST_F(DL, WithInlineStructArray)
{
	SWithInlineStructArray Orig;
	SWithInlineStructArray New;

	Orig.m_lArray[0].m_Int1 = 1337;
	Orig.m_lArray[0].m_Int2 = 7331;
	Orig.m_lArray[1].m_Int1 = 1224;
	Orig.m_lArray[1].m_Int2 = 5678;
	Orig.m_lArray[2].m_Int1 = 9012;
	Orig.m_lArray[2].m_Int2 = 3456;

	DoTheRoundAbout(Ctx, SWithInlineStructArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_lArray[0].m_Int1, New.m_lArray[0].m_Int1);
	EXPECT_EQ(Orig.m_lArray[0].m_Int2, New.m_lArray[0].m_Int2);
	EXPECT_EQ(Orig.m_lArray[1].m_Int1, New.m_lArray[1].m_Int1);
	EXPECT_EQ(Orig.m_lArray[1].m_Int2, New.m_lArray[1].m_Int2);
	EXPECT_EQ(Orig.m_lArray[2].m_Int1, New.m_lArray[2].m_Int1);
	EXPECT_EQ(Orig.m_lArray[2].m_Int2, New.m_lArray[2].m_Int2);
}

TEST_F(DL, WithInlineStructStructArray)
{
	SWithInlineStructStructArray Orig;
	SWithInlineStructStructArray New;

	Orig.m_lArray[0].m_lArray[0].m_Int1 = 1;
	Orig.m_lArray[0].m_lArray[0].m_Int2 = 3;
	Orig.m_lArray[0].m_lArray[1].m_Int1 = 3;
	Orig.m_lArray[0].m_lArray[1].m_Int2 = 7;
	Orig.m_lArray[0].m_lArray[2].m_Int1 = 13;
	Orig.m_lArray[0].m_lArray[2].m_Int2 = 37;
	Orig.m_lArray[1].m_lArray[0].m_Int1 = 1337;
	Orig.m_lArray[1].m_lArray[0].m_Int2 = 7331;
	Orig.m_lArray[1].m_lArray[1].m_Int1 = 1;
	Orig.m_lArray[1].m_lArray[1].m_Int2 = 337;
	Orig.m_lArray[1].m_lArray[2].m_Int1 = 133;
	Orig.m_lArray[1].m_lArray[2].m_Int2 = 7;

	DoTheRoundAbout(Ctx, SWithInlineStructStructArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_lArray[0].m_lArray[0].m_Int1, New.m_lArray[0].m_lArray[0].m_Int1);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[0].m_Int2, New.m_lArray[0].m_lArray[0].m_Int2);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[1].m_Int1, New.m_lArray[0].m_lArray[1].m_Int1);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[1].m_Int2, New.m_lArray[0].m_lArray[1].m_Int2);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[2].m_Int1, New.m_lArray[0].m_lArray[2].m_Int1);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[2].m_Int2, New.m_lArray[0].m_lArray[2].m_Int2);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[0].m_Int1, New.m_lArray[1].m_lArray[0].m_Int1);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[0].m_Int2, New.m_lArray[1].m_lArray[0].m_Int2);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[1].m_Int1, New.m_lArray[1].m_lArray[1].m_Int1);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[1].m_Int2, New.m_lArray[1].m_lArray[1].m_Int2);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[2].m_Int1, New.m_lArray[1].m_lArray[2].m_Int1);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[2].m_Int2, New.m_lArray[1].m_lArray[2].m_Int2);
}

TEST_F(DL, PodArray1)
{
	uint32 Data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	SPodArray1 Orig = { { Data, 8 } };
	SPodArray1* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, SPodArray1::TYPE_ID, &Orig, Loaded);

	New = (SPodArray1*)&Loaded[0];

	EXPECT_EQ(Orig.m_lArray.m_nCount, New->m_lArray.m_nCount);
	EXPECT_EQ(Orig.m_lArray[0], New->m_lArray[0]);
	EXPECT_EQ(Orig.m_lArray[1], New->m_lArray[1]);
	EXPECT_EQ(Orig.m_lArray[2], New->m_lArray[2]);
	EXPECT_EQ(Orig.m_lArray[3], New->m_lArray[3]);
	EXPECT_EQ(Orig.m_lArray[4], New->m_lArray[4]);
	EXPECT_EQ(Orig.m_lArray[5], New->m_lArray[5]);
	EXPECT_EQ(Orig.m_lArray[6], New->m_lArray[6]);
	EXPECT_EQ(Orig.m_lArray[7], New->m_lArray[7]);
}

TEST_F(DL, PodArray2)
{
	uint32 Data1[] = { 1337, 7331,  13, 37, 133 } ;
	uint32 Data2[] = {    7,    1, 337 } ;

	SPodArray1 OrigArray[] = { { { Data1, 5 } }, { { Data2, 3 } } } ;

	SPodArray2 Orig = { { OrigArray, 2 } };
	SPodArray2* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, SPodArray2::TYPE_ID, &Orig, Loaded);

	New = (SPodArray2*)&Loaded[0];

	EXPECT_EQ(Orig.m_lArray.m_nCount, New->m_lArray.m_nCount);

	EXPECT_EQ(Orig.m_lArray[0].m_lArray.m_nCount, New->m_lArray[0].m_lArray.m_nCount);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray.m_nCount, New->m_lArray[1].m_lArray.m_nCount);

	EXPECT_EQ(Orig.m_lArray[0].m_lArray[0], New->m_lArray[0].m_lArray[0]);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[1], New->m_lArray[0].m_lArray[1]);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[2], New->m_lArray[0].m_lArray[2]);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[3], New->m_lArray[0].m_lArray[3]);
	EXPECT_EQ(Orig.m_lArray[0].m_lArray[4], New->m_lArray[0].m_lArray[4]);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[0], New->m_lArray[1].m_lArray[0]);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[1], New->m_lArray[1].m_lArray[1]);
	EXPECT_EQ(Orig.m_lArray[1].m_lArray[2], New->m_lArray[1].m_lArray[2]);
}

TEST_F(DL, SimpleString)
{
	SStrings Orig = { "cow", "bell" } ;
	SStrings* New;

	SStrings Loaded[5]; // this is so ugly!

	DoTheRoundAbout(Ctx, SStrings::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.m_pStr1, New->m_pStr1);
	EXPECT_STREQ(Orig.m_pStr2, New->m_pStr2);
}

TEST_F(DL, InlineArrayString)
{
	SStringInlineArray Orig = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	SStringInlineArray* New;

	SStringInlineArray Loaded[5]; // this is so ugly!

	DoTheRoundAbout(Ctx, SStringInlineArray::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.m_lpStrings[0], New->m_lpStrings[0]);
	EXPECT_STREQ(Orig.m_lpStrings[1], New->m_lpStrings[1]);
	EXPECT_STREQ(Orig.m_lpStrings[2], New->m_lpStrings[2]);
}

TEST_F(DL, ArrayString)
{
	char* TheStringArray[] = { (char*)"I like", (char*)"the", (char*)"1337 ", (char*)"cowbells of doom!" };
	SStringArray Orig = { { TheStringArray, 4 } };
	SStringArray* New;

	SStringArray Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, SStringArray::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.m_lpStrings[0], New->m_lpStrings[0]);
	EXPECT_STREQ(Orig.m_lpStrings[1], New->m_lpStrings[1]);
	EXPECT_STREQ(Orig.m_lpStrings[2], New->m_lpStrings[2]);
	EXPECT_STREQ(Orig.m_lpStrings[3], New->m_lpStrings[3]);
}

TEST_F(DL, BitField)
{
	STestBits Orig;
	STestBits New;

	Orig.m_Bit1 = 0;
	Orig.m_Bit2 = 2;
	Orig.m_Bit3 = 4;
	Orig.m_make_it_uneven = 17;
	Orig.m_Bit4 = 1;
	Orig.m_Bit5 = 0;
	Orig.m_Bit6 = 5;

	DoTheRoundAbout(Ctx, STestBits::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_Bit1, New.m_Bit1);
	EXPECT_EQ(Orig.m_Bit2, New.m_Bit2);
	EXPECT_EQ(Orig.m_Bit3, New.m_Bit3);
	EXPECT_EQ(Orig.m_make_it_uneven, New.m_make_it_uneven);
	EXPECT_EQ(Orig.m_Bit4, New.m_Bit4);
	EXPECT_EQ(Orig.m_Bit5, New.m_Bit5);
	EXPECT_EQ(Orig.m_Bit6, New.m_Bit6);
}

TEST_F(DL, MoreBits)
{
	SMoreBits Orig;
	SMoreBits New;

	Orig.m_Bit1 = 512;
	Orig.m_Bit2 = 1;

	DoTheRoundAbout(Ctx, SMoreBits::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_Bit1, New.m_Bit1);
	EXPECT_EQ(Orig.m_Bit2, New.m_Bit2);
}

TEST_F(DL, 64BitBitfield)
{
	S64BitBitfield Orig;
	S64BitBitfield New;

	Orig.m_Package  = 2;
	Orig.m_FileType = 13;
	Orig.m_PathHash = 1337;
	Orig.m_FileHash = 0xDEADBEEF;

	DoTheRoundAbout(Ctx, S64BitBitfield::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.m_Package,  New.m_Package);
	EXPECT_EQ(Orig.m_FileType, New.m_FileType);
	EXPECT_EQ(Orig.m_PathHash, New.m_PathHash);
	EXPECT_EQ(Orig.m_FileHash, New.m_FileHash);
}

TEST_F(DL, SimplePtr)
{
	SPods Pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SSimplePtr  Orig = { &Pods, &Pods };
	SSimplePtr* New;

	SSimplePtr Loaded[64]; // this is so ugly!

	DoTheRoundAbout(Ctx, SSimplePtr::TYPE_ID, &Orig, Loaded);

	New = Loaded;
	EXPECT_NE(Orig.m_pPtr1, New->m_pPtr1);
	EXPECT_EQ(New->m_pPtr1, New->m_pPtr2);

	EXPECT_EQ(Orig.m_pPtr1->m_int8,   New->m_pPtr1->m_int8);
	EXPECT_EQ(Orig.m_pPtr1->m_int16,  New->m_pPtr1->m_int16);
	EXPECT_EQ(Orig.m_pPtr1->m_int32,  New->m_pPtr1->m_int32);
	EXPECT_EQ(Orig.m_pPtr1->m_int64,  New->m_pPtr1->m_int64);
	EXPECT_EQ(Orig.m_pPtr1->m_uint8,  New->m_pPtr1->m_uint8);
	EXPECT_EQ(Orig.m_pPtr1->m_uint16, New->m_pPtr1->m_uint16);
	EXPECT_EQ(Orig.m_pPtr1->m_uint32, New->m_pPtr1->m_uint32);
	EXPECT_EQ(Orig.m_pPtr1->m_uint64, New->m_pPtr1->m_uint64);
	EXPECT_EQ(Orig.m_pPtr1->m_fp32,   New->m_pPtr1->m_fp32);
	EXPECT_EQ(Orig.m_pPtr1->m_fp64,   New->m_pPtr1->m_fp64);
}

TEST_F(DL, PtrChain)
{
	SPtrChain Ptr1 = { 1337,   0x0 };
	SPtrChain Ptr2 = { 7331, &Ptr1 };
	SPtrChain Ptr3 = { 13,   &Ptr2 };
	SPtrChain Ptr4 = { 37,   &Ptr3 };

	SPtrChain* New;

	SPtrChain Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, SPtrChain::TYPE_ID, &Ptr4, Loaded);
	New = Loaded;

	EXPECT_NE(Ptr4.m_pNext, New->m_pNext);
	EXPECT_NE(Ptr3.m_pNext, New->m_pNext->m_pNext);
	EXPECT_NE(Ptr2.m_pNext, New->m_pNext->m_pNext->m_pNext);
	EXPECT_EQ(Ptr1.m_pNext, New->m_pNext->m_pNext->m_pNext->m_pNext); // should be equal, 0x0

	EXPECT_EQ(Ptr4.m_Int, New->m_Int);
	EXPECT_EQ(Ptr3.m_Int, New->m_pNext->m_Int);
	EXPECT_EQ(Ptr2.m_Int, New->m_pNext->m_pNext->m_Int);
	EXPECT_EQ(Ptr1.m_Int, New->m_pNext->m_pNext->m_pNext->m_Int);
}

TEST_F(DL, DoublePtrChain)
{
	// tests both circualar ptrs and reference to root-node!

	SDoublePtrChain Ptr1 = { 1337, 0x0,   0x0 };
	SDoublePtrChain Ptr2 = { 7331, &Ptr1, 0x0 };
	SDoublePtrChain Ptr3 = { 13,   &Ptr2, 0x0 };
	SDoublePtrChain Ptr4 = { 37,   &Ptr3, 0x0 };

	Ptr1.m_pPrev = &Ptr2;
	Ptr2.m_pPrev = &Ptr3;
	Ptr3.m_pPrev = &Ptr4;

	SDoublePtrChain* New;

	SDoublePtrChain Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, SDoublePtrChain::TYPE_ID, &Ptr4, Loaded);
	New = Loaded;

	// Ptr4
	EXPECT_NE(Ptr4.m_pNext, New->m_pNext);
	EXPECT_EQ(Ptr4.m_pPrev, New->m_pPrev); // is a null-ptr
	EXPECT_EQ(Ptr4.m_Int,   New->m_Int);

	// Ptr3
	EXPECT_NE(Ptr4.m_pNext->m_pNext, New->m_pNext->m_pNext);
	EXPECT_NE(Ptr4.m_pNext->m_pPrev, New->m_pNext->m_pPrev);
	EXPECT_EQ(Ptr4.m_pNext->m_Int,   New->m_pNext->m_Int);
	EXPECT_EQ(New,                   New->m_pNext->m_pPrev);

	// Ptr2
	EXPECT_NE(Ptr4.m_pNext->m_pNext->m_pNext, New->m_pNext->m_pNext->m_pNext);
	EXPECT_NE(Ptr4.m_pNext->m_pNext->m_pPrev, New->m_pNext->m_pNext->m_pPrev);
	EXPECT_EQ(Ptr4.m_pNext->m_pNext->m_Int,   New->m_pNext->m_pNext->m_Int);
	EXPECT_EQ(New->m_pNext,                   New->m_pNext->m_pNext->m_pPrev);

	// Ptr1
	EXPECT_EQ(Ptr4.m_pNext->m_pNext->m_pNext->m_pNext, New->m_pNext->m_pNext->m_pNext->m_pNext); // is null
	EXPECT_NE(Ptr4.m_pNext->m_pNext->m_pNext->m_pPrev, New->m_pNext->m_pNext->m_pNext->m_pPrev);
	EXPECT_EQ(Ptr4.m_pNext->m_pNext->m_pNext->m_Int,   New->m_pNext->m_pNext->m_pNext->m_Int);
	EXPECT_EQ(New->m_pNext->m_pNext,                   New->m_pNext->m_pNext->m_pNext->m_pPrev);
}

TEST_F(DL, Enum)
{
	EXPECT_EQ(TESTENUM2_VALUE2 + 1, TESTENUM2_VALUE3); // value3 is after value2 but has no value. It sohuld automticallay be one bigger!

	STestingEnum Inst;
	Inst.m_TheEnum = TESTENUM1_VALUE3;

	STestingEnum Loaded;

	DoTheRoundAbout(Ctx, STestingEnum::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Inst.m_TheEnum, Loaded.m_TheEnum);
}

TEST_F(DL, EnumInlineArray)
{
	SInlineArrayEnum Inst = { { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4 } };
	SInlineArrayEnum Loaded;

	DoTheRoundAbout(Ctx, SInlineArrayEnum::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Inst.m_lEnumArr[0], Loaded.m_lEnumArr[0]);
	EXPECT_EQ(Inst.m_lEnumArr[1], Loaded.m_lEnumArr[1]);
	EXPECT_EQ(Inst.m_lEnumArr[2], Loaded.m_lEnumArr[2]);
	EXPECT_EQ(Inst.m_lEnumArr[3], Loaded.m_lEnumArr[3]);
}

TEST_F(DL, EnumArray)
{
	ETestEnum2 Data[8] = { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4, TESTENUM2_VALUE4, TESTENUM2_VALUE3, TESTENUM2_VALUE2, TESTENUM2_VALUE1 } ;
	SArrayEnum Inst = { { Data, 8 } };
	SArrayEnum* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, SArrayEnum::TYPE_ID, &Inst, &Loaded);

	New = (SArrayEnum*)&Loaded[0];

	EXPECT_EQ(Inst.m_lEnumArr.m_nCount, New->m_lEnumArr.m_nCount);
	EXPECT_EQ(Inst.m_lEnumArr[0],       New->m_lEnumArr[0]);
	EXPECT_EQ(Inst.m_lEnumArr[1],       New->m_lEnumArr[1]);
	EXPECT_EQ(Inst.m_lEnumArr[2],       New->m_lEnumArr[2]);
	EXPECT_EQ(Inst.m_lEnumArr[3],       New->m_lEnumArr[3]);
	EXPECT_EQ(Inst.m_lEnumArr[4],       New->m_lEnumArr[4]);
	EXPECT_EQ(Inst.m_lEnumArr[5],       New->m_lEnumArr[5]);
	EXPECT_EQ(Inst.m_lEnumArr[6],       New->m_lEnumArr[6]);
	EXPECT_EQ(Inst.m_lEnumArr[7],       New->m_lEnumArr[7]);
}

TEST_F(DL, EmptyPodArray)
{
	SPodArray1 Inst = { { NULL, 0 } };
	SPodArray1 Loaded;

	DoTheRoundAbout(Ctx, SPodArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.m_lArray.m_nCount);
	EXPECT_EQ(0x0, Loaded.m_lArray.m_pData);
}

TEST_F(DL, EmptyStructArray)
{
	SStructArray1 Inst = { { NULL, 0 } };
	SStructArray1 Loaded;

	DoTheRoundAbout(Ctx, SStructArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.m_lArray.m_nCount);
	EXPECT_EQ(0x0, Loaded.m_lArray.m_pData);
}

TEST_F(DL, EmptyStringArray)
{
	SStringArray Inst = { { NULL, 0 } };
	SStringArray Loaded;

	DoTheRoundAbout(Ctx, SStructArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.m_lpStrings.m_nCount);
	EXPECT_EQ(0x0, Loaded.m_lpStrings.m_pData);
}

TEST_F(DL, BugTest1)
{
	// There was some error packing arrays ;)

	SBugTest1_InArray Arr[3] = { { 1337, 1338, 18 }, { 7331, 8331, 19 }, { 31337, 73313, 20 } } ;
	SBugTest1 Inst;
	Inst.m_lArr.m_pData  = Arr;
	Inst.m_lArr.m_nCount = DL_ARRAY_LENGTH(Arr);

	SBugTest1 Loaded[10];

	DoTheRoundAbout(Ctx, SBugTest1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Arr[0].m_u64_1, Loaded[0].m_lArr[0].m_u64_1);
	EXPECT_EQ(Arr[0].m_u64_2, Loaded[0].m_lArr[0].m_u64_2);
	EXPECT_EQ(Arr[0].m_u16,   Loaded[0].m_lArr[0].m_u16);
	EXPECT_EQ(Arr[1].m_u64_1, Loaded[0].m_lArr[1].m_u64_1);
	EXPECT_EQ(Arr[1].m_u64_2, Loaded[0].m_lArr[1].m_u64_2);
	EXPECT_EQ(Arr[1].m_u16,   Loaded[0].m_lArr[1].m_u16);
	EXPECT_EQ(Arr[2].m_u64_1, Loaded[0].m_lArr[2].m_u64_1);
	EXPECT_EQ(Arr[2].m_u64_2, Loaded[0].m_lArr[2].m_u64_2);
	EXPECT_EQ(Arr[2].m_u16,   Loaded[0].m_lArr[2].m_u16);
}

template<typename T>
const char* ArrayToString(T* _pArr, unsigned int _Count, char* pBuffer, unsigned int nBuffer)
{
	unsigned int Pos = snprintf(pBuffer, nBuffer, "{ %f", _pArr[0]);

	for(unsigned int i = 1; i < _Count && Pos < nBuffer; ++i)
	{
		Pos += snprintf(pBuffer + Pos, nBuffer - Pos, ", %f", _pArr[i]);
	}

	snprintf(pBuffer + Pos, nBuffer - Pos, " }");
	return pBuffer;
}

// TODO: Move this to other file!
#define EXPECT_ARRAY_EQ(_Count, _Expect, _Actual) \
	do \
	{ \
		bool WasEq = true; \
		for(uint i = 0; i < _Count && WasEq; ++i) \
			WasEq = _Expect[i] == _Actual[i]; \
		char ExpectBuf[1024]; \
		char ActualBuf[1024]; \
		char Err[2048]; \
		snprintf(Err, DL_ARRAY_LENGTH(Err), "Arrays diffed!\nExpected:\n%s\nActual:\n%s", \
											ArrayToString(_Expect, _Count, ExpectBuf, DL_ARRAY_LENGTH(ExpectBuf)), \
											ArrayToString(_Actual, _Count, ActualBuf, DL_ARRAY_LENGTH(ActualBuf))); \
		EXPECT_TRUE(WasEq) << Err; \
	} while (false);

TEST_F(DL, BugTest2)
{
	// some error converting from 32-bit-data to 64-bit.

	SBugTest2_WithMat Arr[2] = { { 1337, { 1.0f, 0.0f, 0.0f, 0.0f, 
										   0.0f, 1.0f, 0.0f, 0.0f,
										   0.0f, 0.0f, 1.0f, 0.0f, 
										   0.0f, 0.0f, 1.0f, 0.0f } },

								 { 7331, { 0.0f, 0.0f, 0.0f, 1.0f, 
										   0.0f, 0.0f, 1.0f, 0.0f,
										   0.0f, 1.0f, 0.0f, 0.0f, 
										   1.0f, 0.0f, 0.0f, 0.0f } } };
	SBugTest2 Inst;
	Inst.m_lInstances.m_pData  = Arr;
	Inst.m_lInstances.m_nCount = 2;

	SBugTest2 Loaded[40];

	DoTheRoundAbout(Ctx, SBugTest2::TYPE_ID, &Inst, &Loaded);

 	EXPECT_EQ(Arr[0].m_iSubModel, Loaded[0].m_lInstances[0].m_iSubModel);
 	EXPECT_EQ(Arr[1].m_iSubModel, Loaded[0].m_lInstances[1].m_iSubModel);
 	EXPECT_ARRAY_EQ(16, Arr[0].m_lTransform, Loaded[0].m_lInstances[0].m_lTransform);
 	EXPECT_ARRAY_EQ(16, Arr[1].m_lTransform, Loaded[0].m_lInstances[1].m_lTransform);
}

TEST(DLMisc, EndianIsCorrect)
{
	// Test that DL_ENDIAN_HOST is set correctly
	union
	{
		uint32 i;
		unsigned char c[4];
	} test;
	test.i = 0x01020304;

	if( DL_ENDIAN_HOST == DL_ENDIAN_LITTLE )
	{
		EXPECT_EQ(4, test.c[0]);
		EXPECT_EQ(3, test.c[1]);
		EXPECT_EQ(2, test.c[2]);
		EXPECT_EQ(1, test.c[3]);
	}
	else
	{
		EXPECT_EQ(1, test.c[0]);
		EXPECT_EQ(2, test.c[1]);
		EXPECT_EQ(3, test.c[2]);
		EXPECT_EQ(4, test.c[3]);
	}
}

TEST(DLUtil, ErrorToString)
{
	for(EDLError Err = DL_ERROR_OK; Err < DL_ERROR_INTERNAL_ERROR; Err = EDLError(uint(Err) + 1))
		EXPECT_STRNE("Unknown error!", dl_error_to_string(Err));
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
