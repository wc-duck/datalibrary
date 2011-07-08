/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TYPES_H_INCLUDED
#define DL_DL_TYPES_H_INCLUDED

#include <dl/dl.h>

#include <string.h> // for memcpy
#include <stdio.h>  // for vsnprintf
#include <stdarg.h> // for va_list

// remove me!
#if defined(_MSC_VER)

        typedef signed   __int8  int8;
        typedef signed   __int16 int16;
        typedef signed   __int32 int32;
        typedef signed   __int64 int64;
        typedef unsigned __int8  uint8;
        typedef unsigned __int16 uint16;
        typedef unsigned __int32 uint32;
        typedef unsigned __int32 uint32_t;
        typedef unsigned __int64 uint64;

#elif defined(__GNUC__)

        #include <stdint.h>
        typedef int8_t   int8;
        typedef int16_t  int16;
        typedef int32_t  int32;
        typedef int64_t  int64;
        typedef uint8_t  uint8;
        typedef uint16_t uint16;
        typedef uint32_t uint32;
        typedef uint64_t uint64;

#endif

#define DL_UNUSED (void)
#define DL_ARRAY_LENGTH(Array) (sizeof(Array)/sizeof(Array[0]))
#define DL_ASSERT( expr, ... ) do { if(!(expr)) printf("ASSERT FAIL! %s %s %u\n", #expr, __FILE__, __LINE__); } while( false ) // TODO: implement me plox!

#define DL_JOIN_TOKENS(a,b) DL_JOIN_TOKENS_DO_JOIN(a,b)
#define DL_JOIN_TOKENS_DO_JOIN(a,b) DL_JOIN_TOKENS_DO_JOIN2(a,b)
#define DL_JOIN_TOKENS_DO_JOIN2(a,b) a##b

namespace dl_staticassert
{
	template <bool x> struct STATIC_ASSERTION_FAILURE;
	template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
};
#define DL_STATIC_ASSERT(_Expr, _Msg) enum { DL_JOIN_TOKENS(_static_assert_enum_##_Msg, __LINE__) = sizeof(::dl_staticassert::STATIC_ASSERTION_FAILURE< (bool)( _Expr ) >) }

static const int8  DL_INT8_MAX  = 0x7F;
static const int16 DL_INT16_MAX = 0x7FFF;
static const int32 DL_INT32_MAX = 0x7FFFFFFFL;
static const int64 DL_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const int8  DL_INT8_MIN  = (-DL_INT8_MAX  - 1);
static const int16 DL_INT16_MIN = (-DL_INT16_MAX - 1);
static const int32 DL_INT32_MIN = (-DL_INT32_MAX - 1);
static const int64 DL_INT64_MIN = (-DL_INT64_MAX - 1);

static const uint8  DL_UINT8_MAX  = 0xFFU;
static const uint16 DL_UINT16_MAX = 0xFFFFU;
static const uint32 DL_UINT32_MAX = 0xFFFFFFFFUL;
static const uint64 DL_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
static const uint8  DL_UINT8_MIN  = 0x00U;
static const uint16 DL_UINT16_MIN = 0x0000U;
static const uint32 DL_UINT32_MIN = 0x00000000UL;
static const uint64 DL_UINT64_MIN = 0x0000000000000000ULL;

#if defined( __LP64__ ) || defined( _WIN64 )
        #define DL_INT64_FMT_STR  "%ld"
		#define DL_UINT64_FMT_STR "%lu"
		#define DL_PINT_FMT_STR   "%lu"
        typedef uint64 pint;
#else
        #define DL_INT64_FMT_STR  "%lld"
        #define DL_UINT64_FMT_STR "%llu"
		#define DL_PINT_FMT_STR   "%u"
        typedef uint32 pint;
#endif // defined( __LP64__ ) || defined( _WIN64 )

template<class T>
struct TAlignmentOf
{
        struct CAlign { ~CAlign() {}; unsigned char m_Dummy; T m_T; };
        enum { ALIGNOF = sizeof(CAlign) - sizeof(T) };
};
#define DL_ALIGNMENTOF(Type) TAlignmentOf<Type>::ALIGNOF

enum
{
	DL_MEMBER_NAME_MAX_LEN     = 32,
	DL_TYPE_NAME_MAX_LEN       = 32,
	DL_ENUM_NAME_MAX_LEN       = 32,
	DL_ENUM_VALUE_NAME_MAX_LEN = 32,
};

#include "dl_swap.h"

static const uint32 DL_TYPELIB_VERSION    = 2; // format version for type-libraries.
static const uint32 DL_INSTANCE_VERSION   = 1; // format version for instances.
static const uint32 DL_INSTANCE_VERSION_SWAPED = dl_swap_endian_uint32( DL_INSTANCE_VERSION );
static const uint32 DL_TYPELIB_ID              = ('D'<< 24) | ('L' << 16) | ('T' << 8) | 'L';
static const uint32 DL_TYPELIB_ID_SWAPED       = dl_swap_endian_uint32( DL_TYPELIB_ID );
static const uint32 DL_INSTANCE_ID             = ('D'<< 24) | ('L' << 16) | ('D' << 8) | 'L';
static const uint32 DL_INSTANCE_ID_SWAPED      = dl_swap_endian_uint32( DL_INSTANCE_ID );

