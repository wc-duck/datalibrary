#include <dl/dl.h>
#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>

#include "getopt/getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

static int verbose = 0;
static int unpack = 0;
static int show_info = 0;
static int c_header = 0;

static FILE* output = stdout;
static const char* output_path = 0x0;

std::vector<const char*> inputs;

#define VERBOSE_OUTPUT(fmt, ...) if( verbose ) { fprintf(stderr, fmt "\n", ##__VA_ARGS__); }

static const getopt_option_t option_list[] =
{
	{ "help",     'h', GETOPT_OPTION_TYPE_NO_ARG,   0x0,        'h', "displays this help-message", 0x0 },
	{ "output",   'o', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'o', "output to file", "file" },
	{ "unpack",   'u', GETOPT_OPTION_TYPE_FLAG_SET, &unpack,      1, "force dl_pack to treat input data as a packed instance that should be unpacked.", 0x0 },
	{ "info",     'i', GETOPT_OPTION_TYPE_FLAG_SET, &show_info,   1, "make dl_pack show info about a packed instance.", 0x0 },
	{ "verbose",  'v', GETOPT_OPTION_TYPE_FLAG_SET, &verbose,     1, "verbose output", 0x0 },
	{ "c-header", 'c', GETOPT_OPTION_TYPE_FLAG_SET, &c_header,    1, "", 0x0 },
	GETOPT_OPTIONS_END
};

static int parse_args( int argc, const char** argv )
{
	getopt_context_t go_ctx;
	getopt_create_context( &go_ctx, argc, argv, option_list );

	int opt;
	while( (opt = getopt_next( &go_ctx ) ) != -1 )
	{
		switch(opt)
		{
			case 0:
				/*ignore, flag was set*/
				break;

			case 'h':
			{
				char buffer[2048];
				printf("usage: dltlc.exe [options] [input]...\n\n");
				printf("%s", getopt_create_help_string( &go_ctx, buffer, sizeof(buffer) ) );
				return 0;
			}

			case 'o':
				if( output != stdout )
				{
					fprintf( stderr, "specified -o/--output twice!\n" );
					return 1;
				}

				output = fopen( go_ctx.current_opt_arg, "wb" );
				if( output == 0 )
				{
					fprintf( stderr, "failed to open output: \"%s\"\n", go_ctx.current_opt_arg );
					return 1;
				}

				output_path = go_ctx.current_opt_arg;

				break;

			case '!':
				fprintf( stderr, "incorrect usage of flag \"%s\"\n", go_ctx.current_opt_arg );
				return 1;

			case '?':
				fprintf( stderr, "unknown flag \"%s\"\n", go_ctx.current_opt_arg );
				return 1;

			case '+':
				inputs.push_back( go_ctx.current_opt_arg );
				break;
		}
	}

	return 2;
}

static void error_report_function( const char* msg, void* )
{
	fprintf( stderr, "%s\n", msg );
}

static unsigned char* read_entire_stream( FILE* file, size_t* out_size )
{
	const unsigned int CHUNK_SIZE = 1024;
	size_t         total_size = 0;
	size_t         chunk_size = 0;
	unsigned char* out_buffer = 0;

	do
	{
		out_buffer = (unsigned char*)realloc( out_buffer, CHUNK_SIZE + total_size );
		chunk_size = fread( out_buffer + total_size, 1, CHUNK_SIZE, file );
		total_size += chunk_size;
	}
	while( chunk_size >= CHUNK_SIZE );

	*out_size = total_size;
	return out_buffer;
}

static bool load_typelib( dl_ctx_t ctx, FILE* f )
{
	size_t size = 0;
	unsigned char* data = read_entire_stream( f, &size );

	if( data[2] == 'L' && data[3] == 'D' && data[0] == 'L' && data[1] == 'T' ) // TODO: endianness.
	{
		// ... binary lib ...
		dl_error_t err = dl_context_load_type_library( ctx, data, size );
		if( err != DL_ERROR_OK )
		{
			VERBOSE_OUTPUT( "failed to load typelib with error %s", dl_error_to_string( err ) );
			return false;
		}
	}
	else
	{
		// ... try text ...
		dl_error_t err = dl_context_load_txt_type_library( ctx, (const char*)data, size );
		if( err != DL_ERROR_OK )
		{
			VERBOSE_OUTPUT( "failed to load typelib with error %s", dl_error_to_string( err ) );
			return false;
		}
	}

	free( data );

	return true;
}

