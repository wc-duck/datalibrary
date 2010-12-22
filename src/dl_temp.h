/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TEMP_H_INCLUDED
#define DL_DL_TEMP_H_INCLUDED

// this file contains functionality from other sublibs before DL was broken out.
// these functions should be DL-ified and placed in better files.
// This file should be removed as soon as possible!

#include <dl/dl_defines.h>

#include <ctype.h>
#include <stdarg.h> // for va_list
#include <stdio.h>  // for vsnprintf

/*
	Function: IsAlign
		Return true if ptr has proper alignment.

	Parameters:
		_pPtr      - Pointer to check.
		_Alignment - The alignment to check for.

	Note:
		_Alignment need to be a power of 2!
*/
DL_FORCEINLINE bool IsAlign(const void* _pPtr, pint _Alignment)
{
	return ((pint)_pPtr & (_Alignment - 1)) == 0;
}

/*
	Function: AlignUp
		Return the closest address above or equal to input-ptr that has correct alignment.

	Parameters:
		_pPtr      - Base pointer to check.
		_Alignment - The alignment to use.

	Note:
		_Alignment need to be a power of 2!
*/
template<typename T>
DL_FORCEINLINE T AlignUp(const T _Value, pint _Alignment)
{
	return T( (pint(_Value) + _Alignment - 1) & ~(_Alignment - 1) );
}

// Function: Max
//	Returns the maximum value of the inputs
template<typename T> DL_FORCEINLINE T Max(T _ValA, T _ValB) { return _ValA < _ValB ? _ValB : _ValA; }

/*
	Function: StrFormat
		Builds a null-terminated, printf-formated string into the buffer pointed to.

	Parameters:
		buf      - The buffer to build the string into.
		buf_size - Size of _pBuffer.
		fmt      - printf-format of string to build.
		...      - Argument-list to read format-tokens from.

	Returns:
		Number of chars written to <_pBuffer>.
*/
static inline int StrFormat(char* DL_RESTRICT buf, pint buf_size, const char* DL_RESTRICT fmt, ...)
{
	va_list args;
	va_start( args, fmt );
	int res = vsnprintf( buf, buf_size, fmt, args );
	buf[buf_size - 1] = '\0';
	va_end( args );
	return res;
}

#endif // DL_DL_TEMP_H_INCLUDED
