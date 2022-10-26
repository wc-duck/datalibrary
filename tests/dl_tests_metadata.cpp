#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST( DLBase, metadata_type )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_metadata_count( DL::Ctx, type_with_meta::TYPE_ID, &metadata_count ) );
	EXPECT_EQ( 2U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, type_with_meta::TYPE_ID, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( metadata_instance )->Str );
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, type_with_meta::TYPE_ID, 1, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}

TYPED_TEST( DLBase, metadata_member )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_member_metadata_count( DL::Ctx, type_with_meta::TYPE_ID, 0, &metadata_count ) );
	EXPECT_EQ( 2U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_member_metadata( DL::Ctx, type_with_meta::TYPE_ID, 0, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( metadata_instance )->Str );
	EXPECT_DL_ERR_OK( dl_get_member_metadata( DL::Ctx, type_with_meta::TYPE_ID, 0, 1, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}

TYPED_TEST( DLBase, metadata_enumeration )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_metadata_count( DL::Ctx, EnumWithMeta_TYPE_ID, &metadata_count ) );
	EXPECT_EQ( 2U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, EnumWithMeta_TYPE_ID, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( metadata_instance )->Str );
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, EnumWithMeta_TYPE_ID, 1, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}

TYPED_TEST( DLBase, metadata_enumerator )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_member_metadata_count( DL::Ctx, EnumWithMeta_TYPE_ID, 0, &metadata_count ) );
	EXPECT_EQ( 2U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_member_metadata( DL::Ctx, EnumWithMeta_TYPE_ID, 0, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( metadata_instance )->Str );
	EXPECT_DL_ERR_OK( dl_get_member_metadata( DL::Ctx, EnumWithMeta_TYPE_ID, 0, 1, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}

TYPED_TEST( DLBase, metadata_union )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_metadata_count( DL::Ctx, test_union_simple::TYPE_ID, &metadata_count ) );
	EXPECT_EQ( 2U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, test_union_simple::TYPE_ID, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "apa", reinterpret_cast<const SubString*>( metadata_instance )->Str );
	EXPECT_DL_ERR_OK( dl_get_metadata( DL::Ctx, test_union_simple::TYPE_ID, 1, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "banan", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}

TYPED_TEST( DLBase, metadata_union_member )
{
	uint32_t metadata_count;
	EXPECT_DL_ERR_OK( dl_get_member_metadata_count( DL::Ctx, test_union_simple::TYPE_ID, 2, &metadata_count ) );
	EXPECT_EQ( 1U, metadata_count );
	dl_typeid_t metadata_type_id;
	void* metadata_instance;
	EXPECT_DL_ERR_OK( dl_get_member_metadata( DL::Ctx, test_union_simple::TYPE_ID, 2, 0, &metadata_type_id, &metadata_instance ) );
	EXPECT_EQ( SubString_TYPE_ID, metadata_type_id );
	EXPECT_STREQ( "item3", reinterpret_cast<const SubString*>( metadata_instance )->Str );
}
