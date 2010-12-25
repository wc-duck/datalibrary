/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl.h>
#include "dl_types.h"
#include "dl_binary_writer.h"
#include "container/dl_array.h"

struct SInstance
{
	SInstance() {}
	SInstance(const uint8* _pAddress, const SDLType* _pType, pint _ArrayCount, dl_type_t _Type)
		: m_pAddress(_pAddress)
		, m_ArrayCount(_ArrayCount)
		, m_pType(_pType)
		, m_Type(_Type)
		{ }

	const uint8*   m_pAddress;
	pint           m_ArrayCount;
	pint           m_OffsetAfterPatch;
	const SDLType* m_pType;
	dl_type_t      m_Type;
};

class SConvertContext
{
public:
	SConvertContext( dl_endian_t _SourceEndian, dl_endian_t _TargetEndian, dl_ptr_size_t _SourcePtrSize, dl_ptr_size_t _TargetPtrSize )
		: m_SourceEndian(_SourceEndian)
		, m_TargetEndian(_TargetEndian)
		, m_SourcePtrSize(_SourcePtrSize)
		, m_TargetPtrSize(_TargetPtrSize)
	{}

	bool IsSwapped(const uint8* _Ptr)
	{
		for(unsigned int iInstance = 0; iInstance < m_lInstances.Len(); ++iInstance)
			if(_Ptr == m_lInstances[iInstance].m_pAddress)
				return true;
		return false;
	}

	dl_endian_t m_SourceEndian;
	dl_endian_t m_TargetEndian;
	dl_ptr_size_t m_SourcePtrSize;
	dl_ptr_size_t m_TargetPtrSize;

	CArrayStatic<SInstance, 128> m_lInstances;

	struct PatchPos
	{
		PatchPos() {}
		PatchPos(pint _Pos, pint _OldOffset)
			: m_Pos(_Pos)
			, m_OldOffset(_OldOffset)
		{ }
		pint m_Pos;
		pint m_OldOffset;
	};

	CArrayStatic<PatchPos, 256> m_lPatchOffset;
};

static inline void DLSwapHeader(SDLDataHeader* _pHeader)
{
	_pHeader->m_Id               = DLSwapEndian(_pHeader->m_Id);
	_pHeader->m_Version          = DLSwapEndian(_pHeader->m_Version);
	_pHeader->m_RootInstanceType = DLSwapEndian(_pHeader->m_RootInstanceType);
	_pHeader->m_InstanceSize     = DLSwapEndian(_pHeader->m_InstanceSize);
}

static pint DLInternalReadPtrData( const uint8*  _pPtrData,
								   dl_endian_t   _SourceEndian,
								   dl_ptr_size_t _PtrSize )
{
	switch(_PtrSize)
	{
		case DL_PTR_SIZE_32BIT:
		{
			uint32 Offset = *(uint32*)_pPtrData;

			if(_SourceEndian != DL_ENDIAN_HOST)
				return (pint)DLSwapEndian(Offset);
			else
				return (pint)Offset;
		}
		break;
		case DL_PTR_SIZE_64BIT:
		{
			uint64 Offset = *(uint64*)_pPtrData;

			if(_SourceEndian != DL_ENDIAN_HOST)
				return (pint)DLSwapEndian(Offset);
			else
				return (pint)Offset;
		}
		break;
		default:
			DL_ASSERT(false && "Invalid ptr-size");
	}

	return 0;
}

static void DLInternalReadArrayData( const uint8*  _pArrayData,
									 pint*         _pOffset,
									 uint32*       _pCount,
									 dl_endian_t   _SourceEndian,
									 dl_ptr_size_t _PtrSize )
{
	union { const uint8* m_u8; const uint32* m_u32; const uint64* m_u64; } pArrayData;
	pArrayData.m_u8 = _pArrayData;

	switch(_PtrSize)
	{
		case DL_PTR_SIZE_32BIT:
		{
			uint32 Offset = pArrayData.m_u32[0];
			uint32 Count  = pArrayData.m_u32[1];

			if(_SourceEndian != DL_ENDIAN_HOST)
			{
				*_pOffset = (pint)DLSwapEndian(Offset);
				*_pCount  = DLSwapEndian(Count);
			}
			else
			{
				*_pOffset = (pint)(Offset);
				*_pCount  = Count;
			}
		}
		break;

		case DL_PTR_SIZE_64BIT:
		{
			uint64 Offset = pArrayData.m_u64[0];
			uint32 Count  = pArrayData.m_u32[2];

			if(_SourceEndian != DL_ENDIAN_HOST)
			{
				*_pOffset = (pint)DLSwapEndian(Offset);
				*_pCount  = DLSwapEndian(Count);
			}
			else
			{
				*_pOffset = (pint)(Offset);
				*_pCount  = Count;
			}
		}
		break;

		default:
			DL_ASSERT(false && "Invalid ptr-size");
	}
}

