/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_CONVERT_H_INCLUDED
#define DL_DL_CONVERT_H_INCLUDED

/*
	File: dl_convert.h
		Exposes functionality to convert packed dl-instances between different formats
		regarding pointer-size and endianness.
*/

#include <dl/dl.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	Function: dl_convert
		Converts a packed instance to an other format.

	Parameters:
		_Context     - Handle to valid DL-context.
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.
		_pOutData    - Ptr to memory-area where to place the converted instance.
		_OutDataSize - Size of _pOutData.
		_Endian      - Endian to convert the packed instance to.
		_PtrSize     - Size in bytes of pointers after conversions, valid values 4 and 8.
*/
dl_error_t DL_DLL_EXPORT dl_convert( dl_ctx_t       _Context,
                                     unsigned char* _pData,
                                     unsigned int   _DataSize,
                                     unsigned char* _pOutData,
                                     unsigned int   _OutDataSize,
                                     dl_endian_t    _Endian,
                                     unsigned int   _PtrSize );

/*
	Function: dl_convert_inplace
		Converts a packed instance to an other format inplace.

	Parameters:
		_Context     - Handle to valid DL-context.
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.
		_Endian      - Endian to convert the packed instance to.
		_PtrSize     - Size in bytes of pointers after conversions, valid values 4 and 8.

	Note:
		Function is restricted to converting endianness and converting 8-byte ptr:s to 4-byte ptr:s
*/
dl_error_t DL_DLL_EXPORT dl_convert_inplace( dl_ctx_t     _Context,
                                             unsigned char* _pData,
                                             unsigned int   _DataSize,
                                             dl_endian_t   _Endian,
                                             unsigned int   _PtrSize );

/*
	Function: dl_convert_calc_size
		Calculates size of an instance after _PtrSize-conversion.

	Parameters:
		_Context     - Handle to valid DL-context.
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.
		_PtrSize     - Size in bytes of pointers after conversions, valid values 4 and 8.
		_pResultSize - Ptr where to store the calculated size.
*/
dl_error_t DL_DLL_EXPORT dl_convert_calc_size( dl_ctx_t       _Context,
                                               unsigned char* _pData,
                                               unsigned int   _DataSize,
                                               unsigned int   _PtrSize,
                                               unsigned int*  _pResultSize );

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // DL_DL_CONVERT_H_INCLUDED
