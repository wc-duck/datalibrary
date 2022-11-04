#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, array_pod1)
{
	uint32_t array_data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	PodArray1 original = { { array_data, DL_ARRAY_LENGTH( array_data ) } };

	size_t loaded_size = this->calculate_unpack_size(PodArray1::TYPE_ID, &original);
	PodArray1 *loaded = (PodArray1*)malloc(loaded_size);

	this->do_the_round_about( PodArray1::TYPE_ID, &original, loaded, loaded_size );

	EXPECT_EQ(original.u32_arr.count, loaded[0].u32_arr.count);
	EXPECT_ARRAY_EQ(original.u32_arr.count, original.u32_arr.data, loaded[0].u32_arr.data);
	free(loaded);
}

TYPED_TEST(DLBase, array_with_sub_array)
{
	uint32_t array_data[] = { 1337, 7331 } ;

	PodArray1 original_array[1];
	original_array[0].u32_arr.data  = array_data;
	original_array[0].u32_arr.count = DL_ARRAY_LENGTH(array_data);
	PodArray2 original;
	original.sub_arr.data  = original_array;
	original.sub_arr.count = DL_ARRAY_LENGTH(original_array);
	PodArray2 loaded[16];

	this->do_the_round_about( PodArray2::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.sub_arr.count, loaded[0].sub_arr.count);
	EXPECT_EQ(original.sub_arr[0].u32_arr.count, loaded[0].sub_arr[0].u32_arr.count);
	EXPECT_ARRAY_EQ(original.sub_arr[0].u32_arr.count, original.sub_arr[0].u32_arr.data, loaded[0].sub_arr[0].u32_arr.data);
}

TYPED_TEST(DLBase, array_with_sub_array2)
{
	uint32_t array_data1[] = { 1337, 7331, 13, 37, 133 } ;
	uint32_t array_data2[] = { 7, 1, 337 } ;

	PodArray1 original_array[] = { { { array_data1, DL_ARRAY_LENGTH(array_data1) } }, { { array_data2, DL_ARRAY_LENGTH(array_data2) } } } ;
	PodArray2 original = { { original_array, DL_ARRAY_LENGTH(original_array) } };

	PodArray2 loaded[16];

	this->do_the_round_about( PodArray2::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.sub_arr.count, loaded[0].sub_arr.count);
	EXPECT_EQ(original.sub_arr[0].u32_arr.count, loaded[0].sub_arr[0].u32_arr.count);
	EXPECT_EQ(original.sub_arr[1].u32_arr.count, loaded[0].sub_arr[1].u32_arr.count);
	EXPECT_ARRAY_EQ(original.sub_arr[0].u32_arr.count, original.sub_arr[0].u32_arr.data, loaded[0].sub_arr[0].u32_arr.data);
	EXPECT_ARRAY_EQ(original.sub_arr[1].u32_arr.count, original.sub_arr[1].u32_arr.data, loaded[0].sub_arr[1].u32_arr.data);
}

TYPED_TEST(DLBase, array_string)
{
	const char* array_data[] = { "I like", "the", "1337 ", "cowbells of doom!" };
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_string_merge)
{
	const char* array_data[] = { "cowbell, cowbell", "cowbell, cowbell" };
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[20];

	this->do_the_round_about(StringArray::TYPE_ID, &original, &loaded[0], sizeof(loaded) / 2);

	size_t merged_from_txt_size = 0; // Two long but identical strings, should be merged
	EXPECT_DL_ERR_OK(dl_txt_unpack_loaded_calc_size(this->Ctx, original.TYPE_ID, (unsigned char*)&original, &merged_from_txt_size));
	char* txt_buffer = (char*)malloc( merged_from_txt_size );
	EXPECT_DL_ERR_OK(dl_txt_unpack_loaded(this->Ctx, original.TYPE_ID, (unsigned char*)&original, txt_buffer, merged_from_txt_size, &merged_from_txt_size));
	EXPECT_DL_ERR_OK(dl_txt_pack_calc_size(this->Ctx, txt_buffer, &merged_from_txt_size));
	free( txt_buffer );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);

	const char* array_data_2[] = { "cowbell 1", "cowbell 2" };
	original = { { array_data_2, DL_ARRAY_LENGTH(array_data_2) } };

	this->do_the_round_about(StringArray::TYPE_ID, &original, &loaded[10], sizeof(loaded) / 2);

	size_t unique_from_txt_size = 0; // Two shorter and different strings, should not be merged
	EXPECT_DL_ERR_OK(dl_txt_unpack_loaded_calc_size(this->Ctx, original.TYPE_ID, (unsigned char*)&original, &unique_from_txt_size));
	txt_buffer = (char*)malloc( unique_from_txt_size );
	EXPECT_DL_ERR_OK(dl_txt_unpack_loaded(this->Ctx, original.TYPE_ID, (unsigned char*)&original, txt_buffer, unique_from_txt_size, &unique_from_txt_size));
	EXPECT_DL_ERR_OK(dl_txt_pack_calc_size(this->Ctx, txt_buffer, &unique_from_txt_size));
	free( txt_buffer );

	EXPECT_STREQ(original.Strings[0], loaded[10].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[10].Strings[1]);

	size_t merged_store_size = 0; // Two long but identical strings, should be merged
	EXPECT_DL_ERR_OK(dl_instance_calc_size(this->Ctx, loaded[0].TYPE_ID, &loaded[0], &merged_store_size));
	size_t unique_store_size = 0; // Two shorter and different strings, should not be merged
	EXPECT_DL_ERR_OK(dl_instance_calc_size(this->Ctx, loaded[10].TYPE_ID, &loaded[10], &unique_store_size));
	EXPECT_LT(merged_store_size, unique_store_size); // Check that long merged strings take less memory than short unique strings
	EXPECT_LT(merged_from_txt_size, unique_from_txt_size); // Check that long merged strings take less memory than short unique strings
}

