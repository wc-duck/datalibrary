#include <dl/dl_typelib.h>
#include <dl/dl_txt.h>
#include "dl_binary_writer.h"
#include <dl/dl_reflect.h>

#include <stdlib.h>

static const char* dl_context_type_to_string( dl_ctx_t ctx, dl_type_storage_t storage, dl_typeid_t tid )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_STRUCT:
		case DL_TYPE_STORAGE_PTR:
		{
			dl_type_info_t sub_type;
			dl_reflect_get_type_info( ctx, tid, &sub_type );
			return sub_type.name;
		}
		case DL_TYPE_STORAGE_ENUM_UINT32:
		{
			dl_enum_info_t sub_type;
			dl_reflect_get_enum_info( ctx, tid, &sub_type );
			return sub_type.name;
		}
		case DL_TYPE_STORAGE_INT8:   return "int8";
		case DL_TYPE_STORAGE_UINT8:  return "uint8";
		case DL_TYPE_STORAGE_INT16:  return "int16";
		case DL_TYPE_STORAGE_UINT16: return "uint16";
		case DL_TYPE_STORAGE_INT32:  return "int32";
		case DL_TYPE_STORAGE_UINT32: return "uint32";
		case DL_TYPE_STORAGE_INT64:  return "int64";
		case DL_TYPE_STORAGE_UINT64: return "uint64";
		case DL_TYPE_STORAGE_FP32:   return "fp32";
		case DL_TYPE_STORAGE_FP64:   return "fp64";
		case DL_TYPE_STORAGE_STR:    return "string";
		default:
			DL_ASSERT(false);
			return 0x0;
	}
}

#if defined( __GNUC__ ) || defined( __clang__ )
#  ifndef _Printf_format_string_
#	 define _Printf_format_string_
#  endif
static void dl_binary_writer_write_fmt( dl_binary_writer* writer, const char* fmt, ... ) __attribute__( ( __format__( __printf__, 2, 3 ) ) );
#endif

static void dl_binary_writer_write_fmt( dl_binary_writer* writer, _Printf_format_string_ const char* fmt, ... )
{
	char buffer[2048];
	va_list args;
	va_start( args, fmt );
	int res = vsnprintf( buffer, sizeof(buffer), fmt, args );
	buffer[sizeof(buffer) - 1] = '\0';
	va_end( args );
	dl_binary_writer_write( writer, buffer, (size_t)res );
}

static void dl_context_write_txt_enum( dl_ctx_t ctx, dl_binary_writer* writer, dl_typeid_t tid )
{
	dl_enum_info_t enum_info;
	dl_reflect_get_enum_info( ctx, tid, &enum_info );

	dl_binary_writer_write_fmt( writer, "    \"%s\" : {\n"
	                                    "      \"values\" : {\n", enum_info.name );

	dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_info.value_count * sizeof( dl_enum_value_info_t ) );
	dl_reflect_get_enum_values( ctx, tid, values, enum_info.value_count );

	for( unsigned int j = 0; j < enum_info.value_count; ++j )
	{
		dl_binary_writer_write_fmt( writer, "        \"%s\" : %lu", values[j].name, values[j].value.u64 );
		if( j < enum_info.value_count - 1 )
			dl_binary_writer_write( writer, ",\n", 2 );
		else
			dl_binary_writer_write( writer, "\n", 1 );
	}

	free( values );

	dl_binary_writer_write_fmt( writer, "      }" );

	uint32_t metadata_count = 0;
	dl_get_metadata_count( ctx, tid, &metadata_count );
	if( metadata_count )
	{
		dl_binary_writer_write_fmt( writer, ",\n      \"metadata\" : [\n" );
		for( uint32_t i = 0; i < metadata_count; ++i )
		{
			size_t produced_bytes = 0;

			dl_typeid_t metadata_type_id;
			void* metadata_instance;
			dl_get_metadata( ctx, tid, i, &metadata_type_id, &metadata_instance );

			// jumping through hoops, pack the data and write it as json with wrong indent
			size_t packed_size;
			dl_instance_calc_size( ctx, metadata_type_id, metadata_instance, &packed_size );
			uint8_t* packed_instance = (uint8_t*)dl_alloc( &ctx->alloc, packed_size );
			dl_instance_store( ctx, metadata_type_id, metadata_instance, packed_instance, packed_size, &produced_bytes );

			size_t txt_instance_size;
			dl_txt_unpack_calc_size( ctx, metadata_type_id, packed_instance, packed_size, &txt_instance_size );
			char* text_instance = (char*)dl_alloc( &ctx->alloc, txt_instance_size );
			dl_txt_unpack( ctx, metadata_type_id, packed_instance, packed_size, text_instance, txt_instance_size, &produced_bytes );
			dl_free( &ctx->alloc, packed_instance );

			dl_binary_writer_write( writer, text_instance, txt_instance_size - 1 );
			dl_free( &ctx->alloc, text_instance );
			if( i < metadata_count - 1 )
				dl_binary_writer_write( writer, ",\n", 2 );
			else
				dl_binary_writer_write( writer, "\n", 1 );
		}
		dl_binary_writer_write_fmt( writer, "      ]" );
	}

	dl_binary_writer_write_fmt( writer, "\n    }" );
}

