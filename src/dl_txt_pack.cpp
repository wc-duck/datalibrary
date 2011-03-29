/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_txt.h>
#include "dl_types.h"
#include "dl_binary_writer.h"

#include "container/dl_array.h"
#include "container/dl_stack.h"

#include <yajl/yajl_parse.h>

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

#define DL_PACK_ERROR_AND_FAIL(dl_ctx, err, fmt, ...) { dl_log_error( dl_ctx, fmt, ##__VA_ARGS__ ); pCtx->m_ErrorCode = err; return 0x0; }

template<unsigned int TBits>
class CFlagField
{
	DL_STATIC_ASSERT(TBits % 32 == 0, only_even_32_bits);
	uint32 m_Storage[TBits / 32];

	enum
	{
		BITS_PER_STORAGE = sizeof(uint32) * 8,
		BITS_FOR_FIELD   = 5
	};

	unsigned int Field(unsigned int _Bit) { return (_Bit & ~(BITS_PER_STORAGE - 1)) >> BITS_FOR_FIELD; }
	unsigned int Bit  (unsigned int _Bit) { return (_Bit &  (BITS_PER_STORAGE - 1)); }

public:
	CFlagField()            { ClearAll(); }
	CFlagField(bool _AllOn) { MemSet(m_Storage, _AllOn ? 0xFF : 0x00, sizeof(m_Storage)); }

	~CFlagField() {}

	void SetBit  (unsigned int _Bit) { m_Storage[Field(_Bit)] |=  DL_BIT(Bit(_Bit)); }
	void ClearBit(unsigned int _Bit) { m_Storage[Field(_Bit)] &= ~DL_BIT(Bit(_Bit)); }
	void FlipBit (unsigned int _Bit) { m_Storage[Field(_Bit)] ^=  DL_BIT(Bit(_Bit)); }

	void SetAll()   { memset(m_Storage, 0xFF, sizeof(m_Storage)); }
	void ClearAll() { memset(m_Storage, 0x00, sizeof(m_Storage)); }

	bool IsSet(unsigned int _Bit) { return (m_Storage[Field(_Bit)] & DL_BIT(Bit(_Bit))) != 0; }
};

struct SDLPackState
{
	SDLPackState() {}

	SDLPackState(EDLPackState _State, const void* _pValue = 0x0, pint _StructStartPos = 0)
		: m_State(_State)
		, m_pValue(_pValue)
		, m_StructStartPos(_StructStartPos)
		, m_ArrayCount(0)
	{}

	EDLPackState   m_State;

	union
	{
		const void*      m_pValue;
		const SDLType*   m_pType;
		const SDLMember* m_pMember;
		const SDLEnum*   m_pEnum;
	};

	pint            m_StructStartPos;
	pint            m_ArrayCountPatchPos;
	uint32          m_ArrayCount;
	bool            m_IsBackArray;
	CFlagField<128> m_MembersSet;
};

struct SDLPackContext
{
	SDLPackContext() : m_pRootType(0x0), m_ErrorCode(DL_ERROR_OK)
	{
		PushState(DL_PACK_STATE_INSTANCE);
		memset(m_SubdataElements, 0xFF, sizeof(m_SubdataElements));

		// element 1 is root, and it has position 0
		m_SubdataElements[0].m_Pos = 0;
		m_SubdataElements[0].m_Count = 0;
	}

	CDLBinaryWriter* m_Writer;
	dl_ctx_t m_DLContext;

	const SDLType* m_pRootType;
	dl_error_t m_ErrorCode;

	void PushStructState(const SDLType* _pType)
	{
		pint struct_start_pos = 0;

		if( m_StateStack.Top().m_State == DL_PACK_STATE_ARRAY && m_StateStack.Top().m_IsBackArray )
			struct_start_pos = m_Writer->PushBackAlloc( _pType->size[DL_PTR_SIZE_HOST] );
		else
			struct_start_pos = dl_internal_align_up(m_Writer->Tell(), _pType->alignment[DL_PTR_SIZE_HOST]);

		m_StateStack.Push( SDLPackState(DL_PACK_STATE_STRUCT, _pType, struct_start_pos) );
	}

