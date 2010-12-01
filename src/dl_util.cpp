/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include <stdlib.h>
#include <stdio.h>

// TODO: DLType sent to these functions should be used for type-checks, make it possible to ignore in some way?

dl_error_t dl_util_load_from_file( dl_ctx_t    dl_ctx,   dl_typeid_t         type,
                                   const char* filename, dl_util_file_type_t filetype,
                                   void**      out_instance )
{
	// TODO: this function need to handle alignment for _ppInstance
	// TODO: this function should take an allocator for the user to be able to control allocations.

	FILE* in_file = fopen(filename, "rb");

	if(in_file == 0x0) { fclose(in_file); return DL_ERROR_UTIL_FILE_NOT_FOUND; }

	fseek(in_file, 0, SEEK_END);
	unsigned int file_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	unsigned char* file_content = (unsigned char*)malloc((unsigned int)(file_size) + 1);
	size_t read_bytes = fread(file_content, sizeof(unsigned char), file_size, in_file);
	fclose(in_file);
	if( read_bytes != file_size )
		return DL_ERROR_INTERNAL_ERROR;
	file_content[file_size] = '\0';

	dl_error_t error = DL_ERROR_OK;
	dl_instance_info_t info;

	error = dl_instance_get_info( file_content, file_size, &info);

	dl_util_file_type_t in_file_type = DL_UTIL_FILE_TYPE_TEXT;
	if( error == DL_ERROR_OK ) // could read it as binary!
		in_file_type = DL_UTIL_FILE_TYPE_BINARY;

	if( (in_file_type & filetype) == 0 )
		return DL_ERROR_UTIL_FILE_TYPE_MISMATCH;
	
	unsigned char* load_instance = 0x0;
	unsigned int   load_size = 0;

	switch(in_file_type)
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			error = dl_convert_calc_size( dl_ctx, type, file_content, file_size, sizeof(void*), &load_size);

			if( error != DL_ERROR_OK ) { free( file_content ); return error; }

			// convert if needed
			if( load_size > file_size )
			{
				load_instance = (unsigned char*)malloc(load_size);

				error = dl_convert( dl_ctx, type, file_content, file_size, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*));

				free( file_content );
			}
			else
			{
				load_instance = file_content;
				load_size     = file_size;
				error = dl_convert_inplace( dl_ctx, type, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*));
			}

			if( error != DL_ERROR_OK ) { free( load_instance ); return error; }
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calc needed space
			unsigned int packed_size = 0;
			error = dl_txt_pack_calc_size(dl_ctx, (char*)file_content, &packed_size);

			if(error != DL_ERROR_OK) { free(file_content); return error; }

			load_instance = (unsigned char*)malloc(packed_size);

			error = dl_txt_pack(dl_ctx, (char*)file_content, load_instance, packed_size);

			free(file_content);

			if(error != DL_ERROR_OK) { free(load_instance); return error; }
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	error = dl_instance_load(dl_ctx, type, load_instance, load_instance, load_size);

	*out_instance = load_instance;

	return error;
}

dl_error_t dl_util_load_from_file_inplace( dl_ctx_t    dl_ctx,       dl_typeid_t         type,
                                           const char* filename,     dl_util_file_type_t filetype,
                                           void*       out_instance, unsigned int        out_instance_size )
{
	(void)dl_ctx; (void)filename; (void)type; (void)filetype; (void)out_instance; (void)out_instance_size;
	return DL_ERROR_INTERNAL_ERROR; // TODO: Build me
}

dl_error_t dl_util_store_to_file( dl_ctx_t    dl_ctx,     dl_typeid_t         type,
                                  const char* filename,   dl_util_file_type_t filetype,
                                  dl_endian_t out_endian, unsigned int        out_ptr_size,
                                  void*       instance )
{
	if( filetype == DL_UTIL_FILE_TYPE_AUTO )
		return DL_ERROR_INVALID_PARAMETER;

	unsigned int packed_size = 0;

	// calculate pack-size
	dl_error_t error = dl_instance_calc_size( dl_ctx, type, instance, &packed_size );

	if( error != DL_ERROR_OK)
		return error;

	// alloc memory
	unsigned char* packed_instance = (unsigned char*)malloc(packed_size);

	// pack data
	error =  dl_instance_store( dl_ctx, type, instance, packed_instance, packed_size );

	if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

	unsigned int out_size = 0;
	unsigned char* out_data = 0x0;

	switch( filetype )
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			// calc convert size
			error = dl_convert_calc_size( dl_ctx, type, packed_instance, packed_size, out_ptr_size, &out_size);

			if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

			// convert
			if( out_size > packed_size )
			{
				// new alloc
				out_data = (unsigned char*)malloc(out_size);

				// convert
				error = dl_convert( dl_ctx, type, packed_instance, packed_size, out_data, out_size, out_endian, out_ptr_size );

				free(packed_instance);

				if( error != DL_ERROR_OK ) { free(out_data); return error; }
			}
			else
			{
				out_data = packed_instance;

				error = dl_convert_inplace( dl_ctx, type, packed_instance, packed_size, out_endian, out_ptr_size );

				if( error != DL_ERROR_OK ) { free(out_data); return error; }
			}
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calculate pack-size
			error = dl_txt_unpack_calc_size( dl_ctx, type, packed_instance, packed_size, &out_size);

			if( error != DL_ERROR_OK ) { free(packed_instance); return error; }

			// alloc data
			out_data = (unsigned char*)malloc(out_size);

			// pack data
			error = dl_txt_unpack( dl_ctx, type, packed_instance, packed_size, (char*)out_data, out_size);

			free(packed_instance);

			if( error != DL_ERROR_OK ) { free(out_data); return error; }
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	FILE* out_file = fopen(filename, "");
	if( out_file != 0x0 )
		fwrite( out_data, out_size, 1, out_file );
	else
		error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	free( out_data );
	fclose( out_file );

	return error;
}
