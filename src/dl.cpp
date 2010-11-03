#include <dl/dl.h>

#include "dl_types.h"
#include "dl_swap.h"

#include "dl_temp.h"
#include "container/dl_array.h"

#include <stdio.h> // printf
#include <stdlib.h> // malloc, free

static void* DLMallocAlloc(unsigned int  _Size, unsigned int _Alignment) { DL_UNUSED(_Alignment); return malloc(_Size); }
static void  DLMallocFree (void* _pPtr) { free(_pPtr); }
static SDLAllocFunctions g_DLMallocFreeFuncs = { DLMallocAlloc, DLMallocFree };

EDLError DLContextCreate(HDLContext* _pContext, SDLAllocFunctions* _pDLAllocFuncs, SDLAllocFunctions* _pInstanceAllocFuncs)
{
	if(_pDLAllocFuncs == 0x0)
		_pDLAllocFuncs = &g_DLMallocFreeFuncs;

	SDLContext* pCtx = (SDLContext*)_pDLAllocFuncs->m_Alloc(sizeof(SDLContext), sizeof(void*));

	if(pCtx == 0x0)
		return DL_ERROR_OUT_OF_LIBRARY_MEMORY;

	pCtx->m_DLAllocFuncs       = _pDLAllocFuncs;
	pCtx->m_InstanceAllocFuncs = _pInstanceAllocFuncs;

	pCtx->m_nTypes = 0;
	pCtx->m_nEnums = 0;

	*_pContext = pCtx;

	return DL_ERROR_OK;
}

EDLError DLContextDestroy(HDLContext _Context)
{
	_Context->m_DLAllocFuncs->m_Free(_Context->m_TypeInfoData);
	_Context->m_DLAllocFuncs->m_Free(_Context->m_EnumInfoData);
	_Context->m_DLAllocFuncs->m_Free(_Context->m_pDefaultInstances);
	_Context->m_DLAllocFuncs->m_Free(_Context);
	return DL_ERROR_OK;
}

EDLError DLInternalConvertNoHeader( HDLContext     _Context,
                                    unsigned char* _pData,
                                    unsigned char* _pBaseData,
                                    unsigned char* _pOutData,
                                    unsigned int   _OutDataSize,
                                    unsigned int*  _pNeededSize,
                                    ECpuEndian     _SourceEndian,
                                    ECpuEndian     _TargetEndian,
                                    EDLPtrSize     _SourcePtrSize,
                                    EDLPtrSize     _TargetPtrSize,
                                    const SDLType* _pRootType,
                                    unsigned int   _BaseOffset );

struct SPatchedInstances
{
	CArrayStatic<const uint8*, 1024> m_lpPatched;

	bool IsPatched(const uint8* _pInstance)
	{
		for (uint iPatch = 0; iPatch < m_lpPatched.Len(); ++iPatch)
			if(m_lpPatched[iPatch] == _pInstance)
				return true;
		return false;
	}
};

static void DLPatchPtr(const uint8* _pPtr, const uint8* _pBaseData)
{
	union ReadMe
	{
		pint         m_Offset;
		const uint8* m_RealData;
	};

	ReadMe* pReadMe     = (ReadMe*)_pPtr;
	if (pReadMe->m_Offset == DL_NULL_PTR_OFFSET[DL_PTR_SIZE_HOST])
		pReadMe->m_RealData = 0x0;
	else
		pReadMe->m_RealData = _pBaseData + pReadMe->m_Offset;
};