	void PushEnumState(const SDLEnum* _pMember)
	{
		m_StateStack.Push( SDLPackState(DL_PACK_STATE_ENUM, _pMember) );
	}

	void PushMemberState( EDLPackState _State, const SDLMember* _pMember)
	{
		m_StateStack.Push( SDLPackState(_State, _pMember) );
	}

	void PushState( EDLPackState _State ) 
	{
		DL_ASSERT(_State != DL_PACK_STATE_STRUCT && "Please use PushStructState()");
		m_StateStack.Push(_State);
	}

	void PopState() { m_StateStack.Pop(); }

	void ArrayItemPop()
	{
		SDLPackState& OldState = m_StateStack.Top();

		m_StateStack.Pop();

		if(OldState.m_State == DL_PACK_STATE_STRUCT)
		{
			// end should be just after the current struct. but we might not be there since
			// members could have been written in an order that is not according to type.
			m_Writer->SeekSet(OldState.m_StructStartPos + OldState.m_pType->size[DL_PTR_SIZE_HOST]);
		}

		if( m_StateStack.Top().m_State == DL_PACK_STATE_ARRAY )
		{
			m_StateStack.Top().m_ArrayCount++;

			switch(OldState.m_State)
			{
				case DL_PACK_STATE_STRUCT: PushStructState(OldState.m_pType); break;
				case DL_PACK_STATE_ENUM:   PushEnumState(OldState.m_pEnum);   break;
				default:                   PushState(OldState.m_State);       break;
			}
		}
	}

	EDLPackState CurrentPackState() { return m_StateStack.Top().m_State; }

	// positions where we have only an ID that referre to subdata!
	struct SPatchPosition
	{
		SPatchPosition() {}
		SPatchPosition(unsigned int _ID, pint _WritePos, const SDLMember* _pMember)
			: m_ID(_ID)
			, m_WritePos(_WritePos)
			, m_pMember(_pMember) {}
		unsigned int m_ID;
		pint m_WritePos;
		const SDLMember* m_pMember;
	};

	CArrayStatic<SPatchPosition, 128> m_PatchPosition;

	void AddPatchPosition(unsigned int _ID)
	{
		m_PatchPosition.Add(SPatchPosition(_ID, m_Writer->Tell(), m_StateStack.Top().m_pMember));
	}

	void RegisterSubdataElement(unsigned int _Element, pint _Pos)
	{
		DL_ASSERT(m_SubdataElements[_Element].m_Pos == pint(-1) && "Subdata element already registered!");
		m_SubdataElements[_Element].m_Pos = _Pos;
		m_CurrentSubdataElement = _Element;
	}

	pint   SubdataElementPos(unsigned int _Element)   { return m_SubdataElements[_Element].m_Pos; }
	uint32 SubdataElementCount(unsigned int _Element) { return m_SubdataElements[_Element].m_Count; }

	struct SStringItem
	{
		SStringItem() {}
		SStringItem(pint _Pos, const char* _pStr, pint _Len)
			: m_Pos(_Pos)
			, m_pStr(_pStr)
			, m_Len(_Len) {}
		pint        m_Pos;
		const char* m_pStr;
		pint        m_Len;
	};

	CStackStatic<SDLPackState, 128> m_StateStack;

private:
	unsigned int m_CurrentSubdataElement;
	// positions for different subdata-elements;
	struct
	{
		pint m_Pos;
		uint32 m_Count;
	} m_SubdataElements[128];
};

static int dl_internal_pack_on_null( void* _pCtx )
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;
	DL_ASSERT(pCtx->CurrentPackState() == DL_PACK_STATE_SUBDATA_ID);
	pCtx->m_Writer->Write(pint(-1)); // mark Null-ptr
	pCtx->PopState();
	return 1;
}

