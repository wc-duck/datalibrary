#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>

#include "dl_binary_writer.h"

#include <stdlib.h>
#include <ctype.h>

#if defined( __GNUC__ )
static void dl_binary_writer_write_string_fmt( dl_binary_writer* writer, const char* fmt, ... ) __attribute__((format( printf, 2, 3 )));
#endif

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
	dl_binary_writer_write_string_fmt( writer, "#ifndef __DL_AUTOGEN_HEADER_%s_INCLUDED\n", module_name_uppercase );
	dl_binary_writer_write_string_fmt( writer, "#define __DL_AUTOGEN_HEADER_%s_INCLUDED\n\n", module_name_uppercase );

	dl_binary_writer_write_string_fmt( writer, "#include <stdint.h>\n\n" );
	dl_binary_writer_write_string_fmt( writer, "#include <stddef.h> // for size_t\n\n" );

	dl_binary_writer_write_string_fmt( writer, "#ifndef __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n" );
	dl_binary_writer_write_string_fmt( writer, "#define __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n" );
	dl_binary_writer_write_string_fmt( writer, "#  if defined(_MSC_VER)\n" );
	dl_binary_writer_write_string_fmt( writer, "#    define DL_ALIGN(x) __declspec(align(x))\n" );
	dl_binary_writer_write_string_fmt( writer, "#  elif defined(__GNUC__)\n" );
	dl_binary_writer_write_string_fmt( writer, "#    define DL_ALIGN(x) __attribute__((aligned(x)))\n" );
	dl_binary_writer_write_string_fmt( writer, "#  else\n" );
	dl_binary_writer_write_string_fmt( writer, "#    error \"Unsupported compiler\"\n" );
	dl_binary_writer_write_string_fmt( writer, "#  endif\n" );
	dl_binary_writer_write_string_fmt( writer, "#endif // __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n\n" );
}

static void dl_context_write_c_header_end( dl_binary_writer* writer, const char* module_name_uppercase )
{
	dl_binary_writer_write_string_fmt( writer, "#endif // __DL_AUTOGEN_HEADER_%s_INCLUDED\n\n", module_name_uppercase );
}

static void dl_context_write_c_header_enums( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_typeid_t* tids = (dl_typeid_t*)malloc( ctx_info.num_enums * sizeof( dl_typeid_t ) );
	dl_reflect_loaded_enumids( ctx, tids, ctx_info.num_enums );

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

static void dl_context_write_type( dl_ctx_t ctx, dl_type_t storage, dl_typeid_t tid, dl_binary_writer* writer )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_STRUCT:
		{
			dl_type_info_t sub_type;
			dl_reflect_get_type_info( ctx, tid, &sub_type );
			dl_binary_writer_write_string_fmt(writer, "struct %s", sub_type.name);
			return;
		}
		case DL_TYPE_STORAGE_ENUM:
		{
			dl_enum_info_t sub_type;
			dl_reflect_get_enum_info( ctx, tid, &sub_type );
			dl_binary_writer_write_string_fmt(writer, "enum %s", sub_type.name);
			return;
		}
		case DL_TYPE_STORAGE_INT8:   dl_binary_writer_write_string_fmt(writer, "int8_t"); return;
		case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_string_fmt(writer, "uint8_t"); return;
		case DL_TYPE_STORAGE_INT16:  dl_binary_writer_write_string_fmt(writer, "int16_t"); return;
		case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_string_fmt(writer, "uint16_t"); return;
		case DL_TYPE_STORAGE_INT32:  dl_binary_writer_write_string_fmt(writer, "int32_t"); return;
		case DL_TYPE_STORAGE_UINT32: dl_binary_writer_write_string_fmt(writer, "uint32_t"); return;
		case DL_TYPE_STORAGE_INT64:  dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) int64_t"); return;
		case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) uint64_t"); return;
		case DL_TYPE_STORAGE_FP32:   dl_binary_writer_write_string_fmt(writer, "float"); return;
		case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) double"); return;
		case DL_TYPE_STORAGE_STR:    dl_binary_writer_write_string_fmt(writer, "const char*"); return;
		default:
			dl_binary_writer_write_string_fmt(writer, "UNKNOWN");
			return;
	}
}

static void dl_context_write_operator_array_access_type( dl_ctx_t ctx, dl_type_t storage, dl_typeid_t tid, dl_binary_writer* writer )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT64:  dl_binary_writer_write_string_fmt(writer, "int64_t"); return;
		case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_string_fmt(writer, "uint64_t"); return;
		case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_string_fmt(writer, "double"); return;
		default:
			dl_context_write_type( ctx, storage, tid, writer );
			return;
	}
}

