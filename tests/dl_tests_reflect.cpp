/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl_reflect.h>

#include "dl_test_common.h"
#include "generated/sized_enums.h"

template<class T>
struct TAlignmentOf
{
        struct CAlign { ~CAlign() {}; unsigned char m_Dummy; T m_T; };
        enum { ALIGNOF = sizeof(CAlign) - sizeof(T) };
};
#define DL_ALIGNMENTOF(Type) (unsigned int)TAlignmentOf<Type>::ALIGNOF

class DLReflect : public DL {};

TEST_F(DLReflect, pods)
{
	dl_type_info_t   Info;
	dl_member_info_t Members[128];
	memset( &Info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK(dl_reflect_get_type_info( Ctx, Pods::TYPE_ID, &Info ));
	EXPECT_DL_ERR_OK(dl_reflect_get_type_members( Ctx, Pods::TYPE_ID, Members, DL_ARRAY_LENGTH(Members) ));

	EXPECT_EQ   ((uint32_t)Pods::TYPE_ID, Info.tid );
	EXPECT_STREQ("Pods", Info.name);
	EXPECT_EQ   (10u,    Info.member_count);

	EXPECT_STREQ("i8",                   Members[0].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[0].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT8,   Members[0].storage);

	EXPECT_STREQ("i16"  ,                Members[1].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[1].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT16,  Members[1].storage);

	EXPECT_STREQ("i32",                  Members[2].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[2].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT32,  Members[2].storage);

	EXPECT_STREQ("i64",                  Members[3].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[3].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_INT64,  Members[3].storage);

	EXPECT_STREQ("u8",                   Members[4].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[4].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT8,  Members[4].storage);

	EXPECT_STREQ("u16",                  Members[5].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[5].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT16, Members[5].storage);

	EXPECT_STREQ("u32",                  Members[6].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[6].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT32, Members[6].storage);

	EXPECT_STREQ("u64",                  Members[7].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[7].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_UINT64, Members[7].storage);

	EXPECT_STREQ("f32",                  Members[8].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[8].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP32,   Members[8].storage);

	EXPECT_STREQ("f64",                  Members[9].name);
	EXPECT_EQ   (DL_TYPE_ATOM_POD,       Members[9].atom);
	EXPECT_EQ   (DL_TYPE_STORAGE_FP64,   Members[9].storage);
}

TEST_F(DLReflect, enums)
{
	dl_enum_info enum_info;
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_int8_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_int8_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_int8", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_INT8, enum_info.storage);
		EXPECT_EQ   (                       4u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_int16_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_int16_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_int16", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_INT16, enum_info.storage);
		EXPECT_EQ   (                        4u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_int32_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_int32_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_int32", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_INT32, enum_info.storage);
		EXPECT_EQ   (                        4u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_int64_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_int64_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_int64", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_INT64, enum_info.storage);
		EXPECT_EQ   (                   4u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_uint8_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_uint8_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_uint8", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_UINT8, enum_info.storage);
		EXPECT_EQ   (                        3u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_uint16_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_uint16_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_uint16", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_UINT16, enum_info.storage);
		EXPECT_EQ   (                         3u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_uint32_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_uint32_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_uint32", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_UINT32, enum_info.storage);
		EXPECT_EQ   (                         3u, enum_info.value_count);
	}
	{
		EXPECT_DL_ERR_OK(dl_reflect_get_enum_info(Ctx, enum_uint64_TYPE_ID, &enum_info));
		EXPECT_EQ   (        enum_uint64_TYPE_ID, enum_info.tid);
		EXPECT_STREQ(              "enum_uint64", enum_info.name);
		EXPECT_EQ   (DL_TYPE_STORAGE_ENUM_UINT64, enum_info.storage);
		EXPECT_EQ   (                         3u, enum_info.value_count);
	}
}

TEST_F(DLReflect, enum_member_size_align)
{
	dl_member_info members[8];
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, sized_enums_inl_array::TYPE_ID, members, DL_ARRAY_LENGTH(members)));

	sized_enums_inl_array dummy;

#define CHECK_INL_ARRAY(mem, mem_name, type) \
	EXPECT_STREQ(#mem_name,                                                 mem.name); \
	EXPECT_EQ(DL_TYPE_ATOM_INLINE_ARRAY,                                    mem.atom); \
	EXPECT_EQ(type,                                                         mem.storage); \
	EXPECT_EQ(sizeof(dummy.mem_name),                                       mem.size); \
	EXPECT_EQ((unsigned int)((uint8_t*)&dummy.mem_name - (uint8_t*)&dummy), mem.offset); \
	EXPECT_EQ(DL_ARRAY_LENGTH(dummy.mem_name),                              mem.array_count); \
	EXPECT_EQ(0u,                                                           mem.bitfield_bits)

	CHECK_INL_ARRAY(members[0], e_int8,   DL_TYPE_STORAGE_ENUM_INT8);
	CHECK_INL_ARRAY(members[1], e_int16,  DL_TYPE_STORAGE_ENUM_INT16);
	CHECK_INL_ARRAY(members[2], e_int32,  DL_TYPE_STORAGE_ENUM_INT32);
	CHECK_INL_ARRAY(members[3], e_int64,  DL_TYPE_STORAGE_ENUM_INT64);
	CHECK_INL_ARRAY(members[4], e_uint8,  DL_TYPE_STORAGE_ENUM_UINT8);
	CHECK_INL_ARRAY(members[5], e_uint16, DL_TYPE_STORAGE_ENUM_UINT16);
	CHECK_INL_ARRAY(members[6], e_uint32, DL_TYPE_STORAGE_ENUM_UINT32);
	CHECK_INL_ARRAY(members[7], e_uint64, DL_TYPE_STORAGE_ENUM_UINT64);

	EXPECT_EQ(DL_ALIGNMENTOF(int8_t),   members[0].alignment);
	EXPECT_EQ(DL_ALIGNMENTOF(int16_t),  members[1].alignment);
	EXPECT_EQ(DL_ALIGNMENTOF(int32_t),  members[2].alignment);
	EXPECT_EQ(                     8u,  members[3].alignment);
	EXPECT_EQ(DL_ALIGNMENTOF(uint8_t),  members[4].alignment);
	EXPECT_EQ(DL_ALIGNMENTOF(uint16_t), members[5].alignment);
	EXPECT_EQ(DL_ALIGNMENTOF(uint32_t), members[6].alignment);
	EXPECT_EQ(                     8u,  members[7].alignment);

#undef CHECK_INL_ARRAY
}

#define CHECK_TYPE_INFO_CORRECT( TYPE_NAME, MEM_COUNT ) { \
	dl_type_info_t ti; \
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, TYPE_NAME::TYPE_ID, &ti ) ); \
	EXPECT_EQ((uint32_t)TYPE_NAME::TYPE_ID,      ti.tid); \
	EXPECT_STREQ( #TYPE_NAME,                    ti.name ); \
	EXPECT_EQ(sizeof(TYPE_NAME),                 ti.size); \
	EXPECT_EQ((size_t)DL_ALIGNMENTOF(TYPE_NAME), ti.alignment); \
	EXPECT_EQ(MEM_COUNT,                         ti.member_count); \
}

TEST_F(DLReflect, get_type_info)
{
	CHECK_TYPE_INFO_CORRECT( Pods, 10u );
	CHECK_TYPE_INFO_CORRECT( Pods2, 2u );
	CHECK_TYPE_INFO_CORRECT( Strings, 2u );
	CHECK_TYPE_INFO_CORRECT( PtrChain, 2u );
	CHECK_TYPE_INFO_CORRECT( MoreBits, 2u );
	CHECK_TYPE_INFO_CORRECT( MorePods, 2u );
	CHECK_TYPE_INFO_CORRECT( TestBits, 7u );
	CHECK_TYPE_INFO_CORRECT( PodArray1, 1u );
	CHECK_TYPE_INFO_CORRECT( PodArray2, 1u );
	CHECK_TYPE_INFO_CORRECT( SimplePtr, 2u );
	CHECK_TYPE_INFO_CORRECT( StringArray, 1u );
	CHECK_TYPE_INFO_CORRECT( StructArray1, 1u );
	CHECK_TYPE_INFO_CORRECT( Pod2InStruct, 2u );
	CHECK_TYPE_INFO_CORRECT( WithInlineArray, 1u );
	CHECK_TYPE_INFO_CORRECT( StringInlineArray, 1u );
	CHECK_TYPE_INFO_CORRECT( ArrayEnum, 1u );
	CHECK_TYPE_INFO_CORRECT( InlineArrayEnum, 1u );
	CHECK_TYPE_INFO_CORRECT( Pod2InStructInStruct, 1u );
	CHECK_TYPE_INFO_CORRECT( WithInlineStructArray, 1u );
	CHECK_TYPE_INFO_CORRECT( WithInlineStructStructArray, 1u );
	CHECK_TYPE_INFO_CORRECT( DoublePtrChain, 3u );
	CHECK_TYPE_INFO_CORRECT( A128BitAlignedType, 1u );
	CHECK_TYPE_INFO_CORRECT( BugTest1, 1u );
	CHECK_TYPE_INFO_CORRECT( BugTest1_InArray, 3u );
	CHECK_TYPE_INFO_CORRECT( circular_array, 2u );
	CHECK_TYPE_INFO_CORRECT( circular_array_ptr_holder, 1u );
	CHECK_TYPE_INFO_CORRECT( BitBitfield64, 4u );
	CHECK_TYPE_INFO_CORRECT( test_union_simple, 3u );
	CHECK_TYPE_INFO_CORRECT( test_union_array, 2u );
	CHECK_TYPE_INFO_CORRECT( sized_enums, 8u );
	CHECK_TYPE_INFO_CORRECT( sized_enums_inl_array, 8u );
	CHECK_TYPE_INFO_CORRECT( sized_enums_array, 8u );
}

TEST_F(DLReflect, specified_alignment)
{
	int align = (int)DL_ALIGNMENTOF(A128BitAlignedType);
	EXPECT_EQ( 128, align );
}

TEST_F(DLReflect, is_extern_reflected)
{
	dl_type_info_t info;
	memset( &info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods_TYPE_ID, &info ) );
	EXPECT_FALSE( info.is_extern );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, vec3_test_TYPE_ID, &info ) );
	EXPECT_TRUE( info.is_extern );
}

