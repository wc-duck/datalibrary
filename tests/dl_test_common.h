/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TEST_COMMON_H_INCLUDED
#define DL_DL_TEST_COMMON_H_INCLUDED

// header file generated from unittest-type lib
#include "generated/unittest.h"
#include "generated/unittest2.h"

#include <stdio.h>

#if defined(_MSC_VER)
	#define snprintf _snprintf // ugly fugly.
#endif // defined(_MSC_VER)

#define DL_UNITTEST_ALL // all tests are run! undef to disabla all unittests

#define EXPECT_DL_ERR_EQ(_Expect, _Res) { EXPECT_EQ(_Expect, _Res) << "Result:   " << dl_error_to_string(_Res) ; }
#define EXPECT_DL_ERR_OK(_Res) EXPECT_DL_ERR_EQ( DL_ERROR_OK, _Res)
#define DL_ARRAY_LENGTH(Array) (sizeof(Array)/sizeof(Array[0]))

template<typename T>
const char* ArrayToString(T* _pArr, unsigned int _Count, char* pBuffer, unsigned int nBuffer)
{
	unsigned int Pos = snprintf(pBuffer, nBuffer, "{ %f", (float)_pArr[0]);

	for(unsigned int i = 1; i < _Count && Pos < nBuffer; ++i)
	{
		Pos += snprintf(pBuffer + Pos, nBuffer - Pos, ", %f", (float)_pArr[i]);
	}

	snprintf(pBuffer + Pos, nBuffer - Pos, " }");
	return pBuffer;
}

#define EXPECT_ARRAY_EQ(_Count, _Expect, _Actual) \
	{ \
		bool WasEq = true; \
		for(unsigned int i = 0; i < _Count && WasEq; ++i) \
			WasEq = _Expect[i] == _Actual[i]; \
		char ExpectBuf[1024]; \
		char ActualBuf[1024]; \
		char Err[2048]; \
		snprintf(Err, DL_ARRAY_LENGTH(Err), "Expected:\n%s\nActual:\n%s", \
											ArrayToString(_Expect, _Count, ExpectBuf, DL_ARRAY_LENGTH(ExpectBuf)), \
											ArrayToString(_Actual, _Count, ActualBuf, DL_ARRAY_LENGTH(ActualBuf))); \
		EXPECT_TRUE(WasEq) << Err; \
	}

struct DL : public ::testing::Test
{
	virtual void SetUp()
	{
		// bake the unittest-type library into the exe!
		static const unsigned char TypeLib1[] =
		{
			#include "generated/unittest.bin.h"
		};
		static const unsigned char TypeLib2[] =
		{
			#include "generated/unittest2.bin.h"
		};

		dl_create_params_t p;
		DL_CREATE_PARAMS_SET_DEFAULT(p);

		EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_create( &Ctx, &p ) );
		EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_type_library(Ctx, TypeLib1, sizeof(TypeLib1)) );
		EXPECT_DL_ERR_EQ( DL_ERROR_OK, dl_context_load_type_library(Ctx, TypeLib2, sizeof(TypeLib2)) );
	}

	virtual void TearDown()
	{
		EXPECT_EQ(DL_ERROR_OK, dl_context_destroy(Ctx));
	}

	dl_ctx_t Ctx;
};

#endif // DL_DL_TEST_COMMON_H_INCLUDED