static int dl_internal_pack_on_bool( void* pack_ctx, int value )
{
	DL_UNUSED(pack_ctx); DL_UNUSED(value);
	DL_ASSERT(false && "No support for bool yet!");
	return 1;
}

template<typename T> DL_FORCEINLINE bool Between(T _Val, T _Min, T _Max) { return _Val <= _Max && _Val >= _Min; }

static int dl_internal_pack_on_number(void* _pCtx, const char* _pStringVal, unsigned int _StringLen)
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	union { int64 m_Signed; uint64 m_Unsigned; } Min;
	union { int64 m_Signed; uint64 m_Unsigned; } Max;
	const char* pFmt = "";

	EDLPackState State = pCtx->CurrentPackState();

	if((State & DL_TYPE_ATOM_MASK) == DL_TYPE_ATOM_BITFIELD)
	{
		uint64 Val;
		if(sscanf(_pStringVal, DL_UINT64_FMT_STR, &Val) != 1)
			DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as bitfield member!", _StringLen, _pStringVal);

		unsigned int BFBits   = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED);
		unsigned int BFOffset = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED);

		uint64 MaxVal = (uint64(1) << BFBits) - uint64(1);
		if(Val > MaxVal)
			DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Value " DL_UINT64_FMT_STR" will not fit in a bitfield with %u bits!", Val, BFBits);

		dl_type_t StorageType = dl_type_t(State & DL_TYPE_STORAGE_MASK);

		switch(StorageType)
		{
			case DL_TYPE_STORAGE_UINT8:  pCtx->m_Writer->Write((uint8) DL_INSERT_BITS(pCtx->m_Writer->Read<uint8>(),  uint8(Val),  DLBitFieldOffset( sizeof(uint8), BFOffset, BFBits), BFBits)); break;
			case DL_TYPE_STORAGE_UINT16: pCtx->m_Writer->Write((uint16)DL_INSERT_BITS(pCtx->m_Writer->Read<uint16>(), uint16(Val), DLBitFieldOffset(sizeof(uint16), BFOffset, BFBits), BFBits)); break;
			case DL_TYPE_STORAGE_UINT32: pCtx->m_Writer->Write((uint32)DL_INSERT_BITS(pCtx->m_Writer->Read<uint32>(), uint32(Val), DLBitFieldOffset(sizeof(uint32), BFOffset, BFBits), BFBits)); break;
			case DL_TYPE_STORAGE_UINT64: pCtx->m_Writer->Write((uint64)DL_INSERT_BITS(pCtx->m_Writer->Read<uint64>(), uint64(Val), DLBitFieldOffset(sizeof(uint64), BFOffset, BFBits), BFBits)); break;

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

		case DL_PACK_STATE_POD_FP32:   pCtx->m_Writer->Write((float)atof(_pStringVal)); pCtx->ArrayItemPop(); return 1;
		case DL_PACK_STATE_POD_FP64:   pCtx->m_Writer->Write(       atof(_pStringVal)); pCtx->ArrayItemPop(); return 1;

		case DL_PACK_STATE_SUBDATA_ID:
		{
			uint32 ID;
			if(sscanf(_pStringVal, "%u", &ID) != 1)
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", _StringLen, _pStringVal);

			pCtx->AddPatchPosition( (unsigned int)ID );

			// this is the pos that will be patched, but we need to make room for the array now!
			pCtx->m_Writer->WriteZero(pCtx->m_PatchPosition[pCtx->m_PatchPosition.Len() - 1].m_pMember->size[DL_PTR_SIZE_HOST]);

			pCtx->PopState();
		}
		return 1;

		default:
			DL_ASSERT(false && "This should not happen!");
			return 0;
	}

	union { int64 m_Signed; uint64 m_Unsigned; } Val;
	if(sscanf(_pStringVal, pFmt, &Val.m_Signed) != 1)
		DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct integer type!", _StringLen, _pStringVal);

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:
		case DL_PACK_STATE_POD_INT16:
		case DL_PACK_STATE_POD_INT32:
		case DL_PACK_STATE_POD_INT64:
			if(!Between(Val.m_Signed, Min.m_Signed, Max.m_Signed))
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, DL_INT64_FMT_STR " will not fit in type", Val.m_Signed);
			break;
		case DL_PACK_STATE_POD_UINT8:
		case DL_PACK_STATE_POD_UINT16:
		case DL_PACK_STATE_POD_UINT32:
		case DL_PACK_STATE_POD_UINT64:
			if(!Between(Val.m_Unsigned, Min.m_Unsigned, Max.m_Unsigned))
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, DL_UINT64_FMT_STR " will not fit in type", Val.m_Unsigned);
			break;
		default:
			break;
	}

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:   pCtx->m_Writer->Write(  (int8)Val.m_Signed); break;
		case DL_PACK_STATE_POD_INT16:  pCtx->m_Writer->Write( (int16)Val.m_Signed); break;
		case DL_PACK_STATE_POD_INT32:  pCtx->m_Writer->Write( (int32)Val.m_Signed); break;
		case DL_PACK_STATE_POD_INT64:  pCtx->m_Writer->Write( (int64)Val.m_Signed); break;
		case DL_PACK_STATE_POD_UINT8:  pCtx->m_Writer->Write( (uint8)Val.m_Unsigned); break;
		case DL_PACK_STATE_POD_UINT16: pCtx->m_Writer->Write((uint16)Val.m_Unsigned); break;
		case DL_PACK_STATE_POD_UINT32: pCtx->m_Writer->Write((uint32)Val.m_Unsigned); break;
		case DL_PACK_STATE_POD_UINT64: pCtx->m_Writer->Write((uint64)Val.m_Unsigned); break;
		default: break;
	}

	pCtx->ArrayItemPop();

	return 1;
}

