/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TYPES_H_INCLUDED
#define DL_DL_TYPES_H_INCLUDED

#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <dl/dl.h>

#include <string.h> // for memcpy
#include <stdio.h>  // for vsnprintf
#include <stdarg.h> // for va_list

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

#if defined( __LP64__ )
	#define DL_INT64_FMT_STR  "%ld"
	#define DL_UINT64_FMT_STR "%lu"
	#define DL_PINT_FMT_STR   "%lu"
#elif defined( _WIN64 )
	#define DL_INT64_FMT_STR  "%lld"
	#define DL_UINT64_FMT_STR "%llu"
	#define DL_PINT_FMT_STR   "%llu"
#else
	#define DL_INT64_FMT_STR  "%lld"
	#define DL_UINT64_FMT_STR "%llu"
	#define DL_PINT_FMT_STR   "%u"
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

#if defined( __GNUC__ )
	#define DL_UNUSED __attribute__((unused))
#else
	#define DL_UNUSED
#endif

static const uint32_t DL_UNUSED DL_TYPELIB_VERSION    = 3; // format version for type-libraries.
static const uint32_t DL_UNUSED DL_INSTANCE_VERSION   = 1; // format version for instances.
static const uint32_t DL_UNUSED DL_INSTANCE_VERSION_SWAPED = dl_swap_endian_uint32( DL_INSTANCE_VERSION );
static const uint32_t DL_UNUSED DL_TYPELIB_ID              = ('D'<< 24) | ('L' << 16) | ('T' << 8) | 'L';
static const uint32_t DL_UNUSED DL_TYPELIB_ID_SWAPED       = dl_swap_endian_uint32( DL_TYPELIB_ID );
static const uint32_t DL_UNUSED DL_INSTANCE_ID             = ('D'<< 24) | ('L' << 16) | ('D' << 8) | 'L';
static const uint32_t DL_UNUSED DL_INSTANCE_ID_SWAPED      = dl_swap_endian_uint32( DL_INSTANCE_ID );

#undef DL_UNUSED

static const uintptr_t DL_NULL_PTR_OFFSET[2] =
{
	(uintptr_t)0xFFFFFFFF, // DL_PTR_SIZE_32BIT
	(uintptr_t)-1          // DL_PTR_SIZE_64BIT
};

struct dl_typelib_header
{
	uint32_t id;
	uint32_t version;

	uint32_t type_count;		// number of types in typelibrary
	uint32_t types_size;		// number of bytes that are types

	uint32_t enum_count;		// number of enums in typelibrary
	uint32_t enums_size;		// number of bytes that are enums

	uint32_t default_value_size;
};

struct SDLDataHeader
{
	uint32_t    id;
	uint32_t    version;
	dl_typeid_t root_instance_type;
	uint32_t    instance_size;
	uint8_t     is_64_bit_ptr; // currently uses uint8 instead of bitfield to be compiler-compliant.
	uint8_t     pad[3];
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
	uint32_t    size[2];
	uint32_t    alignment[2];
	uint32_t    offset[2];
	uint32_t    default_value_offset; // if M_UINT32_MAX, default value is not present, otherwise offset into default-value-data.

	dl_type_t AtomType()       const { return dl_type_t( type & DL_TYPE_ATOM_MASK); }
	dl_type_t StorageType()    const { return dl_type_t( type & DL_TYPE_STORAGE_MASK); }
	uint32_t  BitFieldBits()   const { return DL_EXTRACT_BITS(type, DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_BITS_USED); }
	uint32_t  BitFieldOffset() const { return DL_EXTRACT_BITS(type, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED); }
	bool      IsSimplePod()    const { return StorageType() >= DL_TYPE_STORAGE_INT8 && StorageType() <= DL_TYPE_STORAGE_FP64; }
};

#if defined( _MSC_VER )
#pragma warning(push)
#pragma warning(disable:4200) // disable warning for 0-size array
#endif // defined( _MSC_VER )

struct SDLType
{
	char      name[DL_TYPE_NAME_MAX_LEN];
	uint32_t  size[2];
	uint32_t  alignment[2];
	uint32_t  member_count;
	SDLMember members[0];
};

struct SDLEnum
{
	char        name[DL_ENUM_NAME_MAX_LEN];
	uint32_t    value_count;
	struct
	{
		char     name[DL_ENUM_VALUE_NAME_MAX_LEN];
		uint32_t value;
	} values[0];
};

#if defined( _MSC_VER )
#pragma warning(pop)
#endif // defined( _MSC_VER )

