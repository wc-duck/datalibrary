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

static_assert(int8_min  == INT8_MIN,    "should be equal!");
static_assert(int8_max  == INT8_MAX,    "should be equal!");
static_assert(int16_min == INT16_MIN,   "should be equal!");
static_assert(int16_max == INT16_MAX,   "should be equal!");
static_assert(int32_min == INT32_MIN,   "should be equal!");
static_assert(int32_max == INT32_MAX,   "should be equal!");
static_assert(int64_min == INT64_MIN,   "should be equal!");
static_assert(int64_max == INT64_MAX,   "should be equal!");
static_assert(uint8_min  == 0,          "should be equal!");
static_assert(uint8_max  == UINT8_MAX,  "should be equal!");
static_assert(uint16_min == 0,          "should be equal!");
static_assert(uint16_max == UINT16_MAX, "should be equal!");
static_assert(uint32_min == 0,          "should be equal!");
static_assert(uint32_max == UINT32_MAX, "should be equal!");
static_assert(uint64_min == 0,          "should be equal!");
static_assert(uint64_max == UINT64_MAX, "should be equal!");

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

    EXPECT_EQ(original.e_int8,   loaded.e_int8);
    EXPECT_EQ(original.e_int16,  loaded.e_int16);
    EXPECT_EQ(original.e_int32,  loaded.e_int32);
    EXPECT_EQ(original.e_int64,  loaded.e_int64);
    EXPECT_EQ(original.e_uint8,  loaded.e_uint8);
    EXPECT_EQ(original.e_uint16, loaded.e_uint16);
    EXPECT_EQ(original.e_uint32, loaded.e_uint32);
    EXPECT_EQ((uint64_t)original.e_uint64, (uint64_t)loaded.e_uint64);
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

    EXPECT_EQ(original.e_int8,   loaded.e_int8);
    EXPECT_EQ(original.e_int16,  loaded.e_int16);
    EXPECT_EQ(original.e_int32,  loaded.e_int32);
    EXPECT_EQ(original.e_int64,  loaded.e_int64);
    EXPECT_EQ(original.e_uint8,  loaded.e_uint8);
    EXPECT_EQ(original.e_uint16, loaded.e_uint16);
    EXPECT_EQ(original.e_uint32, loaded.e_uint32);
    EXPECT_EQ((uint64_t)original.e_uint64, (uint64_t)loaded.e_uint64);
};

TYPED_TEST(DLBase, sized_enums_simple_min)
{
    sized_enums original;
    original.e_int8   = int8_min;
	original.e_int16  = int16_min;
	original.e_int32  = int32_min;
	original.e_int64  = int64_min;
	original.e_uint8  = uint8_min;
	original.e_uint16 = uint16_min;
	original.e_uint32 = uint32_min;
	original.e_uint64 = uint64_min;

    sized_enums loaded;
    this->do_the_round_about( sized_enums::TYPE_ID, &original, &loaded, sizeof(loaded) );

    EXPECT_EQ(original.e_int8,   loaded.e_int8);
    EXPECT_EQ(original.e_int16,  loaded.e_int16);
    EXPECT_EQ(original.e_int32,  loaded.e_int32);
    EXPECT_EQ(original.e_int64,  loaded.e_int64);
    EXPECT_EQ(original.e_uint8,  loaded.e_uint8);
    EXPECT_EQ(original.e_uint16, loaded.e_uint16);
    EXPECT_EQ(original.e_uint32, loaded.e_uint32);
    EXPECT_EQ((uint64_t)original.e_uint64, (uint64_t)loaded.e_uint64);
};