static int dl_internal_pack_on_string( void* _pCtx, const unsigned char* str_value, unsigned int str_len )
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	EDLPackState State = pCtx->CurrentPackState();
	switch(State)
	{
		case DL_PACK_STATE_INSTANCE_TYPE:
			DL_ASSERT(pCtx->m_pRootType == 0x0);
			// we are packing an instance, get the type plox!
			pCtx->m_pRootType = dl_internal_find_type(pCtx->m_DLContext, dl_internal_hash_buffer(str_value, str_len, 0));

			if(pCtx->m_pRootType == 0x0) 
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TYPE_NOT_FOUND, "Could not find type %.*s in loaded types!", str_len, str_value);

			pCtx->PopState(); // back to last state plox!
		break;
		case DL_PACK_STATE_STRING_ARRAY:
		{
			// seek end
			pCtx->m_Writer->SeekEnd();

			pint offset = pCtx->m_Writer->Tell();

			pCtx->m_Writer->WriteString( str_value, str_len );

			pint array_elem_pos = pCtx->m_Writer->PushBackAlloc( sizeof( char* ) );
			pCtx->m_Writer->SeekSet( array_elem_pos );
			pCtx->m_Writer->Write( offset );

			pCtx->ArrayItemPop(); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_STRING:
		{
			pint curr = pCtx->m_Writer->Tell();
			pCtx->m_Writer->SeekEnd();
			pint offset = pCtx->m_Writer->Tell();
			pCtx->m_Writer->WriteString( str_value, str_len );
			pCtx->m_Writer->SeekSet( curr );

			pCtx->m_Writer->Write( offset ); // make room for ptr!
			pCtx->ArrayItemPop(); // back to last state plox!
		}
		break;
		case DL_PACK_STATE_ENUM:
		{
			uint32 EnumValue = dl_internal_find_enum_value(pCtx->m_StateStack.Top().m_pEnum, (const char*)str_value, str_len);
			pCtx->m_Writer->Write(EnumValue);
			pCtx->ArrayItemPop();
		}
		break;
		default:
			DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Unexpected string \"%.*s\"!", str_len, str_value);
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
		if( le_type->members[i].AtomType() == DL_TYPE_ATOM_ARRAY )
			return true;

	return false;
}

