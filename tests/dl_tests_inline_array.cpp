#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, inline_array_pod) // TODO: add tests for all inline_array-types
{
	WithInlineArray Orig;
	WithInlineArray New;

	Orig.Array[0] = 1337;
	Orig.Array[1] = 7331;
	Orig.Array[2] = 1234;

	this->do_the_round_about( WithInlineArray::TYPE_ID, &Orig, &New, sizeof(WithInlineArray) );

	EXPECT_EQ(Orig.Array[0], New.Array[0]);
	EXPECT_EQ(Orig.Array[1], New.Array[1]);
	EXPECT_EQ(Orig.Array[2], New.Array[2]);
}

TYPED_TEST(DLBase, inline_array_struct)
{
	WithInlineStructArray Orig;
	WithInlineStructArray New;

	Orig.Array[0].Int1 = 1337;
	Orig.Array[0].Int2 = 7331;
	Orig.Array[1].Int1 = 1224;
	Orig.Array[1].Int2 = 5678;
	Orig.Array[2].Int1 = 9012;
	Orig.Array[2].Int2 = 3456;

	this->do_the_round_about( WithInlineStructArray::TYPE_ID, &Orig, &New, sizeof(WithInlineStructArray) );

	EXPECT_EQ(Orig.Array[0].Int1, New.Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Int2, New.Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Int1, New.Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Int2, New.Array[1].Int2);
	EXPECT_EQ(Orig.Array[2].Int1, New.Array[2].Int1);
	EXPECT_EQ(Orig.Array[2].Int2, New.Array[2].Int2);
}

TYPED_TEST(DLBase, inline_array_struct_in_struct)
{
	WithInlineStructStructArray Orig;
	WithInlineStructStructArray New;

	Orig.Array[0].Array[0].Int1 = 1;
	Orig.Array[0].Array[0].Int2 = 3;
	Orig.Array[0].Array[1].Int1 = 3;
	Orig.Array[0].Array[1].Int2 = 7;
	Orig.Array[0].Array[2].Int1 = 13;
	Orig.Array[0].Array[2].Int2 = 37;
	Orig.Array[1].Array[0].Int1 = 1337;
	Orig.Array[1].Array[0].Int2 = 7331;
	Orig.Array[1].Array[1].Int1 = 1;
	Orig.Array[1].Array[1].Int2 = 337;
	Orig.Array[1].Array[2].Int1 = 133;
	Orig.Array[1].Array[2].Int2 = 7;

	this->do_the_round_about( WithInlineStructStructArray::TYPE_ID, &Orig, &New, sizeof(WithInlineStructStructArray) );

	EXPECT_EQ(Orig.Array[0].Array[0].Int1, New.Array[0].Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Array[0].Int2, New.Array[0].Array[0].Int2);
	EXPECT_EQ(Orig.Array[0].Array[1].Int1, New.Array[0].Array[1].Int1);
	EXPECT_EQ(Orig.Array[0].Array[1].Int2, New.Array[0].Array[1].Int2);
	EXPECT_EQ(Orig.Array[0].Array[2].Int1, New.Array[0].Array[2].Int1);
	EXPECT_EQ(Orig.Array[0].Array[2].Int2, New.Array[0].Array[2].Int2);
	EXPECT_EQ(Orig.Array[1].Array[0].Int1, New.Array[1].Array[0].Int1);
	EXPECT_EQ(Orig.Array[1].Array[0].Int2, New.Array[1].Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Array[1].Int1, New.Array[1].Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Array[1].Int2, New.Array[1].Array[1].Int2);
	EXPECT_EQ(Orig.Array[1].Array[2].Int1, New.Array[1].Array[2].Int1);
	EXPECT_EQ(Orig.Array[1].Array[2].Int2, New.Array[1].Array[2].Int2);
}

TYPED_TEST(DLBase, inline_array_string)
{
	StringInlineArray original = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_string_with_commas)
{
	StringInlineArray original = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_subptr_patched)
{
	InlineArrayWithSubString original = { { { (char*)"a,pa" }, { (char*)"kos,sa" } } };
	InlineArrayWithSubString loaded[5];

	this->do_the_round_about( InlineArrayWithSubString::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ( original.Array[0].Str, loaded[0].Array[0].Str );
	EXPECT_STREQ( original.Array[1].Str, loaded[0].Array[1].Str );
}

TYPED_TEST(DLBase, inline_array_string_null)
{
	StringInlineArray original = { { (char*)"awsum", 0x0, (char*)"FTW!" } } ;
	StringInlineArray loaded[5];

	this->do_the_round_about( StringInlineArray::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Strings[0], loaded[0].Strings[0]);
	EXPECT_STREQ(original.Strings[1], loaded[0].Strings[1]);
	EXPECT_STREQ(original.Strings[2], loaded[0].Strings[2]);
}

TYPED_TEST(DLBase, inline_array_enum)
{
	InlineArrayEnum original = { { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4 } };
	InlineArrayEnum loaded;

	this->do_the_round_about( InlineArrayEnum::TYPE_ID, &original, &loaded, sizeof(InlineArrayEnum) );

	EXPECT_EQ(original.EnumArr[0], loaded.EnumArr[0]);
	EXPECT_EQ(original.EnumArr[1], loaded.EnumArr[1]);
	EXPECT_EQ(original.EnumArr[2], loaded.EnumArr[2]);
	EXPECT_EQ(original.EnumArr[3], loaded.EnumArr[3]);
}

TYPED_TEST(DLBase, inline_array_enum_size)
{
	test_inline_array_size_from_enum original = { { 1, 2, 3, 4, 5, 6, 7 }, { 3, 4, 5 } };
	test_inline_array_size_from_enum loaded;

	EXPECT_EQ( (uint32_t)TESTENUM2_VALUE1, DL_ARRAY_LENGTH(original.arr1) );
	EXPECT_EQ( (uint32_t)TESTENUM1_VALUE4, DL_ARRAY_LENGTH(original.arr2) );

	this->do_the_round_about( test_inline_array_size_from_enum::TYPE_ID, &original, &loaded, sizeof(loaded) );

	EXPECT_ARRAY_EQ( DL_ARRAY_LENGTH(original.arr1), original.arr1, loaded.arr1 );
	EXPECT_ARRAY_EQ( DL_ARRAY_LENGTH(original.arr2), original.arr2, loaded.arr2 );
}

