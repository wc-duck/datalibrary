/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_types.h"
#include "dl_binary_writer.h"
#include "dl_patch_ptr.h"

#include <dl/dl_txt.h>

#include "container/dl_array.h"

#include <yajl/yajl_parse.h>

#include <stdlib.h>

/*
	PACKING OF ARRAYS, How its done!

	if the elemf the array is not containing subarrays ( or other variable size data-types if they ever is added )
	each array element is packed after the last other data section as usual.
	example:

	[ struct a (arr1) (arr2) ] [ arr1_elem1, arr1_elem1, arr1_elem1 ] [ arr2_elem1 ...

	if, on the other hand the elements of the array-type has sub-arrays each element of the array will be written at the end of
	the packbuffer and when the end of the array is reached it will be reversed into place as an ordinary array.
	example:
	[ struct a (arr1) ] [ arr2_elem1, arr2_elem1, arr2_elem1 ] [ arr3_elem1 ...        (empty)        ... arr1_elem2 (offset to arr2), arr1_elem1 (offset to arr2) ]

	this sholud work since an array with no subdata can be completely packed before the next instance so we might write that directly. The same
	concept should hold in the case of subarrays for the subarrays since then we will start building a new array from the back and it will be completed
	before we need to write another element to the parent array.
*/

enum dl_pack_state
{
	DL_PACK_STATE_POD_INT8   = DL_TYPE_STORAGE_INT8,
	DL_PACK_STATE_POD_INT16  = DL_TYPE_STORAGE_INT16,
	DL_PACK_STATE_POD_INT32  = DL_TYPE_STORAGE_INT32,
	DL_PACK_STATE_POD_INT64  = DL_TYPE_STORAGE_INT64,
	DL_PACK_STATE_POD_UINT8  = DL_TYPE_STORAGE_UINT8,
	DL_PACK_STATE_POD_UINT16 = DL_TYPE_STORAGE_UINT16,
	DL_PACK_STATE_POD_UINT32 = DL_TYPE_STORAGE_UINT32,
	DL_PACK_STATE_POD_UINT64 = DL_TYPE_STORAGE_UINT64,
	DL_PACK_STATE_POD_FP32   = DL_TYPE_STORAGE_FP32,
	DL_PACK_STATE_POD_FP64   = DL_TYPE_STORAGE_FP64,
	DL_PACK_STATE_STRING     = DL_TYPE_STORAGE_STR,

	DL_PACK_STATE_ARRAY  = 0xFF000000,
	DL_PACK_STATE_ENUM,

	DL_PACK_STATE_STRING_ARRAY = DL_PACK_STATE_STRING | DL_PACK_STATE_ARRAY,

	DL_PACK_STATE_SUBDATA_ID,
	DL_PACK_STATE_SUBDATA,

	DL_PACK_STATE_INSTANCE,
	DL_PACK_STATE_INSTANCE_TYPE,
	DL_PACK_STATE_STRUCT,
};

#define DL_PACK_ERROR_AND_FAIL( pack_ctx, err, fmt, ... ) { dl_log_error( pack_ctx->dl_ctx, fmt, ##__VA_ARGS__ ); pack_ctx->error_code = err; return 0x0; }
#define DL_PACK_ERROR_AND_FAIL_IF( cond, pack_ctx, err, fmt, ... ) if( cond ) DL_PACK_ERROR_AND_FAIL( pack_ctx, err, fmt, ##__VA_ARGS__ )

template<unsigned int BITS>
class CFlagField
{
	uint32_t storage[ BITS / 32 ];

	enum { BITS_PER_STORAGE = sizeof(uint32_t) * 8, BITS_FOR_FIELD = 5 };

	unsigned int Field( unsigned int bit ) { return ( bit  & ~(unsigned int)(BITS_PER_STORAGE - 1) ) >> BITS_FOR_FIELD; }
	unsigned int Bit  ( unsigned int bit ) { return ( bit  &  (unsigned int)(BITS_PER_STORAGE - 1) ); }

public:
	CFlagField() { memset( storage, 0x00, sizeof(storage) ); }
	void SetBit( unsigned int bit ) {          storage[ Field( bit ) ] |= (uint32_t)DL_BIT( Bit( bit ) ); }
	bool IsSet ( unsigned int bit ) { return ( storage[ Field( bit ) ] &  (uint32_t)DL_BIT( Bit( bit ) ) ) != 0; }
};

struct SDLPackState
{
	SDLPackState()
		: state( (dl_pack_state)0 )
		, value( 0x0 )
		, struct_start_pos( 0 )
		, array_count_patch_pos( 0 )
		, array_count( 0 )
		, is_back_array(false)
	{}

	SDLPackState( dl_pack_state pack_state , const void* value = 0x0, uintptr_t struct_start_pos = 0 )
		: state( pack_state )
		, value( value )
		, struct_start_pos( struct_start_pos )
		, array_count_patch_pos( 0 )
		, array_count( 0 )
		, is_back_array(false)
	{}

	dl_pack_state   state;

	union
	{
		const void*           value;
		const dl_type_desc*   type;
		const dl_member_desc* member;
		const dl_enum_desc*   enum_type;
	};

	uintptr_t       struct_start_pos;
	uintptr_t       array_count_patch_pos;
	uint32_t        array_count;
	bool            is_back_array;
	CFlagField<128> members_set;
};

struct SDLPackContext
{
	dl_binary_writer*   writer;
	dl_ctx_t            dl_ctx;
	const dl_type_desc* root_type;
	dl_error_t          error_code;
	bool                data_section_found;

	dl_pack_state CurrentPackState() { return state_stack.Top().state; }

	// positions where we have only an ID that referre to subdata!
	struct
	{
		unsigned int     id;
		uintptr_t        write_pos;
		const dl_member_desc* member;
	} patch_pos[128];
	unsigned int patch_pos_count;

	CStackStatic<SDLPackState, 128> state_stack;

	unsigned int curr_subdata_elem;
	uintptr_t subdata_elems_pos[128]; // positions for different subdata-elements;
};

static void dl_txt_pack_ctx_push_state( SDLPackContext* pack_ctx, dl_pack_state state )
{
	DL_ASSERT( state != DL_PACK_STATE_STRUCT && "Please use dl_txt_pack_ctx_push_state_struct" );
	pack_ctx->state_stack.Push( state );
}

