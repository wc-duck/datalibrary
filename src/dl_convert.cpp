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

static inline void DLSwapHeader( SDLDataHeader* header )
{
	header->id                 = dl_swap_endian_uint32( header->id );
	header->version            = dl_swap_endian_uint32( header->version );
	header->root_instance_type = dl_swap_endian_uint32( header->root_instance_type );
	header->instance_size      = dl_swap_endian_uint32( header->instance_size );
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
				return (pint)dl_swap_endian_uint32( Offset );
			else
				return (pint)Offset;
		}
		break;
		case DL_PTR_SIZE_64BIT:
		{
			uint64 Offset = *(uint64*)_pPtrData;

			if(_SourceEndian != DL_ENDIAN_HOST)
				return (pint)dl_swap_endian_uint64( Offset );
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

static void dl_internal_read_array_data( const uint8*  array_data,
										 pint*         offset,
										 uint32*       count,
										 dl_endian_t   source_endian,
										 dl_ptr_size_t ptr_size )
{
	union { const uint8* u8; const uint32* u32; const uint64* u64; };
	u8 = array_data;

	switch( ptr_size )
	{
		case DL_PTR_SIZE_32BIT:
		{
			if( source_endian != DL_ENDIAN_HOST )
			{
				*offset = (pint)dl_swap_endian_uint32( u32[0] );
				*count  =       dl_swap_endian_uint32( u32[1] );
			}
			else
			{
				*offset = (pint)(u32[0]);
				*count  = u32[1];
			}
		}
		break;

		case DL_PTR_SIZE_64BIT:
		{
			if( source_endian != DL_ENDIAN_HOST )
			{
				*offset = (pint)dl_swap_endian_uint64( u64[0] );
				*count  =       dl_swap_endian_uint32( u32[2] );
			}
			else
			{
				*offset = (pint)(u64[0]);
				*count  = u32[2];
			}
		}
		break;

		default:
			DL_ASSERT(false && "Invalid ptr-size");
	}
}

static dl_error_t dl_internal_convert_collect_instances( dl_ctx_t         dl_ctx,
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
					default:
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
				dl_internal_read_array_data( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );

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
						dl_internal_read_array_data( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );

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
						dl_internal_read_array_data( pMemberData, &Offset, &Count, convert_ctx.m_SourceEndian, convert_ctx.m_SourcePtrSize );
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
static T dl_convert_bf_format( T old_val, const SDLMember* bf_members, uint32 bf_members_count, SConvertContext* conv_ctx )
{
	T new_val = 0;

	for( uint32 i = 0; i < bf_members_count; ++i )
	{
		const SDLMember& bf_member = bf_members[i];

		uint32 bf_bits         = bf_member.BitFieldBits();
		uint32 bf_offset       = bf_member.BitFieldOffset();
		uint32 bf_source_offset = dl_bf_offset( conv_ctx->m_SourceEndian, sizeof(T), bf_offset, bf_bits );
		uint32 bf_target_offset = dl_bf_offset( conv_ctx->m_TargetEndian, sizeof(T), bf_offset, bf_bits );

 		T extracted = (T)DL_EXTRACT_BITS( old_val, T(bf_source_offset), T(bf_bits) );
 		new_val     = (T)DL_INSERT_BITS ( new_val, extracted, T(bf_target_offset), T(bf_bits) );
	}

	return new_val;
}

static uint8 dl_convert_bit_field_format_uint8( uint8 old_value, const SDLMember* first_bf_member, uint32 num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->m_SourceEndian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint8( dl_convert_bf_format( dl_swap_endian_uint8( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint16 dl_convert_bit_field_format_uint16( uint16 old_value, const SDLMember* first_bf_member, uint32 num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->m_SourceEndian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint16( dl_convert_bf_format( dl_swap_endian_uint16( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint32 dl_convert_bit_field_format_uint32( uint32 old_value, const SDLMember* first_bf_member, uint32 num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->m_SourceEndian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint32( dl_convert_bf_format( dl_swap_endian_uint32( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint64 dl_convert_bit_field_format_uint64( uint64 old_value, const SDLMember* first_bf_member, uint32 num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->m_SourceEndian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint64( dl_convert_bf_format( dl_swap_endian_uint64( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
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
				dl_internal_read_array_data( pMemberData, &Offset, &Count, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize );

				if(Offset != DL_NULL_PTR_OFFSET[conv_ctx.m_SourcePtrSize])
					conv_ctx.m_lPatchOffset.Add(SConvertContext::PatchPos( dl_binary_writer_tell( writer ), Offset) );
				else
					Offset = DL_NULL_PTR_OFFSET[conv_ctx.m_TargetPtrSize];

				dl_binary_writer_write_ptr( writer, Offset );
				dl_binary_writer_write_4byte( writer, pMemberData + dl_internal_ptr_size( conv_ctx.m_SourcePtrSize ) );
				if( conv_ctx.m_TargetPtrSize == DL_PTR_SIZE_64BIT )
					dl_binary_writer_write_zero( writer, 4 );
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
							uint8 val = dl_convert_bit_field_format_uint8( *(uint8*)pMemberData, &Member, nBFMembers, &conv_ctx );
							dl_binary_writer_write_1byte( writer, &val );
						} break;
						case 2:
						{
							uint16 val = dl_convert_bit_field_format_uint16( *(uint16*)pMemberData, &Member, nBFMembers, &conv_ctx );
							dl_binary_writer_write_2byte( writer, &val );
						} break;
						case 4:
						{
							uint32 val = dl_convert_bit_field_format_uint32( *(uint32*)pMemberData, &Member, nBFMembers, &conv_ctx );
							dl_binary_writer_write_4byte( writer, &val );
						} break;
						case 8:
						{
							uint64 val = dl_convert_bit_field_format_uint64( *(uint64*)pMemberData, &Member, nBFMembers, &conv_ctx );
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
	union { const uint8* u8; const uint16* u16; const uint32* u32; const uint64* u64; const char* str; };
	u8 = inst.m_pAddress;

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
	 				dl_error_t err = dl_internal_convert_write_struct( dl_ctx, u8 + ElemOffset, inst.m_pType, conv_ctx, writer );
	 				if(err != DL_ERROR_OK) return err;
	 			}
			} break;

			case DL_TYPE_STORAGE_STR:
			{
				pint TypeSize = dl_internal_ptr_size(conv_ctx.m_SourcePtrSize);
	 			for(pint ElemOffset = 0; ElemOffset < inst.m_ArrayCount * TypeSize; ElemOffset += TypeSize)
	 			{
	 				pint OrigOffset = DLInternalReadPtrData(u8 + ElemOffset, conv_ctx.m_SourceEndian, conv_ctx.m_SourcePtrSize);
	 				conv_ctx.m_lPatchOffset.Add( SConvertContext::PatchPos( dl_binary_writer_tell( writer ), OrigOffset ) );
	 				dl_binary_writer_write_ptr( writer, OrigOffset );
	 			}
			} break;

			case DL_TYPE_STORAGE_INT8:
			case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_array( writer, u8, inst.m_ArrayCount, sizeof(uint8) ); break;
			case DL_TYPE_STORAGE_INT16:
			case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_array( writer, u16, inst.m_ArrayCount, sizeof(uint16) ); break;
			case DL_TYPE_STORAGE_INT32:
			case DL_TYPE_STORAGE_UINT32:
			case DL_TYPE_STORAGE_FP32:
			case DL_TYPE_STORAGE_ENUM:   dl_binary_writer_write_array( writer, u32, inst.m_ArrayCount, sizeof(uint32) ); break;
			case DL_TYPE_STORAGE_INT64:
			case DL_TYPE_STORAGE_UINT64:
			case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_array( writer, u64, inst.m_ArrayCount, sizeof(uint64) ); break;

			default:
				DL_ASSERT(false && "Unknown storage type!");
		}

		return DL_ERROR_OK;
	}

	if(AtomType == DL_TYPE_ATOM_POD && StorageType == DL_TYPE_STORAGE_STR)
	{
		dl_binary_writer_write_array( writer, u8,  strlen(str) + 1, sizeof(uint8) );
		return DL_ERROR_OK;
	}

	DL_ASSERT(AtomType == DL_TYPE_ATOM_POD);
	DL_ASSERT(StorageType == DL_TYPE_STORAGE_STRUCT || StorageType == DL_TYPE_STORAGE_PTR);
	return dl_internal_convert_write_struct( dl_ctx, u8, inst.m_pType, conv_ctx, writer );
}

#include <algorithm>

bool dl_internal_sort_pred( const SInstance& i1, const SInstance& i2 ) { return i1.m_pAddress < i2.m_pAddress; }

dl_error_t dl_internal_convert_no_header( dl_ctx_t       dl_ctx,
                                          unsigned char* packed_instance, unsigned char* packed_instance_base,
                                          unsigned char* out_instance,    size_t         out_instance_size,
                                          size_t*        needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const SDLType* root_type,       size_t         base_offset )
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

static dl_error_t dl_internal_convert_instance( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                                unsigned char* packed_instance, size_t      packed_instance_size,
                                                unsigned char* out_instance,    size_t      out_instance_size,
                                                dl_endian_t    out_endian,      size_t      out_ptr_size,
                                                size_t*        out_size )
{
	SDLDataHeader* header = (SDLDataHeader*)packed_instance;

	if( packed_instance_size < sizeof(SDLDataHeader) )              return DL_ERROR_MALFORMED_DATA;
	if( header->id != DL_INSTANCE_ID &&
		header->id != DL_INSTANCE_ID_SWAPED )                       return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION &&
		header->version != DL_INSTANCE_VERSION_SWAPED )             return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type &&
		header->root_instance_type != dl_swap_endian_uint32(type) ) return DL_ERROR_TYPE_MISMATCH;
	if( out_ptr_size != 4 && out_ptr_size != 8 )                    return DL_ERROR_INVALID_PARAMETER;

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

	dl_endian_t src_endian = header->id == DL_INSTANCE_ID ? DL_ENDIAN_HOST : dl_other_endian( DL_ENDIAN_HOST );

	if(src_endian == out_endian && src_ptr_size == dst_ptr_size)
	{
		if(out_instance != 0x0)
			memmove(out_instance, packed_instance, packed_instance_size); // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!

		*out_size = packed_instance_size; // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!
		return DL_ERROR_OK;
	}

	dl_typeid_t root_type_id = src_endian != DL_ENDIAN_HOST ? dl_swap_endian_uint32( header->root_instance_type ) : header->root_instance_type;

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
												    0u );

	if(out_instance != 0x0)
	{
		SDLDataHeader* pNewHeader = (SDLDataHeader*)out_instance;
		pNewHeader->id                 = DL_INSTANCE_ID;
		pNewHeader->version            = DL_INSTANCE_VERSION;
		pNewHeader->root_instance_type = type;
		pNewHeader->instance_size      = uint32(*out_size);
		pNewHeader->is_64_bit_ptr      = out_ptr_size == 4 ? 0 : 1;

		if(DL_ENDIAN_HOST != out_endian)
			DLSwapHeader(pNewHeader);
	}

	*out_size += sizeof(SDLDataHeader);
	return err;
}

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

dl_error_t dl_convert_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type,
		                       unsigned char* packed_instance, size_t      packed_instance_size,
		                       dl_endian_t    out_endian,      size_t      out_ptr_size,
		                       size_t*        produced_bytes )
{
	size_t dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return dl_internal_convert_instance( dl_ctx, type, packed_instance, packed_instance_size, packed_instance, packed_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                       unsigned char* packed_instance, size_t      packed_instance_size,
                       unsigned char* out_instance,    size_t      out_instance_size,
                       dl_endian_t    out_endian,      size_t      out_ptr_size,
                       size_t*        produced_bytes )
{
	DL_ASSERT(out_instance != packed_instance && "Src and destination can not be the same!");
	size_t dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return dl_internal_convert_instance( dl_ctx, type, packed_instance, packed_instance_size, out_instance, out_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert_calc_size( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                 unsigned char* packed_instance, size_t      packed_instance_size,
                                 size_t         out_ptr_size,    size_t*     out_size )
{
	return dl_convert( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, DL_ENDIAN_HOST, out_ptr_size, out_size );
}

#ifdef __cplusplus
}
#endif  // __cplusplus
