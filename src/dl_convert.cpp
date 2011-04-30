/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl.h>
#include <dl/dl_convert.h>
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
	_pHeader->id               = DLSwapEndian(_pHeader->id);
	_pHeader->version          = DLSwapEndian(_pHeader->version);
	_pHeader->root_instance_type = DLSwapEndian(_pHeader->root_instance_type);
	_pHeader->instance_size     = DLSwapEndian(_pHeader->instance_size);
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

static pint dl_internal_ptr_size(dl_ptr_size_t size_enum)
{
	switch(size_enum)
	{
		case DL_PTR_SIZE_32BIT: return 4;
		case DL_PTR_SIZE_64BIT: return 8;
		default: DL_ASSERT("unknown ptr size!"); return 0;
	}
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

static dl_error_t dl_internal_convert_collect_instances( dl_ctx_t       dl_ctx,
														 const SDLType*   type,
														 const uint8*     data,
														 const uint8*     base_data,
														 SConvertContext& convert_ctx )
{
	for(uint32 iMember = 0; iMember < type->member_count; ++iMember)
	{
		const SDLMember& Member = type->members[iMember];
		const uint8* pMemberData = data + Member.offset[convert_ctx.m_SourcePtrSize];

		dl_type_t AtomType    = Member.AtomType();
		dl_type_t StorageType = Member.StorageType();

		switch(AtomType)
		{
			case DL_TYPE_ATOM_POD:
			{
				switch( StorageType )
				{
					case DL_TYPE_STORAGE_STR:
					{
						pint Offset = DLInternalReadPtrData( pMemberData, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );
						convert_ctx.m_lInstances.Add(SInstance(base_data + Offset, 0x0, 1337, Member.type));
					}
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						pint Offset = DLInternalReadPtrData(pMemberData, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize);

						const uint8* pPtrData = base_data + Offset;

						if(Offset != DL_NULL_PTR_OFFSET[convert_ctx.m_SourcePtrSize] && !convert_ctx.IsSwapped(pPtrData))
						{
							convert_ctx.m_lInstances.Add(SInstance(pPtrData, pSubType, 0, Member.type));
							dl_internal_convert_collect_instances(dl_ctx, pSubType, base_data + Offset, base_data, convert_ctx);
						}
					}
					break;
					case DL_TYPE_STORAGE_STRUCT:
					{
						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;
						dl_internal_convert_collect_instances(dl_ctx, pSubType, pMemberData, base_data, convert_ctx);
					}
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
						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						for (pint ElemOffset = 0; ElemOffset < Member.size[convert_ctx.m_SourcePtrSize]; ElemOffset += pSubType->size[convert_ctx.m_SourcePtrSize])
							dl_internal_convert_collect_instances(dl_ctx, pSubType, pMemberData + ElemOffset, base_data, convert_ctx);
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						// TODO: This might be optimized if we look at all the data in i inline-array of strings as 1 instance continious in memory.
						// I am not sure if that is true for all cases right now!

						uint32 PtrSize = (uint32)dl_internal_ptr_size(convert_ctx.m_SourcePtrSize);
						uint32 Count = Member.size[convert_ctx.m_SourcePtrSize] / PtrSize;

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint Offset = DLInternalReadPtrData(data + (iElem * PtrSize), convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize);
							convert_ctx.m_lInstances.Add(SInstance(base_data + Offset, 0x0, Count, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STR)));
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
				DLInternalReadArrayData( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );

				if(Offset == DL_NULL_PTR_OFFSET[convert_ctx.m_SourcePtrSize])
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

						uint32 PtrSize = (uint32)dl_internal_ptr_size(convert_ctx.m_SourcePtrSize);
						const uint8* pArrayData = base_data + Offset;

						convert_ctx.m_lInstances.Add(SInstance(base_data + Offset, 0x0, Count, Member.type));

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint ElemOffset = DLInternalReadPtrData(pArrayData + (iElem * PtrSize), convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize);
							convert_ctx.m_lInstances.Add(SInstance(base_data + ElemOffset, 0x0, Count, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STR)));
						}
					}
					break;

					case DL_TYPE_STORAGE_STRUCT:
					{
						DLInternalReadArrayData( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );

						const uint8* pArrayData = base_data + Offset;

						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						convert_ctx.m_lInstances.Add(SInstance(pArrayData, pSubType, Count, Member.type));

						for (pint ElemOffset = 0; ElemOffset < pSubType->size[convert_ctx.m_SourcePtrSize] * Count; ElemOffset += pSubType->size[convert_ctx.m_SourcePtrSize])
							dl_internal_convert_collect_instances(dl_ctx, pSubType, pArrayData + ElemOffset, base_data, convert_ctx);
					}
					break;

					default:
					{
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
						DLInternalReadArrayData( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );
						convert_ctx.m_lInstances.Add(SInstance(base_data + Offset, 0x0, Count, Member.type));
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

static dl_error_t dl_internal_convert_write_struct( dl_ctx_t          dl_ctx,
													const uint8*      data,
													const SDLType*    type,
													SConvertContext&  conv_ctx,
													dl_binary_writer* writer )
{
	dl_binary_writer_align( writer, type->alignment[conv_ctx.m_TargetPtrSize] );
	pint Pos = dl_binary_writer_tell( writer );
	dl_binary_writer_reserve( writer, type->size[conv_ctx.m_TargetPtrSize] );

	for(uint32 iMember = 0; iMember < type->member_count; ++iMember)
	{
		const SDLMember& Member  = type->members[iMember];
		const uint8* pMemberData = data + Member.offset[conv_ctx.m_SourcePtrSize];

		dl_binary_writer_align( writer, Member.alignment[conv_ctx.m_TargetPtrSize] );

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
						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;
						dl_internal_convert_write_struct( dl_ctx, pMemberData, pSubType, conv_ctx, writer );
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						pint Offset = DLInternalReadPtrData(pMemberData, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize);
						conv_ctx.m_lPatchOffset.Add( SConvertContext::PatchPos( dl_binary_writer_tell( writer ), Offset ) );
						dl_binary_writer_write_ptr( writer, 0x0 );
					}
					break;
					case DL_TYPE_STORAGE_PTR:
					{
						pint Offset = DLInternalReadPtrData(pMemberData, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize);

						if (Offset != DL_NULL_PTR_OFFSET[conv_ctx.m_SourcePtrSize])
							conv_ctx.m_lPatchOffset.Add(SConvertContext::PatchPos( dl_binary_writer_tell( writer ), Offset ) );

						dl_binary_writer_write_ptr( writer, pint(-1) );
					}
					break;
					default:
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);
						dl_binary_writer_write_swap( writer, pMemberData, Member.size[conv_ctx.m_SourcePtrSize] );
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
						const SDLType* pSubType = dl_internal_find_type(dl_ctx, Member.type_id);
						if(pSubType == 0x0)
							return DL_ERROR_TYPE_NOT_FOUND;

						pint MemberSize  = Member.size[conv_ctx.m_SourcePtrSize];
						pint SubtypeSize = pSubType->size[conv_ctx.m_SourcePtrSize];
						for (pint ElemOffset = 0; ElemOffset < MemberSize; ElemOffset += SubtypeSize)
							dl_internal_convert_write_struct( dl_ctx, pMemberData + ElemOffset, pSubType, conv_ctx, writer );
					}
					break;
					case DL_TYPE_STORAGE_STR:
					{
						pint PtrSizeSource = dl_internal_ptr_size(conv_ctx.m_SourcePtrSize);
						pint PtrSizeTarget = dl_internal_ptr_size(conv_ctx.m_TargetPtrSize);
						uint32 Count = Member.size[conv_ctx.m_SourcePtrSize] / (uint32)PtrSizeSource;
						pint Pos = dl_binary_writer_tell( writer );

						for (pint iElem = 0; iElem < Count; ++iElem)
						{
							pint OldOffset = DLInternalReadPtrData(pMemberData + (iElem * PtrSizeSource), conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize);
							conv_ctx.m_lPatchOffset.Add(SConvertContext::PatchPos(Pos + (iElem * PtrSizeTarget), OldOffset));
						}

						dl_binary_writer_write_zero( writer, Member.size[conv_ctx.m_TargetPtrSize] );
					}
					break;
					default:
					{
						DL_ASSERT(Member.IsSimplePod() || StorageType == DL_TYPE_STORAGE_ENUM);

						pint   PodSize   = DLPodSize(Member.type);
						uint32 ArraySize = Member.size[conv_ctx.m_SourcePtrSize];

						switch(PodSize)
						{
							case 1: dl_binary_writer_write_array( writer, pMemberData, ArraySize / sizeof( uint8), sizeof( uint8) ); break;
							case 2: dl_binary_writer_write_array( writer, pMemberData, ArraySize / sizeof(uint16), sizeof(uint16) ); break;
							case 4: dl_binary_writer_write_array( writer, pMemberData, ArraySize / sizeof(uint32), sizeof(uint32) ); break;
							case 8: dl_binary_writer_write_array( writer, pMemberData, ArraySize / sizeof(uint64), sizeof(uint64) ); break;
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
				DLInternalReadArrayData( pMemberData, &Offset, &Count, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize );

				if(Offset != DL_NULL_PTR_OFFSET[conv_ctx.m_SourcePtrSize])
					conv_ctx.m_lPatchOffset.Add(SConvertContext::PatchPos( dl_binary_writer_tell( writer ), Offset) );
				else
					Offset = DL_NULL_PTR_OFFSET[conv_ctx.m_TargetPtrSize];

				dl_binary_writer_write_ptr( writer, Offset );
				dl_binary_writer_write_4byte( writer, pMemberData + dl_internal_ptr_size( conv_ctx.m_SourcePtrSize ) );
			}
			break;

			case DL_TYPE_ATOM_BITFIELD:
			{
				uint32 j = iMember;

				do { j++; } while(j < type->member_count && type->members[j].AtomType() == DL_TYPE_ATOM_BITFIELD);

				if(conv_ctx.m_SourceEndian != conv_ctx.m_TargetEndian)
				{
					uint32 nBFMembers = j - iMember;

					switch(Member.size[conv_ctx.m_SourcePtrSize])
					{
						case 1:
						{
							uint8 val = DLConvertBitFieldFormat( *(uint8*)pMemberData, &Member, nBFMembers, conv_ctx );
							dl_binary_writer_write_1byte( writer, &val );
						} break;
						case 2:
						{
							uint16 val = DLConvertBitFieldFormat( *(uint16*)pMemberData, &Member, nBFMembers, conv_ctx );
							dl_binary_writer_write_2byte( writer, &val );
						} break;
						case 4:
						{
							uint32 val = DLConvertBitFieldFormat( *(uint32*)pMemberData, &Member, nBFMembers, conv_ctx );
							dl_binary_writer_write_4byte( writer, &val );
						} break;
						case 8:
						{
							uint64 val = DLConvertBitFieldFormat( *(uint64*)pMemberData, &Member, nBFMembers, conv_ctx );
							dl_binary_writer_write_8byte( writer, &val );
						} break;
						default:
							DL_ASSERT(false && "Not supported pod-size or bitfield-size!");
					}
				}
				else
					dl_binary_writer_write( writer, pMemberData, Member.size[conv_ctx.m_SourcePtrSize] );

				iMember = j - 1;
			}
			break;

			default:
				DL_ASSERT(false && "Invalid ATOM-type!");
		}
	}

	// we need to write our entire size with zeroes. Our entire size might be less than the sum of teh members.
	pint PosDiff = dl_binary_writer_tell( writer ) - Pos;

	if(PosDiff < type->size[conv_ctx.m_TargetPtrSize])
		dl_binary_writer_write_zero( writer, type->size[conv_ctx.m_TargetPtrSize] - PosDiff );

	DL_ASSERT( dl_binary_writer_tell( writer ) - Pos == type->size[conv_ctx.m_TargetPtrSize] );

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_convert_write_instance( dl_ctx_t          dl_ctx,
													  const SInstance&  inst,
													  pint*             new_offset,
													  SConvertContext&  conv_ctx,
													  dl_binary_writer* writer )
{
	union { const uint8* m_u8; const uint16* m_u16; const uint32* m_u32; const uint64* m_u64; const char* m_str; } Data;
	Data.m_u8 = inst.m_pAddress;

	dl_binary_writer_seek_end( writer ); // place instance at the end!

	if(inst.m_pType != 0x0)
		dl_binary_writer_align( writer, inst.m_pType->alignment[conv_ctx.m_TargetPtrSize] );

	*new_offset = dl_binary_writer_tell( writer );

	dl_type_t AtomType    = dl_type_t(inst.m_Type & DL_TYPE_ATOM_MASK);
	dl_type_t StorageType = dl_type_t(inst.m_Type & DL_TYPE_STORAGE_MASK);

	if(AtomType == DL_TYPE_ATOM_ARRAY)
	{
		switch(StorageType)
		{
			case DL_TYPE_STORAGE_STRUCT:
			{
				pint TypeSize = inst.m_pType->size[conv_ctx.m_SourcePtrSize];
	 			for (pint ElemOffset = 0; ElemOffset < inst.m_ArrayCount * TypeSize; ElemOffset += TypeSize)
	 			{
	 				dl_error_t err = dl_internal_convert_write_struct( dl_ctx, Data.m_u8 + ElemOffset, inst.m_pType, conv_ctx, writer );
	 				if(err != DL_ERROR_OK) return err;
	 			}
			} break;

			case DL_TYPE_STORAGE_STR:
			{
				pint TypeSize = dl_internal_ptr_size(conv_ctx.m_SourcePtrSize);
	 			for(pint ElemOffset = 0; ElemOffset < inst.m_ArrayCount * TypeSize; ElemOffset += TypeSize)
	 			{
	 				pint OrigOffset = DLInternalReadPtrData(Data.m_u8 + ElemOffset, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize);
	 				conv_ctx.m_lPatchOffset.Add( SConvertContext::PatchPos( dl_binary_writer_tell( writer ), OrigOffset ) );
	 				dl_binary_writer_write_ptr( writer, OrigOffset );
	 			}
			} break;

			case DL_TYPE_STORAGE_INT8:
			case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_array( writer, Data.m_u8, inst.m_ArrayCount, sizeof(uint8) ); break;
			case DL_TYPE_STORAGE_INT16:
			case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_array( writer, Data.m_u16, inst.m_ArrayCount, sizeof(uint16) ); break;
			case DL_TYPE_STORAGE_INT32:
			case DL_TYPE_STORAGE_UINT32:
			case DL_TYPE_STORAGE_FP32:
			case DL_TYPE_STORAGE_ENUM:   dl_binary_writer_write_array( writer, Data.m_u32, inst.m_ArrayCount, sizeof(uint32) ); break;
			case DL_TYPE_STORAGE_INT64:
			case DL_TYPE_STORAGE_UINT64:
			case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_array( writer, Data.m_u64, inst.m_ArrayCount, sizeof(uint64) ); break;

			default:
				DL_ASSERT(false && "Unknown storage type!");
		}

		return DL_ERROR_OK;
	}

	if(AtomType == DL_TYPE_ATOM_POD && StorageType == DL_TYPE_STORAGE_STR)
	{
		dl_binary_writer_write_array( writer, Data.m_u8,  strlen(Data.m_str) + 1, sizeof(uint8) );
		return DL_ERROR_OK;
	}

	DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
	DL_ASSERT(StorageType == DL_TYPE_STORAGE_STRUCT || StorageType == DL_TYPE_STORAGE_PTR);
	return dl_internal_convert_write_struct( dl_ctx, Data.m_u8, inst.m_pType, conv_ctx, writer );
}