static void dl_txt_pack_ctx_push_state_struct( SDLPackContext* pack_ctx, const dl_type_desc* type )
{
	uintptr_t struct_start_pos = 0;

	if( pack_ctx->state_stack.Top().state == DL_PACK_STATE_ARRAY && 
		pack_ctx->state_stack.Top().is_back_array )
		struct_start_pos = dl_binary_writer_push_back_alloc( pack_ctx->writer, type->size[DL_PTR_SIZE_HOST] );
	else
		struct_start_pos = dl_internal_align_up( dl_binary_writer_tell( pack_ctx->writer ), type->alignment[DL_PTR_SIZE_HOST]);

	pack_ctx->state_stack.Push( SDLPackState(DL_PACK_STATE_STRUCT, type, struct_start_pos) );
}

static void dl_txt_pack_ctx_push_state_enum( SDLPackContext* pack_ctx, const dl_enum_desc* enum_type )
{
	pack_ctx->state_stack.Push( SDLPackState(DL_PACK_STATE_ENUM, enum_type) );
}

static void dl_txt_pack_ctx_push_state_member( SDLPackContext* pack_ctx, const dl_member_desc* member )
{
	pack_ctx->state_stack.Push( SDLPackState( dl_pack_state( member->type ), 0x0 ) );
}

static void dl_txt_pack_ctx_push_state_ptr_member( SDLPackContext* pack_ctx, const dl_member_desc* member )
{
	pack_ctx->state_stack.Push( SDLPackState( DL_PACK_STATE_SUBDATA_ID, member ) );
}

static void dl_txt_pack_ctx_pop_state( struct SDLPackContext* pack_ctx )
{
	pack_ctx->state_stack.Pop();
}

static void dl_txt_pack_ctx_pop_array_item( struct SDLPackContext* pack_ctx )
{
	SDLPackState& old_state = pack_ctx->state_stack.Top();

	pack_ctx->state_stack.Pop();

	if( old_state.state == DL_PACK_STATE_STRUCT )
	{
		// end should be just after the current struct. but we might not be there since
		// members could have been written in an order that is not according to type.
		dl_binary_writer_seek_set( pack_ctx->writer, old_state.struct_start_pos + old_state.type->size[DL_PTR_SIZE_HOST] );
	}

	if( pack_ctx->state_stack.Top().state == DL_PACK_STATE_ARRAY )
	{
		pack_ctx->state_stack.Top().array_count++;

		switch(old_state.state)
		{
			case DL_PACK_STATE_STRUCT: dl_txt_pack_ctx_push_state_struct( pack_ctx, old_state.type );    break;
			case DL_PACK_STATE_ENUM:   dl_txt_pack_ctx_push_state_enum( pack_ctx, old_state.enum_type ); break;
			default:                   dl_txt_pack_ctx_push_state( pack_ctx, old_state.state);           break;
		}
	}
}

static void dl_txt_pack_ctx_add_patch_pos( SDLPackContext* pack_ctx, unsigned int id )
{
	unsigned int pp_index = pack_ctx->patch_pos_count++;
	DL_ASSERT( pp_index < DL_ARRAY_LENGTH(pack_ctx->patch_pos) );

	pack_ctx->patch_pos[pp_index].id        = id;
	pack_ctx->patch_pos[pp_index].write_pos = dl_binary_writer_tell( pack_ctx->writer );
	pack_ctx->patch_pos[pp_index].member    = pack_ctx->state_stack.Top().member;
}

static void dl_txt_pack_ctx_register_sub_element( SDLPackContext* pack_ctx, unsigned int elem, uintptr_t pos )
{
	DL_ASSERT( pack_ctx->subdata_elems_pos[elem] == (uintptr_t)-1 && "Subdata element already registered!");
	pack_ctx->subdata_elems_pos[elem] = pos;
	pack_ctx->curr_subdata_elem = elem;
}

static void dl_txt_pack_ctx_init( SDLPackContext* pack_ctx )
{
	pack_ctx->root_type = 0x0;
	pack_ctx->error_code = DL_ERROR_OK;
	pack_ctx->patch_pos_count = 0;
	
	dl_txt_pack_ctx_push_state( pack_ctx, DL_PACK_STATE_INSTANCE );
	memset( pack_ctx->subdata_elems_pos, 0xFF, sizeof(pack_ctx->subdata_elems_pos) );
	pack_ctx->subdata_elems_pos[0] = 0; // element 1 is root, and it has position 0
}


static int dl_internal_pack_on_null( void* pack_ctx_in )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;
	dl_pack_state state = pack_ctx->CurrentPackState();
	DL_ASSERT( state == DL_PACK_STATE_SUBDATA_ID || state == DL_PACK_STATE_STRING || state == DL_PACK_STATE_STRING_ARRAY );
	if( state == DL_PACK_STATE_STRING_ARRAY )
	{
		uintptr_t array_elem_pos = dl_binary_writer_push_back_alloc( pack_ctx->writer, sizeof( char* ) );
		dl_binary_writer_seek_set( pack_ctx->writer, array_elem_pos );
	}

	dl_binary_writer_write_ptr( pack_ctx->writer, (uintptr_t)-1 );
	dl_txt_pack_ctx_pop_array_item( pack_ctx );
	return 1;
}