static dl_error_t DLInternalConvertCollectInstances( dl_ctx_t       _Context, 
												   const SDLType*   _pType, 
												   const uint8*     _pData, 
												   const uint8*     _pBaseData,
												   SConvertContext& _ConvertContext )
{
	for(uint32 iMember = 0; iMember < _pType->m_nMembers; ++iMember)
	{
		const SDLMember& Member = _pType->m_lMembers[iMember];
		const uint8* pMemberData = _pData + Member.m_Offset[_ConvertContext.m_SourcePtrSize];

		dl_type_t AtomType    = Member.AtomType();
		dl_type_t StorageType = Member.StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				if(StorageType == DL_TYPE_STORAGE_STR)
				{
					pint Offset = DLInternalReadPtrData( pMemberData, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize );
					_ConvertContext.m_lInstances.Add(SInstance(_pBaseData + Offset, 0x0, 1337, Member.m_Type));
				}
				else if( StorageType == DL_TYPE_STORAGE_PTR )
				{
					const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
					if(pSubType == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;

					pint Offset = DLInternalReadPtrData(pMemberData, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);

					const uint8* pPtrData = _pBaseData + Offset;

					if(Offset != DL_NULL_PTR_OFFSET[_ConvertContext.m_SourcePtrSize] && !_ConvertContext.IsSwapped(pPtrData))
					{
						_ConvertContext.m_lInstances.Add(SInstance(pPtrData, pSubType, 0, Member.m_Type));
						DLInternalConvertCollectInstances(_Context, pSubType, _pBaseData + Offset, _pBaseData, _ConvertContext);
					}
				}
				break;
			}
			break;
			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						for (pint ElemOffset = 0; ElemOffset < Member.m_Size[_ConvertContext.m_SourcePtrSize]; ElemOffset += pSubType->m_Size[_ConvertContext.m_SourcePtrSize])
							DLInternalConvertCollectInstances(_Context, pSubType, pMemberData + ElemOffset, _pBaseData, _ConvertContext);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						// TODO: This might be optimized if we look at all the data in i inline-array of strings as 1 instance continious in memory.
						// I am not sure if that is true for all cases right now!

						uint32 PtrSize = (uint32)DLPtrSize(_ConvertContext.m_SourcePtrSize);
						uint32 Count = Member.m_Size[_ConvertContext.m_SourcePtrSize] / PtrSize;

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint Offset = DLInternalReadPtrData(_pData + (iElem * PtrSize), _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);
							_ConvertContext.m_lInstances.Add(SInstance(_pBaseData + Offset, 0x0, Count, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STR)));
						}
					}
					break;
					default:
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
						// ignore
				}
			}
			break;

			case DL_TYPE_ATOM_ARRAY:
			{
				pint Offset = 0; uint32 Count = 0;
				DLInternalReadArrayData( pMemberData, &Offset, &Count, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize );

				if(Offset == DL_NULL_PTR_OFFSET[_ConvertContext.m_SourcePtrSize])
				{
					DL_ASSERT( Count == 0 );
					break;
				}

				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STR:
					{
						// TODO: This might be optimized if we look at all the data in i inline-array of strings as 1 instance continious in memory.
						// I am not sure if that is true for all cases right now!

						uint32 PtrSize = (uint32)DLPtrSize(_ConvertContext.m_SourcePtrSize);
						const uint8* pArrayData = _pBaseData + Offset;

						_ConvertContext.m_lInstances.Add(SInstance(_pBaseData + Offset, 0x0, Count, Member.m_Type));

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint ElemOffset = DLInternalReadPtrData(pArrayData + (iElem * PtrSize), _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);
							_ConvertContext.m_lInstances.Add(SInstance(_pBaseData + ElemOffset, 0x0, Count, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STR)));
						}
					}
					break;

					case DL_TYPE_STORAGE_STRUCT:
					{
						DLInternalReadArrayData( pMemberData, &Offset, &Count, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize );

						const uint8* pArrayData = _pBaseData + Offset;

						const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						_ConvertContext.m_lInstances.Add(SInstance(pArrayData, pSubType, Count, Member.m_Type));

						for (pint ElemOffset = 0; ElemOffset < pSubType->m_Size[_ConvertContext.m_SourcePtrSize] * Count; ElemOffset += pSubType->m_Size[_ConvertContext.m_SourcePtrSize])
							DLInternalConvertCollectInstances(_Context, pSubType, pArrayData + ElemOffset, _pBaseData, _ConvertContext);
					}
					break;

					default:
					{
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
						DLInternalReadArrayData( pMemberData, &Offset, &Count, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize );
						_ConvertContext.m_lInstances.Add(SInstance(_pBaseData + Offset, 0x0, Count, Member.m_Type));
					}
					break;
				}
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
				// ignore
				break;

			default:
				DL_ASSERT(false && "Invalid ATOM-type!");
		}
	}

	return DL_ERROR_OK;
}

