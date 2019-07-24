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

typedef enum {
	DL_PACKFLAGS_NONE = 0,
	DL_PACKFLAGS_NO_DEFAULTS = 1 << 0, 						// Causes the packing to not write the defaults into the packed instance.
	DL_PACKFLAGS_IS_WRITING_TO_EXISTING_INSTANCE = 1 << 1,	// Assumes that the out_buffer is already filled with an instance. Any dynamic data is written after this and assumed that there is space for.
} dl_pack_flags_t;

/*
	Function: dl_txt_pack
		Pack string of intermediate-data to binary blob that is loadable by dl_instance_load.

	Parameters:
		dl_ctx          - Context to use.
		txt_instance    - Zero-terminated string to pack to binary blob.
		out_buffer      - Buffer to pack data to.
		out_buffer_size - Size of out_buffer.
		produced_bytes  - Number of bytes that would have been written to out_buffer if it was large enough.
		pack_flags        Flags to take into consideration when packing

	Return:
		DL_ERROR_OK on success. Packing an instance to a 0-sized out_buffer is thought of as a success as
		it can be used to calculate the size of a packed instance.
		If out_buffer_size is > 0 but smaller than the required size to pack the instance, an error is
		returned.

	Note:
		The instance after pack will be in current platform endian.
*/
dl_error_t DL_DLL_EXPORT dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_buffer, size_t out_buffer_size, size_t* produced_bytes, dl_pack_flags_t pack_flags );

/*
	Function: dl_txt_pack_calc_size
		Calculate the amount of memory needed to pack intermediate data to binary blob.

	Parameters:
		dl_ctx            - Context to use.
		txt_instance      - Zero-terminated string to calculate binary blob size of.
		out_instance_size - Size required to pack _pTxtData.
		pack_flags          Flags to take into consideration when calculating packing size
*/
dl_error_t DL_DLL_EXPORT dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, size_t* out_instance_size, dl_pack_flags_t pack_flags );

/*
	Function: dl_txt_unpack
		Unpack binary packed instance to text-format.

	Parameters:
		dl_ctx                - Context to use.
		type                  - Type stored in packed_instace.
		packed_instance       - Buffer with packed data.
		packed_instance_size  - Size of packed_instance_size.
		out_txt_instance      - Ptr to buffer where to write txt-data.
		out_txt_instance_size - Size of out_txt_instance.
		produced_bytes        - Number of bytes that would have been written to out_txt_instance if it was large enough.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
dl_error_t DL_DLL_EXPORT dl_txt_unpack( dl_ctx_t dl_ctx,                       dl_typeid_t type,
                                        const unsigned char* packed_instance,  size_t      packed_instance_size,
                                        char*                out_txt_instance, size_t      out_txt_instance_size,
                                        size_t*              produced_bytes );

/*
	Function: dl_txt_unpack_calc_size
		Calculate the amount of memory needed to unpack binary data to intermediate data.

	Parameters:
		dl_ctx                - Context to use.
		type                  - Type stored in packed_instace.
		packed_instance       - Buffer with packed data.
		packed_instance_size  - Size of packed_instance.
		out_txt_instance_size - Size required to unpack packed_instance.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
dl_error_t DL_DLL_EXPORT dl_txt_unpack_calc_size( dl_ctx_t             dl_ctx,               dl_typeid_t type,
                                                  const unsigned char* packed_instance,      size_t      packed_instance_size,
                                                  size_t*              out_txt_instance_size );

#ifdef __cplusplus
}
#endif

#endif // DL_DL_TXT_H_INCLUDED