static void dl_context_write_txt_enums( dl_ctx_t ctx, dl_binary_writer* writer )
{
	dl_binary_writer_write_fmt( writer, "  \"enums\" : {\n" );
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_enums * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_enumids( ctx, tids, ctx_info.num_enums );

	for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
	{
		dl_context_write_txt_enum( ctx, writer, tids[enum_index] );
		if( enum_index < ctx_info.num_enums - 1 )
			dl_binary_writer_write( writer, ",\n", 2 );
		else
			dl_binary_writer_write( writer, "\n", 1 );
	}

	free( tids );

	dl_binary_writer_write_fmt( writer, "  },\n" );
}

static void dl_context_write_txt_member( dl_ctx_t ctx, dl_binary_writer* writer, dl_typeid_t tid, uint32_t member_index, const dl_member_info_t* member )
{
	dl_binary_writer_write_fmt( writer, "        { \"name\" : \"%s\", ", member->name );

	switch( member->atom )
	{
		case DL_TYPE_ATOM_POD:
			dl_binary_writer_write_fmt( writer,
										member->storage == DL_TYPE_STORAGE_PTR
											? "\"type\" : \"%s*\""
											: "\"type\" : \"%s\"",
										dl_context_type_to_string( ctx, member->storage, member->type_id ) );
		break;
		case DL_TYPE_ATOM_BITFIELD:
			dl_binary_writer_write_fmt( writer, "\"type\" : \"bitfield:%u\"", member->bits );
		break;
		case DL_TYPE_ATOM_ARRAY:
			dl_binary_writer_write_fmt( writer, "\"type\" : \"%s[]\"", dl_context_type_to_string( ctx, member->storage, member->type_id ) );
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
			dl_binary_writer_write_fmt( writer,
										member->storage == DL_TYPE_STORAGE_PTR
											? "\"type\" : \"%s*[%u]\""
											: "\"type\" : \"%s[%u]\"",
										dl_context_type_to_string( ctx, member->storage, member->type_id ),
										member->array_count );
		break;
	default:
		DL_ASSERT( false );
	}

	uint32_t metadata_count = 0;
	dl_get_member_metadata_count( ctx, tid, member_index, &metadata_count );
	if( metadata_count )
	{
		dl_binary_writer_write_fmt( writer, ",\n      \"metadata\" : [\n" );
		for( uint32_t i = 0; i < metadata_count; ++i )
		{
			size_t produced_bytes = 0;

			dl_typeid_t metadata_type_id;
			void* metadata_instance;
			dl_get_member_metadata( ctx, tid, member_index, i, &metadata_type_id, &metadata_instance );

			// jumping through hoops, pack the data and write it as json with wrong indent
			size_t packed_size;
			dl_instance_calc_size( ctx, metadata_type_id, metadata_instance, &packed_size );
			uint8_t* packed_instance = (uint8_t*)dl_alloc( &ctx->alloc, packed_size );
			dl_instance_store( ctx, metadata_type_id, metadata_instance, packed_instance, packed_size, &produced_bytes );

			size_t txt_instance_size;
			dl_txt_unpack_calc_size( ctx, metadata_type_id, packed_instance, packed_size, &txt_instance_size );
			char* text_instance = (char*)dl_alloc( &ctx->alloc, txt_instance_size );
			dl_txt_unpack( ctx, metadata_type_id, packed_instance, packed_size, text_instance, txt_instance_size, &produced_bytes );
			dl_free( &ctx->alloc, packed_instance );

			dl_binary_writer_write( writer, text_instance, txt_instance_size - 1 );
			dl_free( &ctx->alloc, text_instance );
			if( i < metadata_count - 1 )
				dl_binary_writer_write( writer, ",\n", 2 );
			else
				dl_binary_writer_write( writer, "\n", 1 );
		}
		dl_binary_writer_write_fmt( writer, "      ]" );
	}
	dl_binary_writer_write( writer, " }", 2 );
}

