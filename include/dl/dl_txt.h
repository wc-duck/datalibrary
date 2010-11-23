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

/*
	Function: dl_txt_pack
		Pack string of intermediate-data to binary blob that is loadable by dl_instance_load.

	Parameters:
		dl_ctx            - Context to use.
		txt_instance      - Zero-terminated string to pack to binary blob.
		out_instance      - Buffer to pack data to.
		out_instance_size - Size of _pPackedData.

	Note:
		The instance after pack will be in current platform endian.
*/
dl_error_t DL_DLL_EXPORT dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_instance, unsigned int out_instance_size );

/*
	Function: dl_txt_pack_calc_size
		Calculate the amount of memory needed to pack intermediate data to binary blob.

	Parameters:
		dl_ctx            - Context to use.
		txt_instance      - Zero-terminated string to calculate binary blob size of.
		out_instance_size - Size required to pack _pTxtData.
*/
dl_error_t DL_DLL_EXPORT dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, unsigned int* out_instance_size );

/*
	Function: dl_txt_unpack
		Unpack binary blob packed with SDBLPack to intermediate-data-format.

	Parameters:
		dl_ctx                - Context to use.
		type                  - Type stored in packed_instace.
		packed_instance       - Buffer with packed data.
		packed_instance_size  - Size of _pPackedData.
		out_txt_instance      - Ptr to buffer where to write txt-data.
		out_txt_instance_size - Size of _pTxtData.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
dl_error_t DL_DLL_EXPORT dl_txt_unpack( dl_ctx_t dl_ctx,                       dl_typeid_t  type,
                                        const unsigned char* packed_instance,  unsigned int packed_instance_size,
                                        char*                out_txt_instance, unsigned int out_txt_instance_size );

/*
	Function: dl_txt_unpack_calc_size
		Calculate the amount of memory needed to unpack binary data to intermediate data.

	Parameters:
		dl_ctx                - Context to use.
		type                  - Type stored in packed_instace.
		packed_instance       - Buffer with packed data.
		packed_instance_size  - Size of _pPackedData.
		out_txt_instance_size - Size required to unpack _pPackedData.

	Note:
		Packed instance to unpack is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
dl_error_t DL_DLL_EXPORT dl_txt_unpack_calc_size( dl_ctx_t dl_ctx,                      dl_typeid_t  type,
                                                  const unsigned char* packed_instance, unsigned int packed_instance_size,
                                                  unsigned int* out_txt_instance_size );

#ifdef __cplusplus
}
#endif

#endif // DL_DL_TXT_H_INCLUDED
