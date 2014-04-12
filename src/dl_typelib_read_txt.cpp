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

struct dl_load_txt_tl_enum
{
	dl_error_t error_code;

	dl_load_txt_tl_state stack[128];
	size_t stack_item;

	dl_ctx_t ctx;


	void push( dl_load_txt_tl_state s )
	{
		stack[++stack_item] = s;
	}

	void pop()
	{
		--stack_item;
	}
};


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
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ROOT:             state->push( DL_LOAD_TXT_TL_STATE_ROOT_MAP );  break;
		case DL_LOAD_TXT_TL_STATE_ENUMS:            state->push( DL_LOAD_TXT_TL_STATE_ENUMS_MAP ); break;
		case DL_LOAD_TXT_TL_STATE_TYPES:            state->push( DL_LOAD_TXT_TL_STATE_TYPES_MAP ); break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:        state->push( DL_LOAD_TXT_TL_STATE_TYPE_MAP );  break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST: state->push( DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP ); break;
		default:
			DL_ASSERT( false );
			return 0;
	}
	return 1;
}

static int dl_load_txt_tl_on_map_key( void* ctx, const unsigned char* str_val, size_t str_len )
{
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

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
			printf("begin parse enum \"%.*s\"\n", (int)str_len, (const char*)str_val);
			break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
		{
			printf("begin parse type \"%.*s\"\n", (int)str_len, (const char*)str_val);

			// ... check that type is not already in tld ...


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
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ROOT_MAP:
			state->pop(); // pop stack
			break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_MAP:
			printf("enums map end!\n");
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_ENUMS_MAP
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_ENUMS
			break;
		case DL_LOAD_TXT_TL_STATE_TYPES_MAP:
			printf("types map end!\n");
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPES_MAP
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPES
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MAP:
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MAP
			break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP:
			state->pop(); // pop DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_MAP
			break;
		default:
			DL_ASSERT( false );
			return 0;
	}

	return 1;
}

static int dl_load_txt_tl_on_string( void* ctx, const unsigned char* str_val, size_t str_len )
{
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_MODULE_NAME:      printf("module name is \"%.*s\"\n",(int)str_len, (const char*)str_val); state->pop(); break;
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST: printf("\tadd enum-value \"%.*s\"\n",(int)str_len, (const char*)str_val); break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_NAME: printf("\t\t\tmember name \"%.*s\"\n",(int)str_len, (const char*)str_val); state->pop(); break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_TYPE:
		{
			printf("\t\t\tmember type \"%.*s\"\n",(int)str_len, (const char*)str_val);

			// ... parse type ...

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
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

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
	dl_load_txt_tl_enum* state = (dl_load_txt_tl_enum*)ctx;

	switch( state->stack[state->stack_item] )
	{
		case DL_LOAD_TXT_TL_STATE_ENUMS_VALUE_LIST: printf("end enum value list!\n");       state->pop(); break;
		case DL_LOAD_TXT_TL_STATE_TYPE_MEMBER_LIST: printf("\t\tend member value list!\n"); state->pop(); break;
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

	dl_load_txt_tl_enum state;
	state.stack_item = 0;
	state.stack[state.stack_item] = DL_LOAD_TXT_TL_STATE_ROOT;
	state.ctx = dl_ctx;

	yajl_handle my_yajl_handle = yajl_alloc( &callbacks, /*&my_yajl_alloc*/0x0, &state );
	yajl_status my_yajl_status = yajl_parse( my_yajl_handle, (const unsigned char*)lib_data, lib_data_size );

	DL_ASSERT( state.stack_item == 0 );

	if( my_yajl_status != yajl_status_ok )
		return DL_ERROR_TXT_PARSE_ERROR; // TODO: better error-report.

	yajl_free( my_yajl_handle );

	return DL_ERROR_INTERNAL_ERROR;
}
