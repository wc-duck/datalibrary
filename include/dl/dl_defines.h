/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_DEFINES_H_INCLUDED
#define DL_DL_DEFINES_H_INCLUDED

#if defined(_MSC_VER)
	#define DL_DLL_EXPORT  __declspec(dllexport)
#elif defined(__GNUC__)
	#define DL_DLL_EXPORT
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