static EDLError DLPatchLoadedPtrs( HDLContext         _Context,
								  SPatchedInstances* _pPatchedInstances,
								  const uint8*       _pInstance,
								  const SDLType*     _pType,
								  const uint8*       _pBaseData )
{
	// TODO: Optimize this please, linear search might not be the best if many subinstances is in file!
	if(_pPatchedInstances->IsPatched(_pInstance))
		return DL_ERROR_OK;

	_pPatchedInstances->m_lpPatched.Add(_pInstance);

	for(uint32 iMember = 0; iMember < _pType->m_nMembers; ++iMember)
	{
		const SDLMember& Member = _pType->m_lMembers[iMember];
		const uint8* pMemberData = _pInstance + Member.m_Offset[DL_PTR_SIZE_HOST];

		EDLType AtomType    = Member.AtomType();
		EDLType StorageType = Member.StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STR: DLPatchPtr(pMemberData, _pBaseData); break;
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pStructType = DLFindType(_Context, Member.m_TypeID);
						DLPatchLoadedPtrs(_Context, _pPatchedInstances, pMemberData, pStructType, _pBaseData);
					}
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						uint8** ppPtr = (uint8**)pMemberData;
						DLPatchPtr(pMemberData, _pBaseData);

						if(*ppPtr != 0x0)
						{
							const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
							DLPatchLoadedPtrs(_Context, _pPatchedInstances, *ppPtr, pSubType, _pBaseData);
						}
					}
					break;
					default:
						// ignore
						break;
				}
			}
			break;
			case DL_TYPE_ATOM_ARRAY:
			{
				if(StorageType == DL_TYPE_STORAGE_STR || StorageType == DL_TYPE_STORAGE_STRUCT)
				{
					DLPatchPtr(pMemberData, _pBaseData);
					const uint8* pArrayData = *(const uint8**)pMemberData;

					uint32 Count = *(uint32*)(pMemberData + sizeof(void*));

					if(Count > 0)
					{
						if(StorageType == DL_TYPE_STORAGE_STRUCT)
						{
							// patch sub-ptrs!
							const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
							uint32 Size = AlignUp(pSubType->m_Size[DL_PTR_SIZE_HOST], pSubType->m_Alignment[DL_PTR_SIZE_HOST]);

							for(uint32 iElemOffset = 0; iElemOffset < Count * Size; iElemOffset += Size)
								DLPatchLoadedPtrs(_Context, _pPatchedInstances, pArrayData + iElemOffset, pSubType, _pBaseData);
						}
						else
						{
							for(uint32 iElemOffset = 0; iElemOffset < Count * sizeof(char*); iElemOffset += sizeof(char*))
								DLPatchPtr(pArrayData + iElemOffset, _pBaseData);
						}
					}
				}
				else // pod
					DLPatchPtr(pMemberData, _pBaseData);
			}
			break;

			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				if(StorageType == DL_TYPE_STORAGE_STR)
				{
					for(pint iElemOffset = 0; iElemOffset < Member.m_Size[DL_PTR_SIZE_HOST]; iElemOffset += sizeof(char*))
						DLPatchPtr(pMemberData + iElemOffset, _pBaseData);
				}
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
			// ignore
			break;

		default:
			M_ASSERT(false && "Unknown atom type");
		}
	}
	return DL_ERROR_OK;
}

static void DLLoadTypeLibraryLoadDefaults(HDLContext _Context, const uint8* _pDefaultData, pint _DefaultDataSize)
{
	_Context->m_pDefaultInstances = (uint8*)_Context->m_DLAllocFuncs->m_Alloc(_DefaultDataSize * 2, sizeof(void*)); // times 2 here need to be fixed!

	uint8* pDst = _Context->m_pDefaultInstances;
	// ptr-patch and convert to native
	for(uint32 iType = 0; iType < _Context->m_nTypes; ++iType)
	{
		const SDLType* pType = _Context->m_TypeLookUp[iType].m_pType;
		for(uint32 iMember = 0; iMember < pType->m_nMembers; ++iMember)
		{
			SDLMember* pMember = (SDLMember*)pType->m_lMembers + iMember;
			if(pMember->m_DefaultValueOffset != DL_UINT32_MAX)
			{
				pDst = AlignUp(pDst, pMember->m_Alignment[DL_PTR_SIZE_HOST]);
				uint32 SrcOffset = pMember->m_DefaultValueOffset;
				pMember->m_DefaultValueOffset = uint32(pint(pDst) - pint(_Context->m_pDefaultInstances));
				uint8* pSrc = (uint8*)_pDefaultData + SrcOffset;

				SOneMemberType Dummy(pMember);

				pint BaseOffset = pint(pDst) - pint(_Context->m_pDefaultInstances);
				unsigned int NeededSize;
				DLInternalConvertNoHeader( _Context,
										   pSrc,
										   (unsigned char*)_pDefaultData,
										   pDst,
										   1337, // need to check this size ;) Should be the remainder of space in m_pDefaultInstances.
										   &NeededSize,
										   ENDIAN_LITTLE, ENDIAN_HOST,
										   DL_PTR_SIZE_32BIT, DL_PTR_SIZE_HOST,
										   Dummy.AsDLType(),
										   (unsigned int)BaseOffset ); // TODO: Ugly cast, remove plox!

				SPatchedInstances PI;
				DLPatchLoadedPtrs(_Context, &PI, pDst, Dummy.AsDLType(), _Context->m_pDefaultInstances);

				pDst += NeededSize;
			}
		}
	}
}

