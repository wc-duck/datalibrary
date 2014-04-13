#include <dl/dl_typelib.h>
#include <yajl/yajl_parse.h>

#include "dl_types.h"

#include <stdio.h> // remove

#define DL_PACK_ERROR_AND_FAIL( pack_ctx, err, fmt, ... ) { dl_log_error( pack_ctx->ctx, fmt, ##__VA_ARGS__ ); pack_ctx->error_code = err; return 0x0; }

enum dl_load_txt_tl_state
{
	DL_LOAD_TXT_TL_STATE_ROOT,
	DL_LOAD_TXT_TL_STATE_ROOT_MAP,
	DL_LOAD_TXT_TL_STATE_MODULE_NAME,

	DL_LOAD_TXT_TL_STATE_ENUMS,
	DL_LOAD_TXT_TL_STATE_ENUMS_MAP,
	DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST,

	DL_LOAD_TXT_TL_STATE_TYPES,
	DL_LOAD_TXT_TL_STATE_TYPES_MAP,

	DL_LOAD_TXT_TL_STATE_TYPE_MAP,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME,
	DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE,

	DL_LOAD_TXT_TL_STATE_INVALID
};

struct dl_load_txt_tl_ctx
{
	dl_error_t error_code;

	dl_load_txt_tl_state stack[128];
	size_t stack_item;

	dl_ctx_t ctx;
	dl_type_desc*   active_type;
	dl_member_desc* active_member;

	dl_enum_desc*       active_enum;
	dl_enum_value_desc* active_enum_value;

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
	return ctx->type_descs + type_index;
}

static dl_member_desc* dl_alloc_member( dl_ctx_t ctx )
{
	// TODO: handle full.

	unsigned int member_index = ctx->member_count;
	++ctx->member_count;

	return ctx->member_descs + member_index;
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
		default:
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
			dl_load_state_name_map map[] = { { "module", DL_LOAD_TXT_TL_STATE_MODULE_NAME },
											 { "enums",  DL_LOAD_TXT_TL_STATE_ENUMS },
											 { "types",  DL_LOAD_TXT_TL_STATE_TYPES } };

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
			printf("begin parse enum \"%.*s\"\n", (int)str_len, (const char*)str_val);
			if( str_len >= DL_ENUM_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "Enumname \"%.*s\" is (currently) to long!", str_len, str_val );

			dl_enum_desc* e = dl_alloc_enum( state->ctx, 0xFFFFFFFF ); // typeid is patched when type is done by using all members etc.
			e->value_start = state->ctx->enum_value_count;
			e->value_count = 0;
			memcpy( e->name, str_val, str_len );
			e->name[ str_len ] = 0;

			state->active_enum = e;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
		{
			printf("begin parse type \"%.*s\"\n", (int)str_len, (const char*)str_val);

			// ... check that type is not already in tld ...

			if( str_len >= DL_TYPE_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "Typename \"%.*s\" is (currently) to long!", str_len, str_val );

			// ... alloc new type ...
			dl_type_desc* type = dl_alloc_type( state->ctx, 0xFFFFFFFF ); // typeid is patched when type is done by using all members etc.
			type->member_start = state->ctx->member_count;
			type->member_count = 0;
			memcpy( type->name, str_val, str_len );
			type->name[ str_len ] = 0;

			state->active_type = type;
		}
		break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MAP:
		{
			printf("\tbegin parse type member \"%.*s\"\n", (int)str_len, (const char*)str_val);

			dl_load_state_name_map map[] = { { "members", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST } };

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
			dl_load_state_name_map map[] = { { "name", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME },
											 { "type", DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE } };

			dl_load_txt_tl_state s = dl_load_state_from_string( map, DL_ARRAY_LENGTH( map ), str_val, str_len );

			if( s == DL_LOAD_TXT_TL_STATE_INVALID )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR,
										"Got key \"%.*s\", in member def, expected \"name\" or \"type\"!",
										str_len, str_val );

			// TODO: check that key was not already set!

			state->push( s );
		}
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
			DL_ASSERT( state->active_type != 0x0 );

			// ... calc size, align and member stuff here + set typeid ...

			state->active_type = 0x0;
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MAP
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP:
			DL_ASSERT( state->active_type != 0x0 );
			DL_ASSERT( state->active_member != 0x0 );

			// ... check that all needed stuff was set ...

			state->active_member = 0x0;

			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