static int dl_internal_pack_on_bool( void* pack_ctx_in, int value )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;
	dl_pack_state state = pack_ctx->CurrentPackState();

	if( ( state & DL_TYPE_ATOM_MASK ) == DL_TYPE_ATOM_BITFIELD )
	{
		unsigned int bf_bits   = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED );
		unsigned int bf_offset = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED );

		dl_type_t storage_type = dl_type_t( state & DL_TYPE_STORAGE_MASK );

		switch( storage_type )
		{
			case DL_TYPE_STORAGE_UINT8:
				dl_binary_writer_write_uint8 ( pack_ctx->writer,
											   (uint8_t)DL_INSERT_BITS( dl_binary_writer_read_uint8( pack_ctx->writer ),
											   (uint8_t)value,
											   dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint8_t), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT16:
				dl_binary_writer_write_uint16( pack_ctx->writer,
											   (uint16_t)DL_INSERT_BITS( dl_binary_writer_read_uint16( pack_ctx->writer ),
											   (uint16_t)value,
											   dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint16_t), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT32:
				dl_binary_writer_write_uint32( pack_ctx->writer,
											   (uint32_t)DL_INSERT_BITS( dl_binary_writer_read_uint32( pack_ctx->writer ),
											   (uint32_t)value,
											   dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint32_t), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT64:
				dl_binary_writer_write_uint64( pack_ctx->writer,
											   (uint64_t)DL_INSERT_BITS( dl_binary_writer_read_uint64( pack_ctx->writer ),
											   (uint64_t)value,
											   dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint64_t), bf_offset, bf_bits ),
											   bf_bits ) );
			break;

			default:
				DL_PACK_ERROR_AND_FAIL( pack_ctx, DL_ERROR_TXT_PARSE_ERROR, "Invalid format, unexpected bool!" );
		}

		dl_txt_pack_ctx_pop_state( pack_ctx );

		return 1;
	}

	switch(state)
	{
		case DL_PACK_STATE_POD_INT8:   dl_binary_writer_write_int8  ( pack_ctx->writer,   (int8_t)value ); break;
		case DL_PACK_STATE_POD_INT16:  dl_binary_writer_write_int16 ( pack_ctx->writer,  (int16_t)value ); break;
		case DL_PACK_STATE_POD_INT32:  dl_binary_writer_write_int32 ( pack_ctx->writer,  (int32_t)value ); break;
		case DL_PACK_STATE_POD_INT64:  dl_binary_writer_write_int64 ( pack_ctx->writer,  (int64_t)value ); break;
		case DL_PACK_STATE_POD_UINT8:  dl_binary_writer_write_uint8 ( pack_ctx->writer,  (uint8_t)value ); break;
		case DL_PACK_STATE_POD_UINT16: dl_binary_writer_write_uint16( pack_ctx->writer, (uint16_t)value ); break;
		case DL_PACK_STATE_POD_UINT32: dl_binary_writer_write_uint32( pack_ctx->writer, (uint32_t)value ); break;
		case DL_PACK_STATE_POD_UINT64: dl_binary_writer_write_uint64( pack_ctx->writer, (uint64_t)value ); break;
		default:
			DL_PACK_ERROR_AND_FAIL( pack_ctx, DL_ERROR_TXT_PARSE_ERROR, "true/false only supported on int*, uint* or bitfield!" );
			break;
	}

	dl_txt_pack_ctx_pop_array_item( pack_ctx );
	return 1;
}

template<typename T> DL_FORCEINLINE bool Between(T _Val, T _Min, T _Max) { return _Val <= _Max && _Val >= _Min; }

