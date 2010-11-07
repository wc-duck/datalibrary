/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include <stdlib.h>
#include <stdio.h>

#include "dl_types.h" // TODO: Remove this include! dl_util should be built only with the external interface

// TODO: DLType sent to these functions should be used for type-checks, make it possible to ignore in some way?

EDLError dl_util_load_from_file( HDLContext _Ctx,
                                 const char* _pFileName,
                                 StrHash _DLType,
                                 EDLUtilFileType file_type,
                                 void** _ppInstance )
{
	// TODO: this function need to handle alignment for _ppInstance
	// TODO: this function should take an allocator for the user to be able to control allocations.

	FILE* in_file = fopen(_pFileName, "rb");

	if(in_file == 0x0) { fclose(in_file); return DL_ERROR_UTIL_FILE_NOT_FOUND; }

	uint32 dl_header_data_id; // TODO: build dl_util with only external interface. Could use dl_instance_info()

	// read only first byte to determine if entire file should be read.
	if( fread( &dl_header_data_id, sizeof(uint32), 1, in_file) != sizeof(uint32) ) { fclose(in_file); return DL_ERROR_MALFORMED_DATA; }

	EDLUtilFileType in_file_type = DL_UTIL_FILE_TYPE_TEXT;
	if( dl_header_data_id != DL_TYPE_DATA_ID || dl_header_data_id != DL_TYPE_DATA_ID_SWAPED )
		in_file_type = DL_UTIL_FILE_TYPE_BINARY;

	if( in_file_type & file_type ) { fclose(in_file); return DL_ERROR_UTIL_FILE_TYPE_MISMATCH; }

	fseek(in_file, 0, SEEK_END);
	unsigned int file_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	unsigned char* file_content = (unsigned char*)malloc((unsigned int)(file_size) + 1);
	fread(file_content, sizeof(unsigned char), file_size, in_file);
	file_content[file_size] = '\0';
	fclose(in_file);

	EDLError error = DL_ERROR_OK;
	unsigned char* load_instance = 0x0;
	unsigned int   load_size = 0;

	switch(in_file_type)
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			error = dl_convert_calc_size( _Ctx, file_content, file_size, sizeof(void*), &load_size);

			if( error != DL_ERROR_OK ) { free( file_content ); return error; }

			// convert if needed
			if( load_size > file_size )
			{
				load_instance = (unsigned char*)malloc(load_size);

				error = dl_convert( _Ctx, file_content, file_size, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*));

				free( file_content );
			}
			else
			{
				load_instance = file_content;
				load_size     = file_size;
				error = dl_convert_inplace( _Ctx, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*));
			}

			if( error != DL_ERROR_OK ) { free( load_instance ); return error; }
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calc needed space
			unsigned int packed_size = 0;
			error = dl_txt_pack_calc_size(_Ctx, (char*)file_content, &packed_size);

			if(error != DL_ERROR_OK) { free(file_content); return error; }

			load_instance = (unsigned char*)malloc(packed_size);

			error = dl_txt_pack(_Ctx, (char*)file_content, load_instance, packed_size);

			free(file_content);

			if(error != DL_ERROR_OK) { free(load_instance); return error; }
		}
		break;
		default:
			M_ASSERT( false );
	}

	error = dl_instance_load(_Ctx, load_instance, load_instance, load_size);

	*_ppInstance = load_instance;

	return error;
}

EDLError dl_util_load_from_file_inplace( HDLContext      _Ctx,
                                         const char*     _pFileName,
                                         StrHash         _DLType,
                                         EDLUtilFileType _FileType,
                                         void*           _ppInstance,
                                         unsigned int    _InstanceSize )
{
	(void)_Ctx; (void)_pFileName; (void)_DLType; (void)_FileType; (void)_ppInstance; (void)_InstanceSize;
	return DL_ERROR_INTERNAL_ERROR; // TODO: Build me
}

EDLError dl_util_store_to_file( HDLContext      _Ctx,
                                const char*     _pFileName,
                                StrHash         _DLType,
                                EDLUtilFileType _FileType,
                                EDLCpuEndian    _Endian,
                                unsigned int    _PtrSize,
                                void*           _pInstance )
{
	if( _FileType == DL_UTIL_FILE_TYPE_AUTO )
		return DL_ERROR_INVALID_PARAMETER;

	unsigned int packed_size = 0;

	// calculate pack-size
	EDLError error = dl_instace_calc_size( _Ctx, _DLType, _pInstance, &packed_size );

	if( error != DL_ERROR_OK)
		return error;

	// alloc memory
	unsigned char* packed_instance = (unsigned char*)malloc(packed_size);

	// pack data
	error =  dl_instace_store( _Ctx, _DLType, _pInstance, packed_instance, packed_size );

	if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

	unsigned int out_size = 0;
	unsigned char* out_data = 0x0;

	switch( _FileType )
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			// calc convert size
			error = dl_convert_calc_size( _Ctx, packed_instance, packed_size, _PtrSize, &out_size);

			if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

			// convert
			if( out_size > packed_size )
			{
				// new alloc
				out_data = (unsigned char*)malloc(out_size);

				// convert
				error = dl_convert( _Ctx, packed_instance, packed_size, out_data, out_size, _Endian, _PtrSize );

				free(packed_instance);

				if( error != DL_ERROR_OK ) { free(out_data); return error; }
			}
			else
			{
				out_data = packed_instance;

				error = dl_convert_inplace( _Ctx, packed_instance, packed_size, _Endian, _PtrSize );

				if( error != DL_ERROR_OK ) { free(out_data); return error; }
			}
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calculate pack-size
			error = dl_txt_unpack_calc_size( _Ctx, packed_instance, packed_size, &out_size);

			if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

			// alloc data
			out_data = (unsigned char*)malloc(out_size);

			// pack data
			error = dl_txt_unpack( _Ctx, packed_instance, packed_size, (char*)out_data, out_size);

			free(packed_instance);

			if( error != DL_ERROR_OK ) { free(out_data); return error; }
		}
		break;
		default:
			M_ASSERT( false );
	}

	FILE* out_file = fopen(_pFileName, "");
	if( out_file != 0x0 )
		fwrite( out_data, out_size, 1, out_file );
	else
		error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	free( out_data );
	fclose( out_file );

	return error;
}