static int dl_internal_pack_on_map_key(void* _pCtx, const unsigned char* _pStringVal, unsigned int _StringLen)  
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	switch(pCtx->CurrentPackState())
	{
		case DL_PACK_STATE_INSTANCE:
			if(_StringLen != 4 && _StringLen != 7) 
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\" or \"data\"!", _StringLen, _pStringVal);

			if(strncmp((const char*)_pStringVal, "type", _StringLen) == 0)
			{
				if(pCtx->m_pRootType != 0x0) 
					DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance set two times!");
				pCtx->PushState(DL_PACK_STATE_INSTANCE_TYPE);
			}
			else if (strncmp((const char*)_pStringVal, "data", _StringLen) == 0)
			{
				if(pCtx->m_pRootType == 0x0) 
					DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance not set or after data-segment!");
				pCtx->PushStructState(pCtx->m_pRootType);
			}
			else if (strncmp((const char*)_pStringVal, "subdata", _StringLen) == 0)
			{
				pCtx->PushState(DL_PACK_STATE_SUBDATA);
			}
			else
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\", \"data\" or \"subdata\"!", _StringLen, _pStringVal);
		break;

		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& State = pCtx->m_StateStack.Top();

			unsigned int MemberID = dl_internal_find_member( State.m_pType, dl_internal_hash_buffer(_pStringVal, _StringLen, 0) );

			if(MemberID > State.m_pType->member_count) 
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_MEMBER_NOT_FOUND, "Type \"%s\" has no member named \"%.*s\"!", State.m_pType->name, _StringLen, _pStringVal);

			const SDLMember* pMember = State.m_pType->members + MemberID;

			if(State.m_MembersSet.IsSet(MemberID)) 
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_MEMBER_SET_TWICE, "Trying to set Member \"%.*s\" twice!", _StringLen, _pStringVal );

			State.m_MembersSet.SetBit(MemberID);

			// seek to position for member! ( members can come in any order in the text-file so we need to seek )
			pint MemberPos = pCtx->m_StateStack.Top().m_StructStartPos + pMember->offset[DL_PTR_SIZE_HOST];
			pCtx->m_Writer->SeekSet(MemberPos);
			DL_ASSERT(pCtx->m_Writer->InBackAllocArea() || dl_internal_is_align((void*)MemberPos, pMember->alignment[DL_PTR_SIZE_HOST]));

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
							const SDLType* pSubType = dl_internal_find_type(pCtx->m_DLContext, pMember->type_id);
							if(pSubType == 0x0)
								DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TYPE_NOT_FOUND, "Type of member \"%s\" not in type library!", pMember->name);
							pCtx->PushStructState(pSubType); 
						}
						break;
						case DL_TYPE_STORAGE_PTR:  pCtx->PushMemberState(DL_PACK_STATE_SUBDATA_ID, pMember); break;
						case DL_TYPE_STORAGE_ENUM: pCtx->PushEnumState(dl_internal_find_enum(pCtx->m_DLContext, pMember->type_id)); break;
						default:
							DL_ASSERT(pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_STR);
							pCtx->PushState(EDLPackState(StorageType));
							break;
					}
				}
				break;
				case DL_TYPE_ATOM_ARRAY:
					// save position for array to be able to write ptr and count on array-end.
					pCtx->m_Writer->Write(pCtx->m_Writer->NeededSize());
					array_count_patch_pos = pCtx->m_Writer->Tell();
					pCtx->m_Writer->Write(uint32(0)); // count need to be patched later!
					pCtx->m_Writer->SeekEnd();

					if( StorageType == DL_TYPE_STORAGE_STR )
					{
						pCtx->PushState(DL_PACK_STATE_ARRAY);
						pCtx->m_StateStack.Top().m_ArrayCountPatchPos = array_count_patch_pos;
						pCtx->m_StateStack.Top().m_IsBackArray        = true;

						pCtx->PushState( DL_PACK_STATE_STRING_ARRAY );
						break;
					}

					array_has_sub_ptrs = dl_internal_has_sub_ptr(pCtx->m_DLContext, pMember->type_id); // TODO: Optimize this, store info in type-data
				case DL_TYPE_ATOM_INLINE_ARRAY: // FALLTHROUGH
				{
					pCtx->PushState(DL_PACK_STATE_ARRAY);
					pCtx->m_StateStack.Top().m_ArrayCountPatchPos = array_count_patch_pos;
					pCtx->m_StateStack.Top().m_IsBackArray        = array_has_sub_ptrs;

					switch(StorageType)
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const SDLType* pSubType = dl_internal_find_type(pCtx->m_DLContext, pMember->type_id);
							if(pSubType == 0x0)
								DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TYPE_NOT_FOUND, "Type of array \"%s\" not in type library!", pMember->name );
							pCtx->PushStructState(pSubType);
						}
						break;
						case DL_TYPE_STORAGE_ENUM:
						{
							const SDLEnum* pEnum = dl_internal_find_enum(pCtx->m_DLContext, pMember->type_id);
							if(pEnum == 0x0) 
								DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TYPE_NOT_FOUND, "Enum-type of array \"%s\" not in type library!", pMember->name);
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
			if(sscanf((char*)_pStringVal, "%u", &ID) != 1)
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", _StringLen, _pStringVal);

			const SDLMember* pMember = 0x0;

			// check that we have referenced this before so we know its type!
			for (unsigned int iPatchPos = 0; iPatchPos < pCtx->m_PatchPosition.Len(); ++iPatchPos)
				if(pCtx->m_PatchPosition[iPatchPos].m_ID == ID)
				{
					pMember = pCtx->m_PatchPosition[iPatchPos].m_pMember;
					break;
				}

			if(pMember == 0)
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "An item with id %u has not been encountered in the earlier part of the document, hence the type could not be deduced!", ID);

			dl_type_t AtomType = pMember->AtomType();
			dl_type_t StorageType = pMember->StorageType();

			DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
			DL_ASSERT(StorageType == DL_TYPE_STORAGE_PTR);
			const SDLType* pSubType = dl_internal_find_type(pCtx->m_DLContext, pMember->type_id);
			if(pSubType == 0x0)
				DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TYPE_NOT_FOUND, "Type of ptr \"%s\" not in type library!", pMember->name);
			pCtx->m_Writer->Align(pSubType->alignment[DL_PTR_SIZE_HOST]);
			pCtx->PushStructState(pSubType);

			// register this element as it will be written here!
			pCtx->RegisterSubdataElement(ID, pCtx->m_Writer->Tell());
		}
		break;

		default:
			DL_ASSERT(false && "This should not happen!");
	}

	return 1;
}

