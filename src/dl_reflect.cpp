/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_types.h"

#include <dl/dl_reflect.h>

// TODO: Write unittests for new functionality!!!

dl_error_t dl_reflect_context_info( dl_ctx_t dl_ctx, dl_type_context_info_t* info )
{
	info->num_types = dl_ctx->type_count;
	info->num_enums = dl_ctx->enum_count;
	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_types( dl_ctx_t dl_ctx, dl_typeid_t* out_types, unsigned int out_types_size )
{
	if( dl_ctx->type_count > out_types_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	memcpy( out_types, dl_ctx->type_ids, sizeof( dl_typeid_t ) * dl_ctx->type_count );
	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_enums( dl_ctx_t dl_ctx, dl_typeid_t* out_enums, unsigned int out_enums_size )
{
	if( dl_ctx->enum_count > out_enums_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	for( unsigned int e = 0; e < dl_ctx->enum_count; ++e )
		out_enums[e] = dl_ctx->enum_lookup[e].type_id;

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_id( dl_ctx_t dl_ctx, const char* type_name, dl_typeid_t* out_type_id )
{
	const dl_type_desc* pType = dl_internal_find_type_by_name( dl_ctx, type_name );
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	*out_type_id = dl_internal_typeid_of( dl_ctx, pType );

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_type_info_t* out_type )
{
	const dl_type_desc* pType = dl_internal_find_type(dl_ctx, type);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	out_type->name         = pType->name;
	out_type->size         = pType->size[DL_PTR_SIZE_HOST];
	out_type->alignment    = pType->alignment[DL_PTR_SIZE_HOST];
	out_type->member_count = pType->member_count;

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_info_t* out_enum_info )
{
	const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, type );
	if( e == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	out_enum_info->name        = e->name;
	out_enum_info->value_count = e->value_count;

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_type_members( dl_ctx_t dl_ctx, dl_typeid_t type, dl_member_info_t* out_members, unsigned int members_size )
{
	const dl_type_desc* pType = dl_internal_find_type( dl_ctx, type );
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	if(members_size < pType->member_count)
		return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t nMember = 0; nMember < pType->member_count; ++nMember )
	{
		const dl_member_desc* Member = dl_get_type_member( dl_ctx, pType, nMember );

		out_members[nMember].name    = Member->name;
		out_members[nMember].type    = Member->type;
		out_members[nMember].type_id = Member->type_id;

		if(Member->AtomType() == DL_TYPE_ATOM_INLINE_ARRAY)
		{
			switch(Member->StorageType())
			{
				// TODO: This switch could be skipped if inline-array count were built in to Member->m_Type
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* pSubType = dl_internal_find_type( dl_ctx, Member->type_id );
					if(pSubType == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;

					out_members[nMember].array_count = Member->size[DL_PTR_SIZE_HOST] / pSubType->size[DL_PTR_SIZE_HOST];
				}
				break;
				case DL_TYPE_STORAGE_STR: out_members[nMember].array_count = (unsigned int)(Member->size[DL_PTR_SIZE_HOST] / sizeof(char*)); break;
				default:
					out_members[nMember].array_count = Member->size[DL_PTR_SIZE_HOST] / (uint32_t)DLPodSize(Member->type); break;
			}
		}
	}

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_values( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_value_info_t* out_values, unsigned int out_values_size )
{
	const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, type );
	if( e == 0x0 )                       return DL_ERROR_TYPE_NOT_FOUND;
	if( out_values_size < e->value_count ) return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t value = 0; value < e->value_count; ++value )
	{
		out_values[value].name  = e->values[value].name;
		out_values[value].value = e->values[value].value;
	}

	return DL_ERROR_OK;
}
