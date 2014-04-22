#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>

#include "dl_binary_writer.h"

#include <stdlib.h>
#include <ctype.h>

static void dl_binary_writer_write_string_fmt( dl_binary_writer* writer, const char* fmt, ... )
{
	char buffer[2048];
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	size_t written = (size_t)vsnprintf( buffer, DL_ARRAY_LENGTH( buffer ), fmt, arg_ptr );
	va_end(arg_ptr);

	dl_binary_writer_write( writer, buffer, written );
}

static void dl_context_write_c_header_begin( dl_binary_writer* writer, const char* module_name_uppercase )
{
	dl_binary_writer_write_string_fmt( writer, "/* Auto generated header for dl type library */\n" );
	dl_binary_writer_write_string_fmt( writer, "#ifndef %s_INCLUDED\n", module_name_uppercase );
	dl_binary_writer_write_string_fmt( writer, "#define %s_INCLUDED\n\n", module_name_uppercase );

	dl_binary_writer_write_string_fmt( writer, "#include <stdint.h>\n\n", module_name_uppercase );
}

static void dl_context_write_c_header_end( dl_binary_writer* writer, const char* module_name_uppercase )
{
	dl_binary_writer_write_string_fmt( writer, "#endif // %s_INCLUDED\n\n", module_name_uppercase );
}

static void dl_context_write_c_header_enums( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_enums * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_enums( ctx, tids, ctx_info.num_enums );

	for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
	{
		dl_enum_info_t enum_info;
		dl_reflect_get_enum_info( ctx, tids[enum_index], &enum_info );

		dl_binary_writer_write_string_fmt( writer, "enum %s\n{\n", enum_info.name );

		dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_info.value_count * sizeof( dl_enum_value_info_t ) );
		dl_reflect_get_enum_values( ctx, tids[enum_index], values, enum_info.value_count );

		dl_binary_writer_write_string_fmt( writer, "    %s = %u", values[0].name, values[0].value);
		for( unsigned int j = 1; j < enum_info.value_count; ++j )
			dl_binary_writer_write_string_fmt( writer, ",\n    %s = %u", values[j].name, values[j].value);

		free( values );
		dl_binary_writer_write_string_fmt( writer, "\n};\n\n" );
	}

	free( tids );
}

static const char* dl_context_builtin_to_string( dl_type_t storage )
{
	switch( storage )
	{
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

static unsigned int dl_context_builtin_size( dl_type_t storage )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT8:   return 1;
		case DL_TYPE_STORAGE_UINT8:  return 1;
		case DL_TYPE_STORAGE_INT16:  return 2;
		case DL_TYPE_STORAGE_UINT16: return 2;
		case DL_TYPE_STORAGE_INT32:  return 4;
		case DL_TYPE_STORAGE_UINT32: return 4;
		case DL_TYPE_STORAGE_INT64:  return 8;
		case DL_TYPE_STORAGE_UINT64: return 8;
		case DL_TYPE_STORAGE_FP32:   return 4;
		case DL_TYPE_STORAGE_FP64:   return 8;
		case DL_TYPE_STORAGE_STR:    return DL_PTR_SIZE_HOST == DL_PTR_SIZE_32BIT ? 4 : 8;
		default:
			return 0x0;
	}
}

