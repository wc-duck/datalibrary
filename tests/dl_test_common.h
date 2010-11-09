/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TEST_COMMON_H_INCLUDED
#define DL_DL_TEST_COMMON_H_INCLUDED

// header file generated from unittest-type lib
#include "generated/unittest.h"

#define M_EXPECT_DL_ERR_EQ(_Expect, _Res) do { EXPECT_EQ(_Expect, _Res) << "Result:   " << dl_error_to_string(_Res) ; } while (0);

static void* MyAlloc(unsigned int  _Size, unsigned int _Alignment) { (void)_Alignment; return malloc(_Size); }
static void  MyFree (void* _pPtr) { free(_pPtr); }

class DL : public ::testing::Test
{
protected:
	SDLAllocFunctions MyAllocs;

	virtual void SetUp()
	{
		// bake the unittest-type library into the exe!
		static const uint8 TypeLib[] =
		{
			#include "generated/unittest.bin.h"
		};

		MyAllocs.m_Alloc = MyAlloc;
		MyAllocs.m_Free  = MyFree;

		EDLError err = dl_context_create(&Ctx, &MyAllocs, &MyAllocs);
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

		err = dl_context_load_type_library(Ctx, TypeLib, sizeof(TypeLib));
		M_EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
	}

	virtual void TearDown()
	{
		EXPECT_EQ(DL_ERROR_OK, dl_context_destroy(Ctx));
	}

public:
	HDLContext Ctx;
};

#endif // DL_DL_TEST_COMMON_H_INCLUDED
