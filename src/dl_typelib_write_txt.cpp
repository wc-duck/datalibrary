#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>
#include "dl_types.h"

#include <stdlib.h>

#include <yajl/yajl_gen.h>

static const char* dl_context_type_to_string( dl_ctx_t ctx, dl_type_t storage, dl_typeid_t tid )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_STRUCT:
		{
			dl_type_info_t sub_type;
			dl_reflect_get_type_info( ctx, tid, &sub_type );
			return sub_type.name;
		}
		case DL_TYPE_STORAGE_ENUM:
		{
			dl_enum_info_t sub_type;
			dl_reflect_get_enum_info( ctx, tid, &sub_type );
			return sub_type.name;
		}
		case DL_TYPE_STORAGE_INT8:   return "int8_t";
		case DL_TYPE_STORAGE_UINT8:  return "uint8_t";
		case DL_TYPE_STORAGE_INT16:  return "int16_t";
		case DL_TYPE_STORAGE_UINT16: return "uint16_t";
		case DL_TYPE_STORAGE_INT32:  return "int32_t";
		case DL_TYPE_STORAGE_UINT32: return "uint32_t";
		case DL_TYPE_STORAGE_INT64:  return "int64_t";
		case DL_TYPE_STORAGE_UINT64: return "uint64_t";
		case DL_TYPE_STORAGE_FP32:   return "float";
		case DL_TYPE_STORAGE_FP64:   return "double";
		case DL_TYPE_STORAGE_STR:    return "const char*";
		default:
			return 0x0;
	}
}

struct dl_write_text_context
{
	char*  buffer;
	size_t buffer_size;
	size_t write_pos;
};

#if defined( __GNUC__ )
static void dl_gen_string( yajl_gen gen, const char* fmt, ... ) __attribute__((format( printf, 2, 3 )));
#endif

static void dl_gen_string( yajl_gen gen, const char* fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	char buffer[2048];
	int res = vsnprintf( buffer, DL_ARRAY_LENGTH( buffer ), fmt, args );
	buffer[ DL_ARRAY_LENGTH( buffer ) - 1 ] = '\0';
	va_end( args );
	yajl_gen_string( gen, (const unsigned char*)buffer, res );
}

static void dl_internal_write_text_callback( void* ctx, const char* str, size_t len )
{
	dl_write_text_context* write_ctx = (dl_write_text_context*)ctx;

	if( write_ctx->write_pos <= write_ctx->buffer_size )
	{
		size_t left = write_ctx->buffer_size - write_ctx->write_pos;
		if( left < len )
			memcpy( write_ctx->buffer + write_ctx->write_pos, str, left );
		else
			memcpy( write_ctx->buffer + write_ctx->write_pos, str, len );
	}

	write_ctx->write_pos += len;
}

static const size_t UNPACK_STORAGE_SIZE = 1024;

static void* dl_internal_unpack_malloc( void* ctx, size_t size )
{
	(void)size;
	DL_ASSERT( size <= UNPACK_STORAGE_SIZE );
	return (void*) ctx;
}
static void dl_internal_unpack_free( void* ctx, void* ptr )
{
	(void)ctx; (void)ptr;
	// just ignore free, the data should be allocated on the stack!
}

static void* dl_internal_unpack_realloc_func( void*, void*, size_t )
{
	DL_ASSERT(false && "yajl should never need to realloc");
	return 0x0;
}

static void dl_context_write_txt_typelib_begin( dl_ctx_t ctx, yajl_gen gen )
{
	(void)ctx;
	yajl_gen_map_open( gen );
}

static void dl_context_write_txt_typelib_end( dl_ctx_t ctx, yajl_gen gen )
{
	(void)ctx;
	yajl_gen_map_close( gen );
}

static void dl_context_write_txt_enum( dl_ctx_t ctx, yajl_gen gen, dl_typeid_t tid )
{
	dl_enum_info_t enum_info;
	dl_reflect_get_enum_info( ctx, tid, &enum_info );

	dl_gen_string( gen, "%s", enum_info.name );
	yajl_gen_array_open( gen );

	dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_info.value_count * sizeof( dl_enum_value_info_t ) );
	dl_reflect_get_enum_values( ctx, tid, values, enum_info.value_count );

	int64_t curr_val = -1;

	for( unsigned int j = 0; j < enum_info.value_count; ++j )
	{
		if( curr_val + 1 == values[j].value )
		{
			dl_gen_string( gen, "%s", values[j].name );
		}
		else if( false /* TODO: check if we have one header-name and one value name? */ )
		{

		}
		else
		{
			yajl_gen_map_open( gen );
			dl_gen_string( gen, "%s", values[j].name );
			yajl_gen_integer( gen, values[j].value );
			yajl_gen_map_close( gen );
		}

		curr_val = values[j].value;
	}

	free( values );

	yajl_gen_array_close( gen );
}

