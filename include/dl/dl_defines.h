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
	typedef unsigned __int32 dl_typeid_t;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t dl_typeid_t;
#endif

#endif // DL_DL_DEFINES_H_INCLUDED
