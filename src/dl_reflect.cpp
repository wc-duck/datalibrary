#include <dl/dl_reflect.h>

#include "dl_types.h"

EDLError DLReflectGetTypeInfo(HDLContext _Context, StrHash _TypeID, SDLTypeInfo* _pType, SDLMemberInfo* _pMembers, pint _nMembers)
{
	const SDLType* pType = DLFindType(_Context, _TypeID);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	if(_nMembers < pType->m_nMembers)
		return DL_ERROR_BUFFER_TO_SMALL;

	_pType->m_Name     = pType->m_Name;
	_pType->m_nMembers = pType->m_nMembers;

	for(uint32 nMember = 0; nMember < pType->m_nMembers; ++nMember)
	{
		const SDLMember& Member = pType->m_lMembers[nMember];

		_pMembers[nMember].m_Name   = Member.m_Name;
		_pMembers[nMember].m_Type   = Member.m_Type;
		_pMembers[nMember].m_TypeID = Member.m_TypeID;

		if(Member.AtomType() == DL_TYPE_ATOM_INLINE_ARRAY)
		{
			switch(Member.StorageType())
			{
				// TODO: This switch could be skipped if inline-array count were built in to Member.m_Type
				case DL_TYPE_STORAGE_STRUCT: 
				{
					const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
					if(pSubType == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;

					_pMembers[nMember].m_ArrayCount = Member.m_Size[DL_PTR_SIZE_HOST] / pSubType->m_Size[DL_PTR_SIZE_HOST];
				}
				break;
				case DL_TYPE_STORAGE_STR: _pMembers[nMember].m_ArrayCount = Member.m_Size[DL_PTR_SIZE_HOST] / sizeof(char*); break;
				default:
					M_ASSERT(Member.IsSimplePod());
					_pMembers[nMember].m_ArrayCount = Member.m_Size[DL_PTR_SIZE_HOST] / (uint32)DLPodSize(Member.m_Type); break;
			}
		}
	}

	return DL_ERROR_OK;
}

uint32 DLSizeOfType(HDLContext _Context, StrHash _TypeHash)
{
	const SDLType* pType = DLFindType(_Context, _TypeHash);
	return pType == 0x0 ? 0 : pType->m_Size[DL_PTR_SIZE_HOST];
}

uint32 DLAlignmentOfType(HDLContext _Context, StrHash _TypeHash)
{
	const SDLType* pType = DLFindType(_Context, _TypeHash);
	return pType == 0x0 ? 0 : pType->m_Alignment[DL_PTR_SIZE_HOST];
}