static int dl_internal_pack_on_number( void* pack_ctx_in, const char* str_val, size_t str_len )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;

	union { int64_t sign; uint64_t unsign; } Min;
	union { int64_t sign; uint64_t unsign; } Max;
	const char* fmt = "";

	dl_pack_state state = pack_ctx->CurrentPackState();

	if( (state & DL_TYPE_ATOM_MASK) == DL_TYPE_ATOM_BITFIELD )
	{
		uint64_t bf_value;
		DL_PACK_ERROR_AND_FAIL_IF( sscanf( str_val, DL_UINT64_FMT_STR, &bf_value ) != 1, 
								   pack_ctx, 
								   DL_ERROR_TXT_PARSE_ERROR, 
								   "Could not parse %.*s as bitfield member!", 
								   (int)str_len, str_val );

		unsigned int bf_bits   = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED );
		unsigned int bf_offset = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED );

		uint64_t max_val = (uint64_t(1) << bf_bits) - uint64_t(1);
		DL_PACK_ERROR_AND_FAIL_IF( bf_value > max_val, 
								   pack_ctx, 
								   DL_ERROR_TXT_PARSE_ERROR, 
								   "Value " DL_UINT64_FMT_STR" will not fit in a bitfield with %u bits!", 
								   bf_value, bf_bits  );

		dl_type_t storage_type = dl_type_t( state & DL_TYPE_STORAGE_MASK );

		switch( storage_type )
		{
			case DL_TYPE_STORAGE_UINT8:
				dl_binary_writer_write_uint8 ( pack_ctx->writer,
											   (uint8_t)DL_INSERT_BITS( dl_binary_writer_read_uint8( pack_ctx->writer ),
																	    (uint8_t)bf_value,
																	    dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint8_t), bf_offset, bf_bits ), bf_bits ) );
				break;
			case DL_TYPE_STORAGE_UINT16:
				dl_binary_writer_write_uint16( pack_ctx->writer,
											   (uint16_t)DL_INSERT_BITS( dl_binary_writer_read_uint8( pack_ctx->writer ),
																	     (uint16_t)bf_value,
																	     dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint16_t), bf_offset, bf_bits ), bf_bits ) );
				break;
			case DL_TYPE_STORAGE_UINT32:
				dl_binary_writer_write_uint32( pack_ctx->writer,
											   (uint32_t)DL_INSERT_BITS( dl_binary_writer_read_uint32( pack_ctx->writer ),
																	     (uint32_t)bf_value,
																	     dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint32_t), bf_offset, bf_bits ), bf_bits ) );
				break;
			case DL_TYPE_STORAGE_UINT64:
				dl_binary_writer_write_uint64( pack_ctx->writer,
											   (uint64_t)DL_INSERT_BITS( dl_binary_writer_read_uint64( pack_ctx->writer ),
																	     (uint64_t)bf_value,
																	     dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint64_t), bf_offset, bf_bits ), bf_bits ) );
				break;

			default:
				DL_ASSERT( false && "This should not happen!" );
				break;
		}

		dl_txt_pack_ctx_pop_state( pack_ctx );

		return 1;
	}

	switch( state )
	{
		case DL_PACK_STATE_POD_INT8:   Min.sign   = (int64_t)INT8_MIN;   Max.sign   =  (int64_t)INT8_MAX;   fmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT16:  Min.sign   = (int64_t)INT16_MIN;  Max.sign   =  (int64_t)INT16_MAX;  fmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT32:  Min.sign   = (int64_t)INT32_MIN;  Max.sign   =  (int64_t)INT32_MAX;  fmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT64:  Min.sign   = (int64_t)INT64_MIN;  Max.sign   =  (int64_t)INT64_MAX;  fmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT8:  Min.unsign = 0;                   Max.unsign = (uint64_t)UINT8_MAX;  fmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT16: Min.unsign = 0;                   Max.unsign = (uint64_t)UINT16_MAX; fmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT32: Min.unsign = 0;                   Max.unsign = (uint64_t)UINT32_MAX; fmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT64: Min.unsign = 0;                   Max.unsign = (uint64_t)UINT64_MAX; fmt = DL_UINT64_FMT_STR; break;

		case DL_PACK_STATE_POD_FP32:   dl_binary_writer_write_fp32( pack_ctx->writer, (float)atof(str_val) ); dl_txt_pack_ctx_pop_array_item( pack_ctx ); return 1;
		case DL_PACK_STATE_POD_FP64:   dl_binary_writer_write_fp64( pack_ctx->writer,        atof(str_val) ); dl_txt_pack_ctx_pop_array_item( pack_ctx ); return 1;

		case DL_PACK_STATE_SUBDATA_ID:
		{
			uint32_t ID;
			DL_PACK_ERROR_AND_FAIL_IF( sscanf( str_val, "%u", &ID ) != 1, 
									   pack_ctx, 
									   DL_ERROR_TXT_PARSE_ERROR, 
									   "Could not parse %.*s as correct ID!", 
									   (int)str_len, str_val );

			dl_txt_pack_ctx_add_patch_pos( pack_ctx, (unsigned int)ID );

			// this is the pos that will be patched, but we need to make room for the array now!
			dl_binary_writer_write_zero( pack_ctx->writer, pack_ctx->patch_pos[ pack_ctx->patch_pos_count - 1 ].member->size[DL_PTR_SIZE_HOST] );

			dl_txt_pack_ctx_pop_state( pack_ctx );
		}
		return 1;

		default:
			DL_PACK_ERROR_AND_FAIL( pack_ctx, DL_ERROR_TXT_PARSE_ERROR, "Invalid format, unexpected number!" );
	}

	union { int64_t sign; uint64_t unsign; } value;
	DL_PACK_ERROR_AND_FAIL_IF( sscanf( str_val, fmt, &value.sign ) != 1, 
							   pack_ctx, 
							   DL_ERROR_TXT_PARSE_ERROR, 
							   "Could not parse %.*s as correct integer type!", 
							   (int)str_len, str_val );

	switch( state )
	{
		case DL_PACK_STATE_POD_INT8:
		case DL_PACK_STATE_POD_INT16:
		case DL_PACK_STATE_POD_INT32:
		case DL_PACK_STATE_POD_INT64:
			DL_PACK_ERROR_AND_FAIL_IF( !Between( value.sign, Min.sign, Max.sign ), 
										pack_ctx, 
										DL_ERROR_TXT_PARSE_ERROR, 
										DL_INT64_FMT_STR " will not fit in type", 
										value.sign );
			break;
		case DL_PACK_STATE_POD_UINT8:
		case DL_PACK_STATE_POD_UINT16:
		case DL_PACK_STATE_POD_UINT32:
		case DL_PACK_STATE_POD_UINT64:
			DL_PACK_ERROR_AND_FAIL_IF( !Between( value.unsign, Min.unsign, Max.unsign ), 
										pack_ctx, 
										DL_ERROR_TXT_PARSE_ERROR, 
										DL_UINT64_FMT_STR " will not fit in type", 
										value.unsign );
			break;
		default:
			break;
	}

	switch( state )
	{
		case DL_PACK_STATE_POD_INT8:   dl_binary_writer_write_int8  ( pack_ctx->writer,   (int8_t)value.sign );   break;
		case DL_PACK_STATE_POD_INT16:  dl_binary_writer_write_int16 ( pack_ctx->writer,  (int16_t)value.sign );   break;
		case DL_PACK_STATE_POD_INT32:  dl_binary_writer_write_int32 ( pack_ctx->writer,  (int32_t)value.sign );   break;
		case DL_PACK_STATE_POD_INT64:  dl_binary_writer_write_int64 ( pack_ctx->writer,  (int64_t)value.sign );   break;
		case DL_PACK_STATE_POD_UINT8:  dl_binary_writer_write_uint8 ( pack_ctx->writer,  (uint8_t)value.unsign ); break;
		case DL_PACK_STATE_POD_UINT16: dl_binary_writer_write_uint16( pack_ctx->writer, (uint16_t)value.unsign ); break;
		case DL_PACK_STATE_POD_UINT32: dl_binary_writer_write_uint32( pack_ctx->writer, (uint32_t)value.unsign ); break;
		case DL_PACK_STATE_POD_UINT64: dl_binary_writer_write_uint64( pack_ctx->writer, (uint64_t)value.unsign ); break;
		default: break;
	}

	dl_txt_pack_ctx_pop_array_item( pack_ctx );

	return 1;
}

