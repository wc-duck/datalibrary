/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_reflect.h>

#include "dl_types.h"

// TODO: Write unittests for new functionality!!!

dl_error_t dl_reflect_num_types_loaded( dl_ctx_t dl_ctx, unsigned int* out_num_loaded_types )
{
	*out_num_loaded_types = dl_ctx->m_nTypes; // need to add enums aswell
	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_types( dl_ctx_t dl_ctx, dl_typeid_t* out_types, unsigned int out_types_size )
{
	if( dl_ctx->m_nTypes > out_types_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	for( unsigned int type = 0; type < dl_ctx->m_nTypes; ++type )
		out_types[type] = dl_ctx->m_TypeLookUp[type].m_TypeID;

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_id( dl_ctx_t dl_ctx, const char* type_name, dl_typeid_t* out_type_id )
{
	*out_type_id = DLHashString( type_name );
	const SDLType* pType = DLFindType(dl_ctx, *out_type_id);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_info(dl_ctx_t _Context, dl_typeid_t _TypeID, dl_type_info_t* _pType )
{
	const SDLType* pType = DLFindType(_Context, _TypeID);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	_pType->name         = pType->m_Name;
	_pType->size         = pType->m_Size[DL_PTR_SIZE_HOST];
	_pType->alignment    = pType->m_Alignment[DL_PTR_SIZE_HOST];
	_pType->member_count = pType->m_nMembers;

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_type_members( dl_ctx_t dl_ctx, dl_typeid_t type, dl_member_info_t* out_members, unsigned int members_size )
{
	const SDLType* pType = DLFindType( dl_ctx, type );
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	if(members_size < pType->m_nMembers)
		return DL_ERROR_BUFFER_TO_SMALL;

	for(uint32 nMember = 0; nMember < pType->m_nMembers; ++nMember)
	{
		const SDLMember& Member = pType->m_lMembers[nMember];

		out_members[nMember].name   = Member.m_Name;
		out_members[nMember].type   = Member.m_Type;
		out_members[nMember].type_id = Member.m_TypeID;

		if(Member.AtomType() == DL_TYPE_ATOM_INLINE_ARRAY)
		{
			switch(Member.StorageType())
			{
				// TODO: This switch could be skipped if inline-array count were built in to Member.m_Type
				case DL_TYPE_STORAGE_STRUCT:
				{
					const SDLType* pSubType = DLFindType( dl_ctx, Member.m_TypeID );
					if(pSubType == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;

					out_members[nMember].array_count = Member.m_Size[DL_PTR_SIZE_HOST] / pSubType->m_Size[DL_PTR_SIZE_HOST];
				}
				break;
				case DL_TYPE_STORAGE_STR: out_members[nMember].array_count = Member.m_Size[DL_PTR_SIZE_HOST] / sizeof(char*); break;
				default:
					out_members[nMember].array_count = Member.m_Size[DL_PTR_SIZE_HOST] / (uint32)DLPodSize(Member.m_Type); break;
			}
		}
	}

	return DL_ERROR_OK;
}
