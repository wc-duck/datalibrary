#include <dl/dl_typelib.h>
#include <dl/dl_txt.h>

#include <yajl/yajl_parse.h>

#include "dl_types.h"

#include <stdio.h> // remove
#include <stdlib.h> // atol
#include <ctype.h>

#define DL_PACK_ERROR_AND_FAIL( pack_ctx, err, fmt, ... ) { dl_log_error( pack_ctx->ctx, fmt, ##__VA_ARGS__ ); pack_ctx->error_code = err; return 0x0; }

enum dl_load_txt_tl_state
{
	DL_LOAD_TXT_TL_STATE_ROOT,
	DL_LOAD_TXT_TL_STATE_ROOT_MAP,
	DL_LOAD_TXT_TL_STATE_MODULE_NAME,
	DL_LOAD_TXT_TL_STATE_USERCODE,

	DL_LOAD_TXT_TL_STATE_ENUMS,
	DL_LOAD_TXT_TL_STATE_ENUMS_MAP,
	DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST,
	DL_LOAD_TXT_TL_STATE_ENUM_MAP,
	DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP,

	DL_LOAD_TXT_TL_STATE_TYPES,
	DL_LOAD_TXT_TL_STATE_TYPES_MAP,

	DL_LOAD_TXT_TL_STATE_TYPE_MAP,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST,
	DL_LOAD_TXT_TL_STATE_TYPE_ALIGN,
	DL_LOAD_TXT_TL_STATE_TYPE_EXTERN,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_BITS,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_COMMENT,

	DL_LOAD_TXT_TL_STATE_INVALID
};

struct dl_load_txt_tl_ctx
{
	dl_error_t error_code;

	const char* src_str;
	yajl_handle yajl;

	dl_load_txt_tl_state stack[128];
	size_t stack_item;

	size_t def_value_start;
	int cur_default_value_depth;

	dl_ctx_t ctx;
	dl_type_desc*   active_type;
	dl_member_desc* active_member;

	dl_enum_desc*       active_enum;
	dl_enum_value_desc* active_enum_value;

	uint32_t curr_enum_val;

	void push( dl_load_txt_tl_state s )
	{
		stack[++stack_item] = s;
	}

	void pop()
	{
		--stack_item;
	}
};

static dl_type_desc* dl_alloc_type( dl_ctx_t ctx, dl_typeid_t tid )
{
	// TODO: handle full.

	unsigned int type_index = ctx->type_count;
	++ctx->type_count;

	ctx->type_ids[ type_index ] = tid;
	dl_type_desc* type = ctx->type_descs + type_index;
	memset( type, 0x0, sizeof( dl_type_desc ) );

	return type;
}

static dl_member_desc* dl_alloc_member( dl_ctx_t ctx )
{
	// TODO: handle full.

	unsigned int member_index = ctx->member_count;
	++ctx->member_count;

	dl_member_desc* member = ctx->member_descs + member_index;
	memset( member, 0x0, sizeof( dl_member_desc ) );
	member->default_value_offset = 0xFFFFFFFF;
	member->default_value_size = 0;
	return member;
}

static dl_enum_desc* dl_alloc_enum( dl_ctx_t ctx, dl_typeid_t tid )
{
	// TODO: handle full.

	unsigned int enum_index = ctx->enum_count;
	++ctx->enum_count;

	ctx->enum_ids[ enum_index ] = tid;
	return ctx->enum_descs + enum_index;
}

static dl_enum_value_desc* dl_alloc_enum_value( dl_ctx_t ctx )
{
	// TODO: handle full.

	unsigned int value_index = ctx->enum_value_count;
	++ctx->enum_value_count;

	return ctx->enum_value_descs + value_index;
}

static void dl_load_txt_tl_handle_default( dl_load_txt_tl_ctx* state )
{
	if( state->cur_default_value_depth == 0 )
	{
		uint32_t haxx_value = ( (uint32_t)state->def_value_start ) | (uint32_t)( ( yajl_get_bytes_consumed( state->yajl ) - state->def_value_start ) << 16 );
		// haxx
		state->active_member->default_value_offset = haxx_value;

		state->pop();
	}
}

struct dl_load_state_name_map
{
	const char* name;
	dl_load_txt_tl_state state;
};

