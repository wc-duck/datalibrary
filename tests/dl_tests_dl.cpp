/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include "dl_test_common.h"

#include <float.h>

#include "dl_tests_base.h"

TYPED_TEST(DLBase, bug1)
{
	// There was some error packing arrays ;)

	BugTest1_InArray array_data[3] = { { 1337, 1338, 18 }, { 7331, 8331, 19 }, { 31337, 73313, 20 } } ;
	BugTest1 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };

	BugTest1 DL_ALIGN(8) loaded[10];

	this->do_the_round_about( BugTest1::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(array_data[0].u64_1, loaded[0].Arr[0].u64_1);
	EXPECT_EQ(array_data[0].u64_2, loaded[0].Arr[0].u64_2);
	EXPECT_EQ(array_data[0].u16,   loaded[0].Arr[0].u16);
	EXPECT_EQ(array_data[1].u64_1, loaded[0].Arr[1].u64_1);
	EXPECT_EQ(array_data[1].u64_2, loaded[0].Arr[1].u64_2);
	EXPECT_EQ(array_data[1].u16,   loaded[0].Arr[1].u16);
	EXPECT_EQ(array_data[2].u64_1, loaded[0].Arr[2].u64_1);
	EXPECT_EQ(array_data[2].u64_2, loaded[0].Arr[2].u64_2);
	EXPECT_EQ(array_data[2].u16,   loaded[0].Arr[2].u16);
}

TYPED_TEST(DLBase, bug2)
{
	// some error converting from 32-bit-data to 64-bit.

	BugTest2_WithMat array_data[2] = { { 1337, { 1.0f, 0.0f, 0.0f, 0.0f,
										  	  	 0.0f, 1.0f, 0.0f, 0.0f,
												 0.0f, 0.0f, 1.0f, 0.0f,
												 0.0f, 0.0f, 1.0f, 0.0f } },

									   { 7331, { 0.0f, 0.0f, 0.0f, 1.0f,
											     0.0f, 0.0f, 1.0f, 0.0f,
												 0.0f, 1.0f, 0.0f, 0.0f,
												 1.0f, 0.0f, 0.0f, 0.0f } } };
	BugTest2 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };

	BugTest2 loaded[40];

	this->do_the_round_about( BugTest2::TYPE_ID, &original, &loaded, sizeof(loaded) );

 	EXPECT_EQ(array_data[0].iSubModel, loaded[0].Instances[0].iSubModel);
 	EXPECT_EQ(array_data[1].iSubModel, loaded[0].Instances[1].iSubModel);
 	EXPECT_ARRAY_EQ(16, array_data[0].Transform, loaded[0].Instances[0].Transform);
 	EXPECT_ARRAY_EQ(16, array_data[1].Transform, loaded[0].Instances[1].Transform);
}

