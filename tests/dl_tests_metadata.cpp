#include <gtest/gtest.h>
#include <dl/dl_reflect.h>
#include "dl_tests_base.h"

TYPED_TEST( DLBase, metadata_type )
{
	dl_type_info_t type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( DL::Ctx, type_with_meta::TYPE_ID, &type_info ) );
	EXPECT_EQ( 2U, type_info.metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, type_info.metadata_type_ids[0] );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( type_info.metadata_instances[0] )->Str );
	EXPECT_EQ( SubString_TYPE_ID, type_info.metadata_type_ids[1] );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( type_info.metadata_instances[1] )->Str );
}

TYPED_TEST( DLBase, metadata_member )
{
	dl_type_info_t type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( DL::Ctx, type_with_meta::TYPE_ID, &type_info ) );
	EXPECT_EQ( 1U, type_info.member_count );
	dl_member_info_t members;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( DL::Ctx, type_with_meta::TYPE_ID, &members, 1 ) );
	EXPECT_EQ( 2U, members.metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, members.metadata_type_ids[0] );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( members.metadata_instances[0] )->Str );
	EXPECT_EQ( SubString_TYPE_ID, members.metadata_type_ids[1] );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( members.metadata_instances[1] )->Str );
}

TYPED_TEST( DLBase, metadata_enumeration )
{
	dl_enum_info_t enum_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_enum_info( DL::Ctx, EnumWithMeta_TYPE_ID, &enum_info ) );
	EXPECT_EQ( 2U, enum_info.metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, enum_info.metadata_type_ids[0] );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( enum_info.metadata_instances[0] )->Str );
	EXPECT_EQ( SubString_TYPE_ID, enum_info.metadata_type_ids[1] );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( enum_info.metadata_instances[1] )->Str );
}

TYPED_TEST( DLBase, metadata_enumerator )
{
	dl_enum_info_t enum_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_enum_info( DL::Ctx, EnumWithMeta_TYPE_ID, &enum_info ) );
	EXPECT_EQ( 1U, enum_info.value_count );
	dl_enum_value_info_t values;
	EXPECT_DL_ERR_OK( dl_reflect_get_enum_values( DL::Ctx, EnumWithMeta_TYPE_ID, &values, 1 ) );
	EXPECT_EQ( 2U, values.metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, values.metadata_type_ids[0] );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( values.metadata_instances[0] )->Str );
	EXPECT_EQ( SubString_TYPE_ID, values.metadata_type_ids[1] );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( values.metadata_instances[1] )->Str );
}

TYPED_TEST( DLBase, metadata_union )
{
	dl_type_info_t type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( DL::Ctx, test_union_simple::TYPE_ID, &type_info ) );
	EXPECT_EQ( 2U, type_info.metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, type_info.metadata_type_ids[0] );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( type_info.metadata_instances[0] )->Str );
	EXPECT_EQ( SubString_TYPE_ID, type_info.metadata_type_ids[1] );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( type_info.metadata_instances[1] )->Str );
}

TYPED_TEST( DLBase, metadata_union_member )
{
	dl_type_info_t type_info;
	EXPECT_DL_ERR_OK( dl_reflect_get_type_info( DL::Ctx, test_union_simple::TYPE_ID, &type_info ) );
	EXPECT_EQ( 3U, type_info.member_count );
	dl_member_info_t members[3];
	EXPECT_DL_ERR_OK( dl_reflect_get_type_members( DL::Ctx, test_union_simple::TYPE_ID, members, DL_ARRAY_LENGTH(members) ) );
	EXPECT_EQ( 1U, members[2].metadata_count );
	EXPECT_EQ( SubString_TYPE_ID, members[2].metadata_type_ids[0] );
	EXPECT_STREQ( "item3", reinterpret_cast<const SubString*>( members[2].metadata_instances[0] )->Str );
}
