/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dl_patch_alloc_funcs( dl_alloc_func& alloc_func, dl_realloc_func& realloc_func, dl_free_func& free_func )
{
	if (alloc_func == nullptr)
		alloc_func = [](size_t size, void*) -> void* { return malloc(size); };
	if (free_func == nullptr)
		free_func = [](void* ptr, void*) -> void { free(ptr); };
	if (realloc_func == nullptr)
	{
		realloc_func = [](void* ptr, size_t size, size_t old_size, void*) -> void*
						{
							void* new_ptr = malloc( size );
							if( ptr != nullptr && new_ptr != nullptr )
							{
								memcpy( new_ptr, ptr, old_size );
								free( ptr );
							}
							return new_ptr;
						};
	}
}

static unsigned char* dl_read_entire_stream( dl_realloc_func realloc_func, void* alloc_ctx, FILE* file, size_t* out_size )		 
{
	const unsigned int CHUNK_SIZE = 1024;
	size_t         total_size = 0;
	size_t         chunk_size = 0;
	unsigned char* out_buffer = 0;

	do
	{
		out_buffer = (unsigned char*)realloc_func( out_buffer, CHUNK_SIZE + total_size, total_size, alloc_ctx );
		chunk_size = fread( out_buffer + total_size, 1, CHUNK_SIZE, file );
		total_size += chunk_size;
	}
	while( chunk_size >= CHUNK_SIZE );

	// Might need 1 extra byte to write string null terminator
	if( total_size % CHUNK_SIZE == 0 )
		out_buffer = (unsigned char*)realloc_func( out_buffer, 1 + total_size, total_size, alloc_ctx );

	*out_size = total_size;
	return out_buffer;
}

static dl_error_t dl_util_load_from_buffer( dl_ctx_t            dl_ctx,     dl_typeid_t  type,
											uint8_t*            buffer,     size_t       buffer_size,
											dl_util_file_type_t filetype,   void**       out_instance,
											dl_typeid_t*        out_type,   void**       allocated_mem,
											dl_alloc_func       alloc_func, dl_free_func free_func,
											void*               alloc_ctx )
{
	// TODO: this function need to handle alignment for out_instance
	dl_error_t error = DL_ERROR_OK;
	dl_instance_info_t info;

	error = dl_instance_get_info( buffer, buffer_size, &info );

	dl_util_file_type_t in_file_type = error == DL_ERROR_OK ? DL_UTIL_FILE_TYPE_BINARY : DL_UTIL_FILE_TYPE_TEXT;

	if( ( in_file_type & filetype ) == 0 )
	{
		free_func( (void*) buffer, alloc_ctx );
		return DL_ERROR_UTIL_FILE_TYPE_MISMATCH;
	}

	unsigned char* load_instance = 0x0;
	size_t         load_size = 0;

	switch(in_file_type)
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			if( type == 0 ) // autodetect root struct type
				type = info.root_type;

			error = dl_convert( dl_ctx, type, buffer, buffer_size, 0x0, 0, DL_ENDIAN_HOST, sizeof(void*), &load_size );

			if( error != DL_ERROR_OK ) { free_func( (void*) buffer, alloc_ctx ); return error; }

			// convert if needed
			if( load_size > buffer_size || info.ptrsize < sizeof(void*) )
			{
				load_instance = (unsigned char*)alloc_func( load_size, alloc_ctx );

				error = dl_convert( dl_ctx, type, buffer, buffer_size, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 );

				free_func( (void*) buffer, alloc_ctx );
			}
			else
			{
				load_instance = buffer;
				load_size     = buffer_size;
				error = dl_convert_inplace( dl_ctx, type, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 );
			}

			if( error != DL_ERROR_OK ) { free_func( load_instance, alloc_ctx ); return error; }
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calc needed space
			size_t packed_size = 0;
			error = dl_txt_pack( dl_ctx, (char*)buffer, 0x0, 0, &packed_size );

			if(error != DL_ERROR_OK) { free_func( (void*) buffer, alloc_ctx ); return error; }

			load_instance = (unsigned char*)alloc_func( packed_size, alloc_ctx );

			error = dl_txt_pack(dl_ctx, (char*)buffer, load_instance, packed_size, 0x0);

			load_size = packed_size;

			free_func( (void*) buffer, alloc_ctx );

			if(error != DL_ERROR_OK) { free_func( load_instance, alloc_ctx ); return error; }

			if( type == 0 ) // autodetect type
			{
				error = dl_instance_get_info( load_instance, packed_size, &info );
				if(error != DL_ERROR_OK) { free_func( load_instance, alloc_ctx ); return error; }
				type = info.root_type;
			}
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	error = dl_instance_load_inplace( dl_ctx, type, load_instance, load_size, out_instance, 0x0 );
	*allocated_mem = load_instance;

	if( out_type != 0x0 )
		*out_type = type;

	return error;
}