static dl_load_txt_tl_state dl_load_state_from_string( dl_load_state_name_map* map, size_t map_len, const unsigned char* str_val, size_t str_len )
{
	for( size_t i = 0; i < map_len; ++i )
		if( strncmp( (const char*)str_val, map[i].name, str_len ) == 0 )
			return map[i].state;

	return DL_LOAD_TXT_TL_STATE_INVALID;
}

static int dl_load_txt_tl_on_map_start( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ROOT:  state->push( DL_LOAD_TXT_TL_STATE_ROOT_MAP );  break;
		case DL_LOAD_TXT_TL_STATE_ENUMS: state->push( DL_LOAD_TXT_TL_STATE_ENUMS_MAP ); break;
		case DL_LOAD_TXT_TL_STATE_TYPES: state->push( DL_LOAD_TXT_TL_STATE_TYPES_MAP ); break;

		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
			DL_ASSERT( state->active_type != 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum == 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );
			state->push( DL_LOAD_TXT_TL_STATE_TYPE_MAP );
			break;

		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST:
		{
			DL_ASSERT( state->active_type != 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum == 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );

			state->active_member = dl_alloc_member( state->ctx );

			state->push( DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP );
		}
		break;

		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST:
		{
			DL_ASSERT( state->active_type == 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum != 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );

			state->active_enum_value = dl_alloc_enum_value( state->ctx );

			state->push( DL_LOAD_TXT_TL_STATE_ENUM_MAP );
		}
		break;

		case DL_LOAD_TXT_TL_STATE_ENUM_MAP:
			DL_ASSERT( state->active_type == 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum != 0x0 );
			DL_ASSERT( state->active_enum_value != 0x0 );

			state->push( DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP );
			break;

		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			++state->cur_default_value_depth;
			break;
		default:
			printf("%u\n", state->stack[state->stack_item] );
			DL_ASSERT( false );
			return 0;
	}
	return 1;
}

