/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_DEFINES_H_INCLUDED
#define DL_DL_DEFINES_H_INCLUDED

// TODO: Fix for other compilers and platforms
#define DL_COMPILER_MSVC
#define DL_TAG_WINAPI

#if defined(DL_COMPILER_MSVC)
	#define DL_FORCEINLINE __forceinline
	#define DL_DLL_EXPORT  __declspec(dllexport)
	#define DL_ALIGN(x)    __declspec(align(x))
	#define DL_RESTRICT    __restrict
#elif defined(DL_COMPILER_GCC)
	#define DL_FORCEINLINE inline __attribute__((always_inline))
	#define DL_DLL_EXPORT
	#define DL_ALIGN(x)    __attribute__((aligned(x)))
	#define DL_RESTRICT    __restrict__
#endif

#define DL_UNUSED (void)

template<class T>
struct TAlignmentOf
{
	struct CAlign { ~CAlign() {}; unsigned char m_Dummy; T m_T; };
	enum { ALIGNOF = sizeof(CAlign) - sizeof(T) };
};
#define DL_ALIGNMENTOF(Type) TAlignmentOf<Type>::ALIGNOF

// remove me!
#if defined(DL_COMPILER_MSVC)
	typedef unsigned int uint;
	typedef signed   __int8  int8;
	typedef signed   __int16 int16;
	typedef signed   __int32 int32;
	typedef signed   __int64 int64;
	typedef unsigned __int8  uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	typedef long long fint;
	typedef int       bint;


	// this is not even correct on all platforms!
	typedef unsigned long pint;

#elif defined(DL_COMPILER_GCC)
	#include <stdint.h>
	typedef unsigned int uint;
	typedef int8_t   int8;
	typedef int16_t  int16;
	typedef int32_t  int32;
	typedef int64_t  int64;
	typedef uint8_t  uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;

	typedef uint32_t pint;

	typedef long long fint;
	typedef int       bint;
#endif

typedef float  fp32;
typedef double fp64;

// remove me!
typedef uint32 StrHash;

// remove me!
enum ECpuEndian
{
	ENDIAN_BIG,
	ENDIAN_LITTLE,

// #if defined(M_CPU_ENDIAN_LITTLE)
	ENDIAN_HOST = ENDIAN_LITTLE,
// #elif defined(M_CPU_ENDIAN_BIG)
// 	ENDIAN_HOST = ENDIAN_BIG,
// #else // defined(M_CPU_ENDIAN_*)
// 	#error no endianness specified
// #endif // defined(M_CPU_ENDIAN_*)
};

static const int8  DL_INT8_MAX  = 0x7F;
static const int16 DL_INT16_MAX = 0x7FFF;
static const int32 DL_INT32_MAX = 0x7FFFFFFFL;
static const int64 DL_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const int8  DL_INT8_MIN  = (-DL_INT8_MAX  - 1);
static const int16 DL_INT16_MIN = (-DL_INT16_MAX - 1);
static const int32 DL_INT32_MIN = (-DL_INT32_MAX - 1);
static const int64 DL_INT64_MIN = (-DL_INT64_MAX - 1);

static const uint8  DL_UINT8_MAX  = 0xFFU;
static const uint16 DL_UINT16_MAX = 0xFFFFU;
static const uint32 DL_UINT32_MAX = 0xFFFFFFFFUL;
static const uint64 DL_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
static const uint8  DL_UINT8_MIN  = 0x00U;
static const uint16 DL_UINT16_MIN = 0x0000U;
static const uint32 DL_UINT32_MIN = 0x00000000UL;
static const uint64 DL_UINT64_MIN = 0x0000000000000000ULL;

// remove me?
#define DL_JOIN_TOKENS(a,b) DL_JOIN_TOKENS_DO_JOIN(a,b)
#define DL_JOIN_TOKENS_DO_JOIN(a,b) DL_JOIN_TOKENS_DO_JOIN2(a,b)
#define DL_JOIN_TOKENS_DO_JOIN2(a,b) a##b

namespace dl_staticassert
{
	template <bool x> struct STATIC_ASSERTION_FAILURE;
	template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
};
#define DL_STATIC_ASSERT(_Expr, _Msg) enum { DL_JOIN_TOKENS(_static_assert_enum_##_Msg, __LINE__) = sizeof(::dl_staticassert::STATIC_ASSERTION_FAILURE< (bool)( _Expr ) >) }

#define M_ASSERT(_Expr, ...)

#define DL_ARRAY_LENGTH(Array) (sizeof(Array)/sizeof(Array[0]))

#endif // DL_DL_DEFINES_H_INCLUDED
