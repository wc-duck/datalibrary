/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TYPELIB_H_INCLUDED
#define DL_DL_TYPELIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	File: dl_typelib.h

		File containing functions to work with type libraries.
		These are used to implement the typelibrary compiler but are exposed here for all to use.

		Note: using this should not be needed by most applications since typelibraries could be built by
		      an offline tool.
*/

#include <dl/dl.h>

/*
	Function pointer type: dl_include_handler
	    Is responsible for locating and loading a type-library into dl_ctx.

	Parameters:
	    handler_ctx     - context passed in to dl_context_load_txt_type_library_with_includes()
	    dl_ctx          - dl-context to load typelib into.
	    file_to_include - the path to the file to include
 */
typedef dl_error_t ( *dl_include_handler )( void* handler_ctx, dl_ctx_t dl_ctx, const char* file_to_include );

/*
 	Function: dl_context_load_txt_type_library
		Load type-library from text representation into dl_ctx.

	Parameters:
		dl_ctx          - dl-context to load typelib into..
		lib_data        - pointer to loaded txt-type library to load.
		lib_data_size   - size of string pointed to by lib_data.
		include_handler - a callback function which tries to load the given type library, can be nullptr
		handler_ctx     - this will be passed straight down to the handler callback


	Note:
		This function do not have the same rules of memory allocation and might allocate memory behind the scenes.
 */
dl_error_t DL_DLL_EXPORT dl_context_load_txt_type_library( dl_ctx_t dl_ctx, const char* lib_data, size_t lib_data_size, dl_include_handler include_handler, void* handler_ctx );

/*
	Function: dl_context_write_type_library
		Write all types loaded in dl_ctx to a dl-typelibrary.

	Parameters:
		dl_ctx         - dl-context to write to buffer.
		out_lib        - buffer to write typelib to.
		out_lib_size   - size of out_lib.
		produced_bytes - number of bytes that would have been written to out_buffer if it was large enough.

	Return:
		DL_ERROR_OK on success. Storing an instance to a 0-sized out_buffer is thought of as a success as
		it can be used to calculate the size of a stored instance.
		If out_buffer_size is > 0 but smaller than the required size to store the instance, an error is
		returned.

	Note:
		This function do not have the same rules of memory allocation and might allocate memory behind the scenes.
 */
dl_error_t DL_DLL_EXPORT dl_context_write_type_library( dl_ctx_t dl_ctx, unsigned char* out_lib, size_t out_lib_size, size_t* produced_bytes );

/*
 	 Function: dl_context_write_type_library
		Write all types loaded in dl_ctx to a dl-typelibrary in text format.

	Parameters:
		dl_ctx         - dl-context to write to buffer.
		out_lib        - buffer to write typelib to.
		out_lib_size   - size of out_lib.
		produced_bytes - number of bytes that would have been written to out_buffer if it was large enough.

	Return:
		DL_ERROR_OK on success. Storing an instance to a 0-sized out_buffer is thought of as a success as
		it can be used to calculate the size of a stored instance.
		If out_buffer_size is > 0 but smaller than the required size to store the instance, an error is
		returned.

	Note:
		This function do not have the same rules of memory allocation and might allocate memory behind the scenes.
*/
dl_error_t DL_DLL_EXPORT dl_context_write_txt_type_library( dl_ctx_t dl_ctx, char* out_lib, size_t out_lib_size, size_t* produced_bytes );

/*
	Function: dl_context_write_type_library
		Write all types loaded in dl_ctx to a dl-typelibrary to a c-header used to load data packed by dl.

	Parameters:
		dl_ctx          - dl-context to write to buffer.
		module_name     - name of generated module.
		out_header      - buffer to write c-header to.
		out_header_size - size of out_header.
		produced_bytes  - number of bytes that would have been written to out_header if it was large enough.

	Return:
		DL_ERROR_OK on success. Storing an instance to a 0-sized out_buffer is thought of as a success as
		it can be used to calculate the size of a stored instance.
		If out_buffer_size is > 0 but smaller than the required size to store the instance, an error is
		returned.

	Note:
		This function do not have the same rules of memory allocation and might allocate memory behind the scenes.
*/
dl_error_t DL_DLL_EXPORT dl_context_write_type_library_c_header( dl_ctx_t dl_ctx, const char* module_name, char* out_header, size_t out_header_size, size_t* produced_bytes );

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // DL_DL_TYPELIB_H_INCLUDED