TYPED_TEST(DLBase, array_string_null)
{
	const char* array_data[] = { "I like", "the", 0x0, "cowbells of doom!" };
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_string_with_commas)
{
	const char* array_data[] = { "I li,ke", "the", "13,37 ", "cowbe,lls of doom!" };
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
}

TYPED_TEST(DLBase, array_struct)
{
	Pods2 array_data[4] = { { 1, 2}, { 3, 4 }, { 5, 6 }, { 7, 8 } } ;
	StructArray1 original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StructArray1 loaded[16];

	this->do_the_round_about( StructArray1::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.Array.count,   loaded[0].Array.count);
	EXPECT_EQ(original.Array[0].Int1, loaded[0].Array[0].Int1);
	EXPECT_EQ(original.Array[0].Int2, loaded[0].Array[0].Int2);
	EXPECT_EQ(original.Array[1].Int1, loaded[0].Array[1].Int1);
	EXPECT_EQ(original.Array[1].Int2, loaded[0].Array[1].Int2);
	EXPECT_EQ(original.Array[2].Int1, loaded[0].Array[2].Int1);
	EXPECT_EQ(original.Array[2].Int2, loaded[0].Array[2].Int2);
	EXPECT_EQ(original.Array[3].Int1, loaded[0].Array[3].Int1);
	EXPECT_EQ(original.Array[3].Int2, loaded[0].Array[3].Int2);
}

TYPED_TEST(DLBase, array_enum)
{
	TestEnum2 array_data[8] = { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4, TESTENUM2_VALUE4, TESTENUM2_VALUE3, TESTENUM2_VALUE2, TESTENUM2_VALUE1 } ;
	ArrayEnum original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	ArrayEnum loaded[16];

	this->do_the_round_about( ArrayEnum::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.EnumArr.count, loaded[0].EnumArr.count);
	EXPECT_EQ(original.EnumArr[0],    loaded[0].EnumArr[0]);
	EXPECT_EQ(original.EnumArr[1],    loaded[0].EnumArr[1]);
	EXPECT_EQ(original.EnumArr[2],    loaded[0].EnumArr[2]);
	EXPECT_EQ(original.EnumArr[3],    loaded[0].EnumArr[3]);
	EXPECT_EQ(original.EnumArr[4],    loaded[0].EnumArr[4]);
	EXPECT_EQ(original.EnumArr[5],    loaded[0].EnumArr[5]);
	EXPECT_EQ(original.EnumArr[6],    loaded[0].EnumArr[6]);
	EXPECT_EQ(original.EnumArr[7],    loaded[0].EnumArr[7]);
}

TYPED_TEST(DLBase, array_pod_empty)
{
	PodArray1 original = { { NULL, 0 } };
	PodArray1 loaded;

	this->do_the_round_about( PodArray1::TYPE_ID, &original, &loaded, sizeof(PodArray1) );

	EXPECT_EQ(0u,  loaded.u32_arr.count);
	EXPECT_EQ(0x0, loaded.u32_arr.data);
}

TYPED_TEST(DLBase, array_struct_empty)
{
	StructArray1 original = { { NULL, 0 } };
	StructArray1 loaded;

	this->do_the_round_about( StructArray1::TYPE_ID, &original, &loaded, sizeof(StructArray1) );

	EXPECT_EQ(0u,  loaded.Array.count);
	EXPECT_EQ(0x0, loaded.Array.data);
}

