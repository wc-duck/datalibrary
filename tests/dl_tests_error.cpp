/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include "dl_test_common.h"

class DLError : public DL {};

TEST_F(DLError, all_errors_defined_in_error_to_string)
{
	for(dl_error_t Err = DL_ERROR_OK; Err < DL_ERROR_INTERNAL_ERROR; Err = (dl_error_t)((unsigned int)Err + 1))
		EXPECT_STRNE("Unknown error!", dl_error_to_string(Err));
}

TEST_F(DLError, buffer_to_small_returned)
{
	Pods p;
	unsigned char packed[1024];

	EXPECT_DL_ERR_EQ( DL_ERROR_OK,              dl_instance_store( Ctx, Pods::TYPE_ID, &p, packed,  0, 0x0 ) ); // pack to 0-size out_buffer is ok, calculating size
	EXPECT_DL_ERR_EQ( DL_ERROR_BUFFER_TOO_SMALL, dl_instance_store( Ctx, Pods::TYPE_ID, &p, packed, 10, 0x0 ) ); // test buffer smaller than header
	EXPECT_DL_ERR_EQ( DL_ERROR_BUFFER_TOO_SMALL, dl_instance_store( Ctx, Pods::TYPE_ID, &p, packed, 21, 0x0 ) ); // test buffer to small
}

TEST_F(DLError, type_mismatch_returned)
{
	// testing that DL_ERROR_TYPE_MISMATCH is returned if provided type is not matching type stored in instance

	unused u;
	size_t dummy;
	unsigned char packed[sizeof(unused) * 10]; // large enough buffer!
	unsigned char swaped[sizeof(unused) * 10]; // large enough buffer!
	unsigned char bus_buffer[sizeof(unused) * 10]; // large enough buffer!
	         char bus_text[sizeof(unused) * 10]; // large enough buffer!

	dl_endian_t other_endian = DL_ENDIAN_HOST == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE;

	EXPECT_DL_ERR_OK( dl_instance_store( Ctx, unused::TYPE_ID, &u, packed, DL_ARRAY_LENGTH(packed), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_convert( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), swaped, DL_ARRAY_LENGTH(swaped), other_endian, sizeof(void*), 0x0 ) );

	// test all functions in...

#define EXPECT_DL_ERR_TYPE_MISMATCH( err ) EXPECT_DL_ERR_EQ( DL_ERROR_TYPE_MISMATCH, err )
	Pods p;
	// dl.h
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_instance_load( Ctx, Pods::TYPE_ID, &p, sizeof(Pods), packed, DL_ARRAY_LENGTH(packed), 0x0 ) );

	// dl_convert.h
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert( Ctx, Pods::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), bus_buffer, DL_ARRAY_LENGTH(bus_buffer), other_endian,   sizeof(void*), 0x0 ) );
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert( Ctx, Pods::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), bus_buffer, DL_ARRAY_LENGTH(bus_buffer), DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );

	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert_inplace( Ctx, Pods::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), other_endian,   sizeof(void*), 0x0 ) );
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert_inplace( Ctx, Pods::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );

	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert_calc_size( Ctx, Pods::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), sizeof(void*), &dummy ) );
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_convert_calc_size( Ctx, Pods::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), sizeof(void*), &dummy ) );

	// dl_txt.h
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_txt_unpack( Ctx, Pods::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), bus_text, DL_ARRAY_LENGTH(bus_text), 0x0 ) );
	EXPECT_DL_ERR_TYPE_MISMATCH( dl_txt_unpack_calc_size( Ctx, Pods::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), &dummy ) );
#undef EXPECT_DL_ERR_TYPE_MISMATCH
}

TEST_F(DLError, typelib_version_mismatch_returned)
{
	static const unsigned char typelib[] =
	{
		#include "generated/unittest.bin.h"
	};

	uint32_t modded_type_lib[sizeof(typelib)/sizeof(uint32_t) + 1];
	memcpy( modded_type_lib, typelib, sizeof(typelib) );

	// testing that errors are returned correctly by modding data.
	uint32_t* lib_version = modded_type_lib + 1;

	EXPECT_EQ(4u, *lib_version);

	*lib_version = 0xFFFFFFFF;

	dl_ctx_t tmp_ctx = 0;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	EXPECT_DL_ERR_OK( dl_context_create( &tmp_ctx, &p ) );
	EXPECT_DL_ERR_EQ( DL_ERROR_VERSION_MISMATCH, dl_context_load_type_library( tmp_ctx, (unsigned char*)modded_type_lib, sizeof(modded_type_lib) ) );
	EXPECT_DL_ERR_OK( dl_context_destroy( tmp_ctx ) );
}

TEST_F(DLError, version_mismatch_returned)
{
	unused u;
	size_t dummy;
	unsigned char packed[sizeof(unused) * 10]; // large enough buffer!
	unsigned char swaped[sizeof(unused) * 10]; // large enough buffer!
	unsigned char bus_buffer[sizeof(unused) * 10]; // large enough buffer!
			 char bus_text[sizeof(unused) * 10]; // large enough buffer!

	dl_endian_t other_endian = DL_ENDIAN_HOST == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE;

	EXPECT_DL_ERR_OK( dl_instance_store( Ctx, unused::TYPE_ID, &u, packed, DL_ARRAY_LENGTH(packed), 0x0 ) );
	EXPECT_DL_ERR_OK( dl_convert( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), swaped, DL_ARRAY_LENGTH(swaped), other_endian, sizeof(void*), 0x0 ) );

	// testing that errors are returned correctly by modding data.
	union
	{
		unsigned int*  instance_version;
		unsigned char* instance;
	} conv;
	conv.instance = packed;
	unsigned int* instance_version;
	instance_version = conv.instance_version + 1;
	EXPECT_EQ(1u, *instance_version);
	*instance_version = 0xFFFFFFFF;
	conv.instance = swaped;
	instance_version = conv.instance_version + 1;
	EXPECT_EQ(0x01000000u, *instance_version);
	*instance_version = 0xFFFFFFFF;

	// test all functions in...

	#define EXPECT_DL_ERR_VERSION_MISMATCH( err ) EXPECT_DL_ERR_EQ( DL_ERROR_VERSION_MISMATCH, err )
		Pods p;
		// dl.h
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_instance_load( Ctx, unused::TYPE_ID, &p, sizeof(Pods), packed, DL_ARRAY_LENGTH(packed), 0x0 ) );

		// dl_convert.h
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), bus_buffer, DL_ARRAY_LENGTH(bus_buffer), other_endian,   sizeof(void*), 0x0 ) );
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert( Ctx, unused::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), bus_buffer, DL_ARRAY_LENGTH(bus_buffer), DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );

		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert_inplace( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), other_endian,   sizeof(void*), 0x0 ) );
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert_inplace( Ctx, unused::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), DL_ENDIAN_HOST, sizeof(void*), 0x0 ) );

		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert_calc_size( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), sizeof(void*), &dummy ) );
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_convert_calc_size( Ctx, unused::TYPE_ID, swaped, DL_ARRAY_LENGTH(swaped), sizeof(void*), &dummy ) );

		// dl_txt.h
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_txt_unpack( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), bus_text, DL_ARRAY_LENGTH(bus_text), 0x0 ) );
		EXPECT_DL_ERR_VERSION_MISMATCH( dl_txt_unpack_calc_size( Ctx, unused::TYPE_ID, packed, DL_ARRAY_LENGTH(packed), &dummy ) );
	#undef EXPECT_DL_ERR_VERSION_MISMATCH
}