static void DLReadTLHeader(SDLTypeLibraryHeader* _pHeader, const uint8* _pData)
{
	memcpy(_pHeader, _pData, sizeof(SDLTypeLibraryHeader));

	if(ENDIAN_HOST == ENDIAN_BIG)
	{
		_pHeader->m_Id          = DLSwapEndian(_pHeader->m_Id);
		_pHeader->m_Version     = DLSwapEndian(_pHeader->m_Version);

		_pHeader->m_nTypes      = DLSwapEndian(_pHeader->m_nTypes);
		_pHeader->m_TypesOffset = DLSwapEndian(_pHeader->m_TypesOffset);
		_pHeader->m_TypesSize   = DLSwapEndian(_pHeader->m_TypesSize);

		_pHeader->m_nEnums      = DLSwapEndian(_pHeader->m_nEnums);
		_pHeader->m_EnumsOffset = DLSwapEndian(_pHeader->m_EnumsOffset);
		_pHeader->m_EnumsSize   = DLSwapEndian(_pHeader->m_EnumsSize);

		_pHeader->m_DefaultValuesOffset = DLSwapEndian(_pHeader->m_DefaultValuesOffset);
		_pHeader->m_DefaultValuesSize   = DLSwapEndian(_pHeader->m_DefaultValuesSize);
	}
}