static int dl_internal_pack_on_string( void* pack_ctx_in, const unsigned char* str_value, size_t str_len )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;

	dl_pack_state State = pack_ctx->CurrentPackState();
	switch(State)
	{
		case DL_PACK_STATE_INSTANCE_TYPE:
		{
			DL_ASSERT( pack_ctx->root_type == 0x0 );
			// we are packing an instance, get the type plox!

			char type_name[32] = { 0x0 };
			strncpy( type_name, (const char*)str_value, str_len );
			pack_ctx->root_type = dl_internal_find_type_by_name( pack_ctx->dl_ctx, type_name );

			DL_PACK_ERROR_AND_FAIL_IF( pack_ctx->root_type == 0x0, 
									   pack_ctx, 
									   DL_ERROR_TYPE_NOT_FOUND, 
									   "Could not find type %.*s in loaded types!", 
									   (int)str_len, str_value );

			dl_txt_pack_ctx_pop_state( pack_ctx ); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_STRING_ARRAY:
		{
			dl_binary_writer_seek_end( pack_ctx->writer );
			uintptr_t offset = dl_binary_writer_tell( pack_ctx->writer );
			dl_binary_writer_write_string( pack_ctx->writer, str_value, str_len );
			uintptr_t array_elem_pos = dl_binary_writer_push_back_alloc( pack_ctx->writer, sizeof( char* ) );
			dl_binary_writer_seek_set( pack_ctx->writer, array_elem_pos );
			dl_binary_writer_write( pack_ctx->writer, &offset, sizeof(uintptr_t) );

			dl_txt_pack_ctx_pop_array_item( pack_ctx ); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_STRING:
		{
			uintptr_t curr = dl_binary_writer_tell( pack_ctx->writer );
			dl_binary_writer_seek_end( pack_ctx->writer );
			uintptr_t offset = dl_binary_writer_tell( pack_ctx->writer );
			dl_binary_writer_write_string( pack_ctx->writer, str_value, str_len );
			dl_binary_writer_seek_set( pack_ctx->writer, curr );
			dl_binary_writer_write( pack_ctx->writer, &offset, sizeof(uintptr_t) );
			dl_txt_pack_ctx_pop_array_item( pack_ctx ); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_ENUM:
		{
			uint32_t enum_value;
			const dl_enum_desc* enum_type = pack_ctx->state_stack.Top().enum_type;
			if( !dl_internal_find_enum_value( pack_ctx->dl_ctx, enum_type, (const char*)str_value, str_len, &enum_value ) )
			{
				DL_PACK_ERROR_AND_FAIL( pack_ctx,
										DL_ERROR_TXT_INVALID_ENUM_VALUE, 
										"Enum \"%s\" do not have the value \"%.*s\"!", 
										dl_internal_enum_name( pack_ctx->dl_ctx, enum_type ), (int)str_len, str_value );
			}
			dl_binary_writer_write( pack_ctx->writer, &enum_value, sizeof(uint32_t) );
			dl_txt_pack_ctx_pop_array_item( pack_ctx );
		}
		break;
		default:
			DL_PACK_ERROR_AND_FAIL( pack_ctx, 
									DL_ERROR_TXT_PARSE_ERROR, 
									"Unexpected string \"%.*s\"!",
									(int)str_len, str_value );
			break;
	}

	return 1;
}

// TODO: This function is not optimal on any part and should be removed, store this info in the member as one bit!
static bool dl_internal_has_sub_ptr( dl_ctx_t dl_ctx, dl_typeid_t type_id )
{
	const dl_type_desc* type = dl_internal_find_type( dl_ctx, type_id );
	if(type == 0) // not sturct
		return false;

	for(unsigned int i = 0; i < type->member_count; ++i)
	{
		dl_type_t st = dl_get_type_member( dl_ctx, type, i )->StorageType();
		dl_type_t at = dl_get_type_member( dl_ctx, type, i )->AtomType();
		if( st == DL_TYPE_STORAGE_STR || at == DL_TYPE_ATOM_ARRAY )
			return true;
	}
	return false;
}

static int dl_internal_pack_on_map_key( void* pack_ctx, const unsigned char* str_val, size_t str_len )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	switch( pCtx->CurrentPackState() )
	{
		case DL_PACK_STATE_INSTANCE:
			DL_PACK_ERROR_AND_FAIL_IF( str_len != 4 && str_len != 7, 
									   pCtx, 
									   DL_ERROR_TXT_PARSE_ERROR, 
									   "Got key \"%.*s\", expected \"type\" or \"data\"!", 
									   (int)str_len, str_val );

			if( strncmp( (const char*)str_val, "type", str_len ) == 0 )
			{
				DL_PACK_ERROR_AND_FAIL_IF( pCtx->root_type != 0x0, 
										   pCtx, 
										   DL_ERROR_TXT_PARSE_ERROR, 
										   "Type for root-instance set two times!" );
				dl_txt_pack_ctx_push_state( pCtx, DL_PACK_STATE_INSTANCE_TYPE );
			}
			else if( strncmp( (const char*)str_val, "data", str_len ) == 0 )
			{
				DL_PACK_ERROR_AND_FAIL_IF( pCtx->root_type == 0x0, 
					                       pCtx, 
					                       DL_ERROR_TXT_PARSE_ERROR, 
					                       "Type for root-instance not set or after data-segment!" );
				DL_PACK_ERROR_AND_FAIL_IF( pCtx->data_section_found,
										   pCtx,
										   DL_ERROR_TXT_MULTIPLE_DATA_SECTIONS,
										   "Multiple \"data\" sections in text-data." );

				pCtx->data_section_found = true;
				dl_txt_pack_ctx_push_state_struct( pCtx, pCtx->root_type );
			}
			else if( strncmp( (const char*)str_val, "subdata", str_len ) == 0 )
			{
				dl_txt_pack_ctx_push_state( pCtx, DL_PACK_STATE_SUBDATA );
			}
			else
				DL_PACK_ERROR_AND_FAIL( pCtx, 
										DL_ERROR_TXT_PARSE_ERROR, 
										"Got key \"%.*s\", expected \"type\", \"data\" or \"subdata\"!", 
										(int)str_len, str_val );
		break;

		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& state = pCtx->state_stack.Top();

			unsigned int member_id = dl_internal_find_member( pCtx->dl_ctx, state.type, dl_internal_hash_buffer(str_val, str_len) );

			DL_PACK_ERROR_AND_FAIL_IF( member_id > state.type->member_count, 
									   pCtx, 
									   DL_ERROR_MEMBER_NOT_FOUND, 
									   "Type \"%s\" has no member named \"%.*s\"!", 
									   dl_internal_type_name( pCtx->dl_ctx, state.type ),
									   (int)str_len, str_val );

			const dl_member_desc* member = dl_get_type_member( pCtx->dl_ctx, state.type, member_id );

			DL_PACK_ERROR_AND_FAIL_IF( state.members_set.IsSet( member_id ), 
									   pCtx, 
									   DL_ERROR_TXT_MEMBER_SET_TWICE, 
									   "Trying to set Member \"%.*s\" twice!", 
									   (int)str_len, str_val );

			state.members_set.SetBit( member_id );

			// seek to position for member! ( members can come in any order in the text-file so we need to seek )
			uintptr_t mem_pos = pCtx->state_stack.Top().struct_start_pos + member->offset[DL_PTR_SIZE_HOST];
			dl_binary_writer_seek_set( pCtx->writer, mem_pos );
			DL_ASSERT( dl_binary_writer_in_back_alloc_area( pCtx->writer ) || dl_internal_is_align((void*)mem_pos, member->alignment[DL_PTR_SIZE_HOST]) );

			uintptr_t array_count_patch_pos = 0; // for inline array, keep as 0.
			bool      array_has_sub_ptrs    = false;

			dl_type_t atom_type    = member->AtomType();
			dl_type_t storage_type = member->StorageType();

			switch( atom_type )
			{
				case DL_TYPE_ATOM_POD:
				{
					switch( storage_type )
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const dl_type_desc* pSubType = dl_internal_find_type( pCtx->dl_ctx, member->type_id );
							DL_PACK_ERROR_AND_FAIL_IF( pSubType == 0x0, 
													   pCtx, 
													   DL_ERROR_TYPE_NOT_FOUND, 
													   "Type of member \"%s\" not in type library!", 
													   dl_internal_member_name( pCtx->dl_ctx, member ) );
							dl_txt_pack_ctx_push_state_struct( pCtx, pSubType );
						}
						break;
						case DL_TYPE_STORAGE_PTR:  dl_txt_pack_ctx_push_state_ptr_member( pCtx, member ); break;
						case DL_TYPE_STORAGE_ENUM: dl_txt_pack_ctx_push_state_enum( pCtx, dl_internal_find_enum(pCtx->dl_ctx, member->type_id) ); break;
						default:
							DL_ASSERT(member->IsSimplePod() || storage_type == DL_TYPE_STORAGE_STR);
							dl_txt_pack_ctx_push_state( pCtx, dl_pack_state( storage_type ) );
							break;
					}
				}
				break;
				case DL_TYPE_ATOM_ARRAY:
				{
					// calc alignemnt needed for array
					uintptr_t array_align = dl_internal_align_of_type( pCtx->dl_ctx, member->type, member->type_id, DL_PTR_SIZE_HOST );
					dl_binary_writer_needed_size_align_up( pCtx->writer, array_align );

					// save position for array to be able to write ptr and count on array-end.
					dl_binary_writer_write_pint( pCtx->writer, dl_binary_writer_needed_size( pCtx->writer ) );
					array_count_patch_pos = dl_binary_writer_tell( pCtx->writer );
					dl_binary_writer_write_uint32( pCtx->writer, 0 ); // count need to be patched later!
					dl_binary_writer_seek_end( pCtx->writer );

					if( storage_type == DL_TYPE_STORAGE_STR )
					{
						dl_txt_pack_ctx_push_state( pCtx, DL_PACK_STATE_ARRAY );
						pCtx->state_stack.Top().array_count_patch_pos = array_count_patch_pos;
						pCtx->state_stack.Top().is_back_array        = true;

						dl_txt_pack_ctx_push_state( pCtx, DL_PACK_STATE_STRING_ARRAY );
						break;
					}

					array_has_sub_ptrs = dl_internal_has_sub_ptr(pCtx->dl_ctx, member->type_id); // TODO: Optimize this, store info in type-data
				}
				case DL_TYPE_ATOM_INLINE_ARRAY: // FALLTHROUGH
				{
					dl_txt_pack_ctx_push_state( pCtx, DL_PACK_STATE_ARRAY );
					pCtx->state_stack.Top().array_count_patch_pos = array_count_patch_pos;
					pCtx->state_stack.Top().is_back_array         = array_has_sub_ptrs;

					switch( storage_type )
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const dl_type_desc* pSubType = dl_internal_find_type(pCtx->dl_ctx, member->type_id);
							DL_PACK_ERROR_AND_FAIL_IF( pSubType == 0x0, 
													   pCtx, 
													   DL_ERROR_TYPE_NOT_FOUND, 
													   "Type of array \"%s\" not in type library!", 
													   dl_internal_member_name( pCtx->dl_ctx, member ) );
							dl_txt_pack_ctx_push_state_struct( pCtx, pSubType );
						}
						break;
						case DL_TYPE_STORAGE_ENUM:
						{
							const dl_enum_desc* the_enum = dl_internal_find_enum( pCtx->dl_ctx, member->type_id );
							DL_PACK_ERROR_AND_FAIL_IF( the_enum == 0x0, 
								                       pCtx, 
								                       DL_ERROR_TYPE_NOT_FOUND, 
								                       "Enum-type of array \"%s\" not in type library!", 
								                       dl_internal_member_name( pCtx->dl_ctx, member ) );
							dl_txt_pack_ctx_push_state_enum( pCtx, the_enum );
						}
						break;
						default: // default is a standard pod-type
							DL_ASSERT( member->IsSimplePod() || storage_type == DL_TYPE_STORAGE_STR );
							dl_txt_pack_ctx_push_state( pCtx, dl_pack_state( storage_type ) );
							break;
					}
				}
				break;
				case DL_TYPE_ATOM_BITFIELD:
					dl_txt_pack_ctx_push_state_member( pCtx, member );
				break;
				default:
					DL_ASSERT(false && "Invalid ATOM-type!");
			}
		}
		break;

		case DL_PACK_STATE_SUBDATA:
		{
			// found a subdata item! the map-key need to be a id!

			uint32_t ID;
			DL_PACK_ERROR_AND_FAIL_IF( sscanf((char*)str_val, "%u", &ID) != 1, 
									   pCtx, 
									   DL_ERROR_TXT_PARSE_ERROR, 
									   "Could not parse %.*s as correct ID!", 
									   (int)str_len, str_val );

			const dl_member_desc* member = 0x0;

			// check that we have referenced this before so we know its type!
			for( unsigned int patch_pos = 0; patch_pos < pCtx->patch_pos_count; ++patch_pos )
				if(pCtx->patch_pos[patch_pos].id == ID)
				{
					member = pCtx->patch_pos[patch_pos].member;
					break;
				}

			DL_PACK_ERROR_AND_FAIL_IF( member == 0, 
									   pCtx,
									   DL_ERROR_TXT_PARSE_ERROR, 
									   "An item with id %u has not been encountered in the earlier part of the document, hence the type could not be deduced!", 
									   ID );

			dl_type_t atom_type    = member->AtomType();
			dl_type_t storage_type = member->StorageType();

			DL_ASSERT( atom_type    == DL_TYPE_ATOM_POD );
			DL_ASSERT( storage_type == DL_TYPE_STORAGE_PTR );

			const dl_type_desc* sub_type = dl_internal_find_type( pCtx->dl_ctx, member->type_id );
			DL_PACK_ERROR_AND_FAIL_IF( sub_type == 0x0, 
									   pCtx, 
									   DL_ERROR_TYPE_NOT_FOUND, 
									   "Type of ptr \"%s\" not in type library!",
									   dl_internal_member_name( pCtx->dl_ctx, member ) );
			dl_binary_writer_seek_end( pCtx->writer );
			dl_binary_writer_align( pCtx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );
			dl_txt_pack_ctx_push_state_struct( pCtx, sub_type );

			// register this element as it will be written here!
			dl_txt_pack_ctx_register_sub_element( pCtx, ID, dl_binary_writer_tell( pCtx->writer ) );
		}
		break;

		default:
			DL_ASSERT(false && "This should not happen!");
			break;
	}

	return 1;
}

