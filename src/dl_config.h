#ifndef DL_CONFIG_H_INCLUDED
#define DL_CONFIG_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

/**
 * Define to your own config-file if you want to override any of the defaults that DL provides.
 * 
 * @example
 *   #define DL_USER_CONFIG "my_dl_config.h"
 * 
 * @note 
 *   It's really important that all build-targets using dl is compiled with the same DL_USER_CONFIG,
 *   at least IF overriding the hash-functions as hashes must be consistent between compile and load.
 * 
 * This allows the user to configure how DL handles:
 *
 * Asserts:
 * 
 *   DL exposes 2 macros that can be overridden for asserts, DL_ASSERT(expr) and DL_ASSERT_MSG(expr, fmt, ...)
 *   where the first one is a bog-standard assert with the same api as assert() and DL_ASSERT_MSG support
 *   assert-messages with printf-style string and arguments.
 *   As a user you can opt in to override none, one or both of these.
 *   If you implement none of them the standard assert() from assert.h will be used. If only one is implemented
 *   the other one will be implemented with the other.
 * 
 * Hash-function:
 * 
 *   DL also allows the user to override what hash-function is used internally. This might be useful if you
 *   want to use the same one throughout your code, track what hashes are generated and maybe other things.
 * 
 *   Two functions are exposed for the user to override, DL_HASH_BUFFER(buffer, byte_count) and 
 *   DL_HASH_STRING(str). You are allowed to override None, only DL_HASH_BUFFER or both.
 *   If DL_HASH_STRING is not defined it will be implemented via DL_HASH_BUFFER + strlen().
 * 
 *   Important to note here is that DL_HASH_BUFFER(str, strlen(str)) is required to be the same as 
 *   DL_HASH_STRING().
 * 
 *   The 2 macros must expand to something converting to an uint32_t in the end.
 * 
 *   If none of the macros are defined, dl will use a fairly simple default hash-function.
 */
#if defined(DL_USER_CONFIG)
#  include DL_USER_CONFIG
#endif


// Asserts

#if !defined(DL_ASSERT_MSG)
#  if defined(DL_ASSERT)
#    define DL_ASSERT_MSG(expr, fmt, ...) DL_ASSERT(expr)
#  else
#    include <assert.h>
#    define DL_ASSERT_MSG(expr, fmt, ...) assert(expr)
#  endif
#endif

#if !defined(DL_ASSERT)
#  define DL_ASSERT(expr) DL_ASSERT_MSG(expr, "")
#endif


// Hash function

static inline uint32_t dl_internal_hash_buffer( const uint8_t* buffer, size_t bytes )
{
#if defined(DL_HASH_BUFFER)
	return DL_HASH_BUFFER(buffer, bytes);
#else
    uint32_t hash = 5381;
    for (unsigned int i = 0; i < bytes; i++)
        hash = (hash * uint32_t(33)) + *((uint8_t*)buffer + i);
    return hash - 5381;
#endif
}

static inline uint32_t dl_internal_hash_string( const char* str )
{
#if defined(DL_HASH_STRING)
	return DL_HASH_STRING(str);
#else
    uint32_t hash = 5381;
    for (unsigned int i = 0; str[i] != 0; i++)
        hash = (hash * uint32_t(33)) + uint32_t(str[i]);
    return hash - 5381; // So empty string == 0
#endif
}

#endif // DL_CONFIG_H_INCLUDED
