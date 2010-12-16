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

	DL_PACK_STATE_SUBDATA_ID,
	DL_PACK_STATE_SUBDATA,

	DL_PACK_STATE_INSTANCE,
	DL_PACK_STATE_INSTANCE_TYPE,
	DL_PACK_STATE_STRUCT,
};

#define DL_PACK_ERROR_AND_FAIL(err, fmt, ...) { DL_LOG_DL_ERROR(fmt, ##__VA_ARGS__); pCtx->m_ErrorCode = err; return 0x0; }

template<unsigned int TBits>
class CFlagField
{
	DL_STATIC_ASSERT(TBits % 32 == 0, only_even_32_bits);
	uint32 m_Storage[TBits / 32];

	enum
	{
		BITS_PER_STORAGE = DL_BITS_IN_TYPE(uint32),
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
			struct_start_pos = m_Writer->PushBackAlloc( _pType->m_Size[DL_PTR_SIZE_HOST] );
		else
			struct_start_pos = AlignUp(m_Writer->Tell(), _pType->m_Alignment[DL_PTR_SIZE_HOST]);

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
			m_Writer->SeekSet(OldState.m_StructStartPos + OldState.m_pType->m_Size[DL_PTR_SIZE_HOST]);
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

	CArrayStatic<SStringItem, 1024> m_lStrings;
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

static int DLOnNull(void* _pCtx)
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;
	DL_ASSERT(pCtx->CurrentPackState() == DL_PACK_STATE_SUBDATA_ID);
	pCtx->m_Writer->Write(pint(-1)); // mark Null-ptr
	pCtx->PopState();
	return 1;
}

static int DLOnBool(void* _pCtx, int _Boolean)
{
	DL_UNUSED(_pCtx); DL_UNUSED(_Boolean);
	DL_ASSERT(false && "No support for bool yet!");
	return 1;
}

template<typename T> DL_FORCEINLINE bool Between(T _Val, T _Min, T _Max) { return _Val <= _Max && _Val >= _Min; }

static int DLOnNumber(void* _pCtx, const char* _pStringVal, unsigned int _StringLen)
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
			DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as bitfield member!", _StringLen, _pStringVal);

		unsigned int BFBits   = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED);
		unsigned int BFOffset = DL_EXTRACT_BITS(State, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED);

		uint64 MaxVal = (uint64(1) << BFBits) - uint64(1);
		if(Val > MaxVal)
			DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Value " DL_UINT64_FMT_STR" will not fit in a bitfield with %u bits!", Val, BFBits);

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
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", _StringLen, _pStringVal);

			pCtx->AddPatchPosition( (unsigned int)ID );

			// this is the pos that will be patched, but we need to make room for the array now!
			pCtx->m_Writer->WriteZero(pCtx->m_PatchPosition[pCtx->m_PatchPosition.Len() - 1].m_pMember->m_Size[DL_PTR_SIZE_HOST]);

			pCtx->PopState();
		}
		return 1;

		default:
			DL_ASSERT(false && "This should not happen!");
			return 0;
	}

	union { int64 m_Signed; uint64 m_Unsigned; } Val;
	if(sscanf(_pStringVal, pFmt, &Val.m_Signed) != 1)
		DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct integer type!", _StringLen, _pStringVal);

	switch(State)
	{
		case DL_PACK_STATE_POD_INT8:
		case DL_PACK_STATE_POD_INT16:
		case DL_PACK_STATE_POD_INT32:
		case DL_PACK_STATE_POD_INT64:
			if(!Between(Val.m_Signed, Min.m_Signed, Max.m_Signed))
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, DL_INT64_FMT_STR " will not fit in type", Val.m_Signed);
			break;
		case DL_PACK_STATE_POD_UINT8:
		case DL_PACK_STATE_POD_UINT16:
		case DL_PACK_STATE_POD_UINT32:
		case DL_PACK_STATE_POD_UINT64:
			if(!Between(Val.m_Unsigned, Min.m_Unsigned, Max.m_Unsigned))
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, DL_UINT64_FMT_STR " will not fit in type", Val.m_Unsigned);
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