template<typename T>
static T DLConvertBitFieldFormat(T _OldValue, const SDLMember* _plBFMember, uint32 _nBFMembers, SConvertContext& _ConvertContext)
{
	if(_ConvertContext.m_SourceEndian != DL_ENDIAN_HOST)
		_OldValue = DLSwapEndian(_OldValue);

	T NewValue = 0;

	for( uint32 iBFMember = 0; iBFMember < _nBFMembers; ++iBFMember)
	{
		const SDLMember& BFMember = _plBFMember[iBFMember];

		uint32 BFBits   = BFMember.BitFieldBits();
		uint32 BFOffset = BFMember.BitFieldOffset();
		uint32 BFSourceOffset = DLBitFieldOffset(_ConvertContext.m_SourceEndian, sizeof(T), BFOffset, BFBits);
		uint32 BFTargetOffset = DLBitFieldOffset(_ConvertContext.m_TargetEndian, sizeof(T), BFOffset, BFBits);

 		T Extracted = DL_EXTRACT_BITS(_OldValue, T(BFSourceOffset), T(BFBits));
 		NewValue    = (T)DL_INSERT_BITS(NewValue, Extracted, T(BFTargetOffset), T(BFBits));
	}

	if(_ConvertContext.m_SourceEndian != DL_ENDIAN_HOST)
		return DLSwapEndian(NewValue);

	return NewValue;
}

