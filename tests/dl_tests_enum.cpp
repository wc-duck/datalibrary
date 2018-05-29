#include <gtest/gtest.h>

#include "dl_test_common.h"
#include "dl_tests_base.h"
#include "generated/sized_enums.h"

static_assert(sizeof(enum_int8)   == sizeof(int8_t),   "size of enum is incorrect");
static_assert(sizeof(enum_int16)  == sizeof(int16_t),  "size of enum is incorrect");
static_assert(sizeof(enum_int32)  == sizeof(int32_t),  "size of enum is incorrect");
static_assert(sizeof(enum_int64)  == sizeof(int64_t),  "size of enum is incorrect");
static_assert(sizeof(enum_uint8)  == sizeof(uint8_t),  "size of enum is incorrect");
static_assert(sizeof(enum_uint16) == sizeof(uint16_t), "size of enum is incorrect");
static_assert(sizeof(enum_uint32) == sizeof(uint32_t), "size of enum is incorrect");
static_assert(sizeof(enum_uint64) == sizeof(uint64_t), "size of enum is incorrect");

static_assert(DL_ALIGNOF(enum_int8)   == DL_ALIGNOF(int8_t),   "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int16)  == DL_ALIGNOF(int16_t),  "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int32)  == DL_ALIGNOF(int32_t),  "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int64)  == DL_ALIGNOF(int64_t),  "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint8)  == DL_ALIGNOF(uint8_t),  "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint16) == DL_ALIGNOF(uint16_t), "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint32) == DL_ALIGNOF(uint32_t), "alignment of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint64) == DL_ALIGNOF(uint64_t), "alignment of enum is incorrect");

TYPED_TEST(DLBase, sized_enums_simple)
{
    sized_enums original;
    original.e_int8   = int8_1;
	original.e_int16  = int16_1;
	original.e_int32  = int32_1;
	original.e_int64  = int64_1;
	original.e_uint8  = uint8_1;
	original.e_uint16 = uint16_1;
	original.e_uint32 = uint32_1;
	original.e_uint64 = uint64_1;

    sized_enums loaded;
    this->do_the_round_about( sized_enums::TYPE_ID, &original, &loaded, sizeof(loaded) );

    EXPECT_EQ(int8_1,   original.e_int8);
    EXPECT_EQ(int16_1,  original.e_int16);
    EXPECT_EQ(int32_1,  original.e_int32);
    EXPECT_EQ(int64_1,  original.e_int64);
    EXPECT_EQ(uint8_1,  original.e_uint8);
    EXPECT_EQ(uint16_1, original.e_uint16);
    EXPECT_EQ(uint32_1, original.e_uint32);
    EXPECT_EQ(uint64_1, original.e_uint64);
};

TYPED_TEST(DLBase, sized_enums_simple_neg)
{
    sized_enums original;
    original.e_int8   = int8_neg;
	original.e_int16  = int16_neg;
	original.e_int32  = int32_neg;
	original.e_int64  = int64_neg;
	original.e_uint8  = uint8_1;
	original.e_uint16 = uint16_1;
	original.e_uint32 = uint32_1;
	original.e_uint64 = uint64_1;

    sized_enums loaded;
    this->do_the_round_about( sized_enums::TYPE_ID, &original, &loaded, sizeof(loaded) );

    EXPECT_EQ(int8_neg,  original.e_int8);
    EXPECT_EQ(int16_neg, original.e_int16);
    EXPECT_EQ(int32_neg, original.e_int32);
    EXPECT_EQ(int64_neg, original.e_int64);
    EXPECT_EQ(uint8_1,   original.e_uint8);
    EXPECT_EQ(uint16_1,  original.e_uint16);
    EXPECT_EQ(uint32_1,  original.e_uint32);
    EXPECT_EQ(uint64_1,  original.e_uint64);
};