static const pint DL_NULL_PTR_OFFSET[2] =
{
	pint(DL_UINT32_MAX), // DL_PTR_SIZE_32BIT
	pint(-1)            // DL_PTR_SIZE_64BIT
};

struct SDLTypeLibraryHeader
{
	uint32 id;
	uint32 version;

	uint32 type_count;		// number of types in typelibrary
	uint32 types_size;		// number of bytes that are types

	uint32 enum_count;		// number of enums in typelibrary
	uint32 enums_size;		// number of bytes that are enums

	uint32 default_value_size;
};

struct SDLDataHeader
{
	uint32      id;
	uint32      version;
	dl_typeid_t root_instance_type;
	uint32      instance_size;
	uint8       is_64_bit_ptr; // currently uses uint8 instead of bitfield to be compiler-compliant.
	uint8       pad[3];
};

enum dl_ptr_size_t
{
	DL_PTR_SIZE_32BIT = 0,
	DL_PTR_SIZE_64BIT = 1,

	DL_PTR_SIZE_HOST = sizeof(void*) == 4 ? DL_PTR_SIZE_32BIT : DL_PTR_SIZE_64BIT
};

struct SDLMember
{
	char        name[DL_MEMBER_NAME_MAX_LEN];
	dl_type_t   type;
	dl_typeid_t type_id;
	uint32      size[2];
	uint32      alignment[2];
	uint32      offset[2];
	uint32      default_value_offset; // if M_UINT32_MAX, default value is not present, otherwise offset into default-value-data.

	dl_type_t AtomType()       const { return dl_type_t( type & DL_TYPE_ATOM_MASK); }
	dl_type_t StorageType()    const { return dl_type_t( type & DL_TYPE_STORAGE_MASK); }
	uint32    BitFieldBits()   const { return DL_EXTRACT_BITS(type, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED); }
	uint32    BitFieldOffset() const { return DL_EXTRACT_BITS(type, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED); }
	bool      IsSimplePod()    const { return StorageType() >= DL_TYPE_STORAGE_INT8 && StorageType() <= DL_TYPE_STORAGE_FP64; }
};

#if defined( _MSC_VER )
#pragma warning(push)
#pragma warning(disable:4200) // disable warning for 0-size array
#endif // defined( _MSC_VER )

struct SDLType
{
	char      name[DL_TYPE_NAME_MAX_LEN];
	uint32    size[2];
	uint32    alignment[2];
	uint32    member_count;
	SDLMember members[0];
};

struct SDLEnum
{
	char        name[DL_ENUM_NAME_MAX_LEN];
	dl_typeid_t type_id;
	uint32      value_count;
	struct
	{
		char    name[DL_ENUM_VALUE_NAME_MAX_LEN];
		uint32  value;
	} values[0];
};

#if defined( _MSC_VER )
#pragma warning(pop)
#endif // defined( _MSC_VER )

typedef struct
{
	dl_typeid_t type_id;
	uint32      offset;
} dl_type_lookup_t;

struct dl_context
{
	void* (*alloc_func)( unsigned int size, unsigned int alignment, void* alloc_ctx );
	void  (*free_func) ( void* ptr, void* alloc_ctx );
	void* alloc_ctx;

	dl_error_msg_handler error_msg_func;
	void*                error_msg_ctx;

	dl_type_lookup_t type_lookup[128]; // dynamic alloc?
	dl_type_lookup_t enum_lookup[128]; // dynamic alloc?

	unsigned int type_count;
	uint8*       type_info_data;
	unsigned int type_info_data_size;

	unsigned int enum_count;
	uint8*       enum_info_data;
	unsigned int enum_info_data_size;

	uint8* default_data;
};

inline void dl_log_error( dl_ctx_t dl_ctx, const char* fmt, ... )
{
	if( dl_ctx->error_msg_func == 0x0 )
		return;

	char buffer[256];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buffer, DL_ARRAY_LENGTH(buffer), fmt, args );
	va_end(args);

	buffer[DL_ARRAY_LENGTH(buffer) - 1] = '\0';

	dl_ctx->error_msg_func( buffer, dl_ctx->error_msg_ctx );
}

template<typename T>
DL_FORCEINLINE T    dl_internal_align_up( const T value,   pint alignment ) { return T( (pint(value) + alignment - 1) & ~(alignment - 1) ); }
DL_FORCEINLINE bool dl_internal_is_align( const void* ptr, pint alignment ) { return ((pint)ptr & (alignment - 1)) == 0; }