static int dl_internal_pack_on_map_start( void* pack_ctx )
{
	// check that we are in a correct state here!
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	switch( pCtx->CurrentPackState() )
	{
		case DL_PACK_STATE_SUBDATA:
		case DL_PACK_STATE_INSTANCE:
			break;
		case DL_PACK_STATE_STRUCT:
			if( dl_binary_writer_in_back_alloc_area( pCtx->writer ) )
				dl_binary_writer_seek_end( pCtx->writer );
			else
				dl_binary_writer_reserve( pCtx->writer, pCtx->state_stack.Top().type->size[DL_PTR_SIZE_HOST] );
			break;
		default:
			DL_PACK_ERROR_AND_FAIL( pCtx, DL_ERROR_TXT_PARSE_ERROR, "Did not expect map-open here!");
			break;
	}
	return 1;
}

static void dl_internal_txt_pack_finalize( SDLPackContext* pack_ctx )
{
	// patch subdata
	for(unsigned int patch_pos = 0; patch_pos < pack_ctx->patch_pos_count; ++patch_pos)
	{
		unsigned int id      = pack_ctx->patch_pos[patch_pos].id;
		uintptr_t member_pos = pack_ctx->patch_pos[patch_pos].write_pos;

		dl_binary_writer_seek_set( pack_ctx->writer, member_pos );
		dl_binary_writer_write_pint( pack_ctx->writer, pack_ctx->subdata_elems_pos[id] );
	}
}