static int DLOnString(void* _pCtx, const unsigned char* _pStringVal, unsigned int _StringLen)
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	EDLPackState State = pCtx->CurrentPackState();
	switch(State)
	{
		case DL_PACK_STATE_INSTANCE_TYPE:
			DL_ASSERT(pCtx->m_pRootType == 0x0);
			// we are packing an instance, get the type plox!
			pCtx->m_pRootType = DLFindType(pCtx->m_DLContext, DLHashBuffer(_pStringVal, _StringLen, 0));

			if(pCtx->m_pRootType == 0x0) 
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TYPE_NOT_FOUND, "Could not find type %.*s in loaded types!", _StringLen, _pStringVal);

			pCtx->PopState(); // back to last state plox!
		break;
		case DL_PACK_STATE_STRING:
			pCtx->m_lStrings.Add(SDLPackContext::SStringItem(pCtx->m_Writer->Tell(), (char*)_pStringVal, _StringLen));
			pCtx->m_Writer->Write((void*)0); // make room for ptr!
			pCtx->ArrayItemPop(); // back to last state plox!
		break;
		case DL_PACK_STATE_ENUM:
		{
			uint32 EnumValue = DLFindEnumValue(pCtx->m_StateStack.Top().m_pEnum, (const char*)_pStringVal, _StringLen);
			pCtx->m_Writer->Write(EnumValue);
			pCtx->ArrayItemPop();
		}
		break;
		default:
			DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Unexpected string \"%.*s\"!", _StringLen, _pStringVal);
	}

	return 1;
}

// TODO: This function is not optimal on any part and should be removed, store this info in the member as one bit!
static bool dl_internal_has_sub_ptr( dl_ctx_t dl_ctx, dl_typeid_t type )
{
	const SDLType* le_type = DLFindType( dl_ctx, type );

	if(le_type == 0) // not sturct
		return false;

	for(unsigned int i = 0; i < le_type->m_nMembers; ++i)
		if( le_type->m_lMembers[i].AtomType() == DL_TYPE_ATOM_ARRAY )
			return true;

	return false;
}