static void write_tl_as_text( dl_ctx_t ctx, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_txt_type_library( ctx, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query typelib-txt size with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	char* outdata = (char*)malloc( res_size );

	err = dl_context_write_txt_type_library( ctx, outdata, res_size, 0x0 );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to write typelib-txt size with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	fwrite( outdata, res_size, 1, out );
	free( outdata );
}

static void write_tl_as_c_header( dl_ctx_t ctx, const char* module_name, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_type_library_c_header( ctx, module_name, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query c-header size for typelib with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	char* outdata = (char*)malloc( res_size );

	err = dl_context_write_type_library_c_header( ctx, module_name, outdata, res_size, 0x0 );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to write c-header for typelib with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	fwrite( outdata, res_size, 1, out );
	free( outdata );
}

static void write_tl_as_binary( dl_ctx_t ctx, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_type_library( ctx, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query typelib size with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	unsigned char* outdata = (unsigned char*)malloc( res_size );

	err = dl_context_write_type_library( ctx, outdata, res_size, 0x0 );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to write typelib size with error \"%s\"\n", dl_error_to_string( err ) );
		return;
	}

	fwrite( outdata, res_size, 1, out );
	free( outdata );
}

static void show_tl_members( dl_ctx_t ctx, const char* member_fmt, dl_typeid_t tid, unsigned int member_count )
{
	dl_member_info_t* member_info = (dl_member_info_t*)malloc( member_count * sizeof( dl_member_info_t ) );
	dl_reflect_get_type_members( ctx, tid, member_info, member_count );
	for( unsigned int j = 0; j < member_count; ++j )
	{
//		const char* atom = "whoo";
//		switch( member_info->type & DL_TYPE_ATOM_MASK )
//		{
//			case DL_TYPE_ATOM_POD:          atom = ""; break;
//			case DL_TYPE_ATOM_ARRAY:        atom = "[]"; break;
//			case DL_TYPE_ATOM_INLINE_ARRAY: atom = "[x]"; break;
//			case DL_TYPE_ATOM_BITFIELD:     atom = " : bits"; break;
//		}

		// TODO: print name here!
		printf(member_fmt, member_info[j].name, member_info[j].size, member_info[j].alignment, member_info[j].offset);
	}
	free( member_info );
}

static void show_tl_info( dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	// show info here :)
	printf("module name: %s\n", "<fetch the real name :)>" );
	printf("type count:  %u\n", ctx_info.num_types );
	printf("enum count:  %u\n\n", ctx_info.num_enums );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_types * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_types( ctx, tids, ctx_info.num_types );

	size_t max_name_len = 0;
	for( unsigned int i = 0; i < ctx_info.num_types; ++i )
	{
		dl_type_info_t type_info;
		dl_reflect_get_type_info( ctx, tids[i], &type_info );

		size_t len = strlen( type_info.name );
		max_name_len = len > max_name_len ? len : max_name_len;
	}

	char header_fmt[256];
	char item_fmt[256];
	char member_fmt[256];
	snprintf( header_fmt, 256, "%-10s  %%-%lus %5s %s %s\n", "typeid", (long unsigned int)max_name_len, "size", "align", "offset" );
	snprintf( item_fmt,   256, "0x%%08X  %%-%lus %%5u %%5u\n", (long unsigned int)max_name_len );
	snprintf( member_fmt, 256, "   - %%-%lus        %%5u %%5u %%5u\n", (long unsigned int)max_name_len );

	printf("types:\n");
	int header_len = printf( header_fmt, "name");
	for( int i = 0; i < header_len - 1; ++i )
		printf( "-" );
	printf( "\n" );

	for( unsigned int i = 0; i < ctx_info.num_types; ++i )
	{
		dl_type_info_t type_info;
		dl_reflect_get_type_info( ctx, tids[i], &type_info );

		// TODO: not only output data for host-platform =/
		printf( item_fmt, tids[i], type_info.name, type_info.size, type_info.alignment );
		show_tl_members( ctx, member_fmt, tids[i], type_info.member_count );
		printf("\n");
	}

	free( tids );


	printf("\nenums:\n");
	for( int i = 0; i < header_len - 1; ++i )
		printf( "-" );
	printf( "\n" );

	tids = (dl_typeid_t*)malloc( ctx_info.num_enums * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_enums( ctx, tids, ctx_info.num_enums );

	max_name_len = 0;
	for( unsigned int i = 0; i < ctx_info.num_enums; ++i )
	{
		dl_enum_info_t enum_info;
		dl_reflect_get_enum_info( ctx, tids[i], &enum_info );

		printf("%s\n", enum_info.name);

		dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_info.value_count * sizeof( dl_enum_value_info_t ) );
		dl_reflect_get_enum_values( ctx, tids[i], values, enum_info.value_count );

		for( unsigned int j = 0; j < enum_info.value_count; ++j )
			printf("    %s = %u\n", values[j].name, values[j].value);

		free( values );
	}

	free( tids );
}

int main( int argc, const char** argv )
{
	int ret = parse_args( argc, argv );
	if( ret < 2 )
		return ret;

	dl_ctx_t ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = error_report_function;

	dl_context_create( &ctx, &p );

	if( inputs.size() == 0 )
	{
		VERBOSE_OUTPUT( "loading typelib from stdin" );

		if( !load_typelib( ctx, stdin ) )
		{
			return 1;
		}
	}
	else
	{
		for( size_t i = 0; i < inputs.size(); ++i )
		{
			FILE* f = fopen( inputs[i], "rb" );
			if( f == 0x0 )
			{
				fprintf( stderr, "failed to open \"%s\"\n", inputs[i] );
				return 1;
			}

			if( !load_typelib( ctx, f ) )
			{
				fprintf( stderr, "failed to load typelib from \"%s\"\n", inputs[i] );
				fclose( f );
				return 1;
			}

			fclose( f );
		}
	}

	if( show_info )
		show_tl_info( ctx );
	else if( unpack )
		write_tl_as_text( ctx, output );
	else if( c_header )
	{
		if( output == stdout )
			write_tl_as_c_header( ctx, "STDOUT", output );
		else
		{
			const char* module_name = output_path;
			const char* iter = output_path;
			while( *iter )
			{
				if( *iter == '/' || *iter == '\\' )
					module_name = iter + 1;
				++iter;
			}
			write_tl_as_c_header( ctx, module_name, output );
		}
	}
	else
		write_tl_as_binary( ctx, output );

	dl_context_destroy( ctx );

	return 0;
}