EDLError DLLoadTypeLibrary(HDLContext _Context, const unsigned char* _pData, unsigned int _DataSize)
{
	if(_DataSize < sizeof(SDLTypeLibraryHeader))
		return DL_ERROR_MALFORMED_DATA;

	SDLTypeLibraryHeader Header;
	DLReadTLHeader(&Header, _pData);

	if(Header.m_Id != DL_TYPE_LIB_ID ) return DL_ERROR_MALFORMED_DATA;
	if(Header.m_Version != DL_VERSION) return DL_ERROR_VERSION_MISMATCH;

	// store type-info data.
	_Context->m_TypeInfoData = (uint8*)_Context->m_DLAllocFuncs->m_Alloc(Header.m_TypesSize, sizeof(void*));
	memcpy(_Context->m_TypeInfoData, _pData + Header.m_TypesOffset, Header.m_TypesSize);

	// store enum-info data.
	_Context->m_EnumInfoData = (uint8*)_Context->m_DLAllocFuncs->m_Alloc(Header.m_EnumsSize, sizeof(void*));
	memcpy(_Context->m_EnumInfoData, _pData + Header.m_EnumsOffset, Header.m_EnumsSize);

	// read type-lookup table
	SDLTypeLookup* _pFromData = (SDLTypeLookup*)(_pData + sizeof(SDLTypeLibraryHeader));
	for(uint32 i = 0; i < Header.m_nTypes; ++i)
	{
		SDLContext::STypeLookUp& Look = _Context->m_TypeLookUp[i];

		if(ENDIAN_HOST == ENDIAN_BIG)
		{
			Look.m_TypeID = DLSwapEndian(_pFromData->m_TypeID);
			uint32 Offset = DLSwapEndian(_pFromData->m_Offset);
			Look.m_pType  = (SDLType*)(_Context->m_TypeInfoData + Offset);

			Look.m_pType->m_Size[DL_PTR_SIZE_32BIT]      = DLSwapEndian(Look.m_pType->m_Size[DL_PTR_SIZE_32BIT]);
			Look.m_pType->m_Size[DL_PTR_SIZE_64BIT]      = DLSwapEndian(Look.m_pType->m_Size[DL_PTR_SIZE_64BIT]);
			Look.m_pType->m_Alignment[DL_PTR_SIZE_32BIT] = DLSwapEndian(Look.m_pType->m_Alignment[DL_PTR_SIZE_32BIT]);
			Look.m_pType->m_Alignment[DL_PTR_SIZE_64BIT] = DLSwapEndian(Look.m_pType->m_Alignment[DL_PTR_SIZE_64BIT]);
			Look.m_pType->m_nMembers                     = DLSwapEndian(Look.m_pType->m_nMembers);

			for(uint32 i = 0; i < Look.m_pType->m_nMembers; ++i)
			{
				SDLMember* pMember = Look.m_pType->m_lMembers + i;

				pMember->m_Type                         = DLSwapEndian(pMember->m_Type);
				pMember->m_TypeID	                    = DLSwapEndian(pMember->m_TypeID);
				pMember->m_Size[DL_PTR_SIZE_32BIT]      = DLSwapEndian(pMember->m_Size[DL_PTR_SIZE_32BIT]);
				pMember->m_Size[DL_PTR_SIZE_64BIT]      = DLSwapEndian(pMember->m_Size[DL_PTR_SIZE_64BIT]);
				pMember->m_Offset[DL_PTR_SIZE_32BIT]    = DLSwapEndian(pMember->m_Offset[DL_PTR_SIZE_32BIT]);
				pMember->m_Offset[DL_PTR_SIZE_64BIT]    = DLSwapEndian(pMember->m_Offset[DL_PTR_SIZE_64BIT]);
				pMember->m_Alignment[DL_PTR_SIZE_32BIT] = DLSwapEndian(pMember->m_Alignment[DL_PTR_SIZE_32BIT]);
				pMember->m_Alignment[DL_PTR_SIZE_64BIT] = DLSwapEndian(pMember->m_Alignment[DL_PTR_SIZE_64BIT]);
				pMember->m_DefaultValueOffset           = DLSwapEndian(pMember->m_DefaultValueOffset);
			}
		}
		else
		{
			Look.m_TypeID = _pFromData->m_TypeID;
			Look.m_pType  = (SDLType*)(_Context->m_TypeInfoData + _pFromData->m_Offset);
		}

		++_pFromData;
	}

	_Context->m_nTypes += Header.m_nTypes;

	DLLoadTypeLibraryLoadDefaults(_Context, _pData + Header.m_DefaultValuesOffset, Header.m_DefaultValuesSize);

	// read enum-lookup table
	SDLTypeLookup* _pEnumFromData = (SDLTypeLookup*)(_pData + Header.m_TypesOffset + Header.m_TypesSize);
	for(uint32 i = 0; i < Header.m_nEnums; ++i)
	{
		SDLContext::SEnumLookUp& Look = _Context->m_EnumLookUp[i];

		if(ENDIAN_HOST == ENDIAN_BIG)
		{
			Look.m_EnumID = DLSwapEndian(_pEnumFromData->m_TypeID);
			uint32 Offset = DLSwapEndian(_pEnumFromData->m_Offset);
			Look.m_pEnum  = (SDLEnum*)(_Context->m_EnumInfoData + Offset);
			Look.m_pEnum->m_EnumID  = DLSwapEndian(Look.m_pEnum->m_EnumID);
			Look.m_pEnum->m_nValues = DLSwapEndian(Look.m_pEnum->m_nValues);
			for(uint32 i = 0; i < Look.m_pEnum->m_nValues; ++i)
			{
				SDLEnumValue* pValue = Look.m_pEnum->m_lValues + i;
				pValue->m_Value = DLSwapEndian(pValue->m_Value);
			}
		}
		else
		{
			Look.m_EnumID = _pEnumFromData->m_TypeID;
			Look.m_pEnum  = (SDLEnum*)(_Context->m_EnumInfoData + _pEnumFromData->m_Offset);
		}

		++_pEnumFromData;
	}

	_Context->m_nEnums += Header.m_nEnums;

	return DL_ERROR_OK;
}