static int dl_internal_pack_on_map_end( void* pack_ctx_in )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;
	dl_binary_writer* writer = pack_ctx->writer;

	if( pack_ctx->state_stack.Len() == 1 ) // end of top-instance!
	{
		dl_internal_txt_pack_finalize( pack_ctx );
		return 1;
	}

	dl_pack_state curr_pack_state = pack_ctx->CurrentPackState();
	switch( curr_pack_state )
	{
		case DL_PACK_STATE_SUBDATA:
		case DL_PACK_STATE_INSTANCE: dl_txt_pack_ctx_pop_state( pack_ctx ); break;
		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& state = pack_ctx->state_stack.Top();

			// Check that all members are set!
			for( uint32_t member_index = 0; member_index < state.type->member_count; ++member_index )
			{
				const dl_member_desc* member = dl_get_type_member( pack_ctx->dl_ctx, state.type, member_index );

				if( state.members_set.IsSet( member_index ) )
					continue;

				DL_PACK_ERROR_AND_FAIL_IF( member->default_value_offset == UINT32_MAX,
										   pack_ctx, 
										   DL_ERROR_TXT_MEMBER_MISSING, 
										   "Member \"%s\" in struct of type \"%s\" not set!", 
										   dl_internal_member_name( pack_ctx->dl_ctx, member ),
										   dl_internal_type_name( pack_ctx->dl_ctx, state.type ) );

				uintptr_t mem_pos  = state.struct_start_pos + member->offset[DL_PTR_SIZE_HOST];
				uint8_t*  member_default_value = pack_ctx->dl_ctx->default_data + member->default_value_offset;

				uint32_t member_size = member->size[DL_PTR_SIZE_HOST];
				uint8_t* subdata = member_default_value + member_size;

				dl_binary_writer_seek_set( writer, mem_pos );
				dl_binary_writer_write( writer, member_default_value, member->size[DL_PTR_SIZE_HOST] );

				if( member_size != member->default_value_size )
				{
					// ... sub ptrs, copy and patch ...
					dl_binary_writer_seek_end( writer );
					uintptr_t subdata_pos = dl_binary_writer_tell( writer );

					dl_binary_writer_write( writer, subdata, member->default_value_size - member_size );

					uint8_t* member_data = writer->data + mem_pos;
					if( !writer->dummy )
						dl_internal_patch_member( pack_ctx->dl_ctx, member, member_data, (uintptr_t)writer->data, subdata_pos - member_size );
				}
			}

			dl_txt_pack_ctx_pop_array_item( pack_ctx );
		}
		break;
		default:
			DL_ASSERT(false && "This should not happen!");
			break;
	}
	return 1;
}

static int dl_internal_pack_on_array_start( void* pack_ctx_in )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;
	// TODO: this check is really not something to be proud of, but solving the problem in a good way
	//       is harder to do than would be motivated since this file is up for a rewrite!
	//       See issue: https://github.com/wc-duck/datalibrary/issues/15
	if( pack_ctx->state_stack.Len() < 2 )
		return 0;

	dl_pack_state state = pack_ctx->state_stack.m_Data[pack_ctx->state_stack.Len() - 2].state;
	switch( state )
	{
		case DL_PACK_STATE_ARRAY:
		case DL_PACK_STATE_STRING_ARRAY:
			break;
		default:
			return 0;
	}
	return 1;
}