TEST_F(DLReflect, is_union_reflected)
{
	dl_type_info_t info;
	memset( &info, 0x0, sizeof(dl_type_info_t) );

	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods_TYPE_ID, &info ) );
	EXPECT_FALSE( info.is_union );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, test_union_simple_TYPE_ID, &info ) );
	EXPECT_TRUE( info.is_union );
}

#define CHECK_BITFIELD_MEMBER_CORRECT( member, MEMBER_NAME, BITFIELD_BITS, BITFIELD_OFFSET ) { \
	EXPECT_EQ(DL_TYPE_ATOM_BITFIELD, member.atom); \
	EXPECT_STREQ(#MEMBER_NAME,       member.name ); \
	EXPECT_EQ(BITFIELD_BITS,         member.bitfield_bits); \
	EXPECT_EQ(BITFIELD_OFFSET,       member.bitfield_offset); \
}

TEST_F(DLReflect, is_bitfield_reflected)
{
	dl_member_info_t TestBits_members[7];
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, TestBits_TYPE_ID, TestBits_members, DL_ARRAY_LENGTH( TestBits_members ) ) );

	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[0], Bit1, 1, 0);
	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[1], Bit2, 2, 1);
	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[2], Bit3, 3, 3);
	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[4], Bit4, 1, 0);
	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[5], Bit5, 2, 1);
	CHECK_BITFIELD_MEMBER_CORRECT(TestBits_members[6], Bit6, 3, 3);

	uint8_t value = 0b10101;
	TestBits bits = { 0 };
	uint8_t* data = reinterpret_cast<uint8_t*>( &bits );

	#define ASSIGN_BITFIELD( member ) data[member.offset] |= ( value & ( ( 1 << member.bitfield_bits ) - 1 ) ) << member.bitfield_offset

	ASSIGN_BITFIELD( TestBits_members[0] );
	ASSIGN_BITFIELD( TestBits_members[1] );
	ASSIGN_BITFIELD( TestBits_members[2] );
	data[TestBits_members[3].offset] |= value;

	value ^= 0xFFU;
	ASSIGN_BITFIELD( TestBits_members[4] );
	ASSIGN_BITFIELD( TestBits_members[5] );
	ASSIGN_BITFIELD( TestBits_members[6] );

	value ^= 0xFFU;
	EXPECT_EQ( value & 0b1, bits.Bit1 );
	EXPECT_EQ( value & 0b11, bits.Bit2 );
	EXPECT_EQ( value & 0b111, bits.Bit3 );
	EXPECT_EQ( value, bits.make_it_uneven );
	value ^= 0xFFU;
	EXPECT_EQ( value & 0b1, bits.Bit4 );
	EXPECT_EQ( value & 0b11, bits.Bit5 );
	EXPECT_EQ( value & 0b111, bits.Bit6 );
}

