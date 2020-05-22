#ifndef DL_ASSERT_H_INCLUDED
#define DL_ASSERT_H_INCLUDED

// TODO: replace this with a real assert, that can be comipled out!

#include <stdio.h>  // for vsnprintf

#define DL_ASSERT( expr, ... ) do { if(!(expr)) printf("ASSERT FAIL! %s %s %u\n", #expr, __FILE__, __LINE__); } while( false ) // TODO: implement me plox!

// ... clang ...
#if defined( __clang__ )
#  if defined( __cplusplus ) && __has_feature(cxx_static_assert)
#    define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )
#  elif __has_feature(c_static_assert)
#    define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )
#  endif

// ... msvc ...
#elif defined( _MSC_VER ) && ( defined(_MSC_VER) && (_MSC_VER >= 1600) )
#    define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )

// ... gcc ...
#elif defined( __cplusplus )
#  if __cplusplus >= 201103L || ( defined(_MSC_VER) && (_MSC_VER >= 1600) )
#    define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )
#  endif
#elif defined( __STDC__ )
#  if defined( __STDC_VERSION__ )
#    if __STDC_VERSION__ >= 201112L
#      define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )
#    else
#      define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )
#    endif
#  endif
#endif

#if !defined(DL_STATIC_ASSERT)
    #define DL_STATIC_ASSERT(cond, msg)
#endif

#endif // DL_ASSERT_H_INCLUDED