static int dl_internal_pack_on_array_end( void* pack_ctx_in )
{
	SDLPackContext* pack_ctx = (SDLPackContext*)pack_ctx_in;

	const dl_type_desc* type = pack_ctx->state_stack.Top().type;
	dl_pack_state pack_state = pack_ctx->state_stack.Top().state;

	dl_txt_pack_ctx_pop_state( pack_ctx ); // pop of pack state for sub-type
	uint32_t array_count   = pack_ctx->state_stack.Top().array_count;
	uintptr_t patch_pos     = pack_ctx->state_stack.Top().array_count_patch_pos;
	bool     is_back_array = pack_ctx->state_stack.Top().is_back_array;
	dl_txt_pack_ctx_pop_state( pack_ctx ); // pop of pack state for array
	if( patch_pos != 0 ) // not inline array
	{
		if( array_count == 0 )
		{
			// empty array should be null!
			dl_binary_writer_seek_set( pack_ctx->writer, patch_pos - sizeof(uintptr_t) );
			dl_binary_writer_write_pint( pack_ctx->writer, (uintptr_t)-1 );
		}
		else if(is_back_array)
		{
			uint32_t size  = sizeof( char* );
			uint32_t align = sizeof( char* );

			if( type != 0x0 )
			{
				size  = type->size[DL_PTR_SIZE_HOST];
				align = type->alignment[DL_PTR_SIZE_HOST];
			}

			// TODO: HACKZOR!
			// When packing arrays of structs one element to much in pushed in the back-alloc.
			if( pack_state != DL_PACK_STATE_STRING_ARRAY )
				pack_ctx->writer->back_alloc_pos += size;

			uintptr_t array_pos = dl_binary_writer_pop_back_alloc( pack_ctx->writer, array_count, size, align );
			dl_binary_writer_seek_set( pack_ctx->writer, patch_pos - sizeof(uintptr_t) );
			dl_binary_writer_write_pint( pack_ctx->writer, array_pos );
		}
		else
			dl_binary_writer_seek_set( pack_ctx->writer, patch_pos );

		dl_binary_writer_write_uint32( pack_ctx->writer, array_count );
	}

	return 1;
}

// TODO: These allocs need to be controllable!!!
void* dl_internal_pack_alloc  ( void* ctx, size_t sz )            { (void)ctx; return malloc(sz); }
void* dl_internal_pack_realloc( void* ctx, void* ptr, size_t sz ) { (void)ctx; return realloc(ptr, sz); }
void  dl_internal_pack_free   ( void* ctx, void* ptr )            { (void)ctx; free(ptr); }

static dl_error_t dl_internal_txt_pack( SDLPackContext* pack_ctx, const char* text_data )
{
	// this could be incremental later on if needed!

	size_t txt_len = strlen( text_data );
	const unsigned char* txt_data = (const unsigned char*)text_data;

	yajl_alloc_funcs my_yajl_alloc = {
		dl_internal_pack_alloc,
		dl_internal_pack_realloc,
		dl_internal_pack_free,
		0x0,
	};

	yajl_callbacks callbacks = {
		dl_internal_pack_on_null,
		dl_internal_pack_on_bool,
		NULL, // integer,
		NULL, // double,
		dl_internal_pack_on_number,
		dl_internal_pack_on_string,
		dl_internal_pack_on_map_start,
		dl_internal_pack_on_map_key,
		dl_internal_pack_on_map_end,
		dl_internal_pack_on_array_start,
		dl_internal_pack_on_array_end
	};

	yajl_handle my_yajl_handle = yajl_alloc( &callbacks, &my_yajl_alloc, pack_ctx );
	yajl_config( my_yajl_handle, yajl_allow_comments, 1 );

	yajl_status my_yajl_status = yajl_parse( my_yajl_handle, txt_data, (unsigned int)txt_len ); // read file data, pass to parser

	size_t bytes_consumed = yajl_get_bytes_consumed( my_yajl_handle );

	my_yajl_status = yajl_complete_parse( my_yajl_handle ); // parse any remaining buffered data

	if( my_yajl_status != yajl_status_ok )
	{
		if( bytes_consumed != 0 ) // error occured!
		{
			unsigned int line = 1;
			unsigned int column = 0;

			const char* ch  = text_data;
			const char* end = text_data + bytes_consumed;
 
			while( ch != end )
			{
				if( *ch == '\n' )
				{
					++line;
					column = 0;
				}
				else
					++column;

				++ch;
			}

			dl_log_error( pack_ctx->dl_ctx, "At line %u, column %u", line, column);
		}

		char* error_str = (char*)yajl_get_error( my_yajl_handle, 1 /* verbose */, txt_data, (unsigned int)txt_len );
		dl_log_error( pack_ctx->dl_ctx, "%s", error_str );
		yajl_free_error( my_yajl_handle, (unsigned char*)error_str );

		if( pack_ctx->error_code == DL_ERROR_OK )
		{
			yajl_free( my_yajl_handle );
			return DL_ERROR_TXT_PARSE_ERROR;
		}
	}

	if( pack_ctx->root_type == 0x0 )
	{
		dl_log_error( pack_ctx->dl_ctx, "Missing root-element in dl-data" );
		pack_ctx->error_code = DL_ERROR_TXT_PARSE_ERROR;
	}

	yajl_free( my_yajl_handle );
	return pack_ctx->error_code;
}

dl_error_t dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_buffer, size_t out_buffer_size, size_t* produced_bytes )
{
	const bool IS_DUMMY_WRITER = out_buffer_size == 0;
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_buffer + sizeof(dl_data_header), out_buffer_size - sizeof(dl_data_header), IS_DUMMY_WRITER,
						   DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );

	SDLPackContext pack_ctx;
	dl_txt_pack_ctx_init( &pack_ctx );
	pack_ctx.dl_ctx = dl_ctx;
	pack_ctx.writer = &writer;
	pack_ctx.data_section_found = false;

	dl_error_t error = dl_internal_txt_pack( &pack_ctx, txt_instance );

	if(error != DL_ERROR_OK)
		return error;

	if(!pack_ctx.data_section_found)
		return DL_ERROR_TXT_DATA_SECTION_MISSING;

	// write header
	if( out_buffer_size > 0 )
	{
		dl_data_header header;
		header.id                 = DL_INSTANCE_ID;
		header.version            = DL_INSTANCE_VERSION;
		header.root_instance_type = dl_internal_typeid_of( dl_ctx, pack_ctx.root_type );
		header.instance_size      = (uint32_t)dl_binary_writer_needed_size( &writer );
		header.is_64_bit_ptr      = sizeof(void*) == 8 ? 1 : 0;
		memcpy( out_buffer, &header, sizeof(dl_data_header) );
	}

	dl_binary_writer_finalize( &writer );

	if( produced_bytes )
		*produced_bytes = (unsigned int)dl_binary_writer_needed_size( &writer ) + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

dl_error_t dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, size_t* out_instance_size )
{
	return dl_txt_pack( dl_ctx, txt_instance, 0x0, 0, out_instance_size );
}
