#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, string)
{
	Strings original = { "cow", "bell" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, string_null)
{
	Strings original = { 0x0, "bell" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, string_with_commas)
{
	Strings original = { "str,1", "str,2" } ;
	Strings loaded[5];

	this->do_the_round_about( Strings::TYPE_ID, &original, loaded, sizeof(loaded) );

	EXPECT_STREQ(original.Str1, loaded[0].Str1);
	EXPECT_STREQ(original.Str2, loaded[0].Str2);
}

TYPED_TEST(DLBase, escaped_fnutt_in_string )
{
	SubString original = { "whoo\"whaa" };
	SubString loaded[10];

	this->do_the_round_about( SubString::TYPE_ID, &original, loaded, sizeof(loaded) );
	EXPECT_STREQ( original.Str, loaded[0].Str );
}

TYPED_TEST(DLBase, escaped_val_in_string)
{
	Strings Orig = { "cow\nbell\tbopp", "\\\"\"bopp" } ;
	Strings Loaded[16];

	this->do_the_round_about( Strings::TYPE_ID, &Orig, Loaded, sizeof(Loaded) );

	EXPECT_STREQ(Orig.Str1, Loaded[0].Str1);
	EXPECT_STREQ(Orig.Str2, Loaded[0].Str2);
}