struct dl_builtin_type
{
	const char* name;
	uint32_t    size[2];
	uint32_t    alignment[2];
	dl_type_t   type;
};

static const dl_builtin_type BUILTIN_TYPES[] = {
	{ "int8",   { 1, 1 }, { 1, 1 }, DL_TYPE_STORAGE_INT8 },
	{ "uint8",  { 1, 1 }, { 1, 1 }, DL_TYPE_STORAGE_UINT8 },
	{ "int16",  { 2, 2 }, { 2, 2 }, DL_TYPE_STORAGE_INT16 },
	{ "uint16", { 2, 2 }, { 2, 2 }, DL_TYPE_STORAGE_UINT16 },
	{ "int32",  { 4, 4 }, { 4, 4 }, DL_TYPE_STORAGE_INT32 },
	{ "uint32", { 4, 4 }, { 4, 4 }, DL_TYPE_STORAGE_UINT32 },
	{ "int64",  { 8, 8 }, { 8, 8 }, DL_TYPE_STORAGE_INT64 },
	{ "uint64", { 8, 8 }, { 8, 8 }, DL_TYPE_STORAGE_UINT64 },
	{ "fp32",   { 4, 4 }, { 4, 4 }, DL_TYPE_STORAGE_FP32 },
	{ "fp64",   { 8, 8 }, { 8, 8 }, DL_TYPE_STORAGE_FP64 },
	{ "string", { 4, 8 }, { 4, 8 }, DL_TYPE_STORAGE_STR },
};

dl_type_t dl_make_type( dl_type_t atom, dl_type_t storage )
{
	return (dl_type_t)( (unsigned int)atom | (unsigned int)storage );
}

static int dl_parse_type( dl_load_txt_tl_ctx* state, const char* str, size_t str_len, dl_member_desc* member )
{
	// TODO: strip whitespace here!

	for( size_t i = 0; i < DL_ARRAY_LENGTH( BUILTIN_TYPES ); ++i )
	{
		const dl_builtin_type* builtin = &BUILTIN_TYPES[i];
		size_t test_len = strlen( builtin->name );

		// in string length is less than name of builtin type, it can't be it.
		if( test_len >= str_len )
		{
			if( strncmp( builtin->name, str, test_len ) == 0 )
			{
				if( str_len == test_len )
				{
					member->type = dl_make_type( DL_TYPE_ATOM_POD, builtin->type );
					member->size[DL_PTR_SIZE_32BIT] = builtin->size[DL_PTR_SIZE_32BIT];
					member->size[DL_PTR_SIZE_64BIT] = builtin->size[DL_PTR_SIZE_64BIT];
					member->alignment[DL_PTR_SIZE_32BIT] = builtin->alignment[DL_PTR_SIZE_32BIT];
					member->alignment[DL_PTR_SIZE_64BIT] = builtin->alignment[DL_PTR_SIZE_64BIT];
					return 1;
				}

				// TODO: handle error here! example "uint8[" or "uint8*whoo"

				// check for [] for array
				if( str[test_len] == '[' && str[test_len+1] == ']' )
				{
					member->type = dl_make_type( DL_TYPE_ATOM_ARRAY, builtin->type );
					member->size[DL_PTR_SIZE_32BIT] = 4;
					member->size[DL_PTR_SIZE_64BIT] = 8;
					member->alignment[DL_PTR_SIZE_32BIT] = 4;
					member->alignment[DL_PTR_SIZE_64BIT] = 8;
					return 1;
				}

				unsigned int inline_array_len = 0;
				if( sscanf( str + test_len, "[%u]", &inline_array_len ) )
				{
					member->type = dl_make_type( DL_TYPE_ATOM_INLINE_ARRAY, builtin->type );
					member->size[DL_PTR_SIZE_32BIT] = builtin->size[DL_PTR_SIZE_32BIT] * inline_array_len;
					member->size[DL_PTR_SIZE_64BIT] = builtin->size[DL_PTR_SIZE_64BIT] * inline_array_len;
					member->alignment[DL_PTR_SIZE_32BIT] = builtin->alignment[DL_PTR_SIZE_32BIT];
					member->alignment[DL_PTR_SIZE_64BIT] = builtin->alignment[DL_PTR_SIZE_64BIT];
					return 1;
				}

				if( str[test_len] == '*' )
					DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "pointer to pod is not supported!" );
			}
		}
	}

	printf( "couldn't parse %.*s\n", (int)str_len, str );
	return 0;
}

