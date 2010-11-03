/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_UTIL_H_INCLUDED
#define DL_DL_UTIL_H_INCLUDED

/*
	File: dl_reflect.h
		Utility functions for DL. Mostly for making tools writing easier.
*/

#include <dl/dl.h>

/*
	Function: DLUtilLoadInstanceFromFile
		Function that loads a DL-instance from file.

	Note:
		This function allocates dynamic memory and should therefore be used accordingly.
		The pointer returned in _ppInstance need to be freed with free();

	Parameters:
		_Ctx        - Context to use for operations.
		_pFileName  - Path to file to load from.
		_DLType     - The type of the instance to read. (Example SMyDLType::TYPE_ID)
		_ppInstance - Pointer to fill with read instance.

	Returns:
		DL_UTIL_ERROR_OK on success.
*/
EDLError DLUtilLoadInstanceFromFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void** _ppInstance);

/*
	Function: DLUtilLoadInstanceFromTextFile
		Function that loads a DL-instance from text file.

	Note:
		This function allocates dynamic memory and should therefore be used accordingly.
		The pointer returned in _ppInstance need to be freed with free();

	Parameters:
		_Ctx        - Context to use for operations.
		_pFileName  - Path to file to load from.
		_ppInstance - Pointer to fill with read instance.

	Returns:
		DL_UTIL_ERROR_OK on success.
*/
EDLError DLUtilLoadInstanceFromTextFile(HDLContext _Ctx, const char* _pFileName, void** _ppInstance);

/*
	Function: DLUtilLoadInstanceFromTextFile
		Function that loads a DL-instance from text file.
		Data pointed to by _pInstance is assumed to point to a pointer of the correct type.
		If the data will not fit in the buffer pointed to by _pInstance an error will be generated.

	Note:
		This function allocates dynamic memory and should therefore be used accordingly.

	Parameters:
		_Ctx          - Context to use for operations.
		_pFileName    - Path to file to load from.
		_pInstance    - Pointer to instance to fill with read instance.
		_InstanceSize - Size of buffer pointed to by _pInstance

	Returns:
		DL_UTIL_ERROR_OK on success.
*/
EDLError DLUtilLoadInstanceFromTextFileInplace(HDLContext _Ctx, const char* _pFileName, void* _pInstance, unsigned int _InstanceSize);

/*
	Function: DLUtilStoreInstanceToFile
		Function that writes a DL-instance to file.

	Note:
		This function allocates dynamic memory and should therefore be used accordingly.

	Parameters:
		_Ctx        - Context to use for operations.
		_pFileName  - Path to file to write to.
		_DLType     - The type of the instance to write. (Example SMyDLType::TYPE_ID)
		_pInstance  - Pointer to instance to store.
		_OutEndian  - Endian of stored instance in file.
		_OutPtrSize - Pointer-size of data in stored instance.

	Returns:
		DL_UTIL_ERROR_OK on success.
*/
EDLError DLUtilStoreInstanceToFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance, ECpuEndian _OutEndian, unsigned int _OutPtrSize);

/*
	Function: DLUtilStoreInstanceToTextFile
		Function that writes a DL-instance to file in text-format.

	Note:
		This function allocates dynamic memory and should therefore be used accordingly.

	Parameters:
		_Ctx       - Context to use for operations.
		_pFileName - Path to file to write to.
		_DLType    - The type of the instance to write. (Example SMyDLType::TYPE_ID)
		_pInstance - Pointer to instance to store.

	Returns:
		DL_UTIL_ERROR_OK on success.
*/
EDLError DLUtilStoreInstanceToTextFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance);

#endif // DL_DL_UTIL_H_INCLUDED