static dl_error_t DLInternalConvertWriteStruct( dl_ctx_t         _Context,
											    const uint8*     _pData,
											    const SDLType*   _pType,
											    SConvertContext& _ConvertContext,
											    CDLBinaryWriter& _Writer )
{
	_Writer.Align(_pType->m_Alignment[_ConvertContext.m_TargetPtrSize]);
	pint Pos = _Writer.Tell();
	_Writer.Reserve(_pType->m_Size[_ConvertContext.m_TargetPtrSize]);

	for(uint32 iMember = 0; iMember < _pType->m_nMembers; ++iMember)
	{
		const SDLMember& Member  = _pType->m_lMembers[iMember];
		const uint8* pMemberData = _pData + Member.m_Offset[_ConvertContext.m_SourcePtrSize];

		_Writer.Align(Member.m_Alignment[_ConvertContext.m_TargetPtrSize]);

		dl_type_t AtomType    = Member.AtomType();
		dl_type_t StorageType = Member.StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				switch (StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;
						DLInternalConvertWriteStruct(_Context, pMemberData, pSubType, _ConvertContext, _Writer);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						pint Offset = DLInternalReadPtrData(pMemberData, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);
						_ConvertContext.m_lPatchOffset.Add(SConvertContext::PatchPos(_Writer.Tell(), Offset));	
						_Writer.WritePtr(0x0);
					}
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						pint Offset = DLInternalReadPtrData(pMemberData, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);

						if (Offset != DL_NULL_PTR_OFFSET[_ConvertContext.m_SourcePtrSize])
							_ConvertContext.m_lPatchOffset.Add(SConvertContext::PatchPos(_Writer.Tell(), Offset));

						_Writer.WritePtr(pint(-1));
					}
					break;
					default:
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
						_Writer.WriteSwap(pMemberData, Member.m_Size[_ConvertContext.m_SourcePtrSize]);
						break;
				}
			}
			break;
			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				switch(StorageType)
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pSubType = DLFindType(_Context, Member.m_TypeID);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						pint MemberSize  = Member.m_Size[_ConvertContext.m_SourcePtrSize];
						pint SubtypeSize = pSubType->m_Size[_ConvertContext.m_SourcePtrSize];
						for (pint ElemOffset = 0; ElemOffset < MemberSize; ElemOffset += SubtypeSize)
							DLInternalConvertWriteStruct(_Context, pMemberData + ElemOffset, pSubType, _ConvertContext, _Writer);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						pint PtrSizeSource = DLPtrSize(_ConvertContext.m_SourcePtrSize);
						pint PtrSizeTarget = DLPtrSize(_ConvertContext.m_TargetPtrSize);
						uint32 Count = Member.m_Size[_ConvertContext.m_SourcePtrSize] / (uint32)PtrSizeSource;
						pint Pos = _Writer.Tell();

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint OldOffset = DLInternalReadPtrData(pMemberData + (iElem * PtrSizeSource), _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);
							_ConvertContext.m_lPatchOffset.Add(SConvertContext::PatchPos(Pos + (iElem * PtrSizeTarget), OldOffset));
						}

						_Writer.WriteZero(Member.m_Size[_ConvertContext.m_TargetPtrSize]);
					}
					break;
					default:
					{
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);

						pint   PodSize   = DLPodSize(Member.m_Type);
						uint32 ArraySize = Member.m_Size[_ConvertContext.m_SourcePtrSize];

						switch(PodSize)
						{
							case 1: _Writer.WriteArray((uint8* )pMemberData, ArraySize / sizeof(uint8) );  break;
							case 2: _Writer.WriteArray((uint16*)pMemberData, ArraySize / sizeof(uint16) ); break;
							case 4: _Writer.WriteArray((uint32*)pMemberData, ArraySize / sizeof(uint32) ); break;
							case 8: _Writer.WriteArray((uint64*)pMemberData, ArraySize / sizeof(uint64) ); break;
							default:
								DL_ASSERT(false && "Not supported pod-size!");
						}
					}
					break;
				}
			}
			break;

			case DL_TYPE_ATOM_ARRAY:
			{
				pint Offset = 0; uint32 Count = 0;
				DLInternalReadArrayData( pMemberData, &Offset, &Count, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize );

				if(Offset != DL_NULL_PTR_OFFSET[_ConvertContext.m_SourcePtrSize])
					_ConvertContext.m_lPatchOffset.Add(SConvertContext::PatchPos(_Writer.Tell(), Offset));
				else
					Offset = DL_NULL_PTR_OFFSET[_ConvertContext.m_TargetPtrSize];

				_Writer.WritePtr(Offset);
				_Writer.Write(*(uint32*)(pMemberData + DLPtrSize(_ConvertContext.m_SourcePtrSize)));
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
			{
				uint32 j = iMember;

				do { j++; } while(j < _pType->m_nMembers && _pType->m_lMembers[j].AtomType() == DL_TYPE_ATOM_BITFIELD);

				if(_ConvertContext.m_SourceEndian != _ConvertContext.m_TargetEndian)
				{
					uint32 nBFMembers = j - iMember;

					switch(Member.m_Size[_ConvertContext.m_SourcePtrSize])
					{
						case 1: _Writer.Write( DLConvertBitFieldFormat( *(uint8*)pMemberData, &Member, nBFMembers, _ConvertContext) ); break;
						case 2: _Writer.Write( DLConvertBitFieldFormat(*(uint16*)pMemberData, &Member, nBFMembers, _ConvertContext) ); break;
						case 4: _Writer.Write( DLConvertBitFieldFormat(*(uint32*)pMemberData, &Member, nBFMembers, _ConvertContext) ); break;
						case 8: _Writer.Write( DLConvertBitFieldFormat(*(uint64*)pMemberData, &Member, nBFMembers, _ConvertContext) ); break;
						default:
							DL_ASSERT(false && "Not supported pod-size or bitfield-size!");
					}
				}
				else
					_Writer.Write(pMemberData, Member.m_Size[_ConvertContext.m_SourcePtrSize]);

				iMember = j - 1;
			}
			break;

			default:
				DL_ASSERT(false && "Invalid ATOM-type!");
		}
	}

	// we need to write our entire size with zeroes. Our entire size might be less than the sum of teh members.
	pint PosDiff = _Writer.Tell() - Pos;

	if(PosDiff < _pType->m_Size[_ConvertContext.m_TargetPtrSize])
		_Writer.WriteZero(_pType->m_Size[_ConvertContext.m_TargetPtrSize] - PosDiff);

	DL_ASSERT(_Writer.Tell() - Pos == _pType->m_Size[_ConvertContext.m_TargetPtrSize]);

	return DL_ERROR_OK;
}

