/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_util.h>

#include "dl_test_common.h"

const char* TEMP_FILE_NAME = "temp_dl_file.packed";

class DLUtil : public DL
{
public:
	Pods p;

	virtual void SetUp()
	{
		DL::SetUp();

		p.i8  = 1; p.i16 = 2; p.i32 = 3; p.i64 = 4;
		p.u8  = 5; p.u16 = 6; p.u32 = 7; p.u64 = 8;
		p.f32 = 9.10f; p.f64 = 10.11;
	}

	virtual void TearDown()
	{
		DL::TearDown();

		remove( TEMP_FILE_NAME );
	}

	void check_loaded( Pods* p2 )
	{
		EXPECT_EQ( p.i8,  p2->i8 );
		EXPECT_EQ( p.i16, p2->i16 );
		EXPECT_EQ( p.i32, p2->i32 );
		EXPECT_EQ( p.i64, p2->i64 );
		EXPECT_EQ( p.u8,  p2->u8 );
		EXPECT_EQ( p.u16, p2->u16 );
		EXPECT_EQ( p.u32, p2->u32 );
		EXPECT_EQ( p.u64, p2->u64 );
		EXPECT_EQ( p.f32, p2->f32 );
		EXPECT_EQ( p.f64, p2->f64 );
	}
};

TEST_F( DLUtil, store_load_binary )
{
	// write struct to temp-file
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_BINARY,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;
																				
	// read struct from temp-flle.
	EXPECT_DL_ERR_OK( dl_util_load_from_file( Ctx,
											  Pods::TYPE_ID,
											  TEMP_FILE_NAME,
											  DL_UTIL_FILE_TYPE_BINARY,
											  &conv.vp,
											  &stored_type ) );

	dl_typeid_t expect = Pods::TYPE_ID;
	EXPECT_EQ( expect, stored_type );
	check_loaded( conv.p2 );
	free( conv.p2 );
}

TEST_F( DLUtil, store_load_text )
{
	// write struct to temp-file
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_TEXT,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;

	// read struct from temp-flle.
	EXPECT_DL_ERR_OK( dl_util_load_from_file( Ctx,
											  Pods::TYPE_ID,
											  TEMP_FILE_NAME,
											  DL_UTIL_FILE_TYPE_TEXT,
											  &conv.vp,
											  &stored_type ) );

	dl_typeid_t expect = Pods::TYPE_ID;
	EXPECT_EQ( expect, stored_type );
	check_loaded( conv.p2 );
	free( conv.p2 );
}

TEST_F( DLUtil, load_text_from_binary_error )
{
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_BINARY,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;

	EXPECT_DL_ERR_EQ( DL_ERROR_UTIL_FILE_TYPE_MISMATCH, dl_util_load_from_file( Ctx,
																				Pods::TYPE_ID,
																				TEMP_FILE_NAME,
																				DL_UTIL_FILE_TYPE_TEXT,
																				&conv.vp,
																				&stored_type ) );

	EXPECT_EQ( 0x0, conv.p2 ); // should be untouched
}

TEST_F( DLUtil, load_binary_from_text_error )
{
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_TEXT,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;

	EXPECT_DL_ERR_EQ( DL_ERROR_UTIL_FILE_TYPE_MISMATCH, dl_util_load_from_file( Ctx,
																				Pods::TYPE_ID,
																				TEMP_FILE_NAME,
																				DL_UTIL_FILE_TYPE_BINARY,
																				&conv.vp,
																				&stored_type ) );

	EXPECT_EQ( 0x0, conv.p2 ); // should be untouched
}

TEST_F( DLUtil, auto_detect_binary_file_format )
{
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_BINARY,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;

	EXPECT_DL_ERR_OK( dl_util_load_from_file( Ctx,
											  0, // check autodetection of type
											  TEMP_FILE_NAME,
											  DL_UTIL_FILE_TYPE_AUTO,
											  &conv.vp,
											  &stored_type ) );

	dl_typeid_t expect = Pods::TYPE_ID;
	EXPECT_EQ( expect, stored_type );
	check_loaded( conv.p2 );

	free( conv.p2 );
}

TEST_F( DLUtil, auto_detect_text_file_format )
{
	EXPECT_DL_ERR_OK( dl_util_store_to_file( Ctx,
											 Pods::TYPE_ID,
											 TEMP_FILE_NAME,
											 DL_UTIL_FILE_TYPE_TEXT,
											 DL_ENDIAN_HOST,
											 sizeof(void*),
											 &p ) );

	union { Pods* p2; void* vp; } conv;
	conv.p2 = 0x0;
	dl_typeid_t stored_type;

	EXPECT_DL_ERR_OK( dl_util_load_from_file( Ctx,
											  0, // check autodetection of type
											  TEMP_FILE_NAME,
											  DL_UTIL_FILE_TYPE_AUTO,
											  &conv.vp,
											  &stored_type ) );

	dl_typeid_t expect = Pods::TYPE_ID;
	EXPECT_EQ( expect, stored_type );
	check_loaded( conv.p2 );

	free( conv.p2 );
}

// store in other endian and load!
