#ifndef DL_DL_TYPES_H_INCLUDED
#define DL_DL_TYPES_H_INCLUDED

#include <dl/dl.h>

#include <string.h> // for memcpy
#include <stdio.h>

#define DL_LOG_DL_ERROR(_Fmt, ...) fprintf(stderr, "DL: " _Fmt, ##__VA_ARGS__)

enum
{
	DL_MEMBER_NAME_MAX_LEN     = 32,
	DL_TYPE_NAME_MAX_LEN       = 32,
	DL_ENUM_NAME_MAX_LEN       = 32,
	DL_ENUM_VALUE_NAME_MAX_LEN = 32,
};

#include "dl_swap.h"

static const uint32 DL_VERSION = 1;
static const uint32 DL_TYPE_LIB_ID         = ('D'<< 24) | ('L' << 16) | ('T' << 8) | 'L';
static const uint32 DL_TYPE_LIB_ID_SWAPED  = DLSwapEndian(DL_TYPE_LIB_ID);
static const uint32 DL_TYPE_DATA_ID        = ('D'<< 24) | ('L' << 16) | ('D' << 8) | 'L';
static const uint32 DL_TYPE_DATA_ID_SWAPED = DLSwapEndian(DL_TYPE_DATA_ID);

static const pint DL_NULL_PTR_OFFSET[2] = 
{
	pint(DL_UINT32_MAX), // DL_PTR_SIZE_32BIT
	pint(-1)            // DL_PTR_SIZE_64BIT
};

struct SDLTypeLibraryHeader
{
	uint32 m_Id;
	uint32 m_Version;
	
	uint32 m_nTypes;		// number of types in typelibrary
	uint32 m_TypesOffset;	// offset from start of data where types are stored
	uint32 m_TypesSize;		// number of bytes that are types

	uint32 m_nEnums;		// number of enums in typelibrary
	uint32 m_EnumsOffset;   // offset from start of data where enums are stored
	uint32 m_EnumsSize;		// number of bytes that are enums

	uint32 m_DefaultValuesOffset; 
	uint32 m_DefaultValuesSize;
};

struct SDLDataHeader
{
	uint32  m_Id;
	uint32  m_Version;
	StrHash m_RootInstanceType;
	uint32  m_InstanceSize;

	uint8   m_64BitPtr; // currently uses uint8 instead of bitfield to be compiler-compliant.
	uint8   m_Pad[3];
};

struct SDLTypeLookup
{
	StrHash m_TypeID;
	uint32  m_Offset;
};

enum EDLPtrSize
{
	DL_PTR_SIZE_32BIT = 0,
	DL_PTR_SIZE_64BIT = 1,

	DL_PTR_SIZE_HOST = sizeof(void*) == 4 ? DL_PTR_SIZE_32BIT : DL_PTR_SIZE_64BIT
};

struct SDLMember
{
	char m_Name[DL_MEMBER_NAME_MAX_LEN];
	EDLType m_Type;
	StrHash m_TypeID;

	uint32 m_Size[2];
	uint32 m_Alignment[2];
	uint32 m_Offset[2];

	// if M_UINT32_MAX, default value is not present, otherwise offset into default-value-data.
	uint32 m_DefaultValueOffset;

	EDLType AtomType()       const { return EDLType( m_Type & DL_TYPE_ATOM_MASK); }
	EDLType StorageType()    const { return EDLType( m_Type & DL_TYPE_STORAGE_MASK); }
	uint32  BitFieldBits()   const { return DL_EXTRACT_BITS(m_Type, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED); }
	uint32  BitFieldOffset() const { return DL_EXTRACT_BITS(m_Type, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED); }
	bool    IsSimplePod()    const { return StorageType() >= DL_TYPE_STORAGE_INT8 && StorageType() <= DL_TYPE_STORAGE_FP64; }
};

struct SDLEnumValue
{
	char   m_Name[DL_ENUM_VALUE_NAME_MAX_LEN];
	uint32 m_Value;
};

#ifdef M_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4200) // disable warning for 0-size array
#endif //M_COMPILER_MSVC

struct SDLType
{
	char      m_Name[DL_TYPE_NAME_MAX_LEN];
	uint32    m_Size[2];
	uint32    m_Alignment[2];
	// add some bytes for m_HasSubPtrs; for optimizations.
	uint32    m_nMembers;
	SDLMember m_lMembers[0];
};

struct SDLEnum
{
	char    m_Name[DL_ENUM_NAME_MAX_LEN];
	StrHash m_EnumID;

	uint32 m_nValues;
	SDLEnumValue m_lValues[0];
};

#ifdef M_COMPILER_MSVC
#pragma warning(pop)
#endif //M_COMPILER_MSVC

struct SOneMemberType
{
	SOneMemberType(const SDLMember* _pMember)
	{
		m_Size[0] = _pMember->m_Size[0];
		m_Size[1] = _pMember->m_Size[1];
		m_Alignment[0] = _pMember->m_Alignment[0];
		m_Alignment[1] = _pMember->m_Alignment[1];
		m_nMembers = 1;

		memcpy(&m_Member, _pMember, sizeof(SDLMember));
		m_Member.m_Offset[0] = 0;
		m_Member.m_Offset[0] = 0;
	}

	const SDLType* AsDLType() { return (const SDLType*)this; }

	char      m_Name[DL_TYPE_NAME_MAX_LEN];
	uint32    m_Size[2];
	uint32    m_Alignment[2];
	uint32    m_nMembers;
	SDLMember m_Member;
};

DL_STATIC_ASSERT(sizeof(SOneMemberType) - sizeof(SDLMember) == sizeof(SDLType), these_need_same_size);