TEST_F(DLReflect, type_lookup)
{
	dl_typeid_t type_id = 0;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "Pods2", &type_id) );
	EXPECT_TRUE(Pods2::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "StructArray1", &type_id) );
	EXPECT_TRUE(StructArray1::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "StringInlineArray", &type_id) );
	EXPECT_TRUE(StringInlineArray::TYPE_ID == type_id);
	EXPECT_DL_ERR_OK( dl_reflect_get_type_id(Ctx, "TestBits", &type_id) );
	EXPECT_TRUE(TestBits::TYPE_ID == type_id);

	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bloobloo", &type_id) );
	EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_NOT_FOUND, dl_reflect_get_type_id(Ctx, "bopp", &type_id) );
}

TEST_F(DLReflect, type_strings)
{
	// tests that all strings in tlds is reflected as expected... test types/members and enums from both unittest.tld and unittest2.tld that is both loaded in Ctx.
	dl_type_info_t Pods2_type_info;
	dl_type_info_t BugTest1_InArray_type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, Pods2_TYPE_ID, &Pods2_type_info ) );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( Ctx, BugTest1_InArray_TYPE_ID, &BugTest1_InArray_type_info ) );

	EXPECT_EQ( 2u, Pods2_type_info.member_count );
	EXPECT_EQ( 3u, BugTest1_InArray_type_info.member_count );
	EXPECT_STREQ( "Pods2", Pods2_type_info.name );
	EXPECT_STREQ( "BugTest1_InArray", BugTest1_InArray_type_info.name );

	// ... and members ...
	dl_member_info_t Pods2_members[2];
	dl_member_info_t BugTest1_InArray_members[3];
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, Pods2_TYPE_ID, Pods2_members, 2 ) );
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( Ctx, BugTest1_InArray_TYPE_ID, BugTest1_InArray_members, 3 ) );

	EXPECT_STREQ( "Int1", Pods2_members[0].name );
	EXPECT_STREQ( "Int2", Pods2_members[1].name );

	EXPECT_STREQ( "u64_1", BugTest1_InArray_members[0].name );
	EXPECT_STREQ( "u64_2", BugTest1_InArray_members[1].name );
	EXPECT_STREQ( "u16",   BugTest1_InArray_members[2].name );
}

