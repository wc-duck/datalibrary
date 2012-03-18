/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_txt.h>
#include "dl_types.h"
#include "dl_binary_writer.h"

#include "container/dl_array.h"
#include "container/dl_stack.h"

#include <yajl/yajl_parse.h>

#include <stdlib.h>

/*
	PACKING OF ARRAYS, How its done!

	if the elements of the array is not containing subarrays ( or other variable size data-types if they ever is added )
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

enum EDLPackState
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

#define DL_PACK_ERROR_AND_FAIL( dl_ctx, err, fmt, ... ) { dl_log_error( dl_ctx, fmt, ##__VA_ARGS__ ); pCtx->error_code = err; return 0x0; }
#define DL_PACK_ERROR_AND_FAIL_IF( cond, dl_ctx, err, fmt, ... ) if( cond ) DL_PACK_ERROR_AND_FAIL( dl_ctx, err, fmt, ##__VA_ARGS__ )

template<unsigned int TBits>
class CFlagField
{
	DL_STATIC_ASSERT(TBits % 32 == 0, only_even_32_bits);
	uint32 storage[TBits / 32];

	enum
	{
		BITS_PER_STORAGE = sizeof(uint32) * 8,
		BITS_FOR_FIELD   = 5
	};

	unsigned int Field(unsigned int _Bit) { return (_Bit & ~(BITS_PER_STORAGE - 1)) >> BITS_FOR_FIELD; }
	unsigned int Bit  (unsigned int _Bit) { return (_Bit &  (BITS_PER_STORAGE - 1)); }

public:
	CFlagField()            { ClearAll(); }
	CFlagField(bool _AllOn) { memset(storage, _AllOn ? 0xFF : 0x00, sizeof(storage)); }

	~CFlagField() {}

	void SetBit  (unsigned int _Bit) { storage[Field(_Bit)] |=  DL_BIT(Bit(_Bit)); }
	void ClearBit(unsigned int _Bit) { storage[Field(_Bit)] &= ~DL_BIT(Bit(_Bit)); }
	void FlipBit (unsigned int _Bit) { storage[Field(_Bit)] ^=  DL_BIT(Bit(_Bit)); }

	void SetAll()   { memset(storage, 0xFF, sizeof(storage)); }
	void ClearAll() { memset(storage, 0x00, sizeof(storage)); }

	bool IsSet(unsigned int _Bit) { return (storage[Field(_Bit)] & DL_BIT(Bit(_Bit))) != 0; }
};

struct SDLPackState
{
	SDLPackState() {}

	SDLPackState(EDLPackState _State, const void* _pValue = 0x0, pint _StructStartPos = 0)
		: state(_State)
		, value(_pValue)
		, struct_start_pos(_StructStartPos)
		, array_count(0)
	{}

	EDLPackState   state;

	union
	{
		const void*      value;
		const SDLType*   type;
		const SDLMember* member;
		const SDLEnum*   enum_type;
	};

	pint            struct_start_pos;
	pint            array_count_patch_pos;
	uint32          array_count;
	bool            is_back_array;
	CFlagField<128> members_set;
};

struct SDLPackContext
{
	SDLPackContext() : root_type(0x0), error_code(DL_ERROR_OK), patch_pos_count(0)
	{
		PushState(DL_PACK_STATE_INSTANCE);
		memset( subdata_elems_pos, 0xFF, sizeof(subdata_elems_pos) );
		subdata_elems_pos[0] = 0; // element 1 is root, and it has position 0
	}

	dl_binary_writer* writer;
	dl_ctx_t          dl_ctx;
	const SDLType*    root_type;
	dl_error_t        error_code;

	void PushStructState(const SDLType* _pType)
	{
		pint struct_start_pos = 0;

		if( state_stack.Top().state == DL_PACK_STATE_ARRAY && state_stack.Top().is_back_array )
			struct_start_pos = dl_binary_writer_push_back_alloc( writer, _pType->size[DL_PTR_SIZE_HOST] );
		else
			struct_start_pos = dl_internal_align_up( dl_binary_writer_tell( writer ), _pType->alignment[DL_PTR_SIZE_HOST]);

		state_stack.Push( SDLPackState(DL_PACK_STATE_STRUCT, _pType, struct_start_pos) );
	}

	void PushEnumState( const SDLEnum* member )
	{
		state_stack.Push( SDLPackState(DL_PACK_STATE_ENUM, member) );
	}

	void PushMemberState( EDLPackState state, const SDLMember* member )
	{
		state_stack.Push( SDLPackState(state, member) );
	}

	void PushState( EDLPackState state ) 
	{
		DL_ASSERT( state != DL_PACK_STATE_STRUCT && "Please use PushStructState()" );
		state_stack.Push( state );
	}

	void PopState() { state_stack.Pop(); }

	void ArrayItemPop()
	{
		SDLPackState& old_state = state_stack.Top();

		state_stack.Pop();

		if( old_state.state == DL_PACK_STATE_STRUCT )
		{
			// end should be just after the current struct. but we might not be there since
			// members could have been written in an order that is not according to type.
			dl_binary_writer_seek_set( writer, old_state.struct_start_pos + old_state.type->size[DL_PTR_SIZE_HOST] );
		}

		if( state_stack.Top().state == DL_PACK_STATE_ARRAY )
		{
			state_stack.Top().array_count++;

			switch(old_state.state)
			{
				case DL_PACK_STATE_STRUCT: PushStructState(old_state.type);    break;
				case DL_PACK_STATE_ENUM:   PushEnumState(old_state.enum_type); break;
				default:                   PushState(old_state.state);         break;
			}
		}
	}

	EDLPackState CurrentPackState() { return state_stack.Top().state; }

	// positions where we have only an ID that referre to subdata!
	struct
	{
		unsigned int     id;
		pint             write_pos;
		const SDLMember* member;
	}            patch_pos[128];
	unsigned int patch_pos_count;

	void AddPatchPosition( unsigned int id )
	{
		unsigned int pp_index = patch_pos_count++;
		DL_ASSERT( pp_index < DL_ARRAY_LENGTH(patch_pos) );

		patch_pos[pp_index].id        = id;
		patch_pos[pp_index].write_pos = dl_binary_writer_tell( writer );
		patch_pos[pp_index].member    = state_stack.Top().member;
	}

	void RegisterSubdataElement(unsigned int elem, pint pos)
	{
		DL_ASSERT(subdata_elems_pos[elem] == pint(-1) && "Subdata element already registered!");
		subdata_elems_pos[elem] = pos;
		curr_subdata_elem = elem;
	}

	CStackStatic<SDLPackState, 128> state_stack;

	unsigned int curr_subdata_elem;
	pint subdata_elems_pos[128]; // positions for different subdata-elements;
};

static int dl_internal_pack_on_null( void* pack_ctx )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;
	DL_ASSERT(pCtx->CurrentPackState() == DL_PACK_STATE_SUBDATA_ID);
	dl_binary_writer_write_ptr( pCtx->writer, (pint)-1 );
	pCtx->PopState();
	return 1;
}

static int dl_internal_pack_on_bool( void* pack_ctx, int value )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;
	EDLPackState state = pCtx->CurrentPackState();

	if( ( state & DL_TYPE_ATOM_MASK ) == DL_TYPE_ATOM_BITFIELD )
	{
		unsigned int bf_bits   = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED );
		unsigned int bf_offset = DL_EXTRACT_BITS( state, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED );

		dl_type_t storage_type = dl_type_t( state & DL_TYPE_STORAGE_MASK );

		switch( storage_type )
		{
			case DL_TYPE_STORAGE_UINT8:
				dl_binary_writer_write_uint8 ( pCtx->writer,
											   (uint8)DL_INSERT_BITS( dl_binary_writer_read_uint8( pCtx->writer ),
											   uint8( value ),
											   DLBitFieldOffset( sizeof(uint8), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT16:
				dl_binary_writer_write_uint16( pCtx->writer,
											   (uint16)DL_INSERT_BITS( dl_binary_writer_read_uint16( pCtx->writer ),
											   uint16( value ),
											   DLBitFieldOffset( sizeof(uint16), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT32:
				dl_binary_writer_write_uint32( pCtx->writer,
											   (uint32)DL_INSERT_BITS( dl_binary_writer_read_uint32( pCtx->writer ),
											   uint32( value ),
											   DLBitFieldOffset( sizeof(uint32), bf_offset, bf_bits ),
											   bf_bits ) );
			break;
			case DL_TYPE_STORAGE_UINT64:
				dl_binary_writer_write_uint64( pCtx->writer,
											   (uint64)DL_INSERT_BITS( dl_binary_writer_read_uint64( pCtx->writer ),
											   uint64( value ),
											   DLBitFieldOffset( sizeof(uint64), bf_offset, bf_bits ),
											   bf_bits ) );
			break;

			default:
				DL_ASSERT(false && "This should not happen!");
				break;
		}

		pCtx->PopState();

		return 1;
	}

	switch(state)
	{
		case DL_PACK_STATE_POD_INT8:   dl_binary_writer_write_int8  ( pCtx->writer,   (int8)value ); break;
		case DL_PACK_STATE_POD_INT16:  dl_binary_writer_write_int16 ( pCtx->writer,  (int16)value ); break;
		case DL_PACK_STATE_POD_INT32:  dl_binary_writer_write_int32 ( pCtx->writer,  (int32)value ); break;
		case DL_PACK_STATE_POD_INT64:  dl_binary_writer_write_int64 ( pCtx->writer,  (int64)value ); break;
		case DL_PACK_STATE_POD_UINT8:  dl_binary_writer_write_uint8 ( pCtx->writer,  (uint8)value ); break;
		case DL_PACK_STATE_POD_UINT16: dl_binary_writer_write_uint16( pCtx->writer, (uint16)value ); break;
		case DL_PACK_STATE_POD_UINT32: dl_binary_writer_write_uint32( pCtx->writer, (uint32)value ); break;
		case DL_PACK_STATE_POD_UINT64: dl_binary_writer_write_uint64( pCtx->writer, (uint64)value ); break;
		default:
			DL_PACK_ERROR_AND_FAIL( pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "true/false only supported on int*, uint* or bitfield!" );
			break;
	}

	pCtx->ArrayItemPop();
	return 1;
}

template<typename T> DL_FORCEINLINE bool Between(T _Val, T _Min, T _Max) { return _Val <= _Max && _Val >= _Min; }

static int dl_internal_pack_on_number( void* pack_ctx, const char* str_val, unsigned int str_len )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	union { int64 m_Signed; uint64 m_Unsigned; } Min;
	union { int64 m_Signed; uint64 m_Unsigned; } Max;
	const char* pFmt = "";

	EDLPackState State = pCtx->CurrentPackState();

	if((State & DL_TYPE_ATOM_MASK) == DL_TYPE_ATOM_BITFIELD)
	{
		uint64 Val;
		DL_PACK_ERROR_AND_FAIL_IF( sscanf(str_val, DL_UINT64_FMT_STR, &Val) != 1, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as bitfield member!", str_len, str_val);

		unsigned int BFBits   = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED);
		unsigned int BFOffset = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED);

		uint64 MaxVal = (uint64(1) << BFBits) - uint64(1);
		DL_PACK_ERROR_AND_FAIL_IF( Val > MaxVal, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Value " DL_UINT64_FMT_STR" will not fit in a bitfield with %u bits!", Val, BFBits);

		dl_type_t StorageType = dl_type_t(State & DL_TYPE_STORAGE_MASK);

		switch(StorageType)
		{
			case DL_TYPE_STORAGE_UINT8:
				dl_binary_writer_write_uint8 ( pCtx->writer,
											   (uint8)DL_INSERT_BITS( dl_binary_writer_read_uint8( pCtx->writer ),
											   uint8(Val),
											   DLBitFieldOffset( sizeof(uint8), BFOffset, BFBits), BFBits ) );
				break;
			case DL_TYPE_STORAGE_UINT16:
				dl_binary_writer_write_uint16( pCtx->writer,
											   (uint16)DL_INSERT_BITS( dl_binary_writer_read_uint8( pCtx->writer ),
											   uint16(Val),
											   DLBitFieldOffset( sizeof(uint16), BFOffset, BFBits), BFBits ) );
				break;
			case DL_TYPE_STORAGE_UINT32:
				dl_binary_writer_write_uint32( pCtx->writer,
											   (uint32)DL_INSERT_BITS( dl_binary_writer_read_uint32( pCtx->writer ),
											   uint32(Val),
											   DLBitFieldOffset( sizeof(uint32), BFOffset, BFBits), BFBits ) );
				break;
			case DL_TYPE_STORAGE_UINT64:
				dl_binary_writer_write_uint64( pCtx->writer,
											   (uint64)DL_INSERT_BITS( dl_binary_writer_read_uint64( pCtx->writer ),
											   uint64(Val),
											   DLBitFieldOffset( sizeof(uint64), BFOffset, BFBits), BFBits ) );
				break;

			default:
				DL_ASSERT(false && "This should not happen!");
				break;
		}

		pCtx->PopState();

		return 1;
	}

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:   Min.m_Signed  =   int64(DL_INT8_MIN);   Max.m_Signed   =  int64(DL_INT8_MAX);   pFmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT16:  Min.m_Signed  =   int64(DL_INT16_MIN);  Max.m_Signed   =  int64(DL_INT16_MAX);  pFmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT32:  Min.m_Signed  =   int64(DL_INT32_MIN);  Max.m_Signed   =  int64(DL_INT32_MAX);  pFmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_INT64:  Min.m_Signed  =   int64(DL_INT64_MIN);  Max.m_Signed   =  int64(DL_INT64_MAX);  pFmt = DL_INT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT8:  Min.m_Unsigned = uint64(DL_UINT8_MIN);  Max.m_Unsigned = uint64(DL_UINT8_MAX);  pFmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT16: Min.m_Unsigned = uint64(DL_UINT16_MIN); Max.m_Unsigned = uint64(DL_UINT16_MAX); pFmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT32: Min.m_Unsigned = uint64(DL_UINT32_MIN); Max.m_Unsigned = uint64(DL_UINT32_MAX); pFmt = DL_UINT64_FMT_STR; break;
		case DL_PACK_STATE_POD_UINT64: Min.m_Unsigned = uint64(DL_UINT64_MIN); Max.m_Unsigned = uint64(DL_UINT64_MAX); pFmt = DL_UINT64_FMT_STR; break;

		case DL_PACK_STATE_POD_FP32:   dl_binary_writer_write_fp32( pCtx->writer, (float)atof(str_val) ); pCtx->ArrayItemPop(); return 1;
		case DL_PACK_STATE_POD_FP64:   dl_binary_writer_write_fp64( pCtx->writer,        atof(str_val) ); pCtx->ArrayItemPop(); return 1;

		case DL_PACK_STATE_SUBDATA_ID:
		{
			uint32 ID;
			DL_PACK_ERROR_AND_FAIL_IF( sscanf(str_val, "%u", &ID) != 1, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", str_len, str_val);

			pCtx->AddPatchPosition( (unsigned int)ID );

			// this is the pos that will be patched, but we need to make room for the array now!
			dl_binary_writer_write_zero( pCtx->writer, pCtx->patch_pos[ pCtx->patch_pos_count - 1 ].member->size[DL_PTR_SIZE_HOST] );

			pCtx->PopState();
		}
		return 1;

		default:
			DL_ASSERT(false && "This should not happen!");
			return 0;
	}

	union { int64 m_Signed; uint64 m_Unsigned; } Val;
	DL_PACK_ERROR_AND_FAIL_IF( sscanf(str_val, pFmt, &Val.m_Signed) != 1, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct integer type!", str_len, str_val);

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:
		case DL_PACK_STATE_POD_INT16:
		case DL_PACK_STATE_POD_INT32:
		case DL_PACK_STATE_POD_INT64:
			DL_PACK_ERROR_AND_FAIL_IF( !Between(Val.m_Signed, Min.m_Signed, Max.m_Signed), pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, DL_INT64_FMT_STR " will not fit in type", Val.m_Signed);
			break;
		case DL_PACK_STATE_POD_UINT8:
		case DL_PACK_STATE_POD_UINT16:
		case DL_PACK_STATE_POD_UINT32:
		case DL_PACK_STATE_POD_UINT64:
			DL_PACK_ERROR_AND_FAIL_IF( !Between(Val.m_Unsigned, Min.m_Unsigned, Max.m_Unsigned), pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, DL_UINT64_FMT_STR " will not fit in type", Val.m_Unsigned);
			break;
		default:
			break;
	}

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:   dl_binary_writer_write_int8  ( pCtx->writer,   (int8)Val.m_Signed );   break;
		case DL_PACK_STATE_POD_INT16:  dl_binary_writer_write_int16 ( pCtx->writer,  (int16)Val.m_Signed );   break;
		case DL_PACK_STATE_POD_INT32:  dl_binary_writer_write_int32 ( pCtx->writer,  (int32)Val.m_Signed );   break;
		case DL_PACK_STATE_POD_INT64:  dl_binary_writer_write_int64 ( pCtx->writer,  (int64)Val.m_Signed );   break;
		case DL_PACK_STATE_POD_UINT8:  dl_binary_writer_write_uint8 ( pCtx->writer,  (uint8)Val.m_Unsigned ); break;
		case DL_PACK_STATE_POD_UINT16: dl_binary_writer_write_uint16( pCtx->writer, (uint16)Val.m_Unsigned ); break;
		case DL_PACK_STATE_POD_UINT32: dl_binary_writer_write_uint32( pCtx->writer, (uint32)Val.m_Unsigned ); break;
		case DL_PACK_STATE_POD_UINT64: dl_binary_writer_write_uint64( pCtx->writer, (uint64)Val.m_Unsigned ); break;
		default: break;
	}

	pCtx->ArrayItemPop();

	return 1;
}

static int dl_internal_pack_on_string( void* pack_ctx, const unsigned char* str_value, unsigned int str_len )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	EDLPackState State = pCtx->CurrentPackState();
	switch(State)
	{
		case DL_PACK_STATE_INSTANCE_TYPE:
		{
			DL_ASSERT(pCtx->root_type == 0x0);
			// we are packing an instance, get the type plox!

			char type_name[DL_MEMBER_NAME_MAX_LEN] = { 0x0 };
			strncpy( type_name, (const char*)str_value, str_len );
			pCtx->root_type = dl_internal_find_type_by_name( pCtx->dl_ctx, type_name );

			DL_PACK_ERROR_AND_FAIL_IF( pCtx->root_type == 0x0, pCtx->dl_ctx, DL_ERROR_TYPE_NOT_FOUND, "Could not find type %.*s in loaded types!", str_len, str_value);

			pCtx->PopState(); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_STRING_ARRAY:
		{
			dl_binary_writer_seek_end( pCtx->writer );
			pint offset = dl_binary_writer_tell( pCtx->writer );
			dl_binary_writer_write_string( pCtx->writer, str_value, str_len );
			pint array_elem_pos = dl_binary_writer_push_back_alloc( pCtx->writer, sizeof( char* ) );
			dl_binary_writer_seek_set( pCtx->writer, array_elem_pos );
			dl_binary_writer_write( pCtx->writer, &offset, sizeof(pint) );

			pCtx->ArrayItemPop(); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_STRING:
		{
			pint curr = dl_binary_writer_tell( pCtx->writer );
			dl_binary_writer_seek_end( pCtx->writer );
			pint offset = dl_binary_writer_tell( pCtx->writer );
			dl_binary_writer_write_string( pCtx->writer, str_value, str_len );
			dl_binary_writer_seek_set( pCtx->writer, curr );
			dl_binary_writer_write( pCtx->writer, &offset, sizeof(pint) );
			pCtx->ArrayItemPop(); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_ENUM:
		{
			uint32 enum_value;
			const SDLEnum* enum_type = pCtx->state_stack.Top().enum_type;
			if( !dl_internal_find_enum_value( enum_type, (const char*)str_value, str_len, &enum_value ) )
				DL_PACK_ERROR_AND_FAIL( pCtx->dl_ctx, DL_ERROR_TXT_INVALID_ENUM_VALUE, "Enum \"%s\" do not have the value \"%.*s\"!", enum_type->name, str_len, str_value);
			dl_binary_writer_write( pCtx->writer, &enum_value, sizeof(uint32) );
			pCtx->ArrayItemPop();
		}
		break;
		default:
			DL_PACK_ERROR_AND_FAIL( pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Unexpected string \"%.*s\"!", str_len, str_value);
			break;
	}

	return 1;
}

// TODO: This function is not optimal on any part and should be removed, store this info in the member as one bit!
static bool dl_internal_has_sub_ptr( dl_ctx_t dl_ctx, dl_typeid_t type )
{
	const SDLType* le_type = dl_internal_find_type( dl_ctx, type );

	if(le_type == 0) // not sturct
		return false;

	for(unsigned int i = 0; i < le_type->member_count; ++i)
	{
		dl_type_t st = le_type->members[i].StorageType();
		dl_type_t at = le_type->members[i].AtomType();
		if( st == DL_TYPE_STORAGE_STR || at == DL_TYPE_ATOM_ARRAY )
			return true;
	}

	return false;
}

static int dl_internal_pack_on_map_key( void* pack_ctx, const unsigned char* str_val, unsigned int str_len )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	switch(pCtx->CurrentPackState())
	{
		case DL_PACK_STATE_INSTANCE:
			DL_PACK_ERROR_AND_FAIL_IF( str_len != 4 && str_len != 7, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\" or \"data\"!", str_len, str_val);

			if(strncmp((const char*)str_val, "type", str_len) == 0)
			{
				DL_PACK_ERROR_AND_FAIL_IF( pCtx->root_type != 0x0, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance set two times!");
				pCtx->PushState(DL_PACK_STATE_INSTANCE_TYPE);
			}
			else if (strncmp((const char*)str_val, "data", str_len) == 0)
			{
				DL_PACK_ERROR_AND_FAIL_IF( pCtx->root_type == 0x0, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance not set or after data-segment!");
				pCtx->PushStructState(pCtx->root_type);
			}
			else if (strncmp((const char*)str_val, "subdata", str_len) == 0)
			{
				pCtx->PushState(DL_PACK_STATE_SUBDATA);
			}
			else
				DL_PACK_ERROR_AND_FAIL( pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\", \"data\" or \"subdata\"!", str_len, str_val);
		break;

		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& State = pCtx->state_stack.Top();

			unsigned int MemberID = dl_internal_find_member( State.type, dl_internal_hash_buffer(str_val, str_len, 0) );

			DL_PACK_ERROR_AND_FAIL_IF( MemberID > State.type->member_count, pCtx->dl_ctx, DL_ERROR_MEMBER_NOT_FOUND, "Type \"%s\" has no member named \"%.*s\"!", State.type->name, str_len, str_val);

			const SDLMember* pMember = State.type->members + MemberID;

			DL_PACK_ERROR_AND_FAIL_IF( State.members_set.IsSet(MemberID), pCtx->dl_ctx, DL_ERROR_TXT_MEMBER_SET_TWICE, "Trying to set Member \"%.*s\" twice!", str_len, str_val );

			State.members_set.SetBit(MemberID);

			// seek to position for member! ( members can come in any order in the text-file so we need to seek )
			pint MemberPos = pCtx->state_stack.Top().struct_start_pos + pMember->offset[DL_PTR_SIZE_HOST];
			dl_binary_writer_seek_set( pCtx->writer, MemberPos );
			DL_ASSERT( dl_binary_writer_in_back_alloc_area( pCtx->writer ) || dl_internal_is_align((void*)MemberPos, pMember->alignment[DL_PTR_SIZE_HOST]) );

			pint array_count_patch_pos = 0; // for inline array, keep as 0.
			bool array_has_sub_ptrs    = false;

			dl_type_t AtomType    = pMember->AtomType();
			dl_type_t StorageType = pMember->StorageType();

			switch(AtomType)
			{
				case DL_TYPE_ATOM_POD:
				{
					switch(StorageType)
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const SDLType* pSubType = dl_internal_find_type(pCtx->dl_ctx, pMember->type_id);
							DL_PACK_ERROR_AND_FAIL_IF( pSubType == 0x0, pCtx->dl_ctx, DL_ERROR_TYPE_NOT_FOUND, "Type of member \"%s\" not in type library!", pMember->name);
							pCtx->PushStructState(pSubType); 
						}
						break;
						case DL_TYPE_STORAGE_PTR:  pCtx->PushMemberState(DL_PACK_STATE_SUBDATA_ID, pMember); break;
						case DL_TYPE_STORAGE_ENUM: pCtx->PushEnumState(dl_internal_find_enum(pCtx->dl_ctx, pMember->type_id)); break;
						default:
							DL_ASSERT(pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_STR);
							pCtx->PushState(EDLPackState(StorageType));
							break;
					}
				}
				break;
				case DL_TYPE_ATOM_ARRAY:
				{
					// calc alignemnt needed for array
					pint array_align = dl_internal_align_of_type( pCtx->dl_ctx, pMember->type, pMember->type_id, DL_PTR_SIZE_HOST );
					dl_binary_writer_needed_size_align_up( pCtx->writer, array_align );

					// save position for array to be able to write ptr and count on array-end.
					dl_binary_writer_write_pint( pCtx->writer, dl_binary_writer_needed_size( pCtx->writer ) );
					array_count_patch_pos = dl_binary_writer_tell( pCtx->writer );
					dl_binary_writer_write_uint32( pCtx->writer, 0 ); // count need to be patched later!
					dl_binary_writer_seek_end( pCtx->writer );

					if( StorageType == DL_TYPE_STORAGE_STR )
					{
						pCtx->PushState(DL_PACK_STATE_ARRAY);
						pCtx->state_stack.Top().array_count_patch_pos = array_count_patch_pos;
						pCtx->state_stack.Top().is_back_array        = true;

						pCtx->PushState( DL_PACK_STATE_STRING_ARRAY );
						break;
					}

					array_has_sub_ptrs = dl_internal_has_sub_ptr(pCtx->dl_ctx, pMember->type_id); // TODO: Optimize this, store info in type-data
				}
				case DL_TYPE_ATOM_INLINE_ARRAY: // FALLTHROUGH
				{
					pCtx->PushState(DL_PACK_STATE_ARRAY);
					pCtx->state_stack.Top().array_count_patch_pos = array_count_patch_pos;
					pCtx->state_stack.Top().is_back_array         = array_has_sub_ptrs;

					switch(StorageType)
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const SDLType* pSubType = dl_internal_find_type(pCtx->dl_ctx, pMember->type_id);
							DL_PACK_ERROR_AND_FAIL_IF( pSubType == 0x0, pCtx->dl_ctx, DL_ERROR_TYPE_NOT_FOUND, "Type of array \"%s\" not in type library!", pMember->name );
							pCtx->PushStructState(pSubType);
						}
						break;
						case DL_TYPE_STORAGE_ENUM:
						{
							const SDLEnum* pEnum = dl_internal_find_enum(pCtx->dl_ctx, pMember->type_id);
							DL_PACK_ERROR_AND_FAIL_IF( pEnum == 0x0, pCtx->dl_ctx, DL_ERROR_TYPE_NOT_FOUND, "Enum-type of array \"%s\" not in type library!", pMember->name);
							pCtx->PushEnumState(pEnum);
						}
						break;
						default: // default is a standard pod-type
							DL_ASSERT(pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_STR);
							pCtx->PushState(EDLPackState(StorageType));
							break;
					}
				}
				break;
				case DL_TYPE_ATOM_BITFIELD:
					pCtx->PushMemberState(EDLPackState(pMember->type), 0x0);
				break;
				default:
					DL_ASSERT(false && "Invalid ATOM-type!");
			}
		}
		break;

		case DL_PACK_STATE_SUBDATA:
		{
			// found a subdata item! the map-key need to be a id!

			uint32 ID;
			DL_PACK_ERROR_AND_FAIL_IF( sscanf((char*)str_val, "%u", &ID) != 1, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", str_len, str_val);

			const SDLMember* pMember = 0x0;

			// check that we have referenced this before so we know its type!
			for (unsigned int iPatchPos = 0; iPatchPos < pCtx->patch_pos_count; ++iPatchPos)
				if(pCtx->patch_pos[iPatchPos].id == ID)
				{
					pMember = pCtx->patch_pos[iPatchPos].member;
					break;
				}

			DL_PACK_ERROR_AND_FAIL_IF( pMember == 0, pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "An item with id %u has not been encountered in the earlier part of the document, hence the type could not be deduced!", ID);

			dl_type_t AtomType = pMember->AtomType();
			dl_type_t StorageType = pMember->StorageType();

			DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
			DL_ASSERT(StorageType == DL_TYPE_STORAGE_PTR);
			const SDLType* pSubType = dl_internal_find_type(pCtx->dl_ctx, pMember->type_id);
			DL_PACK_ERROR_AND_FAIL_IF( pSubType == 0x0, pCtx->dl_ctx, DL_ERROR_TYPE_NOT_FOUND, "Type of ptr \"%s\" not in type library!", pMember->name);
			dl_binary_writer_seek_end( pCtx->writer );
			dl_binary_writer_align( pCtx->writer, pSubType->alignment[DL_PTR_SIZE_HOST] );
			pCtx->PushStructState(pSubType);

			// register this element as it will be written here!
			pCtx->RegisterSubdataElement(ID, dl_binary_writer_tell( pCtx->writer ) );
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
			DL_PACK_ERROR_AND_FAIL( pCtx->dl_ctx, DL_ERROR_TXT_PARSE_ERROR, "Did not expect map-open here!");
			break;
	}
	return 1;
}

static void dl_internal_txt_pack_finalize( SDLPackContext* pack_ctx )
{
	// patch subdata
	for(unsigned int patch_pos = 0; patch_pos < pack_ctx->patch_pos_count; ++patch_pos)
	{
		unsigned int id = pack_ctx->patch_pos[patch_pos].id;
		pint member_pos = pack_ctx->patch_pos[patch_pos].write_pos;

		dl_binary_writer_seek_set( pack_ctx->writer, member_pos );
		dl_binary_writer_write_pint( pack_ctx->writer, pack_ctx->subdata_elems_pos[id] );
	}
}

static int dl_internal_pack_on_map_end( void* pack_ctx )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	if(pCtx->state_stack.Len() == 1) // end of top-instance!
	{
		dl_internal_txt_pack_finalize( pCtx );
		return 1;
	}

	EDLPackState State = pCtx->CurrentPackState();
	switch(State)
	{
		case DL_PACK_STATE_SUBDATA:
		case DL_PACK_STATE_INSTANCE: pCtx->PopState();     break;
		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& PackState = pCtx->state_stack.Top();

			// Check that all members are set!
			for (uint32 iMember = 0; iMember < PackState.type->member_count; ++iMember)
			{
				const SDLMember* pMember = PackState.type->members + iMember;

				if(!PackState.members_set.IsSet(iMember))
				{
					DL_PACK_ERROR_AND_FAIL_IF( pMember->default_value_offset == DL_UINT32_MAX, 
											   pCtx->dl_ctx, 
											   DL_ERROR_TXT_MEMBER_MISSING, 
											   "Member \"%s\" in struct of type \"%s\" not set!", 
											   pMember->name, 
											   PackState.type->name );

					pint   MemberPos  = PackState.struct_start_pos + pMember->offset[DL_PTR_SIZE_HOST];
					uint8* pDefMember = pCtx->dl_ctx->default_data + pMember->default_value_offset;

					dl_binary_writer_seek_set( pCtx->writer, MemberPos );

					dl_type_t AtomType    = pMember->AtomType();
					dl_type_t StorageType = pMember->StorageType();

					switch(AtomType)
					{
						case DL_TYPE_ATOM_POD:
						{
							switch(StorageType)
							{
								case DL_TYPE_STORAGE_STR:
								{
									dl_binary_writer_seek_end( pCtx->writer );
									pint str_pos = dl_binary_writer_tell( pCtx->writer );
									char* str = *(char**)pDefMember;
									dl_binary_writer_write_string( pCtx->writer, str, strlen( str ) );
									dl_binary_writer_seek_set( pCtx->writer, MemberPos );
									dl_binary_writer_write_pint( pCtx->writer, str_pos );
								}
								break;
								case DL_TYPE_STORAGE_PTR: dl_binary_writer_write_pint( pCtx->writer, (pint)-1 ); break; // can only default to null!
								default:
									DL_ASSERT( pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM );
									dl_binary_writer_write( pCtx->writer, pDefMember, pMember->size[DL_PTR_SIZE_HOST] );
							}
						}
						break;
						case DL_TYPE_ATOM_INLINE_ARRAY:
						{
							switch(StorageType)
							{
								case DL_TYPE_STORAGE_STR:
								{
									char** pArray = (char**)pDefMember;

									uint32 Count = pMember->size[DL_PTR_SIZE_HOST] / sizeof(char*);
									for(uint32 iElem = 0; iElem < Count; ++iElem)
									{
										dl_binary_writer_seek_end( pCtx->writer );
										pint str_pos = dl_binary_writer_tell( pCtx->writer );
										dl_binary_writer_write_string( pCtx->writer, pArray[iElem], strlen( pArray[iElem] ) );
										dl_binary_writer_seek_set( pCtx->writer, MemberPos + sizeof(char*) * iElem );
										dl_binary_writer_write_pint( pCtx->writer, str_pos );
									}
								}
								break;
								default:
									DL_ASSERT( pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM );
									dl_binary_writer_write( pCtx->writer, pDefMember, pMember->size[DL_PTR_SIZE_HOST] );
							}
						}
						break;
						case DL_TYPE_ATOM_ARRAY:
						{
							uint32 Count = *(uint32*)(pDefMember + sizeof(void*));

							dl_binary_writer_write_pint( pCtx->writer, dl_binary_writer_needed_size( pCtx->writer ) );
							dl_binary_writer_write_uint32( pCtx->writer, Count);
							dl_binary_writer_seek_end( pCtx->writer );

							switch(StorageType)
							{
								case DL_TYPE_STORAGE_STR:
								{

									pint ArrayPos = dl_binary_writer_tell( pCtx->writer );
									dl_binary_writer_reserve( pCtx->writer, Count * sizeof(char*) );

									char** pArray = *(char***)pDefMember;
									for(uint32 iElem = 0; iElem < Count; ++iElem)
									{
										dl_binary_writer_seek_end( pCtx->writer );
										pint str_pos = dl_binary_writer_tell( pCtx->writer );
										dl_binary_writer_write_string( pCtx->writer, pArray[iElem], strlen( pArray[iElem] ) );
										dl_binary_writer_seek_set( pCtx->writer, ArrayPos + sizeof(char*) * iElem );
										dl_binary_writer_write_pint( pCtx->writer, str_pos );
									}
								}
								break;
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									dl_binary_writer_write( pCtx->writer, *(uint8**)pDefMember, Count * DLPodSize(pMember->type) );
							}
						}
						break;
						default:
							DL_ASSERT(false && "WHoo?");
					}
				}
			}

			pCtx->ArrayItemPop();
		}
		break;
		default:
			DL_ASSERT(false && "This should not happen!");
			break;
	}
	return 1;
}

static int dl_internal_pack_on_array_start( void* pack_ctx )
{
	DL_UNUSED(pack_ctx);
	return 1;
}

static int dl_internal_pack_on_array_end( void* pack_ctx )
{
	SDLPackContext* pCtx = (SDLPackContext*)pack_ctx;

	const SDLType* pType    = pCtx->state_stack.Top().type;
	EDLPackState pack_state = pCtx->state_stack.Top().state;

	pCtx->PopState(); // pop of pack state for sub-type
	uint32 array_count   = pCtx->state_stack.Top().array_count;
	pint   patch_pos     = pCtx->state_stack.Top().array_count_patch_pos;
	bool   is_back_array = pCtx->state_stack.Top().is_back_array;
	pCtx->PopState(); // pop of pack state for array
	if( patch_pos != 0 ) // not inline array
	{
		if( array_count == 0 )
		{
			// empty array should be null!
			dl_binary_writer_seek_set( pCtx->writer, patch_pos - sizeof(pint) );
			dl_binary_writer_write_pint( pCtx->writer, pint(-1) );
		}
		else if(is_back_array)
		{
			uint32 size  = sizeof( char* );
			uint32 align = sizeof( char* );

			if( pType != 0x0 )
			{
				size  = pType->size[DL_PTR_SIZE_HOST];
				align = pType->alignment[DL_PTR_SIZE_HOST];
			}

			// TODO: HACKZOR!
			// When packing arrays of structs one element to much in pushed in the back-alloc.
			if( pack_state != DL_PACK_STATE_STRING_ARRAY )
				pCtx->writer->back_alloc_pos += size;

			pint array_pos = dl_binary_writer_pop_back_alloc( pCtx->writer, array_count, size, align );
			dl_binary_writer_seek_set( pCtx->writer, patch_pos - sizeof(pint) );
			dl_binary_writer_write_pint( pCtx->writer, array_pos );
		}
		else
			dl_binary_writer_seek_set( pCtx->writer, patch_pos );

		dl_binary_writer_write_uint32( pCtx->writer, array_count );
	}

	return 1;
}

// TODO: These allocs need to be controllable!!!
void* dl_internal_pack_alloc  ( void* ctx, unsigned int sz )            { DL_UNUSED(ctx); return malloc(sz); }
void* dl_internal_pack_realloc( void* ctx, void* ptr, unsigned int sz ) { DL_UNUSED(ctx); return realloc(ptr, sz); }
void  dl_internal_pack_free   ( void* ctx, void* ptr )                  { DL_UNUSED(ctx); free(ptr); }

static dl_error_t dl_internal_txt_pack( SDLPackContext* pack_ctx, const char* text_data )
{
	// this could be incremental later on if needed!

	pint TxtLen = strlen(text_data);
	const unsigned char* TxtData = (const unsigned char*)text_data;

	static const int ALLOW_COMMENTS_IN_JSON = 1;
	static const int CAUSE_ERROR_ON_INVALID_UTF8 = 1;

	yajl_parser_config my_yajl_config = {
		ALLOW_COMMENTS_IN_JSON,
		CAUSE_ERROR_ON_INVALID_UTF8
	};

	yajl_alloc_funcs my_yajl_alloc = {
		dl_internal_pack_alloc,
		dl_internal_pack_realloc,
		dl_internal_pack_free,
		0x0
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

	yajl_handle my_yajl_handle = yajl_alloc( &callbacks, &my_yajl_config, &my_yajl_alloc, (void*)pack_ctx );

	yajl_status my_yajl_status = yajl_parse( my_yajl_handle, TxtData, (unsigned int)TxtLen ); // read file data, pass to parser

	unsigned int BytesConsumed = yajl_get_bytes_consumed( my_yajl_handle );

	my_yajl_status = yajl_parse_complete( my_yajl_handle ); // parse any remaining buffered data

	if (my_yajl_status != yajl_status_ok && my_yajl_status != yajl_status_insufficient_data)
	{
		if(BytesConsumed != 0) // error occured!
		{
			unsigned int Line = 1;
			unsigned int Column = 0;

			const char* Ch = text_data;
			const char* End = text_data + BytesConsumed;

			while(Ch != End)
			{
				if(*Ch == '\n')
				{
					++Line;
					Column = 0;
				}
				else
					++Column;

				++Ch;
			}

			dl_log_error( pack_ctx->dl_ctx, "At line %u, column %u", Line, Column);
		}

		char* pStr = (char*)yajl_get_error(my_yajl_handle, 1 /* verbose */, TxtData, (unsigned int)TxtLen);
		dl_log_error( pack_ctx->dl_ctx, "%s", pStr );
		yajl_free_error(my_yajl_handle, (unsigned char*)pStr);

		if(pack_ctx->error_code == DL_ERROR_OK)
		{
			yajl_free(my_yajl_handle);
			return DL_ERROR_TXT_PARSE_ERROR;
		}
	}

	if(pack_ctx->root_type == 0x0)
	{
		dl_log_error( pack_ctx->dl_ctx, "Missing root-element in dl-data" );
		pack_ctx->error_code = DL_ERROR_TXT_PARSE_ERROR;
	}

	yajl_free(my_yajl_handle);
	return pack_ctx->error_code;
}