static int dl_internal_pack_on_map_start(void* _pCtx)
{
	// check that we are in a correct state here!
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	switch(pCtx->CurrentPackState())
	{
		case DL_PACK_STATE_SUBDATA:
		case DL_PACK_STATE_INSTANCE:
			break;
		case DL_PACK_STATE_STRUCT:
			if( pCtx->m_Writer->InBackAllocArea() )
				pCtx->m_Writer->SeekEnd();
			else
				pCtx->m_Writer->Reserve(pCtx->m_StateStack.Top().m_pType->size[DL_PTR_SIZE_HOST]);
			break;
		default:
			DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_PARSE_ERROR, "Did not expect map-open here!");
	}
	return 1;
}

static void dl_internal_txt_pack_finalize( SDLPackContext* pack_ctx )
{
	// patch subdata
	for(unsigned int patch_pos = 0; patch_pos < pack_ctx->m_PatchPosition.Len(); ++patch_pos)
	{
		unsigned int id = pack_ctx->m_PatchPosition[patch_pos].m_ID;
		pint member_pos = pack_ctx->m_PatchPosition[patch_pos].m_WritePos;

		pack_ctx->m_Writer->SeekSet(member_pos);
		pack_ctx->m_Writer->Write(pack_ctx->SubdataElementPos(id));
	}
}

