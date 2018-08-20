/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TEST_COMMON_H_INCLUDED
#define DL_DL_TEST_COMMON_H_INCLUDED

#include <dl/dl.h>

// header file generated from unittest-type lib
#include "generated/unittest.h"
#include "dl_test_included.h" // TODO: this should be included in unittest2.h someway.
#include "generated/unittest2.h"
#include "generated/sized_enums.h"

#include <stdio.h>

#if defined(_MSC_VER)
	#define snprintf _snprintf // ugly fugly.
#endif // defined(_MSC_VER)

#define ASSERT_DL_ERR_EQ(_Expect, _Res) { dl_error_t err = _Res; ASSERT_EQ(_Expect, err) << "Result:   " << dl_error_to_string(err) ; }
#define ASSERT_DL_ERR_OK(_Res) ASSERT_DL_ERR_EQ( DL_ERROR_OK, _Res)
#define EXPECT_DL_ERR_EQ(_Expect, _Res) { dl_error_t err = _Res; EXPECT_EQ(_Expect, err) << "Result:   " << dl_error_to_string(err) ; }
#define EXPECT_DL_ERR_OK(_Res) EXPECT_DL_ERR_EQ( DL_ERROR_OK, _Res)
#define DL_ARRAY_LENGTH(Array) (uint32_t)(sizeof(Array)/sizeof(Array[0]))

template<typename T>
const char* ArrayToString(T* arr, unsigned int _Count, char* buff, size_t buff_size)
{
	unsigned int Pos = (unsigned int)snprintf(buff, buff_size, "{ %f", (float)arr[0]);

	for(unsigned int i = 1; i < _Count && Pos < buff_size; ++i)
	{
		Pos += (unsigned int)snprintf(buff + Pos, buff_size - Pos, ", %f", (float)arr[i]);
	}

	snprintf(buff + Pos, buff_size - Pos, " }");
	return buff;
}

#define EXPECT_ARRAY_EQ(_Count, _Expect, _Actual) \
	{ \
		bool WasEq = true; \
		for(unsigned int EXPECT_ARRAY_EQ_i = 0; EXPECT_ARRAY_EQ_i < _Count && WasEq; ++EXPECT_ARRAY_EQ_i) \
			WasEq = _Expect[EXPECT_ARRAY_EQ_i] == _Actual[EXPECT_ARRAY_EQ_i]; \
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
	virtual ~DL() {}

	virtual void SetUp();
	virtual void TearDown();

	dl_ctx_t Ctx;
};

#endif // DL_DL_TEST_COMMON_H_INCLUDED
