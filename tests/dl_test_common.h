/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TEST_COMMON_H_INCLUDED
#define DL_DL_TEST_COMMON_H_INCLUDED

#include <dl/dl.h>
#include <dl/dl_txt.h>

// header file generated from unittest-type lib
#include "generated/unittest.h"
#include "generated/unittest2.h"
#include "generated/sized_enums.h"
#include "generated/to_include.h"

#include <stdio.h>

#if defined(_MSC_VER)
	#define snprintf _snprintf // ugly fugly.
#endif // defined(_MSC_VER)

#define ASSERT_DL_ERR_EQ(_Expect, _Res)  { dl_error_t err = _Res; ASSERT_EQ(_Expect, err) << "Result:   " << dl_error_to_string(err) ; }
#define EXPECT_DL_ERR_EQ(_Expect, _Res)  { dl_error_t err = _Res; EXPECT_EQ(_Expect, err) << "Result:   " << dl_error_to_string(err) ; }
#define ASSERT_DL_ERR_OK(_Res)           ASSERT_DL_ERR_EQ( DL_ERROR_OK, _Res)
#define EXPECT_DL_ERR_OK(_Res)           EXPECT_DL_ERR_EQ( DL_ERROR_OK, _Res)
#define DL_ARRAY_LENGTH(Array) (uint32_t)(sizeof(Array)/sizeof(Array[0]))

#define STRINGIFY( ... ) #__VA_ARGS__

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
	{                                                                                                                 \
		bool WasEq = true;                                                                                            \
		for(unsigned int EXPECT_ARRAY_EQ_i = 0; EXPECT_ARRAY_EQ_i < _Count && WasEq; ++EXPECT_ARRAY_EQ_i)             \
			WasEq = _Expect[EXPECT_ARRAY_EQ_i] == _Actual[EXPECT_ARRAY_EQ_i];                                         \
		char ExpectBuf[4096];                                                                                         \
		EXPECT_TRUE(WasEq) << "Expected:\n" << ArrayToString(_Expect, _Count, ExpectBuf, DL_ARRAY_LENGTH(ExpectBuf))  \
						   << "\n"                                                                                    \
							  "Actual:\n"   << ArrayToString(_Actual, _Count, ExpectBuf, DL_ARRAY_LENGTH(ExpectBuf)); \
	}

template <typename T>
static T* dl_txt_test_pack_text(dl_ctx_t Ctx, const char* txt, void* unpack_buffer, size_t unpack_buffer_size)
{
    unsigned char out_text_data[4096];
    EXPECT_DL_ERR_OK( dl_txt_pack( Ctx, txt, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );

    memset( unpack_buffer, 0x0, unpack_buffer_size );
    EXPECT_DL_ERR_OK( dl_instance_load( Ctx, T::TYPE_ID, unpack_buffer, unpack_buffer_size, out_text_data, DL_ARRAY_LENGTH(out_text_data), 0x0 ) );

    return (T*)unpack_buffer;
}

struct DL : public ::testing::Test
{
	virtual ~DL() {}

	virtual void SetUp();
	virtual void TearDown();

	dl_ctx_t Ctx;
};

#endif // DL_DL_TEST_COMMON_H_INCLUDED
