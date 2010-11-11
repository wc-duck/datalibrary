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
		out_members   - Ptr to array to fill with information about the members of the type.
		members_size  - Size of _pMembers.

	Returns:
		DL_ERROR_OK on success, DL_ERROR_BUFFER_TO_SMALL if _pMembers do not fit all members, or other error if apropriate!
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_type_info_t* out_type_info, dl_member_info_t* out_members, unsigned int members_size );

/*
	Function: dl_size_of_type
		Returns the size of a loaded type.

	Parameters:
		dl_ctx - Handle to a valid DL-context.
		type   - Hash of type to check size of.

	Returns:
		The size of the type or 0 if not found in context.
*/
unsigned int dl_size_of_type( dl_ctx_t dl_ctx, dl_typeid_t type );

/*
	Function: dl_alignment_of_type
		Returns the alignment of a loaded type.

	Parameters:
		dl_ctx - Handle to a valid DL-context.
		type   - Hash of type to check alignment of.

	Returns:
		The alignment of the type or 0 if not found in context.
*/
unsigned int dl_alignment_of_type( dl_ctx_t dl_ctx, dl_typeid_t type );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_REFLECT_H_INCLUDED