dl_error_t dl_util_load_from_file( dl_ctx_t     dl_ctx,        dl_typeid_t         type,
								   const char*  filename,      dl_util_file_type_t filetype,
								   void**       out_instance,  dl_typeid_t*        out_type,
								   void**       allocated_mem, dl_alloc_func       alloc_func,
								   dl_free_func free_func,     void*               alloc_ctx )
{
	dl_error_t error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	FILE* in_file = fopen( filename, "rb" );

	if( in_file != 0x0 )
	{
		dl_realloc_func realloc_func = 0;
		dl_patch_alloc_funcs( alloc_func, realloc_func, free_func );

		fseek(in_file, 0, SEEK_END);
		size_t size = static_cast<size_t>(ftell(in_file));
		fseek(in_file, 0, SEEK_SET);
		uint8_t* buffer = (uint8_t*) alloc_func( size + 1, alloc_ctx );
		size_t read = fread(buffer, 1, size, in_file);
		fclose(in_file);
		if (read != size)
			return DL_ERROR_INTERNAL_ERROR;
		buffer[size] = '\0';
		error = dl_util_load_from_buffer( dl_ctx, type, buffer, size + 1, filetype, out_instance, out_type, allocated_mem, alloc_func, free_func, alloc_ctx );
	}

	return error;
}

dl_error_t dl_util_load_from_stream( dl_ctx_t      dl_ctx,        dl_typeid_t         type,
									 FILE*         stream,        dl_util_file_type_t filetype,
									 void**        out_instance,  dl_typeid_t*        out_type,
									 void**        allocated_mem, size_t*             consumed_bytes,
									 dl_alloc_func alloc_func,    dl_realloc_func     realloc_func,
									 dl_free_func  free_func,     void*               alloc_ctx )
{
	dl_patch_alloc_funcs( alloc_func, realloc_func, free_func );
	unsigned char* file_content = dl_read_entire_stream( realloc_func, alloc_ctx, stream, consumed_bytes );
	file_content[*consumed_bytes] = '\0';

	return dl_util_load_from_buffer( dl_ctx, type, file_content, *consumed_bytes, filetype, out_instance, out_type, allocated_mem, alloc_func, free_func, alloc_ctx );
}

dl_error_t dl_util_store_to_file( dl_ctx_t     dl_ctx,    dl_typeid_t         type,
                                  const char*  filename,  dl_util_file_type_t filetype,
                                  dl_endian_t  endian,    size_t              instance_size,
                                  const void*  instance,  dl_alloc_func       alloc_func,
								  dl_free_func free_func, void*               alloc_ctx )
{
	FILE* out_file = fopen( filename, filetype == DL_UTIL_FILE_TYPE_BINARY ? "wb" : "w" );

	dl_error_t error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	if( out_file != 0x0 )
	{
		error = dl_util_store_to_stream( dl_ctx, type, out_file, filetype, endian, instance_size, instance, alloc_func, free_func, alloc_ctx );
		fclose( out_file );
	}

	return error;
}

dl_error_t dl_util_store_to_stream( dl_ctx_t        dl_ctx,    dl_typeid_t         type,
									FILE*           stream,    dl_util_file_type_t filetype,
									dl_endian_t     endian,    size_t              instance_size,
									const void*     instance,  dl_alloc_func       alloc_func,
									dl_free_func    free_func, void*               alloc_ctx )
{
	if( filetype == DL_UTIL_FILE_TYPE_AUTO )
		return DL_ERROR_INVALID_PARAMETER;

	size_t packed_size = 0;

	// calculate pack-size
	dl_error_t error = dl_instance_store( dl_ctx, type, instance, 0x0, 0, &packed_size );

	if( error != DL_ERROR_OK)
		return error;
	
	dl_realloc_func realloc_func = 0;
	dl_patch_alloc_funcs( alloc_func, realloc_func, free_func );

	// alloc memory
	unsigned char* packed_instance = (unsigned char*)alloc_func( packed_size, alloc_ctx );

	// pack data
	error = dl_instance_store( dl_ctx, type, instance, packed_instance, packed_size, 0x0 );

	if( error != DL_ERROR_OK ) { free_func( packed_instance, alloc_ctx ); return error; }

	size_t         out_size = 0;
	unsigned char* out_data = 0x0;

	switch( filetype )
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			// calc convert size
			error = dl_convert( dl_ctx, type, packed_instance, packed_size, 0x0, 0, endian, instance_size, &out_size );

			if( error != DL_ERROR_OK ) { free_func( packed_instance, alloc_ctx ); return error; }

			// convert
			if( out_size > packed_size || instance_size > sizeof(void*) )
			{
				// new alloc
				out_data = (unsigned char*)alloc_func( out_size, alloc_ctx );

				// convert
				error = dl_convert( dl_ctx, type, packed_instance, packed_size, out_data, out_size, endian, instance_size, 0x0 );

				free_func( packed_instance, alloc_ctx );

				if( error != DL_ERROR_OK ) { free_func( out_data, alloc_ctx ); return error; }
			}
			else
			{
				out_data = packed_instance;
				error = dl_convert_inplace( dl_ctx, type, packed_instance, packed_size, endian, instance_size, 0x0 );

				if( error != DL_ERROR_OK ) { free_func( out_data, alloc_ctx ); return error; }
			}
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calculate pack-size
			error = dl_txt_unpack_calc_size( dl_ctx, type, packed_instance, packed_size, &out_size );
			if( error != DL_ERROR_OK ) { free_func( packed_instance, alloc_ctx ); return error; }

			// alloc data
			out_data = (unsigned char*)alloc_func( out_size, alloc_ctx );
			// pack data
			error = dl_txt_unpack( dl_ctx, type, packed_instance, packed_size, (char*)out_data, out_size, 0x0 );
			free_func( packed_instance, alloc_ctx );

			if( error != DL_ERROR_OK ) { free_func( out_data, alloc_ctx ); return error; }
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	fwrite( out_data, out_size, 1, stream );
	free_func( out_data, alloc_ctx );

	return error;
}