struct SDLContext
{
	SDLAllocFunctions* m_DLAllocFuncs;
	SDLAllocFunctions* m_InstanceAllocFuncs;

	struct STypeLookUp { StrHash  m_TypeID; SDLType* m_pType; } m_TypeLookUp[128]; // dynamic alloc?
	struct SEnumLookUp { StrHash  m_EnumID; SDLEnum* m_pEnum; } m_EnumLookUp[128]; // dynamic alloc?

	uint   m_nTypes;
	uint8* m_TypeInfoData;

	uint   m_nEnums;
	uint8* m_EnumInfoData;

	uint8* m_pDefaultInstances;
};

/*
	return a bitfield offset on a particular platform (currently endian-ness is used to set them apart, that might break ;) )
	the bitfield offset are counted from least significant bit or most significant bit on different platforms.
*/
inline uint DLBitFieldOffset(ECpuEndian _Endian, uint _BFSize, uint _Offset, uint _nBits) { return _Endian == ENDIAN_LITTLE ? _Offset : (_BFSize * 8) - _Offset - _nBits; }
inline uint DLBitFieldOffset(uint _BFSize, uint _Offset, uint _nBits)                     { return DLBitFieldOffset(ENDIAN_HOST, _BFSize, _Offset, _nBits); }

DL_FORCEINLINE ECpuEndian DLOtherEndian(ECpuEndian _Endian) { return _Endian == ENDIAN_LITTLE ? ENDIAN_BIG : ENDIAN_LITTLE; }

DL_FORCEINLINE pint DLPtrSize(EDLPtrSize _SizeEnum)
{
	switch(_SizeEnum)
	{
		case DL_PTR_SIZE_32BIT: return 4;
		case DL_PTR_SIZE_64BIT: return 8;
		default: M_ASSERT("unknown ptr size!"); return 0;
	}
}

static inline pint DLPodSize(EDLType _Type)
{
	switch(_Type & DL_TYPE_STORAGE_MASK)
	{
		case DL_TYPE_STORAGE_INT8:  
		case DL_TYPE_STORAGE_UINT8:  return 1;

		case DL_TYPE_STORAGE_INT16: 
		case DL_TYPE_STORAGE_UINT16: return 2;

		case DL_TYPE_STORAGE_INT32: 
		case DL_TYPE_STORAGE_UINT32: 
		case DL_TYPE_STORAGE_FP32: 
		case DL_TYPE_STORAGE_ENUM:   return 4;

		case DL_TYPE_STORAGE_INT64: 
		case DL_TYPE_STORAGE_UINT64: 
		case DL_TYPE_STORAGE_FP64:   return 8;

		default:
			M_ASSERT(false && "This should not happen!");
			return 0;
	}
}

static inline const SDLType* DLFindType(HDLContext _Context, StrHash _TypeHash)
{
	// linear search right now!
	for(uint i = 0; i < _Context->m_nTypes; ++i)
		if(_Context->m_TypeLookUp[i].m_TypeID == _TypeHash)
			return _Context->m_TypeLookUp[i].m_pType;

	return 0x0;
}

static inline const SDLEnum* DLFindEnum(HDLContext _Context, StrHash _EnumHash)
{
	for (uint i = 0; i < _Context->m_nEnums; ++i)
	{
		const SDLContext::SEnumLookUp& LookUp = _Context->m_EnumLookUp[i];

		if(LookUp.m_EnumID == _EnumHash)
			return LookUp.m_pEnum;
	}

	return 0x0;
}

static inline uint32 DLFindEnumValue(const SDLEnum* _pEnum, const char* _Name, uint _NameLen)
{
	for (uint j = 0; j < _pEnum->m_nValues; ++j)
		if(strncmp(_pEnum->m_lValues[j].m_Name, _Name, _NameLen) == 0)
			return _pEnum->m_lValues[j].m_Value;
	return 0;
}

static inline const char* DLFindEnumName(HDLContext _Context, StrHash _EnumHash, uint32 _Value)
{
	const SDLEnum* pEnum = DLFindEnum(_Context, _EnumHash);

	if(pEnum != 0x0)
	{
		for (uint j = 0; j < pEnum->m_nValues; ++j)
			if(pEnum->m_lValues[j].m_Value == _Value)
				return pEnum->m_lValues[j].m_Name;
	}

	return "UnknownEnum!";
}

static StrHash DLHashBuffer(const uint8* _pBuffer, uint _Bytes, StrHash _BaseHash)
{
	M_ASSERT(_pBuffer != 0x0 && "You made wrong!");
	uint32 Hash = _BaseHash + 5381;
	for (uint i = 0; i < _Bytes; i++)
		Hash = (Hash * uint32(33)) + *((uint8*)_pBuffer + i);
	return Hash - 5381;
}

static StrHash DLHashString(const char* _pStr, StrHash _BaseHash = 0)
{
	M_ASSERT(_pStr != 0x0 && "You made wrong!");
	uint32 Hash = _BaseHash + 5381;
	for (uint i = 0; _pStr[i] != 0; i++)
		Hash = (Hash * uint32(33)) + _pStr[i];
	return Hash - 5381; // So empty string == 0
}

static inline uint DLFindMember(const SDLType* _pType, StrHash _NameHash)
{
	// TODO: currently members only hold name, but they should hold a hash!

	for(uint i = 0; i < _pType->m_nMembers; ++i)
	{
		const SDLMember* pMember = _pType->m_lMembers + i;

		if(DLHashString(pMember->m_Name) == _NameHash)
			return i;
	}

	return _pType->m_nMembers + 1;
}

#endif // DL_DL_TYPES_H_INCLUDED