static void dl_context_write_c_header_member( dl_binary_writer* writer, dl_ctx_t ctx, dl_member_info_t* member, bool* last_was_bf )
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
					dl_binary_writer_write_string_fmt( writer, "    struct %s %s;\n", sub_type.name, member->name );
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
					dl_binary_writer_write_string_fmt( writer, "    enum %s %s;\n", sub_type.name, member->name );
				}
				break;
				default:
					dl_binary_writer_write_string_fmt(writer, "    ");
					dl_context_write_type(ctx, storage, member->type_id, writer);
					dl_binary_writer_write_string_fmt(writer, " %s; \n", member->name );
					break;
			}

			break;
		case DL_TYPE_ATOM_BITFIELD:
			if( *last_was_bf && storage == DL_TYPE_STORAGE_UINT64 )
				dl_binary_writer_write_string_fmt( writer, "    uint64_t %s : %u;\n", member->name, member->bits );
			else
			{
				dl_binary_writer_write_string_fmt(writer, "    ");
				dl_context_write_type(ctx, storage, member->type_id, writer);
				dl_binary_writer_write_string_fmt(writer, " %s : %u;\n", member->name, member->bits);
			}
		break;
		case DL_TYPE_ATOM_ARRAY:
		{
			dl_binary_writer_write_string_fmt( writer, "    struct\n    {\n"
													   "        " );
			dl_context_write_type(ctx, storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt( writer, "* data;\n"
													   "        uint32_t count;\n"
													   "    #if defined( __cplusplus )\n"
													   "              ");
			dl_context_write_operator_array_access_type(ctx, storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) { return data[index]; }\n");
			if (storage == DL_TYPE_STORAGE_STR)
			{
				dl_binary_writer_write_string_fmt(writer, "        ");
				dl_context_write_operator_array_access_type(ctx, storage, member->type_id, writer);
				dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) const { return data[index]; }\n");
			}
			else
			{
				dl_binary_writer_write_string_fmt(writer, "        const ");
				dl_context_write_operator_array_access_type(ctx, storage, member->type_id, writer);
				dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) const { return data[index]; }\n");
			}
			dl_binary_writer_write_string_fmt( writer, "    #endif // defined( __cplusplus )\n"
													   "    } %s;\n", member->name );
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			dl_binary_writer_write_string_fmt(writer, "    ");
			dl_context_write_type(ctx, storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt( writer, " %s[%u];\n", member->name, member->array_count );
		}
		break;
		default:
			DL_ASSERT( false );
	}

	*last_was_bf = atom == DL_TYPE_ATOM_BITFIELD;
}

static void dl_context_write_c_header_types( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	dl_type_info_t* type_info = (dl_type_info_t*)malloc( ctx_info.num_types * sizeof( dl_type_info_t ) );
	dl_reflect_loaded_types( ctx, type_info, ctx_info.num_types );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
		dl_binary_writer_write_string_fmt( writer, "#define %s_TYPE_ID (0x%08X)\n", type_info[type_index].name, type_info[type_index].tid );

	dl_binary_writer_write_string_fmt( writer, "\n" );

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_type_info_t* type = &type_info[type_index];

		// if the type is "extern" no header struct should be generated for it.
		// TODO: generate some checks that the extern struct matches the one defined in dl by generating
		//       static asserts on size/alignment and member offset?
		if( type->is_extern )
			continue;

		dl_member_info_t* members = (dl_member_info_t*)malloc( type->member_count * sizeof( dl_member_info_t ) );
		dl_reflect_get_type_members( ctx, type->tid, members, type->member_count );

		if( type->is_union )
		{
			dl_binary_writer_write_string_fmt( writer, "enum %s_type\n"
													   "{\n", type->name );

			// ... generate an enum for union-members ...
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
			{
				dl_binary_writer_write_string_fmt( writer, "    %s_type_%s = 0x%08X", type->name, members[member_index].name, dl_internal_hash_string(members[member_index].name) );

				if( member_index < type->member_count - 1 )
					dl_binary_writer_write_string_fmt( writer, ",\n" );
				else
					dl_binary_writer_write_string_fmt( writer, "\n" );
			}

			dl_binary_writer_write_string_fmt( writer, "};\n\n" );
		}

		// ... the struct need manual align if the struct has higher align than any of its members ...
		unsigned int max_align = 0;
		for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
			// TODO: this might fail if the compiler is run as 32 bit, expose manual align from reflect.
			max_align = members[member_index].alignment > max_align ? members[member_index].alignment : max_align;

		if( max_align < type->alignment )
			dl_binary_writer_write_string_fmt( writer, "struct DL_ALIGN( %u ) %s\n{\n", type->alignment, type->name );
		else
			dl_binary_writer_write_string_fmt( writer, "struct %s\n{\n", type->name );

		dl_binary_writer_write_string_fmt( writer, "#if defined( __cplusplus )\n"
												   "    static const uint32_t TYPE_ID = 0x%08X;\n"
												   "#endif // defined( __cplusplus )\n\n", type->tid );

		if( type->is_union )
		{
			dl_binary_writer_write_string_fmt( writer, "    union\n"
													   "    {\n" );

			// TODO: better indent here!
			bool last_was_bf = false;
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
				dl_context_write_c_header_member( writer, ctx, members + member_index, &last_was_bf );
			dl_binary_writer_write_string_fmt( writer, "    } value;\n" );

			dl_binary_writer_write_string_fmt( writer, "    enum %s_type type;\n", type->name );
		}
		else
		{
			bool last_was_bf = false;
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
				dl_context_write_c_header_member( writer, ctx, members + member_index, &last_was_bf );
		}

		free( members );
		dl_binary_writer_write_string_fmt( writer, "};\n\n" );
	}

	free( type_info );
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
