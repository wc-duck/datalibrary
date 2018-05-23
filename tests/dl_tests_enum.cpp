#include <gtest/gtest.h>

#include "dl_test_common.h"

static_assert(sizeof(enum_int8)   == sizeof(int8_t),   "size of enum is incorrect");
static_assert(sizeof(enum_int16)  == sizeof(int16_t),  "size of enum is incorrect");
static_assert(sizeof(enum_int32)  == sizeof(int32_t),  "size of enum is incorrect");
static_assert(sizeof(enum_int64)  == sizeof(int64_t),  "size of enum is incorrect");
static_assert(sizeof(enum_uint8)  == sizeof(uint8_t),  "size of enum is incorrect");
static_assert(sizeof(enum_uint16) == sizeof(uint16_t), "size of enum is incorrect");
static_assert(sizeof(enum_uint32) == sizeof(uint32_t), "size of enum is incorrect");
static_assert(sizeof(enum_uint64) == sizeof(uint64_t), "size of enum is incorrect");

static_assert(DL_ALIGNOF(enum_int8)   == DL_ALIGNOF(int8_t),   "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int16)  == DL_ALIGNOF(int16_t),  "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int32)  == DL_ALIGNOF(int32_t),  "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_int64)  == DL_ALIGNOF(int64_t),  "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint8)  == DL_ALIGNOF(uint8_t),  "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint16) == DL_ALIGNOF(uint16_t), "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint32) == DL_ALIGNOF(uint32_t), "size of enum is incorrect");
static_assert(DL_ALIGNOF(enum_uint64) == DL_ALIGNOF(uint64_t), "size of enum is incorrect");