TEST_F(DLReflect, struct_with_union_sizes)
{
	dl_type_info_t union_with_weird_members_event_info;
	EXPECT_DL_ERR_OK(dl_reflect_get_type_info(Ctx, union_with_weird_members_event_TYPE_ID, &union_with_weird_members_event_info));
	EXPECT_EQ(union_with_weird_members_event_info.size, sizeof(union_with_weird_members_event));
}

TEST_F(DLReflect, struct_sizes)
{
	dl_type_info_t struct_with_a_bitfield_info;
	EXPECT_DL_ERR_OK(dl_reflect_get_type_info(Ctx, struct_with_a_bitfield_TYPE_ID, &struct_with_a_bitfield_info));
	EXPECT_EQ(struct_with_a_bitfield_info.size, sizeof(struct_with_a_bitfield));

	dl_type_info_t struct_with_some_defaults_info;
	EXPECT_DL_ERR_OK(dl_reflect_get_type_info(Ctx, struct_with_some_defaults_TYPE_ID, &struct_with_some_defaults_info));
	EXPECT_EQ(struct_with_some_defaults_info.size, sizeof(struct_with_some_defaults));

	dl_type_info_t struct_with_one_member_and_default_info;
	EXPECT_DL_ERR_OK(dl_reflect_get_type_info(Ctx, struct_with_one_member_and_default_TYPE_ID, &struct_with_one_member_and_default_info));
	EXPECT_EQ(struct_with_one_member_and_default_info.size, sizeof(struct_with_one_member_and_default));

}