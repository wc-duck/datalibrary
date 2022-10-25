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

#ifndef DL_NODISCARD
	#if( _MSVC_LANG >= 201703L ) || ( __cplusplus >= 201703L )
		#define DL_NODISCARD [[nodiscard]]
	#else
		#define DL_NODISCARD
	#endif
#endif

#include <stdint.h>
typedef uint32_t dl_typeid_t;

#endif // DL_DL_DEFINES_H_INCLUDED