static void dl_context_write_txt_enums( dl_ctx_t ctx, yajl_gen gen )
{
	dl_gen_string( gen, "enums" );
	yajl_gen_map_open( gen );

	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_enums * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_enums( ctx, tids, ctx_info.num_enums );

	for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
		dl_context_write_txt_enum( ctx, gen, tids[enum_index] );

	free( tids );

	yajl_gen_map_close( gen );
}

static void dl_context_write_txt_member( dl_ctx_t ctx, yajl_gen gen, dl_member_info_t* member )
{
	yajl_gen_map_open( gen );

	dl_gen_string( gen, "name" );
	dl_gen_string( gen, "%s", member->name );

	dl_type_t atom    = (dl_type_t)(DL_TYPE_ATOM_MASK & member->type);
	dl_type_t storage = (dl_type_t)(DL_TYPE_STORAGE_MASK & member->type);

	dl_gen_string( gen, "type" );
	switch( atom )
	{
		case DL_TYPE_ATOM_POD:
			if( storage == DL_TYPE_STORAGE_PTR )
			{
				dl_type_info_t sub_type;
				dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
				dl_gen_string( gen, "%s*", sub_type.name );
			}
			else
				dl_gen_string( gen, "%s", dl_context_type_to_string( ctx, storage, member->type_id ) );
		break;
		case DL_TYPE_ATOM_BITFIELD:
			dl_gen_string( gen, "bitfield" );
			dl_gen_string( gen, "bits" );
			yajl_gen_integer( gen, member->bits );
		break;
		case DL_TYPE_ATOM_ARRAY:
			dl_gen_string( gen, "%s[]", dl_context_type_to_string( ctx, storage, member->type_id ) );
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
			dl_gen_string( gen, "%s[%u]", dl_context_type_to_string( ctx, storage, member->type_id ), member->array_count );
		break;
		default:
			DL_ASSERT( false );
	}

	yajl_gen_map_close( gen );
}

static void dl_context_write_txt_type( dl_ctx_t ctx, yajl_gen gen, dl_typeid_t tid )
{
	dl_type_info_t type_info;
	dl_reflect_get_type_info( ctx, tid, &type_info );

	dl_gen_string( gen, "%s", type_info.name );
	yajl_gen_map_open( gen );

	dl_member_info_t* members = (dl_member_info_t*)malloc( type_info.member_count * sizeof( dl_member_info_t ) );
	dl_reflect_get_type_members( ctx, tid, members, type_info.member_count );

	dl_gen_string( gen, "members" );
	yajl_gen_array_open( gen );
	for( unsigned int member_index = 0; member_index < type_info.member_count; ++member_index )
		dl_context_write_txt_member( ctx, gen, &members[member_index] );
	yajl_gen_array_close( gen );

	free( members );

	yajl_gen_map_close( gen );
}

static void dl_context_write_txt_types( dl_ctx_t ctx, yajl_gen gen )
{
	dl_gen_string( gen, "types" );
	yajl_gen_map_open( gen );

	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_types * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_types( ctx, tids, ctx_info.num_types );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
		dl_context_write_txt_type( ctx, gen, tids[type_index] );

	free( tids );

	yajl_gen_map_close( gen );
}

dl_error_t dl_context_write_txt_type_library( dl_ctx_t dl_ctx, char* out_lib, size_t out_lib_size, size_t* produced_bytes )
{
	dl_write_text_context write_ctx = { out_lib, out_lib_size, 0 };
	uint8_t storage[ 1024 ];
	yajl_alloc_funcs alloc_catch = { dl_internal_unpack_malloc, dl_internal_unpack_realloc_func, dl_internal_unpack_free, storage };
	yajl_gen generator = yajl_gen_alloc( &alloc_catch );
	yajl_gen_config( generator, yajl_gen_beautify, 1 );
	yajl_gen_config( generator, yajl_gen_indent_string, " " );
	yajl_gen_config( generator, yajl_gen_print_callback, dl_internal_write_text_callback, &write_ctx );

	dl_context_write_txt_typelib_begin( dl_ctx, generator );

	dl_context_write_txt_enums( dl_ctx, generator );

	dl_context_write_txt_types( dl_ctx, generator );

	dl_context_write_txt_typelib_end( dl_ctx, generator );

	yajl_gen_free( generator );

	if( produced_bytes != 0x0 )
		*produced_bytes = write_ctx.write_pos;

	if( out_lib_size > 0 )
		if( write_ctx.write_pos > write_ctx.buffer_size )
			return DL_ERROR_BUFFER_TO_SMALL;

	return DL_ERROR_OK;
}
