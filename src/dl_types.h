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

struct dl_data_header
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

struct dl_member_desc
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

	void set_size( uint32_t bit32, uint32_t bit64 )
	{
		size[ DL_PTR_SIZE_32BIT ] = bit32;
		size[ DL_PTR_SIZE_64BIT ] = bit64;
	}

	void set_align( uint32_t bit32, uint32_t bit64 )
	{
		alignment[ DL_PTR_SIZE_32BIT ] = bit32;
		alignment[ DL_PTR_SIZE_64BIT ] = bit64;
	}

	void copy_size( const uint32_t* insize )
	{
		size[ DL_PTR_SIZE_32BIT ] = insize[ DL_PTR_SIZE_32BIT ];
		size[ DL_PTR_SIZE_64BIT ] = insize[ DL_PTR_SIZE_64BIT ];
	}

	void copy_align( const uint32_t* inalignment )
	{
		alignment[ DL_PTR_SIZE_32BIT ] = inalignment[ DL_PTR_SIZE_32BIT ];
		alignment[ DL_PTR_SIZE_64BIT ] = inalignment[ DL_PTR_SIZE_64BIT ];
	}

	void SetStorage( dl_type_t storage )
	{
		type = (dl_type_t)( ( (unsigned int)type & ~DL_TYPE_STORAGE_MASK ) | (unsigned int)storage );
	}

	void SetBitFieldBits( unsigned int bits )
	{
		type = (dl_type_t)DL_INSERT_BITS( type, bits, DL_TYPE_BITFIELD_SIZE_MIN_BIT, DL_TYPE_BITFIELD_SIZE_BITS_USED );
	}

	void SetBitFieldOffset( unsigned int offset )
	{
		type = (dl_type_t)DL_INSERT_BITS( type, offset, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED );
	}
};

struct dl_type_desc
{
	char      name[DL_TYPE_NAME_MAX_LEN];
	uint32_t  size[2];
	uint32_t  alignment[2];
	uint32_t  member_count;
	uint32_t  member_start;
};

struct dl_enum_value_desc
{
	char     name[DL_ENUM_VALUE_NAME_MAX_LEN];
	uint32_t value;
};

struct dl_enum_desc
{
	char     name[DL_ENUM_NAME_MAX_LEN];
	uint32_t value_count;
	uint32_t value_start;
};

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

	unsigned int type_count;
	unsigned int enum_count;
	unsigned int member_count;
	unsigned int enum_value_count;

	// TODO: dynamic alloc all type-data!
	dl_typeid_t type_ids[128];      ///< list of all loaded typeid:s in the same order they appear in type_descs
	dl_typeid_t enum_ids[128];      ///< list of all loaded typeid:s for enums in the same order they appear in enum_descs

	dl_type_desc       type_descs[128];    ///< list of all loaded descriptors for types.
	dl_member_desc     member_descs[1024]; ///< list of all loaded descriptors for members in types.
	dl_enum_desc       enum_descs[128];
	dl_enum_value_desc enum_value_descs[256];

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

static inline const dl_type_desc* dl_internal_find_type(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
	// linear search right now!
    for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
    	if( dl_ctx->type_ids[i] == type_id )
    		return &dl_ctx->type_descs[i];

    return 0x0;
}

static inline const dl_type_desc* dl_internal_find_type_by_name( dl_ctx_t dl_ctx, const char* name )
{
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
	{
		dl_type_desc* desc = &dl_ctx->type_descs[i];
		if( strcmp( name, desc->name ) == 0 )
			return desc;
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

static inline dl_typeid_t dl_internal_typeid_of( dl_ctx_t dl_ctx, const dl_type_desc* type )
{
	return dl_ctx->type_ids[ type - dl_ctx->type_descs ];
}

static inline const dl_enum_desc* dl_internal_find_enum( dl_ctx_t dl_ctx, dl_typeid_t type_id )
{
	for( unsigned int i = 0; i < dl_ctx->enum_count; ++i )
		if( dl_ctx->enum_ids[i] == type_id )
			return &dl_ctx->enum_descs[i];

	return 0x0;
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

static inline const dl_member_desc* dl_get_type_member( dl_ctx_t ctx, const dl_type_desc* type, unsigned int member_index )
{
	return &ctx->member_descs[ type->member_start + member_index ];
}

static inline const dl_enum_value_desc* dl_get_enum_value( dl_ctx_t ctx, const dl_enum_desc* e, unsigned int value_index )
{
	return ctx->enum_value_descs + e->value_start + value_index;
}

static inline unsigned int dl_internal_find_member( dl_ctx_t ctx, const dl_type_desc* type, dl_typeid_t name_hash )
{
	for(unsigned int i = 0; i < type->member_count; ++i)
		if(dl_internal_hash_string( dl_get_type_member( ctx, type, i )->name ) == name_hash)
			return i;

	return type->member_count + 1;
}

static inline bool dl_internal_find_enum_value( dl_ctx_t ctx, const dl_enum_desc* e, const char* name, size_t name_len, uint32_t* value )
{
	for( unsigned int j = 0; j < e->value_count; ++j )
	{
		const dl_enum_value_desc* v = dl_get_enum_value( ctx, e, j );
		if( strncmp( v->name, name, name_len ) == 0 )
		{
			*value = v->value;
			return true;
		}
	}
	return false;
}

static inline const char* dl_internal_find_enum_name( dl_ctx_t dl_ctx, dl_typeid_t type_id, uint32_t value )
{
	const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, type_id );

	if( e == 0x0 )
		return "UnknownEnum!";

	for( unsigned int j = 0; j < e->value_count; ++j )
	{
		const dl_enum_value_desc* v = dl_get_enum_value( dl_ctx, e, j );
		if( v->value == value )
			return v->name;
	}

	return "UnknownEnum!";
}

#endif // DL_DL_TYPES_H_INCLUDED
