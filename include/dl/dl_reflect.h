/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_REFLECT_H_INCLUDED
#define DL_DL_REFLECT_H_INCLUDED

/*
	File: dl_reflect.h
		Functions used to get information about types and type-members. Mostly designed for 
		use when binding DL towards other languages.
*/

#include <dl/dl.h>

/*
	Struct: SDLTypeInfo
		Struct used to retrieve information about a specific DL-type.
*/
struct SDLTypeInfo
{
	const char*  m_Name;
	unsigned int m_nMembers;
};

/*
	Struct: SDLMemberInfo
		Struct used to retrieve information about a specific DL-member.
*/
struct SDLMemberInfo
{
	const char*  m_Name;
	EDLType      m_Type;
	StrHash      m_TypeID;
	unsigned int m_ArrayCount;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
	Function: DLReflectGetTypeInfo
		Retrieve information about a certain type in a type-library.

	Parameters:
		_Context  - A valid handle to e DLContext
		_TypeID   - TypeID of the type to get information about.
		_pType    - Ptr to struct to fill with type-information.
		_pMembers - Ptr to array to fill with information about the members of the type.
		_nMembers - Size of _pMembers.

	Returns:
		DL_ERROR_OK on success, DL_ERROR_BUFFER_TO_SMALL if _pMembers do not fit all members, or other error if apropriate!
*/
EDLError DL_DLL_EXPORT DLReflectGetTypeInfo(HDLContext _Context, StrHash _TypeID, SDLTypeInfo* _pType, SDLMemberInfo* _pMembers, unsigned int _nMembers);

/*
	Function: DLSizeOfType
		Returns the size of a loaded type.

	Parameters:
		_Context  - Handle to a valid DL-context.
		_TypeHash - Hash of type to check size of.

	Returns:
		The size of the type or 0 if not found in context.
*/
unsigned int DLSizeOfType(HDLContext _Context, StrHash _TypeHash);

/*
	Function: DLAlignmentOfType
		Returns the alignment of a loaded type.

	Parameters:
		_Context  - Handle to a valid DL-context.
		_TypeHash - Hash of type to check alignment of.

	Returns:
		The alignment of the type or 0 if not found in context.
*/
unsigned int DLAlignmentOfType(HDLContext _Context, StrHash _TypeHash);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_REFLECT_H_INCLUDED