TYPED_TEST(DLBase, bug3)
{
	// testing bug where struct first in struct with ptr in substruct will not get patched on load.

	uint32_t array_data[] = { 1337, 7331, 13, 37 };
	BugTest3 original = { { { array_data, DL_ARRAY_LENGTH( array_data ) } } };
	BugTest3 loaded[4];

	this->do_the_round_about( BugTest3::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_ARRAY_EQ( original.sub.arr.count,  original.sub.arr.data, loaded[0].sub.arr.data);
}

TYPED_TEST(DLBase, bug4)
{
	// testing bug where struct first in struct with ptr in substruct will not get patched on load.

	// TODO: Test with only 1
	const char* strings1[] = { "cow",   "bells",   "rule" };
	const char* strings2[] = { "2cow2", "2bells2", "2rule2", "and the last one" };
	StringArray str_arr[2];
	str_arr[0].Strings.data  = strings1;
	str_arr[0].Strings.count = DL_ARRAY_LENGTH( strings1 );
	str_arr[1].Strings.data  = strings2;
	str_arr[1].Strings.count = DL_ARRAY_LENGTH( strings2 );

	BugTest4 original;
	original.struct_with_str_arr.data  = str_arr;
	original.struct_with_str_arr.count = 2;

	BugTest4 loaded[1024];

	this->do_the_round_about( BugTest4::TYPE_ID, &original, &loaded, sizeof(loaded) );

	unsigned int num_struct = original.struct_with_str_arr.count;
	EXPECT_EQ( num_struct, loaded[0].struct_with_str_arr.count );

	for( unsigned int i = 0; i < num_struct; ++i )
	{
		StringArray* orig =      original.struct_with_str_arr.data + i;
		StringArray* load = loaded[0].struct_with_str_arr.data + i;
		EXPECT_EQ( orig->Strings.count, load->Strings.count );

		for( unsigned int j = 0; j < orig->Strings.count; ++j )
			EXPECT_STREQ( orig->Strings[j], load->Strings[j] );
	}
}

TYPED_TEST(DLBase, str_before_array_bug)
{
	// Test for bug #7, where string written before array would lead to mis-alignment of the array.

	str_before_array_bug_arr_type arr_data[] = { { "apan.ymesjh" } };
	str_before_array_bug mod;

	mod.str = "1234";
	mod.arr.data  = arr_data;
	mod.arr.count = DL_ARRAY_LENGTH( arr_data );

	str_before_array_bug loaded[16];

	this->do_the_round_about( str_before_array_bug::TYPE_ID, &mod, &loaded, sizeof(loaded) );

	EXPECT_STREQ( mod.str,        loaded[0].str );
	EXPECT_EQ   ( mod.arr.count,  loaded[0].arr.count );
	EXPECT_STREQ( mod.arr[0].str, loaded[0].arr[0].str );

	// Since this struct contains first a string and then an array we can use this test
	// to check that the array is correctly aligned after load, especially after txt-packing.
	EXPECT_EQ   ( 0u, (uint64_t)loaded[0].arr.data % DL_ALIGNOF(str_before_array_bug_arr_type));
}

TYPED_TEST(DLBase, bug_first_member_in_struct)
{
	// testing bug where first member of struct do not have the same
	// alignment as parent-struct and this struct is part of an array.

	bug_array_alignment_struct descs[] = {
		{ 1,  2,  3 },
		{ 9, 10, 11 },
	};

	bug_array_alignment desc = { { descs, DL_ARRAY_LENGTH(descs) } };
	bug_array_alignment DL_ALIGN(8) loaded[10];

	this->do_the_round_about( bug_array_alignment::TYPE_ID, &desc, loaded, sizeof(loaded) );

	EXPECT_EQ( desc.components[0].type, loaded[0].components[0].type );
	EXPECT_EQ( desc.components[0].ptr,  loaded[0].components[0].ptr );
	EXPECT_EQ( desc.components[1].type, loaded[0].components[1].type );
	EXPECT_EQ( desc.components[1].ptr,  loaded[0].components[1].ptr );
}

TYPED_TEST(DLBase, bug_test_1)
{
	// Testing bug where padding in array-member was missed in some cases.
	uint8_t arr1[] = { 1, 3, 3, 7 };
	uint8_t arr2[] = { 13, 37 };
	test_array_pad_1 descs[] = {
		{ { arr1, DL_ARRAY_LENGTH(arr1) },  9 },
		{ { arr2, DL_ARRAY_LENGTH(arr2) }, 11 }
	};

	test_array_pad desc = { { descs, DL_ARRAY_LENGTH(descs) } };

	test_array_pad loaded[128];
	this->do_the_round_about( test_array_pad::TYPE_ID, &desc, &loaded, sizeof(loaded) );

	EXPECT_EQ( desc.components[0].type,         loaded[0].components[0].type );

	EXPECT_EQ( desc.components[0].ptr.count, loaded[0].components[0].ptr.count );
	EXPECT_EQ( desc.components[0].ptr[0],    loaded[0].components[0].ptr[0] );
	EXPECT_EQ( desc.components[0].ptr[1],    loaded[0].components[0].ptr[1] );
	EXPECT_EQ( desc.components[0].ptr[2],    loaded[0].components[0].ptr[2] );
	EXPECT_EQ( desc.components[0].ptr[3],    loaded[0].components[0].ptr[3] );

	EXPECT_EQ( desc.components[1].ptr.count, loaded[0].components[1].ptr.count );
	EXPECT_EQ( desc.components[1].ptr[0],    loaded[0].components[1].ptr[0] );
	EXPECT_EQ( desc.components[1].ptr[1],    loaded[0].components[1].ptr[1] );
}

TYPED_TEST( DLBase, per_bug_1 )
{
	// testing that struct with string in it can be used in array.
	per_bug_1 mapping[] = { { "1str1" }, { "2str2" }, { "3str3" } };
	per_bug inst = { { mapping, DL_ARRAY_LENGTH(mapping) } };

	per_bug loaded[128];
	this->do_the_round_about( per_bug::TYPE_ID, &inst, &loaded, sizeof(loaded) );

	EXPECT_EQ( inst.arr.count, loaded[0].arr.count );

	for( uint32_t i = 0; i < loaded[0].arr.count; ++i )
		EXPECT_STREQ( inst.arr[i].str, loaded[0].arr[i].str );
}

TEST(DLMisc, endian_is_correct)
{
	// Test that DL_ENDIAN_HOST is set correctly
	union
	{
		uint32_t i;
		unsigned char c[4];
	} test;
	test.i = 0x01020304;

	if( DL_ENDIAN_HOST == DL_ENDIAN_LITTLE )
	{
		EXPECT_EQ(4, test.c[0]);
		EXPECT_EQ(3, test.c[1]);
		EXPECT_EQ(2, test.c[2]);
		EXPECT_EQ(1, test.c[3]);
	}
	else
	{
		EXPECT_EQ(1, test.c[0]);
		EXPECT_EQ(2, test.c[1]);
		EXPECT_EQ(3, test.c[2]);
		EXPECT_EQ(4, test.c[3]);
	}
}

TEST(DLMisc, built_in_tl_eq_bin_file)
{
	const unsigned char built_in_tl[] =
	{
		#include "generated/unittest.bin.h"
	};
	const char* test_file = "local/generated/unittest.bin";
	
	FILE* f = fopen( test_file, "rb" );
	fseek( f, 0, SEEK_END );
	size_t read_tl_size = (size_t)ftell(f);
	fseek( f, 0, SEEK_SET );
	
	unsigned char* read_tl = (unsigned char*)malloc(read_tl_size);
	
	EXPECT_EQ( read_tl_size, fread( read_tl, 1, read_tl_size, f ) );
	
	fclose( f );
	
	EXPECT_EQ( DL_ARRAY_LENGTH(built_in_tl), read_tl_size );
	EXPECT_EQ( 0, memcmp( read_tl, built_in_tl, read_tl_size ) );
	
	free(read_tl);
}

TYPED_TEST( DLBase, simple_alias_test )
{
	// testing usage of externally defined types
	// the members m1 and m2 are of the type vec3_test
	// that is defined outside the .tld
	//
	// specified in the tld is vec3_test marked as extern
	// that make dl_tlc not emit a definition of the
	// struct to the header.

	alias_test at;
	at.m1.x = 1.0f; at.m1.y = 2.0f; at.m1.z = 3.0f;
	at.m2.x = 4.0f; at.m2.y = 5.0f; at.m2.z = 6.0f;

	alias_test loaded;

	this->do_the_round_about( alias_test::TYPE_ID, &at, &loaded, sizeof(loaded) );

	EXPECT_EQ( at.m1.x, loaded.m1.x ); EXPECT_EQ( at.m1.y, loaded.m1.y ); EXPECT_EQ( at.m1.z, loaded.m1.z );
	EXPECT_EQ( at.m2.x, loaded.m2.x ); EXPECT_EQ( at.m2.y, loaded.m2.y ); EXPECT_EQ( at.m2.z, loaded.m2.z );
}

TEST_F( DL, bug_with_substring_and_inplace_load )
{
	bug_with_substr t1 = { "str1", { "str2" } };
	unsigned char packed_instance[256];

	// store instance to binary
	size_t pack_size;
	EXPECT_DL_ERR_OK( dl_instance_store( this->Ctx, bug_with_substr::TYPE_ID, (void**)(void*)&t1, packed_instance, sizeof( packed_instance ), &pack_size ) );

	bug_with_substr* loaded;
	EXPECT_DL_ERR_OK( dl_instance_load_inplace( this->Ctx, bug_with_substr::TYPE_ID, packed_instance, pack_size, (void**)(void*)&loaded, 0x0 ) );

	EXPECT_STREQ( t1.str, loaded->str );
	EXPECT_STREQ( t1.sub.str, loaded->sub.str );
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::GTEST_FLAG(catch_exceptions) = 0;
	return RUN_ALL_TESTS();
}