static void dl_context_write_c_header_member( dl_binary_writer* writer, dl_ctx_t ctx, dl_member_info_t* member )
{
	dl_type_t atom    = (dl_type_t)(DL_TYPE_ATOM_MASK & member->type);
	dl_type_t storage = (dl_type_t)(DL_TYPE_STORAGE_MASK & member->type);

	switch( atom )
	{
		case DL_TYPE_ATOM_POD:
			switch( storage )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    %s %s;\n", sub_type.name, member->name );
				}
				break;
				case DL_TYPE_STORAGE_PTR:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    const struct %s* %s;\n", sub_type.name, member->name );
				}
				break;
				case DL_TYPE_STORAGE_ENUM:
				{
					dl_enum_info_t sub_type;
					dl_reflect_get_enum_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    %s %s;\n", sub_type.name, member->name );
				}
				break;
				default:
					dl_binary_writer_write_string_fmt( writer, "    %s %s;\n", dl_context_builtin_to_string( storage ), member->name );
					break;
			}

			break;
		case DL_TYPE_ATOM_BITFIELD:
		{
			uint32_t bits = DL_EXTRACT_BITS( member->type, DL_TYPE_BITFIELD_SIZE_MIN_BIT, DL_TYPE_BITFIELD_SIZE_BITS_USED );
			dl_binary_writer_write_string_fmt( writer, "    %s %s : %u;\n", dl_context_builtin_to_string( storage ), member->name, bits );
		}
		break;
		case DL_TYPE_ATOM_ARRAY:
		{
			const char* type_str = "<unknown>";

			switch( storage )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					type_str = sub_type.name;
				}
				break;
				case DL_TYPE_STORAGE_ENUM:
				{
					dl_enum_info_t sub_type;
					dl_reflect_get_enum_info( ctx, member->type_id, &sub_type );
					type_str = sub_type.name;
				}
				break;
				default:
					type_str = dl_context_builtin_to_string( storage );
				break;
			}

			dl_binary_writer_write_string_fmt( writer, "    struct\n    {\n" );
			dl_binary_writer_write_string_fmt( writer, "        %s* data;\n", type_str );
			dl_binary_writer_write_string_fmt( writer, "        uint32_t count;\n" );
			dl_binary_writer_write_string_fmt( writer, "    #if defined( __cplusplus )\n" );
			dl_binary_writer_write_string_fmt( writer, "              %s& operator[] ( size_t index )       { return data[index]; }\n", type_str );
			if( storage == DL_TYPE_STORAGE_STR )
				dl_binary_writer_write_string_fmt( writer, "        %s& operator[] ( size_t index ) const { return data[index]; }\n", type_str );
			else
				dl_binary_writer_write_string_fmt( writer, "        const %s& operator[] ( size_t index ) const { return data[index]; }\n", type_str );
			dl_binary_writer_write_string_fmt( writer, "    #endif // defined( __cplusplus )\n" );
			dl_binary_writer_write_string_fmt( writer, "    } %s;\n", member->name );
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch( storage )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    %s %s[%u];\n", sub_type.name, member->name, member->size / sub_type.size );
				}
				break;
				case DL_TYPE_STORAGE_ENUM:
				{
					dl_enum_info_t sub_type;
					dl_reflect_get_enum_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    %s %s[%u];\n", sub_type.name, member->name, member->size / 4 );
				}
				break;
				default:
					dl_binary_writer_write_string_fmt( writer, "    %s %s[%u];\n", dl_context_builtin_to_string( storage ), member->name, member->size / dl_context_builtin_size( storage ) );
					break;
			}
		}
		break;
		default:
			DL_ASSERT( false );
	}
}

static void dl_context_write_c_header_types( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_types * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_types( ctx, tids, ctx_info.num_types );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_type_info_t type_info;
		dl_reflect_get_type_info( ctx, tids[type_index], &type_info );

		dl_binary_writer_write_string_fmt( writer, "#define %s_TYPE_ID (0x%08X)\n", type_info.name, tids[type_index] );
	}

	dl_binary_writer_write_string_fmt( writer, "\n" );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_type_info_t type_info;
		dl_reflect_get_type_info( ctx, tids[type_index], &type_info );

		dl_member_info_t* members = (dl_member_info_t*)malloc( type_info.member_count * sizeof( dl_member_info_t ) );
		dl_reflect_get_type_members( ctx, tids[type_index], members, type_info.member_count );

		// ... the struct need manual align if the struct has higher align than any of its members ...
		unsigned int max_align = 0;
		for( unsigned int member_index = 0; member_index < type_info.member_count; ++member_index )
			// TODO: this might fail if the compiler is run as 32 bit, expose manual align from reflect.
			max_align = members[member_index].alignment > max_align ? members[member_index].alignment : max_align;

		if( max_align < type_info.alignment )
			dl_binary_writer_write_string_fmt( writer, "struct DL_ALIGN( %u ) %s\n{\n", type_info.alignment, type_info.name );
		else
			dl_binary_writer_write_string_fmt( writer, "struct %s\n{\n", type_info.name );

		dl_binary_writer_write_string_fmt( writer, "#if defined( __cplusplus )\n" );
		dl_binary_writer_write_string_fmt( writer, "    static const uint32_t TYPE_ID = 0x%08X;\n", tids[type_index] );
		dl_binary_writer_write_string_fmt( writer, "#endif // defined( __cplusplus )\n\n" );

		for( unsigned int member_index = 0; member_index < type_info.member_count; ++member_index )
			dl_context_write_c_header_member( writer, ctx, members + member_index );

		free( members );

		dl_binary_writer_write_string_fmt( writer, "};\n\n", type_info.name );
	}

	free( tids );
}

dl_error_t dl_context_write_type_library_c_header( dl_ctx_t dl_ctx, const char* module_name, char* out_header, size_t out_header_size, size_t* produced_bytes )
{
	char MODULE_NAME[128];
	size_t pos = 0;
	const char* iter = module_name;
	while( *iter && pos < 127 )
	{
		if( isalnum( *iter ) )
			MODULE_NAME[pos++] = (char)toupper( *iter );
		else
			MODULE_NAME[pos++] = '_';
		++iter;
	}
	MODULE_NAME[pos] = 0;

	dl_binary_writer writer;
	dl_binary_writer_init( &writer, (uint8_t*)out_header, out_header_size, out_header == 0x0, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );

	dl_context_write_c_header_begin( &writer, MODULE_NAME );

	dl_context_write_c_header_enums( &writer, dl_ctx );

	dl_context_write_c_header_types( &writer, dl_ctx );

	dl_context_write_c_header_end( &writer, MODULE_NAME );

	if( produced_bytes )
		*produced_bytes = dl_binary_writer_needed_size( &writer );

	return DL_ERROR_OK;
}