static dl_error_t DLInternalConvertWriteInstance( dl_ctx_t       _Context,
												const SInstance& _Inst,
												pint*            _pNewOffset,
												SConvertContext& _ConvertContext,
												CDLBinaryWriter& _Writer )
{
	union { const uint8* m_u8; const uint16* m_u16; const uint32* m_u32; const uint64* m_u64; const char* m_str; } Data;
	Data.m_u8 = _Inst.m_pAddress;

	_Writer.SeekEnd(); // place instance at the end!

	if(_Inst.m_pType != 0x0)
		_Writer.Align(_Inst.m_pType->m_Alignment[_ConvertContext.m_TargetPtrSize]);

	*_pNewOffset = _Writer.Tell();

	dl_type_t AtomType    = dl_type_t(_Inst.m_Type & DL_TYPE_ATOM_MASK);
	dl_type_t StorageType = dl_type_t(_Inst.m_Type & DL_TYPE_STORAGE_MASK);

	if(AtomType == DL_TYPE_ATOM_ARRAY)
	{
		switch(StorageType)
		{
			case DL_TYPE_STORAGE_STRUCT:
			{
				pint TypeSize = _Inst.m_pType->m_Size[_ConvertContext.m_SourcePtrSize];
	 			for (pint ElemOffset = 0; ElemOffset < _Inst.m_ArrayCount * TypeSize; ElemOffset += TypeSize)
	 			{
	 				dl_error_t err = DLInternalConvertWriteStruct(_Context, Data.m_u8 + ElemOffset, _Inst.m_pType, _ConvertContext, _Writer);
	 				if(err != DL_ERROR_OK) return err;
	 			}
			} break;

			case DL_TYPE_STORAGE_STR:
			{
				pint TypeSize = DLPtrSize(_ConvertContext.m_SourcePtrSize);
	 			for(pint ElemOffset = 0; ElemOffset < _Inst.m_ArrayCount * TypeSize; ElemOffset += TypeSize)
	 			{
	 				pint OrigOffset = DLInternalReadPtrData(Data.m_u8 + ElemOffset, _ConvertContext.m_SourceEndian, _ConvertContext.m_SourcePtrSize);
	 				_ConvertContext.m_lPatchOffset.Add(SConvertContext::PatchPos(_Writer.Tell(), OrigOffset));
	 				_Writer.WritePtr(OrigOffset);
	 			}
			} break;

			case DL_TYPE_STORAGE_INT8:
			case DL_TYPE_STORAGE_UINT8:  _Writer.WriteArray(Data.m_u8, _Inst.m_ArrayCount ); break;
			case DL_TYPE_STORAGE_INT16:
			case DL_TYPE_STORAGE_UINT16: _Writer.WriteArray(Data.m_u16, _Inst.m_ArrayCount ); break;
			case DL_TYPE_STORAGE_INT32:
			case DL_TYPE_STORAGE_UINT32:
			case DL_TYPE_STORAGE_FP32:
			case DL_TYPE_STORAGE_ENUM:   _Writer.WriteArray(Data.m_u32, _Inst.m_ArrayCount ); break;
			case DL_TYPE_STORAGE_INT64:
			case DL_TYPE_STORAGE_UINT64:
			case DL_TYPE_STORAGE_FP64:   _Writer.WriteArray(Data.m_u64, _Inst.m_ArrayCount ); break;

			default:
				DL_ASSERT(false && "Unknown storage type!");
		}

		return DL_ERROR_OK;
	}

	if(AtomType == DL_TYPE_ATOM_POD && StorageType == DL_TYPE_STORAGE_STR)
	{
		_Writer.WriteArray(Data.m_u8, strlen(Data.m_str) + 1 );
		return DL_ERROR_OK;
	}

	DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
	DL_ASSERT(StorageType == DL_TYPE_STORAGE_STRUCT || StorageType == DL_TYPE_STORAGE_PTR);
	return DLInternalConvertWriteStruct(_Context, Data.m_u8, _Inst.m_pType, _ConvertContext, _Writer);
}

