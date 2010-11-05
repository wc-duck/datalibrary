/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_DEFINES_H_INCLUDED
#define DL_DL_DEFINES_H_INCLUDED

#if defined(_MSC_VER)
	#define DL_FORCEINLINE __forceinline
	#define DL_DLL_EXPORT  __declspec(dllexport)
	#define DL_ALIGN(x)    __declspec(align(x))
	#define DL_RESTRICT    __restrict
#elif defined(__GNUC__)
	#define DL_FORCEINLINE inline __attribute__((always_inline))
	#define DL_DLL_EXPORT
	#define DL_ALIGN(x)    __attribute__((aligned(x)))
	#define DL_RESTRICT    __restrict__
#else
	#error No supported compiler
#endif

// remove me!
#if defined(_MSC_VER)
	typedef unsigned __int32 StrHash;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t StrHash;
#endif

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