static int dl_internal_pack_on_map_end( void* _pCtx )
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	if(pCtx->m_StateStack.Len() == 1) // end of top-instance!
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
			SDLPackState& PackState = pCtx->m_StateStack.Top();

			// Check that all members are set!
			for (uint32 iMember = 0; iMember < PackState.m_pType->member_count; ++iMember)
			{
				const SDLMember* pMember = PackState.m_pType->members + iMember;

				if(!PackState.m_MembersSet.IsSet(iMember))
				{
					if(pMember->default_value_offset == DL_UINT32_MAX) // no default-value available for this!
						DL_PACK_ERROR_AND_FAIL( pCtx->m_DLContext, DL_ERROR_TXT_MEMBER_MISSING, "Member %s in struct of type %s not set!", pMember->name, PackState.m_pType->name );

					pint   MemberPos  = PackState.m_StructStartPos + pMember->offset[DL_PTR_SIZE_HOST];
					uint8* pDefMember = pCtx->m_DLContext->default_data + pMember->default_value_offset;

					pCtx->m_Writer->SeekSet(MemberPos);

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
									pCtx->m_Writer->SeekEnd();
									pint str_pos = pCtx->m_Writer->Tell();
									char* str = *(char**)pDefMember;
									pCtx->m_Writer->WriteString( str, strlen( str ) );
									pCtx->m_Writer->SeekSet( MemberPos );
									pCtx->m_Writer->Write( str_pos );
								}
								break;
								case DL_TYPE_STORAGE_PTR: pCtx->m_Writer->Write(pint(-1)); break; // can only default to null!
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(pDefMember, pMember->size[DL_PTR_SIZE_HOST]);
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
										pCtx->m_Writer->SeekEnd();
										pint str_pos = pCtx->m_Writer->Tell();
										pCtx->m_Writer->WriteString( pArray[iElem], strlen( pArray[iElem] ) );
										pCtx->m_Writer->SeekSet( MemberPos + sizeof(char*) * iElem );
										pCtx->m_Writer->Write( str_pos );
									}
								}
								break;
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(pDefMember, pMember->size[DL_PTR_SIZE_HOST]);
							}
						}
						break;
						case DL_TYPE_ATOM_ARRAY:
						{
							uint32 Count = *(uint32*)(pDefMember + sizeof(void*));

							pCtx->m_Writer->Write(pCtx->m_Writer->NeededSize());
							pCtx->m_Writer->Write(Count);
							pCtx->m_Writer->SeekEnd();

							switch(StorageType)
							{
								case DL_TYPE_STORAGE_STR:
								{
									pint ArrayPos = pCtx->m_Writer->Tell();
									pCtx->m_Writer->Reserve(Count * sizeof(char*));

									char** pArray = *(char***)pDefMember;
									for(uint32 iElem = 0; iElem < Count; ++iElem)
									{
										pCtx->m_Writer->SeekEnd();
										pint str_pos = pCtx->m_Writer->Tell();
										pCtx->m_Writer->WriteString( pArray[iElem], strlen( pArray[iElem] ) );
										pCtx->m_Writer->SeekSet( ArrayPos + sizeof(char*) * iElem );
										pCtx->m_Writer->Write( str_pos );
									}
								}
								break;
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(*(uint8**)pDefMember, Count * DLPodSize(pMember->type));
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
	}
	return 1;
}

static int dl_internal_pack_on_array_start( void* pack_ctx )
{
	DL_UNUSED(pack_ctx);
	return 1;
}