#include <algorithm>

bool dl_internal_sort_pred( const SInstance& i1, const SInstance& i2 ) { return i1.m_pAddress < i2.m_pAddress; }

dl_error_t dl_internal_convert_no_header( dl_ctx_t       dl_ctx,
                                          unsigned char* packed_instance, unsigned char* packed_instance_base,
                                          unsigned char* out_instance,    unsigned int   out_instance_size,
                                          unsigned int*  needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const SDLType* root_type,       unsigned int   base_offset )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_instance, out_instance_size, out_instance == 0x0, src_endian, out_endian, out_ptr_size );

	SConvertContext ConvCtx( src_endian, out_endian, src_ptr_size, out_ptr_size );

	ConvCtx.m_lInstances.Add(SInstance(packed_instance, root_type, 0x0, dl_type_t(DL_TYPE_ATOM_POD | DL_TYPE_STORAGE_STRUCT)));
	dl_error_t err = dl_internal_convert_collect_instances(dl_ctx, root_type, packed_instance, packed_instance_base, ConvCtx);

	// TODO: we need to sort the instances here after their offset!

	SInstance* insts = ConvCtx.m_lInstances.GetBasePtr();
	std::sort( insts, insts + ConvCtx.m_lInstances.Len(), dl_internal_sort_pred );

	for(unsigned int i = 0; i < ConvCtx.m_lInstances.Len(); ++i)
	{
		err = dl_internal_convert_write_instance( dl_ctx, ConvCtx.m_lInstances[i], &ConvCtx.m_lInstances[i].m_OffsetAfterPatch, ConvCtx, &writer );
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

			dl_binary_writer_seek_set( &writer, PP.m_Pos );
			dl_binary_writer_write_ptr( &writer, NewOffset + base_offset );
		}
	}

	dl_binary_writer_seek_end( &writer );
	*needed_size = (unsigned int)dl_binary_writer_tell( &writer );

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
	if( header->id != DL_INSTANCE_ID &&
		header->id != DL_INSTANCE_ID_SWAPED )            return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION &&
		header->version != DL_INSTANCE_VERSION_SWAPED )  return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type &&
		header->root_instance_type != DLSwapEndian(type) ) return DL_ERROR_TYPE_MISMATCH;
	if( out_ptr_size != 4 && out_ptr_size != 8 )           return DL_ERROR_INVALID_PARAMETER;

	dl_ptr_size_t src_ptr_size = header->is_64_bit_ptr != 0 ? DL_PTR_SIZE_64BIT : DL_PTR_SIZE_32BIT;
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

	dl_endian_t src_endian = header->id == DL_INSTANCE_ID ? DL_ENDIAN_HOST : DLOtherEndian(DL_ENDIAN_HOST);

	if(src_endian == out_endian && src_ptr_size == dst_ptr_size)
	{
		if(out_instance != 0x0)
			memmove(out_instance, packed_instance, packed_instance_size); // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!

		*out_size = packed_instance_size; // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!
		return DL_ERROR_OK;
	}

	dl_typeid_t root_type_id = src_endian != DL_ENDIAN_HOST ? DLSwapEndian(header->root_instance_type) : header->root_instance_type;

	const SDLType* root_type = dl_internal_find_type(dl_ctx, root_type_id);
	if(root_type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	dl_error_t err = dl_internal_convert_no_header( dl_ctx,
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
		pNewHeader->id               = DL_INSTANCE_ID;
		pNewHeader->version          = DL_INSTANCE_VERSION;
		pNewHeader->root_instance_type = type;
		pNewHeader->instance_size     = uint32(*out_size);
		pNewHeader->is_64_bit_ptr         = out_ptr_size == 4 ? 0 : 1;

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
		                       dl_endian_t    out_endian,      unsigned int out_ptr_size,
		                       unsigned int*  produced_bytes )
{
	unsigned int dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return DLInternalConvertInstance( dl_ctx, type, packed_instance, packed_instance_size, packed_instance, packed_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert( dl_ctx_t dl_ctx,                dl_typeid_t  type,
                       unsigned char* packed_instance, unsigned int packed_instance_size,
                       unsigned char* out_instance,    unsigned int out_instance_size,
                       dl_endian_t    out_endian,      unsigned int out_ptr_size,
                       unsigned int*  produced_bytes )
{
	DL_ASSERT(out_instance != packed_instance && "Src and destination can not be the same!");
	unsigned int dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return DLInternalConvertInstance( dl_ctx, type, packed_instance, packed_instance_size, out_instance, out_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert_calc_size( dl_ctx_t dl_ctx,                dl_typeid_t   type,
                                 unsigned char* packed_instance, unsigned int  packed_instance_size,
                                 unsigned int   out_ptr_size,    unsigned int* out_size )
{
	return dl_convert( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, DL_ENDIAN_HOST, out_ptr_size, out_size );
}

#ifdef __cplusplus
}
#endif  // __cplusplus