static int dl_load_txt_tl_on_map_key( void* ctx, const unsigned char* str_val, size_t str_len )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ROOT_MAP:
		{
			dl_load_state_name_map map[] = { { "module",   DL_LOAD_TXT_TL_STATE_MODULE_NAME },
											 { "enums",    DL_LOAD_TXT_TL_STATE_ENUMS },
											 { "types",    DL_LOAD_TXT_TL_STATE_TYPES },
											 { "usercode", DL_LOAD_TXT_TL_STATE_USERCODE } };

			dl_load_txt_tl_state s = dl_load_state_from_string( map, DL_ARRAY_LENGTH( map ), str_val, str_len );
			if( s == DL_LOAD_TXT_TL_STATE_INVALID )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR,
										"Got key \"%.*s\", in root-map, expected \"module\", \"enums\" or \"types\"!",
										str_len, str_val );

			state->push( s );
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_MAP:
		{
			if( str_len >= DL_ENUM_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "Enumname \"%.*s\" is (currently) to long!", str_len, str_val );

			// TODO: typeid should be patched when type is done by using all members etc.
			dl_typeid_t tid = dl_internal_hash_buffer( str_val, str_len );

			dl_enum_desc* e = dl_alloc_enum( state->ctx, tid );
			e->value_start = state->ctx->enum_value_count;
			e->value_count = 0;
			memcpy( e->name, str_val, str_len );
			e->name[ str_len ] = 0;

			state->active_enum = e;
			state->curr_enum_val = 0;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
		{
			// ... check that type is not already in tld ...

			if( str_len >= DL_TYPE_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "Typename \"%.*s\" is (currently) to long!", str_len, str_val );

			// ... alloc new type ...
			// TODO: typeid should be patched when type is done by using all members etc.
			dl_typeid_t tid = dl_internal_hash_buffer( str_val, str_len );

			dl_type_desc* type = dl_alloc_type( state->ctx, tid );
			type->member_start = state->ctx->member_count;
			type->member_count = 0;
			memcpy( type->name, str_val, str_len );
			type->name[ str_len ] = 0;
			type->size[ DL_PTR_SIZE_32BIT ] = 0;
			type->size[ DL_PTR_SIZE_64BIT ] = 0;

			state->active_type = type;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MAP:
		{
			dl_load_state_name_map map[] = { { "members", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST },
											 { "align",   DL_LOAD_TXT_TL_STATE_TYPE_ALIGN },
											 { "extern",  DL_LOAD_TXT_TL_STATE_TYPE_EXTERN } };

			dl_load_txt_tl_state s = dl_load_state_from_string( map, DL_ARRAY_LENGTH( map ), str_val, str_len );

			if( s == DL_LOAD_TXT_TL_STATE_INVALID )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR,
										"Got key \"%.*s\", in type, expected \"members\"!",
										str_len, str_val );
			state->push( s );
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP:
		{
			dl_load_state_name_map map[] = { { "name",    DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME },
											 { "type",    DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE },
											 { "bits",    DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_BITS },
											 { "default", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT },
											 { "comment", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_COMMENT } };

			dl_load_txt_tl_state s = dl_load_state_from_string( map, DL_ARRAY_LENGTH( map ), str_val, str_len );

			if( s == DL_LOAD_TXT_TL_STATE_INVALID )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR,
										"Got key \"%.*s\", in member def, expected \"name\" or \"type\"!",
										str_len, str_val );

			if( s == DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT )
			{
				DL_ASSERT( state->cur_default_value_depth == 0 );
				state->def_value_start = yajl_get_bytes_consumed( state->yajl );
			}

			// TODO: check that key was not already set!

			state->push( s );
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP:
		{
			if( str_len >= DL_ENUM_VALUE_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "enum value name \"%.*s\" is (currently) to long!", str_len, str_val );

			dl_enum_value_desc* value = state->active_enum_value;
			memcpy( value->name, str_val, str_len );
			value->name[ str_len ] = 0;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP:
		{

		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			break;
		default:
			printf("unhandled map key %lu %u \"%.*s\"\n", state->stack_item, state->stack[state->stack_item], (int)str_len, (const char*)str_val);
			// DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_tl_on_map_end( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ROOT_MAP:
			state->pop(); // pop stack
			break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_MAP:
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_ENUMS_MAP
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_ENUMS
			break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPES_MAP
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPES
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MAP:
		{
			dl_type_desc* type = state->active_type;

			DL_ASSERT( type != 0x0 );
			DL_ASSERT( state->active_member == 0x0 );

			// ... calc size, align and member stuff here + set typeid ...
			type->member_count = state->ctx->member_count - type->member_start;


			state->active_type = 0x0;
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MAP
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP:
			DL_ASSERT( state->active_type != 0x0 );
			DL_ASSERT( state->active_member != 0x0 );

			// ... check that all needed stuff was set ...

			state->active_member = 0x0;

			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP
			break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP:
		{
			DL_ASSERT( state->active_type == 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum != 0x0 );
			DL_ASSERT( state->active_enum_value != 0x0 );

			state->active_enum_value = 0x0;
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP:
		{
			DL_ASSERT( state->active_type == 0x0 );
			DL_ASSERT( state->active_member == 0x0 );
			DL_ASSERT( state->active_enum != 0x0 );
			DL_ASSERT( state->active_enum_value != 0x0 );

			state->pop(); // pop DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			--state->cur_default_value_depth;
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}


static void dl_set_member_size_and_align_from_builtin( dl_type_t storage, dl_member_desc* member )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT8:
		case DL_TYPE_STORAGE_UINT8:
			member->set_size( 1, 1 );
			member->set_align( 1, 1 );
			break;
		case DL_TYPE_STORAGE_INT16:
		case DL_TYPE_STORAGE_UINT16:
			member->set_size( 2, 2 );
			member->set_align( 2, 2 );
			break;
		case DL_TYPE_STORAGE_FP32:
		case DL_TYPE_STORAGE_INT32:
		case DL_TYPE_STORAGE_UINT32:
		case DL_TYPE_STORAGE_ENUM:
			member->set_size( 4, 4 );
			member->set_align( 4, 4 );
			break;
		case DL_TYPE_STORAGE_FP64:
		case DL_TYPE_STORAGE_INT64:
		case DL_TYPE_STORAGE_UINT64:
			member->set_size( 8, 8 );
			member->set_align( 8, 8 );
			break;
		case DL_TYPE_STORAGE_STR:
		case DL_TYPE_STORAGE_PTR:
			member->set_size( 4, 8 );
			member->set_align( 4, 8 );
			break;
		default:
			DL_ASSERT( false );
	}
}

struct dl_builtin_type
{
	const char* name;
	dl_type_t   type;
};

static const dl_builtin_type BUILTIN_TYPES[] = {
	{ "int8",   DL_TYPE_STORAGE_INT8 },
	{ "uint8",  DL_TYPE_STORAGE_UINT8 },
	{ "int16",  DL_TYPE_STORAGE_INT16 },
	{ "uint16", DL_TYPE_STORAGE_UINT16 },
	{ "int32",  DL_TYPE_STORAGE_INT32 },
	{ "uint32", DL_TYPE_STORAGE_UINT32 },
	{ "int64",  DL_TYPE_STORAGE_INT64 },
	{ "uint64", DL_TYPE_STORAGE_UINT64 },
	{ "fp32",   DL_TYPE_STORAGE_FP32 },
	{ "fp64",   DL_TYPE_STORAGE_FP64 },
	{ "string", DL_TYPE_STORAGE_STR },
};

dl_type_t dl_make_type( dl_type_t atom, dl_type_t storage )
{
	return (dl_type_t)( (unsigned int)atom | (unsigned int)storage );
}

static int dl_parse_type( dl_load_txt_tl_ctx* state, const char* str, size_t str_len, dl_member_desc* member )
{
	// ... strip whitespace ...
	char   type_name[2048];
	size_t type_name_len = 0;
	DL_ASSERT( str_len < DL_ARRAY_LENGTH( type_name ) );

	const char* iter = str;
	const char* end  = str + str_len;

	while( ( isalnum( *iter ) || *iter == '_' ) && ( iter != end ) )
	{
		type_name[type_name_len++] = *iter;
		++iter;
	}
	type_name[type_name_len] = '\0';

	bool is_ptr = false;
	bool is_array = false;
	bool is_inline_array = false;
	unsigned int inline_array_len = 0;

	if( iter != end )
	{
		if( *iter == '*' )
			is_ptr = true;
		if( *iter == '[' )
		{
			++iter;
			if( *iter == ']' )
				is_array = true;
			else
			{
				char num[256];
				size_t num_len = 0;
				while( isdigit(*iter) && iter != end )
				{
					num[num_len++] = *iter;
					++iter;
				}
				num[num_len] = '\0';

				if( *iter != ']' )
					DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "parse error!?!" );

				inline_array_len = (unsigned int)atol( num );
				is_inline_array = true;
			}
		}
	}

	if( strcmp( "bitfield", type_name ) == 0 )
	{
		member->type = dl_make_type( DL_TYPE_ATOM_BITFIELD, DL_TYPE_STORAGE_UINT8 );
		member->type_id = 0;

		// type etc?
		return 1;
	}

	for( size_t i = 0; i < DL_ARRAY_LENGTH( BUILTIN_TYPES ); ++i )
	{
		const dl_builtin_type* builtin = &BUILTIN_TYPES[i];
		if( strcmp( type_name, builtin->name ) == 0 )
		{
			// handle it ...
			if( is_ptr )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "pointer to pod is not supported!" );

			if( is_array )
			{
				member->type = dl_make_type( DL_TYPE_ATOM_ARRAY, builtin->type );
				member->type_id = 0;
				member->set_size( 8, 16 );
				member->set_align( 4, 8 );
				return 1;
			}

			dl_set_member_size_and_align_from_builtin( builtin->type, member );

			if( is_inline_array )
			{
				member->type = dl_make_type( DL_TYPE_ATOM_INLINE_ARRAY, builtin->type );
				member->type_id = 0;
				member->size[DL_PTR_SIZE_32BIT] *= inline_array_len;
				member->size[DL_PTR_SIZE_64BIT] *= inline_array_len;
				member->set_inline_array_cnt( inline_array_len );
				return 1;
			}

			member->type = dl_make_type( DL_TYPE_ATOM_POD, builtin->type );
			member->type_id = 0;
			dl_set_member_size_and_align_from_builtin( builtin->type, member );
			return 1;
		}
	}

	member->type_id = dl_internal_hash_string( type_name );

	if( is_ptr )
	{
		member->type = dl_make_type( DL_TYPE_ATOM_POD, DL_TYPE_STORAGE_PTR );
		member->set_size( 4, 8 );
		member->set_align( 4, 8 );
		return 1;
	}

	if( is_array )
	{
		member->type = dl_make_type( DL_TYPE_ATOM_ARRAY, DL_TYPE_STORAGE_STRUCT );
		member->set_size( 8, 16 );
		member->set_align( 4, 8 );
		return 1;
	}

	if( is_inline_array )
	{
		member->type = dl_make_type( DL_TYPE_ATOM_INLINE_ARRAY, DL_TYPE_STORAGE_STRUCT );
		// hack here is used to later set the size, this can be removed if we store inline array length
		// in the same space as bitfield bits and offset.
		member->set_size( inline_array_len, inline_array_len );
		member->set_align( 0, 0 );
		member->set_inline_array_cnt( inline_array_len );
		return 1;
	}

	member->type = dl_make_type( DL_TYPE_ATOM_POD, DL_TYPE_STORAGE_STRUCT );
	member->set_size( 0, 0 );
	member->set_align( 0, 0 );
	return 1;
}

static int dl_load_txt_tl_on_string( void* ctx, const unsigned char* str_val, size_t str_len )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_MODULE_NAME: state->pop(); break;
		case DL_LOAD_TXT_TL_STATE_USERCODE: break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST:
		{
			if( str_len >= DL_ENUM_VALUE_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "enum value name \"%.*s\" is (currently) to long!", str_len, str_val );

			dl_enum_value_desc* v = dl_alloc_enum_value( state->ctx );
			memcpy( v->name, str_val, str_len );
			v->name[ str_len ] = 0;
			v->value = state->curr_enum_val++;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME:
		{
			if( str_len >= DL_MEMBER_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "member name \"%.*s\" is (currently) to long!", str_len, str_val );

			memcpy( state->active_member->name, str_val, str_len );

			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE:
		{
			if( dl_parse_type( state, (const char*)str_val, str_len, state->active_member ) == 0 )
				return 0;

			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_COMMENT:
		{
			// HANDLE COMMENT HERE!
			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP:
		{
			// do it!
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_on_integer( void * ctx, long long integer )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_BITS:
		{
			DL_ASSERT( state->active_type       != 0x0 );
			DL_ASSERT( state->active_member     != 0x0 );
			DL_ASSERT( state->active_enum       == 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );

			dl_member_desc* member = state->active_member;

			if( member->BitFieldBits() != 0 )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "member has \"bits\" set multiple times!" );

			member->SetBitFieldBits( (unsigned int)integer );
			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_ALIGN:
		{
			DL_ASSERT( state->active_type       != 0x0 );
			DL_ASSERT( state->active_member     == 0x0 );
			DL_ASSERT( state->active_enum       == 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );

			dl_type_desc* type = state->active_type;
			if( type->alignment[DL_PTR_SIZE_32BIT] != 0 )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "%s has \"align\" set multiple times!", type->name );

			type->alignment[DL_PTR_SIZE_32BIT] = (uint32_t)integer;
			type->alignment[DL_PTR_SIZE_64BIT] = (uint32_t)integer;

			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP:
		case DL_LOAD_TXT_TL_STATE_ENUM_MAP_SUBMAP:
		{
			DL_ASSERT( state->active_type       == 0x0 );
			DL_ASSERT( state->active_member     == 0x0 );
			DL_ASSERT( state->active_enum       != 0x0 );
			DL_ASSERT( state->active_enum_value != 0x0 );

			state->active_enum_value->value = (uint32_t)integer;
			state->curr_enum_val = (uint32_t)integer + 1;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			printf("%u\n", state->stack[state->stack_item]);
			DL_ASSERT( false );
			return 0;
	}
	return 1;
}

static int dl_load_txt_on_double( void * ctx, double dbl )
{
	(void)dbl;
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			printf("%u\n", state->stack[state->stack_item]);
			DL_ASSERT( false );
			return 0;
	}
	return 1;
}

static int dl_load_txt_on_array_start( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ENUMS_MAP:
			state->push( DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST );
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST:
			break;
		case DL_LOAD_TXT_TL_STATE_USERCODE:
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			++state->cur_default_value_depth;
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_on_array_end( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST:
		{
			dl_enum_desc* e = state->active_enum;
			DL_ASSERT( e != 0x0 );
			DL_ASSERT( state->active_enum_value == 0x0 );

			e->value_count = state->ctx->enum_value_count - e->value_start;

			state->active_enum = 0x0;
			state->pop();
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST:
			state->pop();
			break;
		case DL_LOAD_TXT_TL_STATE_USERCODE:
			state->pop();
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			--state->cur_default_value_depth;
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_on_null( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_on_bool( void* ctx, int value )
{
	(void)value;
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_TYPE_EXTERN:
			// handle me :)
			state->pop();
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_DEFAULT:
			dl_load_txt_tl_handle_default( state );
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static inline int dl_internal_str_format(char* DL_RESTRICT buf, size_t buf_size, const char* DL_RESTRICT fmt, ...)
{
	va_list args;
	va_start( args, fmt );
	int res = vsnprintf( buf, buf_size, fmt, args );
	buf[buf_size - 1] = '\0';
	va_end( args );
	return res;
}

static void dl_load_txt_build_default_data( dl_ctx_t ctx, const char* lib_data, dl_member_desc* member )
{
	if( member->default_value_offset == 0xFFFFFFFF )
		return;

	uint32_t def_start = member->default_value_offset & 0xFFFF;
	uint32_t def_len   = member->default_value_offset >> 16;

	char def_buffer[2048]; // TODO: no hardcode =/

	// TODO: check that this is not outside the buffers
	dl_type_desc*   def_type   = ctx->type_descs + ctx->type_count;
	dl_member_desc* def_member = ctx->member_descs + ctx->member_count;
	ctx->type_ids[ctx->type_count] = dl_internal_hash_string( "a_type_here" );

	// TODO: check that typename do not exist in the ctx!

	strncpy( def_type->name, "a_type_here", DL_TYPE_NAME_MAX_LEN );
	def_type->size[DL_PTR_SIZE_HOST]      = member->size[DL_PTR_SIZE_HOST];
	def_type->alignment[DL_PTR_SIZE_HOST] = member->alignment[DL_PTR_SIZE_HOST];
	def_type->member_count = 1;
	def_type->member_start = ctx->member_count;

	memcpy( def_member, member, sizeof( dl_member_desc ) );
	def_member->offset[0] = 0;
	def_member->offset[1] = 0;

	++ctx->type_count;
	++ctx->member_count;

	dl_internal_str_format( def_buffer, 2048, "{\"type\":\"a_type_here\",\"data\":{\"%s\"%.*s}}", member->name, (int)def_len, lib_data + def_start );

	size_t prod_bytes;
	dl_txt_pack( ctx, def_buffer, 0x0, 0, &prod_bytes );

	uint8_t* pack_buffer = (uint8_t*)dl_alloc( &ctx->alloc, prod_bytes );

	dl_txt_pack( ctx, def_buffer, pack_buffer, prod_bytes, 0x0 );

	// TODO: convert packed instance to typelib endian/ptrsize here!

	size_t inst_size = prod_bytes - sizeof( dl_data_header );

	ctx->default_data = (uint8_t*)dl_realloc( &ctx->alloc, ctx->default_data, ctx->default_data_size + inst_size, ctx->default_data_size );
	memcpy( ctx->default_data + ctx->default_data_size, pack_buffer + sizeof( dl_data_header ), inst_size );

	dl_free( &ctx->alloc, pack_buffer );

	member->default_value_offset = (uint32_t)ctx->default_data_size;
	member->default_value_size   = (uint32_t)inst_size;
	ctx->default_data_size += inst_size;

	--ctx->type_count;
	--ctx->member_count;
}

static dl_member_desc* dl_load_txt_find_first_bitfield_member( dl_member_desc* start, dl_member_desc* end )
{
	while( start <= end )
	{
		if( start->AtomType() == DL_TYPE_ATOM_BITFIELD )
			return start;
		++start;
	}
	return 0x0;
}

static dl_member_desc* dl_load_txt_find_last_bitfield_member( dl_member_desc* start, dl_member_desc* end )
{
	while( start <= end )
	{
		if( start->AtomType() != DL_TYPE_ATOM_BITFIELD )
			return start - 1;
		++start;
	}
	return end;
}

static void dl_load_txt_fixup_bitfield_members( dl_ctx_t ctx, dl_type_desc* type )
{
	dl_member_desc* start = ctx->member_descs + type->member_start;
	dl_member_desc* end   = start + type->member_count - 1;

	while( true )
	{
		dl_member_desc* group_start = dl_load_txt_find_first_bitfield_member( start, end );
		if( group_start == 0x0 )
			return; // done!
		dl_member_desc* group_end = dl_load_txt_find_last_bitfield_member( group_start, end );

		unsigned int group_bits = 0;
		for( dl_member_desc* iter = group_start; iter <= group_end; ++iter )
		{
			iter->SetBitFieldOffset( group_bits );
			group_bits += iter->BitFieldBits();
		}

		// TODO: handle higher bit-counts than 64!
		dl_type_t storage;
		if     ( group_bits <= 8  ) storage = DL_TYPE_STORAGE_UINT8;
		else if( group_bits <= 16 ) storage = DL_TYPE_STORAGE_UINT16;
		else if( group_bits <= 32 ) storage = DL_TYPE_STORAGE_UINT32;
		else if( group_bits <= 64 ) storage = DL_TYPE_STORAGE_UINT64;
		else
			DL_ASSERT( false );

		for( dl_member_desc* iter = group_start; iter <= group_end; ++iter )
		{
			iter->SetStorage( storage );
			dl_set_member_size_and_align_from_builtin( storage, iter );
		}

		start = group_end + 1;
	}
}

static void dl_load_txt_fixup_enum_members( dl_ctx_t ctx, dl_type_desc* type )
{
	dl_member_desc* iter = ctx->member_descs + type->member_start;
	dl_member_desc* end  = ctx->member_descs + type->member_start + type->member_count;

	for( ; iter != end; ++iter )
	{
		dl_type_t storage = iter->StorageType();
		if( storage == DL_TYPE_STORAGE_STRUCT )
		{
			dl_enum_desc* e = (dl_enum_desc*)dl_internal_find_enum( ctx, iter->type_id );
			if( e != 0x0 ) // this was really an enum!
			{
				iter->SetStorage( DL_TYPE_STORAGE_ENUM );
				if( iter->AtomType() == DL_TYPE_ATOM_POD )
					dl_set_member_size_and_align_from_builtin( DL_TYPE_STORAGE_ENUM, iter );
				else if( iter->AtomType() == DL_TYPE_ATOM_INLINE_ARRAY )
				{
					unsigned int count = iter->size[0];
					dl_set_member_size_and_align_from_builtin( DL_TYPE_STORAGE_ENUM, iter );
					iter->size[DL_PTR_SIZE_32BIT] *= count;
					iter->size[DL_PTR_SIZE_64BIT] *= count;
				}
			}
		}
	}
}

/**
 * return true if the calculation was successful.
 */
bool dl_load_txt_calc_type_size_and_align( dl_ctx_t ctx, dl_type_desc* type )
{
	// ... is the type already processed ...
	if( type->size[0] > 0 )
		return true;

	dl_load_txt_fixup_bitfield_members( ctx, type );
	dl_load_txt_fixup_enum_members( ctx, type );

	uint32_t size[2]  = { 0, 0 };
	uint32_t align[2] = { type->alignment[DL_PTR_SIZE_32BIT], type->alignment[DL_PTR_SIZE_64BIT] };

	unsigned int mem_start = type->member_start;
	unsigned int mem_end   = type->member_start + type->member_count;

	dl_member_desc* bitfield_group_start = 0x0;

	for( unsigned int member_index = mem_start; member_index < mem_end; ++member_index )
	{
		dl_member_desc* member = ctx->member_descs + member_index;

		dl_type_t atom = member->AtomType();
		dl_type_t storage = member->StorageType();

		if( atom == DL_TYPE_ATOM_INLINE_ARRAY || atom == DL_TYPE_ATOM_POD )
		{
			if( storage == DL_TYPE_STORAGE_STRUCT )
			{
				const dl_type_desc* sub_type = dl_internal_find_type( ctx, member->type_id );
				if( sub_type == 0x0 )
				{
					printf("%s->%s need lookup = NOT FOUND!\n", type->name, member->name);
					continue;
				}

				if( sub_type->size[0] == 0 )
					dl_load_txt_calc_type_size_and_align( ctx, (dl_type_desc*)sub_type );

				if( atom == DL_TYPE_ATOM_INLINE_ARRAY )
				{
					member->size[DL_PTR_SIZE_32BIT] *= sub_type->size[DL_PTR_SIZE_32BIT];
					member->size[DL_PTR_SIZE_64BIT] *= sub_type->size[DL_PTR_SIZE_64BIT];
				}
				else
					member->copy_size( sub_type->size );

				member->copy_align( sub_type->alignment );
			}

			bitfield_group_start = 0x0;
		}
		else if( atom == DL_TYPE_ATOM_BITFIELD )
		{
			if( bitfield_group_start )
			{
				member->offset[DL_PTR_SIZE_32BIT] = bitfield_group_start->offset[DL_PTR_SIZE_32BIT];
				member->offset[DL_PTR_SIZE_64BIT] = bitfield_group_start->offset[DL_PTR_SIZE_64BIT];
				continue;
			}
			bitfield_group_start = member;
		}
		else
			bitfield_group_start = 0x0;

		member->offset[DL_PTR_SIZE_32BIT] = dl_internal_align_up( size[DL_PTR_SIZE_32BIT], member->alignment[DL_PTR_SIZE_32BIT] );
		member->offset[DL_PTR_SIZE_64BIT] = dl_internal_align_up( size[DL_PTR_SIZE_64BIT], member->alignment[DL_PTR_SIZE_64BIT] );
		size[DL_PTR_SIZE_32BIT] = member->offset[DL_PTR_SIZE_32BIT] + member->size[DL_PTR_SIZE_32BIT];
		size[DL_PTR_SIZE_64BIT] = member->offset[DL_PTR_SIZE_64BIT] + member->size[DL_PTR_SIZE_64BIT];

		align[DL_PTR_SIZE_32BIT] = member->alignment[DL_PTR_SIZE_32BIT] > align[DL_PTR_SIZE_32BIT] ? member->alignment[DL_PTR_SIZE_32BIT] : align[DL_PTR_SIZE_32BIT];
		align[DL_PTR_SIZE_64BIT] = member->alignment[DL_PTR_SIZE_64BIT] > align[DL_PTR_SIZE_64BIT] ? member->alignment[DL_PTR_SIZE_64BIT] : align[DL_PTR_SIZE_64BIT];
	}

	type->size[DL_PTR_SIZE_32BIT]      = dl_internal_align_up( size[DL_PTR_SIZE_32BIT], align[DL_PTR_SIZE_32BIT] );
	type->size[DL_PTR_SIZE_64BIT]      = dl_internal_align_up( size[DL_PTR_SIZE_64BIT], align[DL_PTR_SIZE_64BIT] );
	type->alignment[DL_PTR_SIZE_32BIT] = align[DL_PTR_SIZE_32BIT];
	type->alignment[DL_PTR_SIZE_64BIT] = align[DL_PTR_SIZE_64BIT];

	return true;
}

dl_error_t dl_context_load_txt_type_library( dl_ctx_t dl_ctx, const char* lib_data, size_t lib_data_size )
{
	// ... parse it ...
//	yajl_alloc_funcs my_yajl_alloc = {
//		0x0, // dl_internal_pack_alloc,
//		0x0, // dl_internal_pack_realloc,
//		0x0, // dl_internal_pack_free,
//		0x0
//	};

	yajl_callbacks callbacks = {
		dl_load_txt_on_null,
		dl_load_txt_on_bool,
		dl_load_txt_on_integer,
		dl_load_txt_on_double,
		0x0, // dl_internal_pack_on_number,
		dl_load_txt_tl_on_string,
		dl_load_txt_tl_on_map_start,
		dl_load_txt_tl_on_map_key,
		dl_load_txt_tl_on_map_end,
		dl_load_txt_on_array_start,
		dl_load_txt_on_array_end
	};

	unsigned int start_type = dl_ctx->type_count;
	unsigned int start_member = dl_ctx->member_count;

	dl_load_txt_tl_ctx state;
	state.stack_item = 0;
	state.stack[state.stack_item] = DL_LOAD_TXT_TL_STATE_ROOT;
	state.ctx = dl_ctx;
	state.active_type = 0x0;
	state.active_member = 0x0;
	state.active_enum = 0x0;
	state.active_enum_value = 0x0;
	state.cur_default_value_depth = 0;
	state.error_code = DL_ERROR_OK;
	state.src_str = lib_data;
	state.yajl = yajl_alloc( &callbacks, /*&my_yajl_alloc*/0x0, &state );
	yajl_config( state.yajl, yajl_allow_comments, 1 );

	yajl_status my_yajl_status = yajl_parse( state.yajl, (const unsigned char*)lib_data, lib_data_size );

	DL_ASSERT( state.stack_item == 0 );

	if( my_yajl_status != yajl_status_ok )
	{
		printf("%s\n", yajl_get_error( state.yajl, 1, (const unsigned char*)lib_data, lib_data_size ) );
		yajl_free( state.yajl );
		return state.error_code;
	}

	yajl_free( state.yajl );

	// ... calculate size of types ...
	for( unsigned int i = start_type; i < dl_ctx->type_count; ++i )
		dl_load_txt_calc_type_size_and_align( dl_ctx, dl_ctx->type_descs + i );

	for( unsigned int i = start_member; i < dl_ctx->member_count; ++i )
		dl_load_txt_build_default_data( dl_ctx, lib_data, dl_ctx->member_descs + i );

	return DL_ERROR_OK;
}