typedef struct
{
	dl_typeid_t type_id;
	uint32_t    offset;
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
	uint8_t*     type_info_data;
	unsigned int type_info_data_size;

	unsigned int enum_count;
	uint8_t*     enum_info_data;
	unsigned int enum_info_data_size;

	uint8_t* default_data;
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
DL_FORCEINLINE T    dl_internal_align_up( const T value,   size_t alignment ) { return T( ((size_t)value + alignment - 1) & ~(alignment - 1) ); }
DL_FORCEINLINE bool dl_internal_is_align( const void* ptr, size_t alignment ) { return ((size_t)ptr & (alignment - 1)) == 0; }

/*
	return a bitfield offset on a particular platform (currently endian-ness is used to set them apart, that might break ;) )
	the bitfield offset are counted from least significant bit or most significant bit on different platforms.
*/
inline unsigned int dl_bf_offset( dl_endian_t endian, unsigned int bf_size, unsigned int offset, unsigned int bits ) { return endian == DL_ENDIAN_LITTLE ? offset : ( bf_size * 8) - offset - bits; }

DL_FORCEINLINE dl_endian_t dl_other_endian( dl_endian_t endian ) { return endian == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE; }

static inline const SDLType* dl_internal_find_type(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
    DL_ASSERT( dl_internal_is_align( dl_ctx->type_lookup, DL_ALIGNMENTOF(dl_type_lookup_t) ) );
	// linear search right now!
    for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
		if(dl_ctx->type_lookup[i].type_id == type_id)
		{
			union
			{
				const uint8_t* data_ptr;
				const SDLType* type_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->type_info_data + dl_ctx->type_lookup[i].offset;
			return ptr_conv.type_ptr;
		}

    return 0x0;
}

static inline const SDLType* dl_internal_find_type_by_name( dl_ctx_t dl_ctx, const char* name )
{
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
	{
		union
		{
			const uint8_t* data_ptr;
			const SDLType* type_ptr;
		} ptr_conv;
		ptr_conv.data_ptr = dl_ctx->type_info_data + dl_ctx->type_lookup[i].offset;

		if( strcmp( name, ptr_conv.type_ptr->name ) == 0 )
			return ptr_conv.type_ptr;
	}
	return 0x0;
}

static inline size_t DLPodSize( dl_type_t type )
{
	switch( type & DL_TYPE_STORAGE_MASK )
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

static inline size_t dl_internal_align_of_type( dl_ctx_t ctx, dl_type_t type, dl_typeid_t type_id, dl_ptr_size_t ptr_size )
{
	switch( type & DL_TYPE_STORAGE_MASK )
	{
		case DL_TYPE_STORAGE_STRUCT: return dl_internal_find_type( ctx, type_id )->alignment[ptr_size];
		case DL_TYPE_STORAGE_STR:
		case DL_TYPE_STORAGE_PTR:    return sizeof(void*);
		default:                     return DLPodSize( type );
	}
}

static inline dl_typeid_t dl_internal_typeid_of( dl_ctx_t dl_ctx, const SDLType* type )
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
				const uint8_t* data_ptr;
				const SDLEnum* enum_ptr;
			} ptr_conv;
			ptr_conv.data_ptr = dl_ctx->enum_info_data + dl_ctx->enum_lookup[i].offset;
			return ptr_conv.enum_ptr;
		}

	return 0x0;
}

static inline bool dl_internal_find_enum_value( const SDLEnum* e, const char* name, size_t name_len, uint32_t* value )
{
	for( unsigned int j = 0; j < e->value_count; ++j )
		if( strncmp( e->values[j].name, name, name_len ) == 0 )
		{
			*value = e->values[j].value;
			return true;
		}
	return false;
}

static inline const char* dl_internal_find_enum_name( dl_ctx_t dl_ctx, dl_typeid_t type_id, uint32_t value )
{
	const SDLEnum* e = dl_internal_find_enum( dl_ctx, type_id );

	if( e == 0x0 )
		return "UnknownEnum!";

	for( unsigned int j = 0; j < e->value_count; ++j )
		if(e->values[j].value == value)
			return e->values[j].name;

	return "UnknownEnum!";
}

DL_FORCEINLINE static uint32_t dl_internal_hash_buffer( const uint8_t* buffer, size_t bytes, uint32_t base_hash )
{
	uint32_t hash = base_hash + 5381;
	for (unsigned int i = 0; i < bytes; i++)
		hash = (hash * uint32_t(33)) + *((uint8_t*)buffer + i);
	return hash - 5381;
}

DL_FORCEINLINE static uint32_t dl_internal_hash_string( const char* str, uint32_t base_hash = 0 )
{
	uint32_t hash = base_hash + 5381;
	for (unsigned int i = 0; str[i] != 0; i++)
		hash = (hash * uint32_t(33)) + str[i];
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
