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
	Function: StrCaseCompareLen
		Compares the two strings, ignoring case, until one or both are terminated by a NULL-char or <_Len> chars has been checked.

	Parameters:
		_pStr1 - First string to compare.
		_pStr2 - Second string to compare.
		_Len   - Maximum chars to check.

	Returns:
		Returns an int representing the relationship between the two strings.
		Value is zero indicates that <_pStr1> and <_pStr2> are equal.
		Value greater than zero indicates that the first char that differs between <_pStr1> and <_pStr2> are greater in <_pStr1>
		Value less than zero indicates that the first char that differs between <_pStr1> and <_pStr2> are greater in <_pStr2>
*/
DL_FORCEINLINE int StrCaseCompareLen(const char* _pStr1, const char* _pStr2, unsigned int _Len)
{
#if defined (_MSC_VER)
	for(unsigned int i = 0; i < _Len; i++)
	{
		int C1 = tolower(_pStr1[i]);
		int C2 = tolower(_pStr2[i]);
		if(C1 < C2) return -1;
		if(C1 > C2) return  1;
		if(C1 == '\0' && C1 == C2) return 0;
	}
	return 0;
#else // defined (_MSC_VER)
	return strncasecmp(_pStr1, _pStr2, _Len);
#endif // defined (_MSC_VER)
}

/*
	Function: StrVFormat
		Builds a null-terminated, printf-formated string into the buffer pointed to.

	Parameters:
		_pBuffer - The buffer to build the string into.
		_Size    - Size of _pBuffer.
		_pFormat - printf-format of string to build.
		_List    - Argument-list to read format-tokens from.

	Returns:
		Number of chars written to <_pBuffer>.
*/
DL_FORCEINLINE int StrVFormat(char* DL_RESTRICT _pBuffer, pint _Size, const char* DL_RESTRICT _pFormat, va_list _List)
{
	int ret = vsnprintf(_pBuffer, _Size, _pFormat, _List);
#if defined(DL_TAG_WINAPI)
	_pBuffer[_Size - 1] = '\0';
#endif // defined(M_TAG_WINAPI)
	return ret;
}

/*
	Function: StrFormat
		Builds a null-terminated, printf-formated string into the buffer pointed to.

	Parameters:
		_pBuffer - The buffer to build the string into.
		_Size    - Size of _pBuffer.
		_pFormat - printf-format of string to build.
		...      - Argument-list to read format-tokens from.

	Returns:
		Number of chars written to <_pBuffer>.
*/
static inline int StrFormat(char* DL_RESTRICT _pBuffer, pint _Size, const char* DL_RESTRICT _pFormat, ...)
{
	va_list Args;
	va_start(Args, _pFormat);
	int res = StrVFormat(_pBuffer, _Size, _pFormat, Args);
	va_end(Args);
	return res;
}

#endif // DL_DL_TEMP_H_INCLUDED