static int dl_internal_pack_on_array_end( void* _pCtx )
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	const SDLType* pType    = pCtx->m_StateStack.Top().m_pType;
	EDLPackState pack_state = pCtx->m_StateStack.Top().m_State;

	pCtx->PopState(); // pop of pack state for sub-type
	uint32 array_count   = pCtx->m_StateStack.Top().m_ArrayCount;
	pint   patch_pos     = pCtx->m_StateStack.Top().m_ArrayCountPatchPos;
	bool   is_back_array = pCtx->m_StateStack.Top().m_IsBackArray;
	pCtx->PopState(); // pop of pack state for array
	if( patch_pos != 0 ) // not inline array
	{
		if( array_count == 0 )
		{
			// empty array should be null!
			pCtx->m_Writer->SeekSet(patch_pos - sizeof(pint));
			pCtx->m_Writer->Write(pint(-1));
		}
		else if(is_back_array)
		{
			pint size  = sizeof( char* );
			pint align = sizeof( char* );

			if( pType != 0x0 )
			{
				size  = pType->size[DL_PTR_SIZE_HOST];
				align = pType->alignment[DL_PTR_SIZE_HOST];
			}

			// TODO: HACKZOR!
			// When packing arrays of structs one element to much in pushed in the back-alloc.
			if( pack_state != DL_PACK_STATE_STRING_ARRAY )
				pCtx->m_Writer->m_BackAllocStack.Pop();

			pint array_pos = pCtx->m_Writer->PopBackAlloc( array_count, size, align );
			pCtx->m_Writer->SeekSet(patch_pos - sizeof(pint));
			pCtx->m_Writer->Write(array_pos);
		}
		else
			pCtx->m_Writer->SeekSet(patch_pos);

		pCtx->m_Writer->Write(array_count);
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

			dl_log_error( pack_ctx->m_DLContext, "At line %u, column %u", Line, Column);
		}

		char* pStr = (char*)yajl_get_error(my_yajl_handle, 1 /* verbose */, TxtData, (unsigned int)TxtLen);
		dl_log_error( pack_ctx->m_DLContext, "%s", pStr );
		yajl_free_error(my_yajl_handle, (unsigned char*)pStr);

		if(pack_ctx->m_ErrorCode == DL_ERROR_OK)
		{
			yajl_free(my_yajl_handle);
			return DL_ERROR_TXT_PARSE_ERROR;
		}
	}

	if(pack_ctx->m_pRootType == 0x0)
	{
		dl_log_error( pack_ctx->m_DLContext, "Missing root-element in dl-data" );
		pack_ctx->m_ErrorCode = DL_ERROR_TXT_PARSE_ERROR;
	}

	yajl_free(my_yajl_handle);
	return pack_ctx->m_ErrorCode;
}

dl_error_t dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_buffer, unsigned int out_buffer_size, unsigned int* produced_bytes )
{
	const bool IS_DUMMY_WRITER = out_buffer_size == 0;
	CDLBinaryWriter Writer(out_buffer + sizeof(SDLDataHeader), out_buffer_size -  sizeof(SDLDataHeader), IS_DUMMY_WRITER, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST);

	SDLPackContext PackContext;
	PackContext.m_DLContext = dl_ctx;
	PackContext.m_Writer    = &Writer;

	dl_error_t error = dl_internal_txt_pack(&PackContext, txt_instance);

	if(error != DL_ERROR_OK)
		return error;

	// write header
	if( out_buffer_size > 0 )
	{
		SDLDataHeader Header;
		Header.id = DL_INSTANCE_ID;
		Header.version = DL_INSTANCE_VERSION;
		Header.root_instance_type = dl_internal_hash_string(PackContext.m_pRootType->name); // TODO: Read typeid from somewhere, instance-id-generation might change!
		Header.instance_size = (uint32)Writer.NeededSize();
		Header.is_64_bit_ptr = sizeof(void*) == 8 ? 1 : 0;
		memcpy(out_buffer, &Header, sizeof(SDLDataHeader));
	}

	if( produced_bytes )
		*produced_bytes = (unsigned int)Writer.NeededSize() + sizeof(SDLDataHeader);

	return DL_ERROR_OK;
}

dl_error_t dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, unsigned int* out_instance_size )
{
	return dl_txt_pack( dl_ctx, txt_instance, 0x0, 0, out_instance_size );
}