TYPED_TEST(DLBase, array_string_empty)
{
	StringArray original = { { NULL, 0 } };
	StringArray loaded;

	this->do_the_round_about( StringArray::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_EQ(0u,  loaded.Strings.count);
	EXPECT_EQ(0x0, loaded.Strings.data);
}

TYPED_TEST(DLBase, big_array_complex_test)
{
	big_array_test original = { { NULL, 0 } };
	original.members.data = (complex_member*)malloc(sizeof(complex_member) * 599);
	original.members.count = 599;
	memset(original.members.data, 0, sizeof(complex_member) * 599);
	original.members.data[1].dynamic_arr.data = (uint32_t*)malloc(sizeof(uint32_t) * 3);
	original.members.data[1].dynamic_arr.count = 3;
	memset(original.members.data[1].dynamic_arr.data, 0, sizeof(uint32_t) * 3);
	original.members.data[1].dynamic_arr.data[0] = 3;
	original.members.data[598].dynamic_arr.data = (uint32_t*)malloc(sizeof(uint32_t) * 3);
	memset(original.members.data[598].dynamic_arr.data, 0, sizeof(uint32_t) * 3);
	original.members.data[598].dynamic_arr.data[1] = 2;
	original.members.data[598].dynamic_arr.count = 3;

	size_t unpack_size = this->calculate_unpack_size(big_array_test::TYPE_ID, &original);
	big_array_test *loaded = (big_array_test*)malloc(unpack_size);
	this->do_the_round_about( big_array_test::TYPE_ID, &original, loaded, unpack_size );

	EXPECT_EQ(599u, loaded->members.count);
	EXPECT_EQ(3u,   loaded->members.data[1].dynamic_arr.count);
	EXPECT_EQ(3u,   loaded->members.data[1].dynamic_arr.data[0]);
	EXPECT_EQ(2u,   loaded->members.data[598].dynamic_arr.data[1]);
	free(original.members.data[598].dynamic_arr.data);
	free(original.members.data[1].dynamic_arr.data);
	free(original.members.data);
	free(loaded);
}

#if !defined(_MSC_VER) || _MSC_VER >= 1700 // can't test ranged for if not supported!

TEST_F(DL, ranged_for_int8)
{
	unsigned char unpack_buffer[1024];
    i8Array* arr = dl_txt_test_pack_text<i8Array>(Ctx, STRINGIFY( { i8Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	int8_t expect[3] = {1,2,3};
	int i = 0;
	for(int8_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_int16)
{
	unsigned char unpack_buffer[1024];
    i16Array* arr = dl_txt_test_pack_text<i16Array>(Ctx, STRINGIFY( { i16Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	int16_t expect[3] = {1,2,3};
	int i = 0;
	for(int16_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_int32)
{
	unsigned char unpack_buffer[1024];
    i32Array* arr = dl_txt_test_pack_text<i32Array>(Ctx, STRINGIFY( { i32Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	int32_t expect[3] = {1,2,3};
	int i = 0;
	for(int32_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_int64)
{
	unsigned char unpack_buffer[1024];
    i64Array* arr = dl_txt_test_pack_text<i64Array>(Ctx, STRINGIFY( { i64Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	int64_t expect[3] = {1,2,3};
	int i = 0;
	for(int64_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_uint8)
{
	unsigned char unpack_buffer[1024];
    u8Array* arr = dl_txt_test_pack_text<u8Array>(Ctx, STRINGIFY( { u8Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	uint8_t expect[3] = {1,2,3};
	int i = 0;
	for(uint8_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_uint16)
{
	unsigned char unpack_buffer[1024];
    u16Array* arr = dl_txt_test_pack_text<u16Array>(Ctx, STRINGIFY( { u16Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	uint16_t expect[3] = {1,2,3};
	int i = 0;
	for(uint16_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_uint32)
{
	unsigned char unpack_buffer[1024];
    u32Array* arr = dl_txt_test_pack_text<u32Array>(Ctx, STRINGIFY( { u32Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	uint32_t expect[3] = {1,2,3};
	int i = 0;
	for(uint32_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_uint64)
{
	unsigned char unpack_buffer[1024];
    u64Array* arr = dl_txt_test_pack_text<u64Array>(Ctx, STRINGIFY( { u64Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	uint64_t expect[3] = {1,2,3};
	int i = 0;
	for(uint64_t v : arr->arr )
	{
		EXPECT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_fp32)
{
	unsigned char unpack_buffer[1024];
    fp32Array* arr = dl_txt_test_pack_text<fp32Array>(Ctx, STRINGIFY( { fp32Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	float expect[3] = {1.0f,2.0f,3.0f};
	int i = 0;
	for(float v : arr->arr )
	{
		EXPECT_FLOAT_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_fp64)
{
	unsigned char unpack_buffer[1024];
    fp64Array* arr = dl_txt_test_pack_text<fp64Array>(Ctx, STRINGIFY( { fp64Array : { arr : [1,2,3] } } ), unpack_buffer, sizeof(unpack_buffer));

	double expect[3] = {1.0,2.0,3.0};
	int i = 0;
	for(double v : arr->arr )
	{
		EXPECT_DOUBLE_EQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

TEST_F(DL, ranged_for_str)
{
	unsigned char unpack_buffer[1024];
    strArray* arr = dl_txt_test_pack_text<strArray>(Ctx, STRINGIFY( { strArray : { arr : ["a", "b", "c"] } } ), unpack_buffer, sizeof(unpack_buffer));

	const char* expect[3] = {"a", "b", "c"};
	int i = 0;
	for(const char* v : arr->arr )
	{
		EXPECT_STREQ(expect[i], v);
		++i;
	}
	EXPECT_EQ(3, i);
}

#endif
