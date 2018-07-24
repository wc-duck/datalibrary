#include <gtest/gtest.h>
#include "dl_tests_base.h"

TYPED_TEST(DLBase, enum)
{
	EXPECT_EQ( 0, TESTENUM1_VALUE1 );
	EXPECT_EQ( 1, TESTENUM1_VALUE2 );
	EXPECT_EQ( 2, TESTENUM1_VALUE3 );
	EXPECT_EQ( 3, TESTENUM1_VALUE4 );

	EXPECT_EQ(TESTENUM2_VALUE2 + 1, TESTENUM2_VALUE3); // value3 is after value2 but has no value. It sohuld automticallay be one bigger!

	TestingEnum Inst;
	Inst.TheEnum = TESTENUM1_VALUE3;

	TestingEnum Loaded;

	this->do_the_round_about( TestingEnum::TYPE_ID, &Inst, &Loaded, sizeof(Loaded) );

	EXPECT_EQ(Inst.TheEnum, Loaded.TheEnum);
}

TYPED_TEST(DLBase, bitfield)
{
	TestBits original;
	TestBits loaded;

	original.Bit1 = 0;
	original.Bit2 = 2;
	original.Bit3 = 4;
	original.make_it_uneven = 17;
	original.Bit4 = 1;
	original.Bit5 = 0;
	original.Bit6 = 5;

	this->do_the_round_about( TestBits::TYPE_ID, &original, &loaded, sizeof(TestBits) );

	EXPECT_EQ(original.Bit1, loaded.Bit1);
	EXPECT_EQ(original.Bit2, loaded.Bit2);
	EXPECT_EQ(original.Bit3, loaded.Bit3);
	EXPECT_EQ(original.make_it_uneven, loaded.make_it_uneven);
	EXPECT_EQ(original.Bit4, loaded.Bit4);
	EXPECT_EQ(original.Bit5, loaded.Bit5);
	EXPECT_EQ(original.Bit6, loaded.Bit6);
}

TYPED_TEST(DLBase, bitfield2)
{
	MoreBits original;
	MoreBits loaded;

	original.Bit1 = 512;
	original.Bit2 = 1;

	this->do_the_round_about( MoreBits::TYPE_ID, &original, &loaded, sizeof(MoreBits) );

	EXPECT_EQ(original.Bit1, loaded.Bit1);
	EXPECT_EQ(original.Bit2, loaded.Bit2);
}

TYPED_TEST(DLBase, bitfield_64bit)
{
	BitBitfield64 original;
	BitBitfield64 loaded;

	original.Package  = 2;
	original.FileType = 13;
	original.PathHash = 1337;
	original.FileHash = 0xDEADBEEF;

	this->do_the_round_about( BitBitfield64::TYPE_ID, &original, &loaded, sizeof(BitBitfield64) );

	EXPECT_EQ(original.Package,  loaded.Package);
	EXPECT_EQ(original.FileType, loaded.FileType);
	EXPECT_EQ(original.PathHash, loaded.PathHash);
	EXPECT_EQ(original.FileHash, loaded.FileHash);
}