EDLError DLLoadInstanceInplace(HDLContext _Context, void* _pInstance, const unsigned char* _pData, unsigned int _DataSize)
{
	SDLDataHeader* pHeader = (SDLDataHeader*)_pData;

	if(_DataSize < sizeof(SDLDataHeader))       return DL_ERROR_MALFORMED_DATA;
	if(pHeader->m_Id == DL_TYPE_DATA_ID_SWAPED) return DL_ERROR_ENDIAN_ERROR;
	if(pHeader->m_Id != DL_TYPE_DATA_ID )       return DL_ERROR_MALFORMED_DATA;
	if(pHeader->m_Version != DL_VERSION)        return DL_ERROR_VERSION_MISMATCH;

	const SDLType* pType = DLFindType(_Context, pHeader->m_RootInstanceType);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	memcpy(_pInstance, _pData + sizeof(SDLDataHeader), pHeader->m_InstanceSize);

	SPatchedInstances PI;
	DLPatchLoadedPtrs(_Context, &PI, (uint8*)_pInstance, pType, (uint8*)_pInstance);

	return DL_ERROR_OK;
}

#if 1 // BinaryWriterVerbose
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...)
#else
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...) printf("DL:" _Fmt, ##__VA_ARGS__)
#endif

// TODO: This should use the CDLBinaryWriter!
class CDLBinStoreContext
{
public:
	CDLBinStoreContext(uint8* _OutData, pint _OutDataSize, bool _Dummy)
		: m_OutData(_OutData)
		, m_OutDataSize(_OutDataSize)
		, m_Dummy(_Dummy)
		, m_WritePtr(0)
		, m_WriteEnd(0) {}

	void Write(void* _pData, pint _DataSize)
	{
		if(!m_Dummy)
		{
			M_ASSERT(m_WritePtr < m_OutDataSize);
			DL_LOG_BIN_WRITER_VERBOSE("Write: %lu + %lu (%lu)", m_WritePtr, _DataSize, *(pint*)_pData);
			memcpy(m_OutData + m_WritePtr, _pData, _DataSize);
		}
		m_WritePtr += _DataSize;
		m_WriteEnd = Max(m_WriteEnd, m_WritePtr);
	};

	void Align(pint _Alignment)
	{
		if(IsAlign((void*)m_WritePtr, _Alignment))
			return;

		pint MoveMe = AlignUp(m_WritePtr, _Alignment) - m_WritePtr;

		if(!m_Dummy)
		{
			memset(m_OutData + m_WritePtr, 0x0, MoveMe);
			DL_LOG_BIN_WRITER_VERBOSE("Align: %lu + %lu", m_WritePtr, MoveMe);
		}
		m_WritePtr += MoveMe;
		m_WriteEnd = Max(m_WriteEnd, m_WritePtr);
	}

	pint Tell()              { return m_WritePtr; }
	void SeekSet(pint Pos)   { DL_LOG_BIN_WRITER_VERBOSE("Seek set: %lu", Pos);                       m_WritePtr = Pos;  }
	void SeekEnd()           { DL_LOG_BIN_WRITER_VERBOSE("Seek end: %lu", m_WriteEnd);                m_WritePtr = m_WriteEnd; }
	void Reserve(pint Bytes) { DL_LOG_BIN_WRITER_VERBOSE("Reserve: end %lu + %lu", m_WriteEnd, Bytes); m_WriteEnd += Bytes; }

	pint FindWrittenPtr(void* _Ptr)
	{
		for (uint iPtr = 0; iPtr < m_WrittenPtrs.Len(); ++iPtr)
			if(m_WrittenPtrs[iPtr].m_pPtr == _Ptr)
				return m_WrittenPtrs[iPtr].m_Pos;

		return pint(-1);
	}

	void AddWrittenPtr(void* _Ptr, pint _Pos) { m_WrittenPtrs.Add(SWrittenPtr(_Pos, _Ptr)); }

private:
	struct SWrittenPtr
	{
 		SWrittenPtr() {}
 		SWrittenPtr(pint _Pos, void* _pPtr) : m_Pos(_Pos), m_pPtr(_pPtr) {}
		pint  m_Pos;
		void* m_pPtr;
	};

	CArrayStatic<SWrittenPtr, 128> m_WrittenPtrs;

	uint8* m_OutData;
	pint   m_OutDataSize;
	bool   m_Dummy;
	pint   m_WritePtr;
	pint   m_WriteEnd;
};

static void DLInternalStoreString(const uint8* _pInstance, CDLBinStoreContext* _pStoreContext)
{
	char* pTheString = *(char**)_pInstance;
	pint Pos = _pStoreContext->Tell();
	_pStoreContext->SeekEnd();
	pint Offset = _pStoreContext->Tell();
	_pStoreContext->Write(pTheString, strlen(pTheString) + 1);
	_pStoreContext->SeekSet(Pos);
	_pStoreContext->Write(&Offset, sizeof(pint));	
}

static EDLError DLInternalStoreInstance(HDLContext _Context, const SDLType* _pType, uint8* _pInstance, CDLBinStoreContext* _pStoreContext);

static EDLError DLInternalStoreMember(HDLContext _Context, const SDLMember* _pMember, uint8* _pInstance, CDLBinStoreContext* _pStoreContext)
{
	_pStoreContext->Align(_pMember->m_Alignment[DL_PTR_SIZE_HOST]);

	EDLType AtomType    = EDLType(_pMember->m_Type & DL_TYPE_ATOM_MASK);
	EDLType StorageType = EDLType(_pMember->m_Type & DL_TYPE_STORAGE_MASK);

	switch (AtomType)
	{
		case DL_TYPE_ATOM_POD:
		{
			switch(StorageType)
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const SDLType* pSubType = DLFindType(_Context, _pMember->m_TypeID);
					if(pSubType == 0x0)
					{
						DL_LOG_DL_ERROR("Could not find subtype for member %s", _pMember->m_Name);
						return DL_ERROR_TYPE_NOT_FOUND;
					}
					DLInternalStoreInstance(_Context, pSubType, _pInstance, _pStoreContext );
				}
				break;
				case DL_TYPE_STORAGE_STR:
					DLInternalStoreString(_pInstance, _pStoreContext);
					break;
				case DL_TYPE_STORAGE_PTR:
				{
					uint8* pData = *(uint8**)_pInstance;
					pint Offset = _pStoreContext->FindWrittenPtr(pData);

					if(pData == 0x0) // Null-pointer, store pint(-1) to signal to patching!
					{
						M_ASSERT(Offset == pint(-1) && "This pointer should not have been found among the written ptrs!");
						// keep the -1 in Offset and store it to ptr.
					}
					else if(Offset == pint(-1)) // has not been written yet!
					{
						pint Pos = _pStoreContext->Tell();
						_pStoreContext->SeekEnd();

						const SDLType* pSubType = DLFindType(_Context, _pMember->m_TypeID);
						pint Size = AlignUp(pSubType->m_Size[DL_PTR_SIZE_HOST], pSubType->m_Alignment[DL_PTR_SIZE_HOST]);
						_pStoreContext->Align(pSubType->m_Alignment[DL_PTR_SIZE_HOST]);

						Offset = _pStoreContext->Tell();

						// write data!
						_pStoreContext->Reserve(Size); // reserve space for ptr so subdata is placed correctly

						_pStoreContext->AddWrittenPtr(pData, Offset);

						DLInternalStoreInstance(_Context, pSubType, pData, _pStoreContext);

						_pStoreContext->SeekSet(Pos);
					}

					_pStoreContext->Write(&Offset, sizeof(pint));
				}
				break;
				default: // default is a standard pod-type
					M_ASSERT(_pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
					_pStoreContext->Write(_pInstance, _pMember->m_Size[DL_PTR_SIZE_HOST]);
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch(StorageType)
			{
				case DL_TYPE_STORAGE_STRUCT:
					_pStoreContext->Write(_pInstance, _pMember->m_Size[DL_PTR_SIZE_HOST]); // TODO: I Guess that this is a bug! Will it fix ptrs well?
					break;
				case DL_TYPE_STORAGE_STR:
				{
					uint32 Count = _pMember->m_Size[DL_PTR_SIZE_HOST] / sizeof(char*);

					for(uint32 iElem = 0; iElem < Count; ++iElem)
						DLInternalStoreString(_pInstance + (iElem * sizeof(char*)), _pStoreContext);
				}
				break;
				default: // default is a standard pod-type
					M_ASSERT(_pMember->IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
					_pStoreContext->Write(_pInstance, _pMember->m_Size[DL_PTR_SIZE_HOST]);
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_ARRAY:
		{
			pint Size = 0;
			const SDLType* pSubType = 0x0;

			uint8* pDataPtr = _pInstance;
			uint32 Count = *(uint32*)(pDataPtr + sizeof(void*));

			pint Offset = 0;

			if(Count > 0)
			{
				pint Pos = _pStoreContext->Tell();
				_pStoreContext->SeekEnd();

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
						pSubType = DLFindType(_Context, _pMember->m_TypeID);
						Size = AlignUp(pSubType->m_Size[DL_PTR_SIZE_HOST], pSubType->m_Alignment[DL_PTR_SIZE_HOST]);
						_pStoreContext->Align(pSubType->m_Alignment[DL_PTR_SIZE_HOST]);
						break;
					case DL_TYPE_STORAGE_STR:
						Size = sizeof(void*);
						_pStoreContext->Align(Size);
						break;
					default:
						Size = DLPodSize(_pMember->m_Type);
						_pStoreContext->Align(Size);
				}

				Offset = _pStoreContext->Tell();

				// write data!
				_pStoreContext->Reserve(Count * Size); // reserve space for array so subdata is placed correctly

				uint8* pData = *(uint8**)pDataPtr;

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
						for (uint iElem = 0; iElem < Count; ++iElem)
							DLInternalStoreInstance(_Context, pSubType, pData + (iElem * Size), _pStoreContext);
						break;
					case DL_TYPE_STORAGE_STR:
						for (uint iElem = 0; iElem < Count; ++iElem)
							DLInternalStoreString(pData + (iElem * Size), _pStoreContext);
						break;
					default:
						for (uint iElem = 0; iElem < Count; ++iElem)
							_pStoreContext->Write(pData + (iElem * Size), Size); break;
				}

				_pStoreContext->SeekSet(Pos);
			}

			// make room for ptr
			_pStoreContext->Write(&Offset, sizeof(pint));

			// write count
			_pStoreContext->Write(&Count, sizeof(uint32));
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_BITFIELD:
			_pStoreContext->Write(_pInstance, _pMember->m_Size[DL_PTR_SIZE_HOST]);
		break;

		default:
			M_ASSERT(false && "Invalid ATOM-type!");
	}

	return DL_ERROR_OK;
}

static EDLError DLInternalStoreInstance(HDLContext _Context, const SDLType* _pType, uint8* _pInstance, CDLBinStoreContext* _pStoreContext)
{
	bool bLastWasBF = false;

	for(uint32 iMember = 0; iMember < _pType->m_nMembers; ++iMember)
	{
		const SDLMember& Member = _pType->m_lMembers[iMember];

		if(!bLastWasBF || Member.AtomType() != DL_TYPE_ATOM_BITFIELD)
		{
			EDLError Err = DLInternalStoreMember(_Context, &Member, _pInstance + Member.m_Offset[DL_PTR_SIZE_HOST], _pStoreContext);
			if(Err != DL_ERROR_OK)
				return Err;
		}

		bLastWasBF = Member.AtomType() == DL_TYPE_ATOM_BITFIELD;
	}

	return DL_ERROR_OK;
}

EDLError DLStoreInstace(HDLContext _Context, StrHash _TypeHash, void* _pInstance, unsigned char* _pData, unsigned int _DataSize)
{
	const SDLType* pType = DLFindType(_Context, _TypeHash);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	// write header
	SDLDataHeader Header;
	Header.m_Id = DL_TYPE_DATA_ID;
	Header.m_Version = DL_VERSION;
	Header.m_RootInstanceType = _TypeHash;
	Header.m_InstanceSize = 0;
	Header.m_64BitPtr = sizeof(void*) == 8 ? 1 : 0;

	memcpy(_pData, &Header, sizeof(SDLDataHeader));

	CDLBinStoreContext StoreContext(_pData + sizeof(SDLDataHeader), _DataSize - sizeof(SDLDataHeader), false);

	StoreContext.Reserve(pType->m_Size[DL_PTR_SIZE_HOST]);
	StoreContext.AddWrittenPtr(_pInstance, 0); // if pointer refere to root-node, it can be found at offset 0

	EDLError err = DLInternalStoreInstance(_Context, pType, (uint8*)_pInstance, &StoreContext);

	// write instance size!
	SDLDataHeader* pHeader = (SDLDataHeader*)_pData;
	StoreContext.SeekEnd();
	pHeader->m_InstanceSize = (uint32)StoreContext.Tell();

	return err;
}

EDLError DLInstaceSizeStored(HDLContext _Context, StrHash _TypeHash, void* _pInstance, unsigned int* _pDataSize)
{
	const SDLType* pType = DLFindType(_Context, _TypeHash);
	if(pType == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	CDLBinStoreContext StoreContext(0, 0, true);

	StoreContext.Reserve(pType->m_Size[DL_PTR_SIZE_HOST]);
	StoreContext.AddWrittenPtr(_pInstance, 0); // if pointer refere to root-node, it can be found at offset 0

	EDLError err = DLInternalStoreInstance(_Context, pType, (uint8*)_pInstance, &StoreContext);

	StoreContext.SeekEnd();
	*_pDataSize = StoreContext.Tell() + sizeof(SDLDataHeader);

	return err;
}

const char* DLErrorToString(EDLError _Err)
{
#define M_DL_ERR_TO_STR(ERR) case ERR: return #ERR
	switch(_Err)
	{
		M_DL_ERR_TO_STR(DL_ERROR_OK);
		M_DL_ERR_TO_STR(DL_ERROR_MALFORMED_DATA);
		M_DL_ERR_TO_STR(DL_ERROR_VERSION_MISMATCH);
		M_DL_ERR_TO_STR(DL_ERROR_OUT_OF_LIBRARY_MEMORY);
		M_DL_ERR_TO_STR(DL_ERROR_OUT_OF_INSTANCE_MEMORY);
		M_DL_ERR_TO_STR(DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR);
		M_DL_ERR_TO_STR(DL_ERROR_TYPE_NOT_FOUND);
		M_DL_ERR_TO_STR(DL_ERROR_MEMBER_NOT_FOUND);
		M_DL_ERR_TO_STR(DL_ERROR_BUFFER_TO_SMALL);
		M_DL_ERR_TO_STR(DL_ERROR_ENDIAN_ERROR);
		M_DL_ERR_TO_STR(DL_ERROR_INVALID_PARAMETER);
		M_DL_ERR_TO_STR(DL_ERROR_UNSUPORTED_OPERATION);

		M_DL_ERR_TO_STR(DL_ERROR_TXT_PARSE_ERROR);
		M_DL_ERR_TO_STR(DL_ERROR_TXT_MEMBER_MISSING);
		M_DL_ERR_TO_STR(DL_ERROR_TXT_MEMBER_SET_TWICE);

		M_DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_NOT_FOUND);

		M_DL_ERR_TO_STR(DL_ERROR_INTERNAL_ERROR);
		default: return "Unknown error!";
	}
#undef M_DL_ERR_TO_STR
}

unsigned int DLInstancePtrSize(const unsigned char* _pData, unsigned int _DataSize)
{
	if(_DataSize < sizeof(SDLDataHeader)) return 0;

	return ((SDLDataHeader*)_pData)->m_64BitPtr ? 8 : 4;
}

ECpuEndian DLInstanceEndian(const unsigned char* _pData, unsigned int _DataSize)
{
	if(_DataSize < sizeof(SDLDataHeader)) return ENDIAN_HOST;

	if(((SDLDataHeader*)_pData)->m_Id == DL_TYPE_DATA_ID_SWAPED)
		return DLOtherEndian(ENDIAN_HOST);

	return ENDIAN_HOST;
}

StrHash DLInstanceRootType(const unsigned char* _pData, unsigned int _DataSize)
{
	const SDLDataHeader* pHeader = (const SDLDataHeader*)_pData;

	if(_DataSize < sizeof(SDLDataHeader))       return 0;
	if(pHeader->m_Id == DL_TYPE_DATA_ID_SWAPED) return 0;
	if(pHeader->m_Id != DL_TYPE_DATA_ID )       return 0;
	if(pHeader->m_Version != DL_VERSION)        return 0;

	return pHeader->m_RootInstanceType;
}
