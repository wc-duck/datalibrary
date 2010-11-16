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
	Struct: dl_type_info_t
		Struct used to retrieve information about a specific DL-type.
*/
typedef struct dl_type_info
{
	const char*  name;
	unsigned int size;
	unsigned int alignment;
	unsigned int member_count;
} dl_type_info_t;

/*
	Struct: dl_member_info_t
		Struct used to retrieve information about a specific DL-member.
*/
typedef struct dl_member_info
{
	const char*  name;
	dl_type_t    type;
	dl_typeid_t  type_id;
	unsigned int array_count;
} dl_member_info_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
	Function: dl_reflect_num_types_loaded
		Get the number of loaded types in the context.

	Parameters:
		dl_ctx              - The dl-context to check number of loaded types in.
		out_num_loaded_type - Number of loaded types returned here.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_num_types_loaded( dl_ctx_t dl_ctx, unsigned int* out_num_loaded_types );

/*
	Function: dl_reflect_loaded_types
		Get the typeid:s of all loaded types in the context.

	Parameters:
		dl_ctx        - The dl-context to fetch loaded types from.
		out_types     - Buffer to return loaded types in.
		out_types_out - Size of out_types.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_loaded_types( dl_ctx_t dl_ctx, dl_typeid_t* out_types, unsigned int out_types_size );

/*
	Function: dl_reflect_get_type_id
		Find typeid of a specified type by name.

	Parameters:
		dl_ctx      - A valid handle to a DLContext to do the type-lookup in.
		type_ame    - Name of type to lookup.
		out_type_id - TypeID returned here.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_id( dl_ctx_t dl_ctx, const char* type_name, dl_typeid_t* out_type );

/*
	Function: dl_reflect_get_type_info
		Retrieve information about a certain type in a type-library.

	Parameters:
		dl_ctx        - A valid handle to a DLContext
		type          - TypeID of the type to get information about.
		out_type_info - Ptr to struct to fill with type-information.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_type_info_t* out_type_info );

/*
	Function: dl_reflect_get_type_members
		Retrieve information about members of a type in a type-library.

	Parameters:
		dl_ctx        - A valid handle to a DLContext
		type          - TypeID of the type to get information about.
		out_members   - Ptr to array to fill with information about the members of the type.
		members_size  - Size of _pMembers.

	Returns:
		DL_ERROR_OK on success, DL_ERROR_BUFFER_TO_SMALL if out_members do not fit all members, or other error if appropriate!
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_members( dl_ctx_t dl_ctx, dl_typeid_t type, dl_member_info_t* out_members, unsigned int out_members_size );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_REFLECT_H_INCLUDED