#include <algorithm>

bool dl_internal_sort_pred( const SInstance& i1, const SInstance& i2 ) { return i1.m_pAddress < i2.m_pAddress; }

dl_error_t DLInternalConvertNoHeader( dl_ctx_t       dl_ctx,
                                      unsigned char* packed_instance,
                                      unsigned char* packed_instance_base,
                                      unsigned char* out_instance,
                                      unsigned int   out_instance_size,
                                      unsigned int*  needed_size,
                                      dl_endian_t    src_endian,
                                      dl_endian_t    out_endian,
                                      dl_ptr_size_t  src_ptr_size,
                                      dl_ptr_size_t  out_ptr_size,
                                      const SDLType* root_type,
                                      unsigned int   base_offset )
{
	// need a parameter for IsSwapping
	CDLBinaryWriter Writer(out_instance, out_instance_size, out_instance == 0x0, src_endian, out_endian, out_ptr_size);
	SConvertContext ConvCtx( src_endian, out_endian, src_ptr_size, out_ptr_size );

	ConvCtx.m_lInstances.Add(SInstance(packed_instance, root_type, 0x0, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STRUCT)));
	dl_error_t err = DLInternalConvertCollectInstances(dl_ctx, root_type, packed_instance, packed_instance_base, ConvCtx);

	// TODO: we need to sort the instances here after their offset!

	SInstance* insts = ConvCtx.m_lInstances.GetBasePtr();
	std::sort( insts, insts + ConvCtx.m_lInstances.Len(), dl_internal_sort_pred );

	for(unsigned int i = 0; i < ConvCtx.m_lInstances.Len(); ++i)
	{
		err = DLInternalConvertWriteInstance( dl_ctx, ConvCtx.m_lInstances[i], &ConvCtx.m_lInstances[i].m_OffsetAfterPatch, ConvCtx, Writer);
		if(err != DL_ERROR_OK) 
			return err;
	}

	if(out_instance != 0x0) // no need to patch data if we are only calculating size
	{
		for(unsigned int i = 0; i < ConvCtx.m_lPatchOffset.Len(); ++i)
		{
			SConvertContext::PatchPos& PP = ConvCtx.m_lPatchOffset[i];

			// find new offset
			pint NewOffset = pint(-1);

			for(unsigned int j = 0; j < ConvCtx.m_lInstances.Len(); ++j )
			{
				pint OldOffset = ConvCtx.m_lInstances[j].m_pAddress - packed_instance_base;

				if(OldOffset == PP.m_OldOffset)
				{
					NewOffset = ConvCtx.m_lInstances[j].m_OffsetAfterPatch;
					break;
				}
			}

			DL_ASSERT(NewOffset != pint(-1) && "We should have found the instance!");

			Writer.SeekSet(PP.m_Pos);
			Writer.WritePtr(NewOffset + base_offset);
		}
	}

	Writer.SeekEnd();
	*needed_size = (unsigned int)Writer.Tell();

	return err;
}