static void dl_context_write_txt_type( dl_ctx_t ctx, dl_binary_writer* writer, dl_typeid_t tid )
{
	dl_type_info_t type_info;
	dl_reflect_get_type_info( ctx, tid, &type_info );

	dl_binary_writer_write_fmt( writer, "    \"%s\" : {\n", type_info.name );

	dl_member_info_t* members = (dl_member_info_t*)malloc( type_info.member_count * sizeof( dl_member_info_t ) );
	dl_reflect_get_type_members( ctx, tid, members, type_info.member_count );

	dl_binary_writer_write_fmt( writer, "      \"members\" : [\n" );
	for( unsigned int member_index = 0; member_index < type_info.member_count; ++member_index )
	{
		dl_context_write_txt_member( ctx, writer, tid, member_index, &members[member_index] );
		if( member_index < type_info.member_count - 1 )
			dl_binary_writer_write( writer, ",\n", 2 );
		else
			dl_binary_writer_write( writer, "\n", 1 );
	}
	dl_binary_writer_write_fmt( writer, "      ]" );

	uint32_t metadata_count = 0;
	dl_get_metadata_count( ctx, tid, &metadata_count );
	if( metadata_count )
	{
		dl_binary_writer_write_fmt( writer, ",\n      \"metadata\" : [\n" );
		for( uint32_t i = 0; i < metadata_count; ++i )
		{
			size_t produced_bytes = 0;

			dl_typeid_t metadata_type_id;
			void* metadata_instance;
			dl_get_metadata( ctx, tid, i, &metadata_type_id, &metadata_instance );

			// jumping through hoops, pack the data and write it as json with wrong indent
			size_t packed_size;
			dl_instance_calc_size( ctx, metadata_type_id, metadata_instance, &packed_size );
			uint8_t* packed_instance = (uint8_t*)dl_alloc( &ctx->alloc, packed_size );
			dl_instance_store( ctx, metadata_type_id, metadata_instance, packed_instance, packed_size, &produced_bytes );

			size_t txt_instance_size;
			dl_txt_unpack_calc_size( ctx, metadata_type_id, packed_instance, packed_size, &txt_instance_size );
			char* text_instance = (char*)dl_alloc( &ctx->alloc, txt_instance_size );
			dl_txt_unpack( ctx, metadata_type_id, packed_instance, packed_size, text_instance, txt_instance_size, &produced_bytes );
			dl_free( &ctx->alloc, packed_instance );

			dl_binary_writer_write( writer, text_instance, txt_instance_size - 1 );
			dl_free( &ctx->alloc, text_instance );
			if( i < metadata_count - 1 )
				dl_binary_writer_write( writer, ",\n", 2 );
			else
				dl_binary_writer_write( writer, "\n", 1 );
		}
		dl_binary_writer_write_fmt( writer, "      ]" );
	}
	dl_binary_writer_write_fmt( writer, "\n    }" );
	free( members );
}

static void dl_context_write_txt_types( dl_ctx_t ctx, dl_binary_writer* writer )
{
	dl_binary_writer_write_fmt( writer, "  \"types\" : {\n" );

	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_types * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_typeids( ctx, tids, ctx_info.num_types );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_context_write_txt_type( ctx, writer, tids[type_index] );
		if( type_index < ctx_info.num_types - 1 )
			dl_binary_writer_write( writer, ",\n", 2 );
		else
			dl_binary_writer_write( writer, "\n", 1 );
	}

	free( tids );
	dl_binary_writer_write_fmt( writer, "  }\n" );
}

dl_error_t dl_context_write_txt_type_library( dl_ctx_t ctx, char* out_lib, size_t out_lib_size, size_t* produced_bytes )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer,
						   (uint8_t*)out_lib,
						   out_lib_size,
						   false,
						   DL_ENDIAN_HOST,
						   DL_ENDIAN_HOST,
						   DL_PTR_SIZE_HOST );

	dl_binary_writer_write( &writer, "{\n", 2 );
	dl_context_write_txt_enums( ctx, &writer );
	dl_context_write_txt_types( ctx, &writer );
	dl_binary_writer_write( &writer, "}\n\0", 3 );

	if( out_lib_size > 0 )
		// if( write_ctx.write_pos > write_ctx.buffer_size )
		if( writer.needed_size > out_lib_size )
			return DL_ERROR_BUFFER_TO_SMALL;
	if( produced_bytes )
		*produced_bytes = writer.needed_size;

	return DL_ERROR_OK;
}