static int dl_load_txt_tl_on_string( void* ctx, const unsigned char* str_val, size_t str_len )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_MODULE_NAME:      printf("module name is \"%.*s\"\n",(int)str_len, (const char*)str_val); state->pop(); break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST:
		{
			printf("\tadd enum-value \"%.*s\"\n",(int)str_len, (const char*)str_val);

			if( str_len >= DL_ENUM_VALUE_NAME_MAX_LEN )
				DL_PACK_ERROR_AND_FAIL( state, DL_ERROR_TXT_PARSE_ERROR, "enum value name \"%.*s\" is (currently) to long!", str_len, str_val );

			dl_enum_value_desc* v = dl_alloc_enum_value( state->ctx );
			memcpy( v->name, str_val, str_len );
			v->name[ str_len ] = 0;
			v->value = 1337; // set correctly ;)
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
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_internal_pack_on_array_start( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ENUMS_MAP:
			printf("begin enum value list!\n");
			state->push( DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST );
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST:
			printf("\t\tbegin member value list!\n");
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_internal_pack_on_array_end( void* ctx )
{
	dl_load_txt_tl_ctx* state = (dl_load_txt_tl_ctx*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST:
			printf("end enum value list!\n");
			state->active_enum = 0x0;
			state->pop();
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST:
			state->pop(); break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
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
		0x0, // dl_internal_pack_on_null,
		0x0, // dl_internal_pack_on_bool,
		NULL, // integer,
		NULL, // double,
		0x0, // dl_internal_pack_on_number,
		dl_load_txt_tl_on_string,
		dl_load_txt_tl_on_map_start,
		dl_load_txt_tl_on_map_key,
		dl_load_txt_tl_on_map_end,
		dl_internal_pack_on_array_start,
		dl_internal_pack_on_array_end
	};

	dl_load_txt_tl_ctx state;
	state.stack_item = 0;
	state.stack[state.stack_item] = DL_LOAD_TXT_TL_STATE_ROOT;
	state.ctx = dl_ctx;
	state.active_type = 0x0;
	state.active_member = 0x0;
	state.active_enum = 0x0;
	state.active_enum_value = 0x0;
	state.error_code = DL_ERROR_OK;

	yajl_handle my_yajl_handle = yajl_alloc( &callbacks, /*&my_yajl_alloc*/0x0, &state );
	yajl_status my_yajl_status = yajl_parse( my_yajl_handle, (const unsigned char*)lib_data, lib_data_size );

	DL_ASSERT( state.stack_item == 0 );

	if( my_yajl_status != yajl_status_ok )
	{
		printf("%s\n", yajl_get_error( my_yajl_handle, 1, (const unsigned char*)lib_data, lib_data_size ) );
		yajl_free( my_yajl_handle );
		return state.error_code;
	}

	yajl_free( my_yajl_handle );

	return DL_ERROR_OK;
}