static dl_error_t DLInternalConvertInstance( dl_ctx_t       dl_ctx,          dl_typeid_t  type,
                                             unsigned char* packed_instance, unsigned int packed_instance_size,
                                             unsigned char* out_instance,    unsigned int out_instance_size,
                                             dl_endian_t    out_endian,      unsigned int out_ptr_size,
                                             unsigned int*  out_size )
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) )     return DL_ERROR_MALFORMED_DATA;
	if( header->m_Id != DL_TYPE_DATA_ID &&
		header->m_Id != DL_TYPE_DATA_ID_SWAPED )           return DL_ERROR_MALFORMED_DATA;
	if( header->m_RootInstanceType != type &&
		header->m_RootInstanceType != DLSwapEndian(type) ) return DL_ERROR_TYPE_MISMATCH;
	if( out_ptr_size != 4 && out_ptr_size != 8 )           return DL_ERROR_INVALID_PARAMETER;

	dl_ptr_size_t src_ptr_size = header->m_64BitPtr != 0 ? DL_PTR_SIZE_64BIT : DL_PTR_SIZE_32BIT;
	dl_ptr_size_t dst_ptr_size;

	switch(out_ptr_size)
	{
		case 4: dst_ptr_size = DL_PTR_SIZE_32BIT; break;
		case 8: dst_ptr_size = DL_PTR_SIZE_64BIT; break;
		default: return DL_ERROR_INVALID_PARAMETER;
	}

	// converting to larger pointersize in an inplace conversion is not possible!
	if( dst_ptr_size > src_ptr_size && packed_instance == out_instance )
		return DL_ERROR_UNSUPPORTED_OPERATION;

	dl_endian_t src_endian = header->m_Id == DL_TYPE_DATA_ID ? DL_ENDIAN_HOST : DLOtherEndian(DL_ENDIAN_HOST);

	if(src_endian == out_endian && src_ptr_size == dst_ptr_size)
	{
		if(out_instance != 0x0)
			memmove(out_instance, packed_instance, packed_instance_size); // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!

		*out_size = packed_instance_size; // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!
		return DL_ERROR_OK;
	}

	dl_typeid_t root_type_id = src_endian != DL_ENDIAN_HOST ? DLSwapEndian(header->m_RootInstanceType) : header->m_RootInstanceType;

	const SDLType* root_type = DLFindType(dl_ctx, root_type_id);
	if(root_type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	dl_error_t err = DLInternalConvertNoHeader( dl_ctx,
											    packed_instance + sizeof(SDLDataHeader),
											    packed_instance + sizeof(SDLDataHeader),
											    out_instance == 0x0 ? 0x0 : out_instance + sizeof(SDLDataHeader),
											    out_instance_size - sizeof(SDLDataHeader),
											    out_size,
											    src_endian,
											    out_endian,
											    src_ptr_size,
											    dst_ptr_size,
											    root_type,
											    0 );

	if(out_instance != 0x0)
	{
		SDLDataHeader* pNewHeader = (SDLDataHeader*)out_instance;
		pNewHeader->m_Id               = DL_TYPE_DATA_ID;
		pNewHeader->m_Version          = DL_VERSION;
		pNewHeader->m_RootInstanceType = type;
		pNewHeader->m_InstanceSize     = uint32(*out_size);
		pNewHeader->m_64BitPtr         = out_ptr_size == 4 ? 0 : 1;

		if(DL_ENDIAN_HOST != out_endian)
			DLSwapHeader(pNewHeader);
	}

	*out_size += sizeof(SDLDataHeader);
	return err;
}

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

dl_error_t dl_convert_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t  type,
		                       unsigned char* packed_instance, unsigned int packed_instance_size,
		                       unsigned int*  out_size,        dl_endian_t  out_endian,
                               unsigned int   out_ptr_size )
{
	unsigned int dummy;
	if( out_size == 0x0 )
		out_size = &dummy;
	return DLInternalConvertInstance( dl_ctx, type, packed_instance, packed_instance_size, packed_instance, packed_instance_size, out_endian, out_ptr_size, out_size );
}

dl_error_t dl_convert( dl_ctx_t dl_ctx,                dl_typeid_t  type,
                       unsigned char* packed_instance, unsigned int packed_instance_size,
                       unsigned char* out_instance,    unsigned int out_instance_size,
                       dl_endian_t    out_endian,      unsigned int out_ptr_size )
{
	DL_ASSERT(out_instance != packed_instance && "Src and destination can not be the same!");
	unsigned int dummy_needed_size;
	return DLInternalConvertInstance( dl_ctx, type, packed_instance, packed_instance_size, out_instance, out_instance_size, out_endian, out_ptr_size, &dummy_needed_size);
}

dl_error_t dl_convert_calc_size( dl_ctx_t dl_ctx,                dl_typeid_t   type,
                                 unsigned char* packed_instance, unsigned int  packed_instance_size,
                                 unsigned int   out_ptr_size,    unsigned int* out_size )
{
	return DLInternalConvertInstance( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, DL_ENDIAN_HOST, out_ptr_size, out_size );
}

#ifdef __cplusplus
}
#endif  // __cplusplus
