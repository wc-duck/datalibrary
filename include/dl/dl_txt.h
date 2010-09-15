/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TXT_H_INCLUDED
#define DL_DL_TXT_H_INCLUDED

/*
	File: dl_txt.h
		Provides functionality to pack txt-format DL-data to binary and unpack binary data to txt-format.
*/

#include <dl/dl.h>

#ifdef __cplusplus
extern "C" {
#endif  

/*
	Function: DLPackText
		Pack string of intermediate-data to binary blob that is loadable by DLLoadInstances and DLLoadInstancesInplace.

	Parameters:
		_Context        - Context to use.
		_pTxtData       - Zero-terminated string to pack to binary blob.
		_pPackedData    - Buffer to pack data to.
		_PackedDataSize - Size of _pPackedData.

	Note:
		The instance after pack will be in current platform endian.
*/
EDLError DL_DLL_EXPORT DLPackText(HDLContext _Context, const char* _pTxtData, uint8* _pPackedData, pint _PackedDataSize);

/*
	Function: DLRequiredTextPackSize
		Calculate the amount of memory needed to pack intermediate data to binary blob.

	Parameters:
		_Context         - Context to use.
		_pTxtData        - Zero-terminated string to calculate binary blob size of.
		_pPackedDataSize - Size required to pack _pTxtData.
*/
EDLError DL_DLL_EXPORT DLRequiredTextPackSize(HDLContext _Context, const char* _pTxtData, pint* _pPackedDataSize);

/*
	Function: DLUnpack
		Unpack binary blob packed with SDBLPack to intermediate-data-format.

	Parameters:
		_Context        - Context to use.
		_pPackedData    - Buffer with packed data.
		_PackedDataSize - Size of _pPackedData.
		_pTxtData       - Ptr to buffer where to write txt-data.
		_TxtDataSize    - Size of _pTxtData.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
EDLError DL_DLL_EXPORT DLUnpack(HDLContext _Context, const uint8* _pPackedData, pint _PackedDataSize, char* _pTxtData, pint _TxtDataSize);

/*
	Function: DLRequiredUnpackSize
		Calculate the amount of memory needed to unpack binary data to intermediate data.

	Parameters:
		_Context        - Context to use.
		_pPackedData    - Buffer with packed data.
		_PackedDataSize - Size of _pPackedData.
		_pTxtDataSize   - Size required to unpack _pPackedData.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
EDLError DL_DLL_EXPORT DLRequiredUnpackSize(HDLContext _Context, const uint8* _pPackedData, pint _PackedDataSize, pint* _pTxtDataSize);

#ifdef __cplusplus
}
#endif  

#endif // DL_DL_TXT_H_INCLUDED