dl_error_t dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_buffer, unsigned int out_buffer_size, unsigned int* produced_bytes )
{
	const bool IS_DUMMY_WRITER = out_buffer_size == 0;
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_buffer + sizeof(SDLDataHeader), out_buffer_size - sizeof(SDLDataHeader), IS_DUMMY_WRITER,
						   DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );

	SDLPackContext PackContext;
	PackContext.dl_ctx = dl_ctx;
	PackContext.writer    = &writer;

	dl_error_t error = dl_internal_txt_pack( &PackContext, txt_instance );

	if(error != DL_ERROR_OK)
		return error;

	// write header
	if( out_buffer_size > 0 )
	{
		SDLDataHeader header;
		header.id                 = DL_INSTANCE_ID;
		header.version            = DL_INSTANCE_VERSION;
		header.root_instance_type = dl_internal_typeid_of( dl_ctx, PackContext.root_type );
		header.instance_size      = (uint32)dl_binary_writer_needed_size( &writer );
		header.is_64_bit_ptr      = sizeof(void*) == 8 ? 1 : 0;
		memcpy( out_buffer, &header, sizeof(SDLDataHeader) );
	}

	dl_binary_writer_finalize( &writer );

	if( produced_bytes )
		*produced_bytes = (unsigned int)dl_binary_writer_needed_size( &writer ) + sizeof(SDLDataHeader);

	return DL_ERROR_OK;
}

dl_error_t dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, unsigned int* out_instance_size )
{
	return dl_txt_pack( dl_ctx, txt_instance, 0x0, 0, out_instance_size );
}