static int DLOnMapKey(void* _pCtx, const unsigned char* _pStringVal, unsigned int _StringLen)  
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	switch(pCtx->CurrentPackState())
	{
		case DL_PACK_STATE_INSTANCE:
			if(_StringLen != 4 && _StringLen != 7) 
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\" or \"data\"!", _StringLen, _pStringVal);

			if(StrCaseCompareLen((const char*)_pStringVal, "type", _StringLen) == 0)
			{
				if(pCtx->m_pRootType != 0x0) 
					DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance set two times!");
				pCtx->PushState(DL_PACK_STATE_INSTANCE_TYPE);
			}
			else if (StrCaseCompareLen((const char*)_pStringVal, "data", _StringLen) == 0)
			{
				if(pCtx->m_pRootType == 0x0) 
					DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Type for root-instance not set or after data-segment!");
				pCtx->PushStructState(pCtx->m_pRootType);
			}
			else if (StrCaseCompareLen((const char*)_pStringVal, "subdata", _StringLen) == 0)
			{
				pCtx->PushState(DL_PACK_STATE_SUBDATA);
			}
			else
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Got key \"%.*s\", expected \"type\", \"data\" or \"subdata\"!", _StringLen, _pStringVal);
		break;

		case DL_PACK_STATE_STRUCT:
		{
			SDLPackState& State = pCtx->m_StateStack.Top();

			unsigned int MemberID = DLFindMember( State.m_pType, DLHashBuffer(_pStringVal, _StringLen, 0) );

			if(MemberID > State.m_pType->m_nMembers) 
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_MEMBER_NOT_FOUND, "Member \"%.*s\" not in type!", _StringLen, _pStringVal);

			const SDLMember* pMember = State.m_pType->m_lMembers + MemberID;

			// printf("packing member: %s->%s\n", State.m_pType->m_Name, pMember->m_Name);

			if(State.m_MembersSet.IsSet(MemberID)) 
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_MEMBER_SET_TWICE, "Trying to set Member \"%.*s\" twice!", _StringLen, _pStringVal );

			State.m_MembersSet.SetBit(MemberID);

			// seek to position for member! ( members can come in any order in the text-file so we need to seek )
			pint MemberPos = pCtx->m_StateStack.Top().m_StructStartPos + pMember->m_Offset[DL_PTR_SIZE_HOST];
			pCtx->m_Writer->SeekSet(MemberPos);
			DL_ASSERT(pCtx->m_Writer->InBackAllocArea() || IsAlign((void*)MemberPos, pMember->m_Alignment[DL_PTR_SIZE_HOST]));

			pint array_count_patch_pos = 0; // for inline array, keep as 0.
			bool array_has_sub_ptrs = false;

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
							const SDLType* pSubType = DLFindType(pCtx->m_DLContext, pMember->m_TypeID);
							if(pSubType == 0x0)
								DL_PACK_ERROR_AND_FAIL( DL_ERROR_TYPE_NOT_FOUND, "Type of member \"%s\" not in type library!", pMember->m_Name);
							pCtx->PushStructState(pSubType); 
						}
						break;
						case DL_TYPE_STORAGE_PTR:  pCtx->PushMemberState(DL_PACK_STATE_SUBDATA_ID, pMember); break;
						case DL_TYPE_STORAGE_ENUM: pCtx->PushEnumState(DLFindEnum(pCtx->m_DLContext, pMember->m_TypeID)); break;
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

					array_has_sub_ptrs = dl_internal_has_sub_ptr(pCtx->m_DLContext, pMember->m_TypeID); // TODO: Optimize this, store info in type-data

				case DL_TYPE_ATOM_INLINE_ARRAY: // FALLTHROUGH
				{
					pCtx->PushState(DL_PACK_STATE_ARRAY);
					pCtx->m_StateStack.Top().m_ArrayCountPatchPos = array_count_patch_pos;
					pCtx->m_StateStack.Top().m_IsBackArray        = array_has_sub_ptrs;

					switch(StorageType)
					{
						case DL_TYPE_STORAGE_STRUCT:
						{
							const SDLType* pSubType = DLFindType(pCtx->m_DLContext, pMember->m_TypeID);
							if(pSubType == 0x0)
								DL_PACK_ERROR_AND_FAIL( DL_ERROR_TYPE_NOT_FOUND, "Type of array \"%s\" not in type library!", pMember->m_Name );
							pCtx->PushStructState(pSubType);
						}
						break;
						case DL_TYPE_STORAGE_ENUM:
						{
							const SDLEnum* pEnum = DLFindEnum(pCtx->m_DLContext, pMember->m_TypeID);
							if(pEnum == 0x0) 
								DL_PACK_ERROR_AND_FAIL( DL_ERROR_TYPE_NOT_FOUND, "Enum-type of array \"%s\" not in type library!", pMember->m_Name);
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
					pCtx->PushMemberState(EDLPackState(pMember->m_Type), 0x0);
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
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Could not parse %.*s as correct ID!", _StringLen, _pStringVal);

			const SDLMember* pMember = 0x0;

			// check that we have referenced this before so we know its type!
			for (unsigned int iPatchPos = 0; iPatchPos < pCtx->m_PatchPosition.Len(); ++iPatchPos)
				if(pCtx->m_PatchPosition[iPatchPos].m_ID == ID)
				{
					pMember = pCtx->m_PatchPosition[iPatchPos].m_pMember;
					break;
				}

			if(pMember == 0)
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "An item with id %u has not been encountered in the earlier part of the document, hence the type could not be deduced!", ID);

			dl_type_t AtomType = pMember->AtomType();
			dl_type_t StorageType = pMember->StorageType();

			DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
			DL_ASSERT(StorageType == DL_TYPE_STORAGE_PTR);
			const SDLType* pSubType = DLFindType(pCtx->m_DLContext, pMember->m_TypeID);
			if(pSubType == 0x0)
				DL_PACK_ERROR_AND_FAIL( DL_ERROR_TYPE_NOT_FOUND, "Type of ptr \"%s\" not in type library!", pMember->m_Name);
			pCtx->m_Writer->Align(pSubType->m_Alignment[DL_PTR_SIZE_HOST]);
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

static int DLOnMapStart(void* _pCtx)
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
			pCtx->m_Writer->Reserve(pCtx->m_StateStack.Top().m_pType->m_Size[DL_PTR_SIZE_HOST]);
			break;
		default:
			DL_PACK_ERROR_AND_FAIL( DL_ERROR_TXT_PARSE_ERROR, "Did not expect map-open here!");
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

	// write strings!
	for(unsigned int str_id = 0; str_id < pack_ctx->m_lStrings.Len(); ++str_id)
	{
		pack_ctx->m_Writer->SeekEnd();

		pint offset = pack_ctx->m_Writer->Tell();

		SDLPackContext::SStringItem& item = pack_ctx->m_lStrings[str_id];

		pack_ctx->m_Writer->Write((void*)item.m_pStr, item.m_Len);
		pack_ctx->m_Writer->Write(uint8(0));

		pack_ctx->m_Writer->SeekSet(item.m_Pos);
		pack_ctx->m_Writer->Write((void*)offset);
	}
}

static int DLOnMapEnd(void* _pCtx)
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
			for (uint32 iMember = 0; iMember < PackState.m_pType->m_nMembers; ++iMember)
			{
				const SDLMember* pMember = PackState.m_pType->m_lMembers + iMember;

				if(!PackState.m_MembersSet.IsSet(iMember))
				{
					if(pMember->m_DefaultValueOffset == DL_UINT32_MAX) // no default-value avaliable for tihs!
						DL_PACK_ERROR_AND_FAIL(DL_ERROR_TXT_MEMBER_MISSING, "Member %s in struct of type %s not set!", pMember->m_Name, PackState.m_pType->m_Name);

					pint   MemberPos = PackState.m_StructStartPos + pMember->m_Offset[DL_PTR_SIZE_HOST];
					uint8* pDefMember = pCtx->m_DLContext->m_pDefaultInstances + pMember->m_DefaultValueOffset;

					pCtx->m_Writer->SeekSet(MemberPos);

					dl_type_t AtomType    = pMember->AtomType();
					dl_type_t StorageType = pMember->StorageType();

					switch(AtomType)
					{
						case DL_TYPE_ATOM_POD:
						{
							switch(StorageType)
							{
								case DL_TYPE_STORAGE_STR: pCtx->m_lStrings.Add(SDLPackContext::SStringItem(MemberPos, *(char**)pDefMember, strlen(*(char**)pDefMember))); break;
								case DL_TYPE_STORAGE_PTR: pCtx->m_Writer->Write(pint(-1)); break; // can only default to null!
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(pDefMember, pMember->m_Size[DL_PTR_SIZE_HOST]);
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

									uint32 Count = pMember->m_Size[DL_PTR_SIZE_HOST] / sizeof(char*);
									for(uint32 iElem = 0; iElem < Count; ++iElem)
										pCtx->m_lStrings.Add(SDLPackContext::SStringItem(MemberPos + sizeof(char*) * iElem, pArray[iElem], strlen(pArray[iElem])));
								}
								break;
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(pDefMember, pMember->m_Size[DL_PTR_SIZE_HOST]);
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
										pCtx->m_lStrings.Add(SDLPackContext::SStringItem(ArrayPos + sizeof(char*) * iElem, pArray[iElem], strlen(pArray[iElem])));
								}
								break;
								default:
									DL_ASSERT(pMember->IsSimplePod() || DL_TYPE_STORAGE_ENUM);
									pCtx->m_Writer->Write(*(uint8**)pDefMember, Count * DLPodSize(pMember->m_Type));
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

static int DLOnArrayStart(void* _pCtx)
{
	DL_UNUSED(_pCtx);
	return 1;
}

static int DLOnArrayEnd(void* _pCtx)
{
	SDLPackContext* pCtx = (SDLPackContext*)_pCtx;

	const SDLType* pType = pCtx->m_StateStack.Top().m_pType;

	pCtx->PopState(); // pop of pack state for sub-type
	uint32 array_count   = pCtx->m_StateStack.Top().m_ArrayCount;
	pint   patch_pos     = pCtx->m_StateStack.Top().m_ArrayCountPatchPos;
	bool   is_back_array = pCtx->m_StateStack.Top().m_IsBackArray;
	pCtx->PopState(); // pop of pack state for array
	if( patch_pos != 0 ) // not inline array
	{
		if(is_back_array)
		{
			pint array_pos = pCtx->m_Writer->PopBackAlloc(array_count, pType->m_Size[DL_PTR_SIZE_HOST], pType->m_Alignment[DL_PTR_SIZE_HOST]);
			pCtx->m_Writer->SeekSet(patch_pos - sizeof(pint));
			pCtx->m_Writer->Write(array_pos);
		}

		if( array_count == 0 )
		{
			// empty array should be null!
			pCtx->m_Writer->SeekSet(patch_pos - sizeof(pint));
			pCtx->m_Writer->Write(pint(-1));
		}
		else
			pCtx->m_Writer->SeekSet(patch_pos);

		pCtx->m_Writer->Write(array_count);
	}

	return 1;
}

static yajl_callbacks g_YAJLParseCallbacks = {
	DLOnNull,
	DLOnBool,
	NULL, // integer,
	NULL, // double,
	DLOnNumber,
	DLOnString,
	DLOnMapStart,
	DLOnMapKey,
	DLOnMapEnd,
	DLOnArrayStart,
	DLOnArrayEnd
};

// TODO: These allocs need to be controllable!!!
void* DLPackAlloc(void* _pCtx, unsigned int _Sz)                { DL_UNUSED(_pCtx); return malloc(_Sz); }
void* DLPackRealloc(void* _pCtx, void* _pPtr, unsigned int _Sz) { DL_UNUSED(_pCtx); return realloc(_pPtr, _Sz); }
void  DLPackFree(void* _pCtx, void* _pPtr)                      { DL_UNUSED(_pCtx); free(_pPtr); }

static dl_error_t DLInternalPack(SDLPackContext* PackContext, const char* _pTxtData)
{
	// this could be incremental later on if needed!

	pint TxtLen = strlen(_pTxtData);
	const unsigned char* TxtData = (const unsigned char*)_pTxtData;

	yajl_handle YAJLHandle;
	yajl_status YAJLStat;

	yajl_parser_config YAJLCfg = { 1, 1 };
	yajl_alloc_funcs YAJLAlloc = { DLPackAlloc, DLPackRealloc, DLPackFree, 0x0 };

	YAJLHandle = yajl_alloc(&g_YAJLParseCallbacks, &YAJLCfg, &YAJLAlloc, (void*)PackContext);

	YAJLStat = yajl_parse(YAJLHandle, TxtData, (unsigned int)TxtLen); // read file data, pass to parser

	unsigned int BytesConsumed = yajl_get_bytes_consumed(YAJLHandle);

	YAJLStat = yajl_parse_complete(YAJLHandle); // parse any remaining buffered data

	if (YAJLStat != yajl_status_ok && YAJLStat != yajl_status_insufficient_data)
	{
		if(BytesConsumed != 0) // error occured!
		{
			unsigned int Line = 1;
			unsigned int Column = 0;

			const char* Ch = _pTxtData;
			const char* End = _pTxtData + BytesConsumed;

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

			DL_LOG_DL_ERROR("At line %u, column %u", Line, Column);
		}

		char* pStr = (char*)yajl_get_error(YAJLHandle, 1 /* verbose */, TxtData, (unsigned int)TxtLen);
		DL_LOG_DL_ERROR("%s", pStr);
		yajl_free_error(YAJLHandle, (unsigned char*)pStr);

		if(PackContext->m_ErrorCode == DL_ERROR_OK)
			return DL_ERROR_TXT_PARSE_ERROR;
	}

	if(PackContext->m_pRootType == 0x0)
	{
		DL_LOG_DL_ERROR("Missing root-element in dl-data");
		return DL_ERROR_TXT_PARSE_ERROR;
	}

	return PackContext->m_ErrorCode;
}

dl_error_t dl_txt_pack(dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_instance, unsigned int out_instance_size)
{
	const bool IS_DUMMY_WRITER = false;
	CDLBinaryWriter Writer(out_instance + sizeof(SDLDataHeader), out_instance_size -  sizeof(SDLDataHeader), IS_DUMMY_WRITER, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST);

	SDLPackContext PackContext;
	PackContext.m_DLContext = dl_ctx;
	PackContext.m_Writer    = &Writer;

	dl_error_t error = DLInternalPack(&PackContext, txt_instance);

	if(error != DL_ERROR_OK)
		return error;

	// write header
	SDLDataHeader Header;
	Header.m_Id = DL_TYPE_DATA_ID;
	Header.m_Version = DL_VERSION;
	Header.m_RootInstanceType = DLHashString(PackContext.m_pRootType->m_Name);
	Header.m_InstanceSize = (uint32)Writer.NeededSize();
	Header.m_64BitPtr = sizeof(void*) == 8 ? 1 : 0;
	memcpy(out_instance, &Header, sizeof(SDLDataHeader));

	return DL_ERROR_OK;
}

dl_error_t dl_txt_pack_calc_size(dl_ctx_t dl_ctx, const char* txt_instance, unsigned int* out_instance_size)
{
	const bool IS_DUMMY_WRITER = true;
	CDLBinaryWriter Writer(0x0, 0, IS_DUMMY_WRITER, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST);

	SDLPackContext PackContext;
	PackContext.m_DLContext = dl_ctx;
	PackContext.m_Writer    = &Writer;

	dl_error_t err = DLInternalPack(&PackContext, txt_instance);

	*out_instance_size = (unsigned int)Writer.NeededSize() + sizeof(SDLDataHeader);

	return err;
}
