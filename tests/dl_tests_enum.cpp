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

TYPED_TEST(DLBase, sized_enums_simple_inl_array)
{
    sized_enums_inl_array original = {
        { int8_1,   int8_neg,   int8_min,   int8_max  }, // e_int8
        { int16_1,  int16_neg,  int16_min,  int16_max }, // e_int16
        { int32_1,  int32_neg,  int32_min,  int32_max }, // e_int32
        { int64_1,  int64_neg,  int64_min,  int64_max }, // e_int64
        { uint8_1,  uint8_min,  uint8_max  }, // e_uint8
        { uint16_1, uint16_min, uint16_max }, // e_uint16
        { uint32_1, uint32_min, uint32_max }, // e_uint32
        { uint64_1, uint64_min, uint64_max } // e_uint64
    };

    sized_enums_inl_array loaded;
    this->do_the_round_about( sized_enums_inl_array::TYPE_ID, &original, &loaded, sizeof(loaded) );

    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_int8),   original.e_int8,   loaded.e_int8);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_int16),  original.e_int16,  loaded.e_int16);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_int32),  original.e_int32,  loaded.e_int32);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_int64),  original.e_int64,  loaded.e_int64);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_uint8),  original.e_uint8,  loaded.e_uint8);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_uint16), original.e_uint16, loaded.e_uint16);
    EXPECT_ARRAY_EQ(DL_ARRAY_LENGTH(original.e_uint32), original.e_uint32, loaded.e_uint32);
    EXPECT_EQ((uint64_t)original.e_uint64[0], (uint64_t)loaded.e_uint64[0]);
    EXPECT_EQ((uint64_t)original.e_uint64[1], (uint64_t)loaded.e_uint64[1]);
    EXPECT_EQ((uint64_t)original.e_uint64[2], (uint64_t)loaded.e_uint64[2]);
};

TYPED_TEST(DLBase, sized_enums_simple_array)
{
    enum_int8   i8[]  = { int8_1,   int8_neg,   int8_min,   int8_max  };
    enum_int16  i16[] = { int16_1,  int16_neg,  int16_min,  int16_max };
    enum_int32  i32[] = { int32_1,  int32_neg,  int32_min,  int32_max };
    enum_int64  i64[] = { int64_1,  int64_neg,  int64_min,  int64_max };
    enum_uint8  u8[]  = { uint8_1,  uint8_min,  uint8_max  };
    enum_uint16 u16[] = { uint16_1, uint16_min, uint16_max }; 
    enum_uint32 u32[] = { uint32_1, uint32_min, uint32_max }; 
    enum_uint64 u64[] = { uint64_1, uint64_min, uint64_max };

    sized_enums_array original = {
        { i8,  DL_ARRAY_LENGTH(i8)  }, // e_int8
        { i16, DL_ARRAY_LENGTH(i16) }, // e_int16
        { i32, DL_ARRAY_LENGTH(i32) }, // e_int32
        { i64, DL_ARRAY_LENGTH(i64) }, // e_int64
        { u8,  DL_ARRAY_LENGTH(u8)  }, // e_uint8
        { u16, DL_ARRAY_LENGTH(u16) }, // e_uint16
        { u32, DL_ARRAY_LENGTH(u32) }, // e_uint32
        { u64, DL_ARRAY_LENGTH(u64) }  // e_uint64
    };

    sized_enums_array loaded[10];
    this->do_the_round_about( sized_enums_array::TYPE_ID, &original, loaded, sizeof(loaded) );

    EXPECT_ARRAY_EQ(original.e_int8.count,   original.e_int8.data,   loaded[0].e_int8.data);
    EXPECT_ARRAY_EQ(original.e_int16.count,  original.e_int16.data,  loaded[0].e_int16.data);
    EXPECT_ARRAY_EQ(original.e_int32.count,  original.e_int32.data,  loaded[0].e_int32.data);
    EXPECT_ARRAY_EQ(original.e_int64.count,  original.e_int64.data,  loaded[0].e_int64.data);
    EXPECT_ARRAY_EQ(original.e_uint8.count,  original.e_uint8.data,  loaded[0].e_uint8.data);
    EXPECT_ARRAY_EQ(original.e_uint16.count, original.e_uint16.data, loaded[0].e_uint16.data);
    EXPECT_ARRAY_EQ(original.e_uint32.count, original.e_uint32.data, loaded[0].e_uint32.data);
    EXPECT_EQ((uint64_t)original.e_uint64[0], (uint64_t)loaded[0].e_uint64[0]);
    EXPECT_EQ((uint64_t)original.e_uint64[1], (uint64_t)loaded[0].e_uint64[1]);
    EXPECT_EQ((uint64_t)original.e_uint64[2], (uint64_t)loaded[0].e_uint64[2]);
};
