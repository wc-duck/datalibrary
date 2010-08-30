#ifndef DL_DL_TEST_COMMON_H_INCLUDED
#define DL_DL_TEST_COMMON_H_INCLUDED

// header file generated from unittest-type lib
#include "generated/dl/unittest.h"

static void* MyAlloc(pint  _Size, pint _Alignment) { M_UNUSED(_Alignment); return malloc(_Size); }
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
			#include "generated/dl/unittest.bin.h"
		};

		MyAllocs.m_Alloc = MyAlloc;
		MyAllocs.m_Free  = MyFree;

		EDLError err = DLContextCreate(&Ctx, &MyAllocs, &MyAllocs);
		EXPECT_EQ(DL_ERROR_OK, err);

		err = DLLoadTypeLibrary(Ctx, TypeLib, sizeof(TypeLib));
		EXPECT_EQ(DL_ERROR_OK, err);
	}

	virtual void TearDown()
	{
		EXPECT_EQ(DL_ERROR_OK, DLContextDestroy(Ctx));
	}

public:
	HDLContext Ctx;
};

#define M_EXPECT_DL_ERR_EQ(_Expect, _Res) do { EXPECT_EQ(_Expect, _Res) << "Result:   " << DLErrorToString(_Res) ; } while (0);

#endif // DL_DL_TEST_COMMON_H_INCLUDED
