/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_INTERNAL_UTIL_H_INCLUDED
#define DL_DL_INTERNAL_UTIL_H_INCLUDED

#include "dl_alloc.h"
#include <stdio.h> // vsnprintf
#include <stdarg.h> // va_start, va_end

template <typename T>
static T* dl_grow_array( dl_allocator* alloc, T* ptr, size_t* cap, size_t min_inc )
{
	size_t old_cap = *cap;
	size_t new_cap = ( ( old_cap < min_inc ) ? old_cap + min_inc : old_cap ) * 2;
	if( new_cap == 0 )
		new_cap = 8;
	*cap = new_cap;
	return (T*)dl_realloc( alloc, ptr, new_cap * sizeof( T ), old_cap * sizeof( T ) );
}

#if defined( __GNUC__ )
static inline int dl_internal_str_format(char* buf, size_t buf_size, const char* fmt, ...) __attribute__((format( printf, 3, 4 )));
#endif

static inline int dl_internal_str_format(char* buf, size_t buf_size, const char* fmt, ...)
{
	va_list args;
	va_start( args, fmt );
	int res = vsnprintf( buf, buf_size, fmt, args );
	buf[buf_size - 1] = '\0';
	va_end( args );
	return res;
}

#endif // DL_DL_UTIL_H_INCLUDED

