#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, array_pod1)
{
	uint32_t array_data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	PodArray1 original = { { array_data, DL_ARRAY_LENGTH( array_data ) } };
	PodArray1 loaded[16];

	this->do_the_round_about( PodArray1::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_EQ(original.u32_arr.count, loaded[0].u32_arr.count);
	EXPECT_ARRAY_EQ(original.u32_arr.count, original.u32_arr.data, loaded[0].u32_arr.data);
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
	const char* array_data[] = { "I like", "the", "1337 ", "cowbells of doom!" }; // TODO: test string-arrays with , in strings.
	StringArray original = { { array_data, DL_ARRAY_LENGTH(array_data) } };
	StringArray loaded[10];

	this->do_the_round_about( StringArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
	EXPECT_STREQ(original.Strings[3], loaded[0].Strings[3]);
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
	const char* array_data[] = { "I li,ke", "the", "13,37 ", "cowbe,lls of doom!" }; // TODO: test string-arrays with , in strings.
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