/*
	return a bitfield offset on a particular platform (currently endian-ness is used to set them apart, that might break ;) )
	the bitfield offset are counted from least significant bit or most significant bit on different platforms.
*/
inline unsigned int DLBitFieldOffset(dl_endian_t _Endian, unsigned int _BFSize, unsigned int _Offset, unsigned int _nBits) { return _Endian == DL_ENDIAN_LITTLE ? _Offset : (_BFSize * 8) - _Offset - _nBits; }
inline unsigned int DLBitFieldOffset(unsigned int _BFSize, unsigned int _Offset, unsigned int _nBits)                      { return DLBitFieldOffset(DL_ENDIAN_HOST, _BFSize, _Offset, _nBits); }

DL_FORCEINLINE dl_endian_t DLOtherEndian(dl_endian_t _Endian) { return _Endian == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE; }

static inline pint DLPodSize(dl_type_t _Type)
{
	switch(_Type & DL_TYPE_STORAGE_MASK)
	{
		case DL_TYPE_STORAGE_INT8:  
		case DL_TYPE_STORAGE_UINT8:  return 1;

		case DL_TYPE_STORAGE_INT16: 
		case DL_TYPE_STORAGE_UINT16: return 2;

		case DL_TYPE_STORAGE_INT32: 
		case DL_TYPE_STORAGE_UINT32: 
		case DL_TYPE_STORAGE_FP32: 
		case DL_TYPE_STORAGE_ENUM:   return 4;

		case DL_TYPE_STORAGE_INT64: 
		case DL_TYPE_STORAGE_UINT64: 
		case DL_TYPE_STORAGE_FP64:   return 8;

		default:
			DL_ASSERT(false && "This should not happen!");
			return 0;
	}
}

static inline const SDLType* dl_internal_find_type(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
    DL_ASSERT( dl_internal_is_align( dl_ctx->type_lookup, DL_ALIGNMENTOF(dl_type_lookup_t) ) );
	// linear search right now!
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
		if(dl_ctx->type_lookup[i].type_id == type_id)
		{
			union
			{
				const uint8*   data_ptr;
				const SDLType* type_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->type_info_data + dl_ctx->type_lookup[i].offset;
			return ptr_conv.type_ptr;
		}

	return 0x0;
}

static inline const dl_typeid_t dl_internal_typeid_of( dl_ctx_t dl_ctx, const SDLType* type )
{
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
		if( (void*)(dl_ctx->type_info_data + dl_ctx->type_lookup[i].offset) == (void*)type )
			return dl_ctx->type_lookup[i].type_id;

	DL_ASSERT( false && "Could not find type!?!" );
	return 0;
}

static inline const SDLEnum* dl_internal_find_enum(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
	for (unsigned int i = 0; i < dl_ctx->enum_count; ++i)
		if( dl_ctx->enum_lookup[i].type_id == type_id )
		{
			union
			{
				const uint8*   data_ptr;
				const SDLEnum* enum_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->enum_info_data + dl_ctx->enum_lookup[i].offset;
			return ptr_conv.enum_ptr;
		}

	return 0x0;
}

static inline bool dl_internal_find_enum_value( const SDLEnum* e, const char* name, unsigned int name_len, uint32* value )
{
	for( unsigned int j = 0; j < e->value_count; ++j )
		if( strncmp( e->values[j].name, name, name_len ) == 0 )
		{
			*value = e->values[j].value;
			return true;
		}
	return false;
}

static inline const char* dl_internal_find_enum_name( dl_ctx_t dl_ctx, dl_typeid_t type_id, uint32 value )
{
	const SDLEnum* e = dl_internal_find_enum( dl_ctx, type_id );

	if( e == 0x0 )
		return "UnknownEnum!";

	for( unsigned int j = 0; j < e->value_count; ++j )
		if(e->values[j].value == value)
			return e->values[j].name;

	return "UnknownEnum!";
}

DL_FORCEINLINE static uint32_t dl_internal_hash_buffer( const uint8* buffer, unsigned int bytes, uint32_t base_hash )
{
	uint32 hash = base_hash + 5381;
	for (unsigned int i = 0; i < bytes; i++)
		hash = (hash * uint32(33)) + *((uint8*)buffer + i);
	return hash - 5381;
}

DL_FORCEINLINE static uint32_t dl_internal_hash_string( const char* str, uint32_t base_hash = 0 )
{
	uint32 hash = base_hash + 5381;
	for (unsigned int i = 0; str[i] != 0; i++)
		hash = (hash * uint32(33)) + str[i];
	return hash - 5381; // So empty string == 0
}

static inline unsigned int dl_internal_find_member( const SDLType* type, dl_typeid_t name_hash )
{
	// TODO: currently members only hold name, but they should hold a hash!

	for(unsigned int i = 0; i < type->member_count; ++i)
		if(dl_internal_hash_string(type->members[i].name) == name_hash)
			return i;

	return type->member_count + 1;
}

#endif // DL_DL_TYPES_H_INCLUDED
