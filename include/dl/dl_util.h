/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_UTIL_H_INCLUDED
#define DL_DL_UTIL_H_INCLUDED

/*
	File: dl_reflect.h
		Utility functions for DL. Mostly for making tools writing easier.

	Note:
		All functions in dl_util.h are implemented with core-dl functionality and
		should not be using anything not available to the ordinary user of dl.
*/

#include <dl/dl.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	Enum: dl_util_file_type_t
		Enumeration of possible file types that can be read by util-functions.

	DL_UTIL_FILE_TYPE_BINARY - File must be an binary instance.
	DL_UTIL_FILE_TYPE_TEXT   - File must be an text instance.
	DL_UTIL_FILE_TYPE_AUTO   - Let the function decide if binary or text.
*/
enum dl_util_file_type_t
{
	DL_UTIL_FILE_TYPE_BINARY = DL_BIT(0),
	DL_UTIL_FILE_TYPE_TEXT   = DL_BIT(1),
	DL_UTIL_FILE_TYPE_AUTO   = DL_UTIL_FILE_TYPE_BINARY  | DL_UTIL_FILE_TYPE_TEXT
};

/*
	Function: dl_util_load_from_file
		Utility function that loads an dl-instance from file.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.
		The pointer returned in out_instance need to be freed with free() when not needed any
		more.

	Parameters:
		dl_ctx       - Context to use for operations.
		type         - Type expected to be found in file, set to 0 if not known.
		filename     - Path to file to load from.
		filetype     - Type of file to read, see dl_util_file_type_t.
		out_instance - Pointer to fill with read instance.
		out_type     - TypeID of instance found in file, can be set to 0x0.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_load_from_file( dl_ctx_t    dl_ctx,       dl_typeid_t         type,
								   const char* filename,     dl_util_file_type_t filetype,
								   void**      out_instance, dl_typeid_t*        out_type );

/*
	Function: dl_util_load_from_stream
		Utility function that loads an dl-instance from an open stream.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.
		The pointer returned in out_instance need to be freed with free() when not needed any
		more.

	Parameters:
		dl_ctx         - Context to use for operations.
		type           - Type expected to be found in file, set to 0 if not known.
		stream         - Open stream to load from.
		filetype       - Type of file to read, see dl_util_file_type_t.
		out_instance   - Pointer to fill with read instance.
		out_type       - TypeID of instance found in file, can be set to 0x0.
		consumed_bytes - Number of bytes read from stream.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_load_from_stream( dl_ctx_t dl_ctx,       dl_typeid_t         type,
									 FILE*    stream,       dl_util_file_type_t filetype,
									 void**   out_instance, dl_typeid_t*        out_type,
									 size_t*  consumed_bytes );

/*
	Function: dl_util_load_from_file_inplace
		Utility function that loads an dl-instance from file to a specified memory-area.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.

	Parameters:
		dl_ctx            - Context to use for operations.
		type              - Type expected to be found in file.
		filename          - Path to file to load from.
		filetype          - Type of file to read, see EDLUtilFileType.
		out_instance      - Pointer to area to load instance to.
		out_instance_size - Size of buffer pointed to by _ppInstance

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_load_from_file_inplace( dl_ctx_t     dl_ctx,       dl_typeid_t         type,
										   const char*  filename,     dl_util_file_type_t filetype,
										   void*        out_instance, size_t              out_instance_size,
										   dl_typeid_t* out_type );

/*
	Function: dl_util_store_to_file
		Utility function that writes an instance to file.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.

	Parameters:
		dl_ctx       - Context to use for operations.
		type         - Type expected to be found in instance.
		filename     - Path to file to store to.
		filetype     - Type of file to write, see EDLUtilFileType.
		out_endian   - Endian of stored instance if binary.
		out_ptr_size - Pointer size of stored instance if binary.
		out_instance - Pointer to instance to write

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_store_to_file( dl_ctx_t    dl_ctx,     dl_typeid_t         type,
								  const char* filename,   dl_util_file_type_t filetype,
								  dl_endian_t out_endian, size_t              out_ptr_size,
								  const void* out_instance );

/*
	Function: dl_util_store_to_stream
		Utility function that writes an instance to an open stream.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.

	Parameters:
		dl_ctx       - Context to use for operations.
		type         - Type expected to be found in instance.
		stream       - Open stream to write to.
		filetype     - Type of file to write, see EDLUtilFileType.
		out_endian   - Endian of stored instance if binary.
		out_ptr_size - Pointer size of stored instance if binary.
		out_instance - Pointer to instance to write

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_store_to_stream( dl_ctx_t    dl_ctx,     dl_typeid_t         type,
									FILE*       stream,     dl_util_file_type_t filetype,
									dl_endian_t out_endian, size_t              out_ptr_size,
									const void* out_instance );

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // DL_DL_UTIL_H_INCLUDED
