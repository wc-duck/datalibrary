/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TYPES_H_INCLUDED
#define DL_DL_TYPES_H_INCLUDED

#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <dl/dl.h>
#include "dl_hash.h"
#include "dl_alloc.h"
#include "dl_swap.h"
#include "dl_assert.h"

#include <stdarg.h> // for va_list

#define DL_ARRAY_LENGTH(Array) (sizeof(Array)/sizeof(Array[0]))

#if defined( __LP64__ ) && !defined(__APPLE__)
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

#if defined( __GNUC__ )
	#define DL_UNUSED __attribute__((unused))
#else
	#define DL_UNUSED
#endif

static const uint32_t DL_UNUSED DL_TYPELIB_VERSION         = 4; // format version for type-libraries.
static const uint32_t DL_UNUSED DL_INSTANCE_VERSION        = 1; // format version for instances.
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
	uint32_t enum_count;		// number of enums in typelibrary
	uint32_t member_count;
	uint32_t enum_value_count;
	uint32_t enum_alias_count;

	uint32_t default_value_size;
	uint32_t typeinfo_strings_size;
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
	uint32_t    name;
	dl_type_t   type;
	dl_typeid_t type_id;
	uint32_t    size[2];
	uint32_t    alignment[2];
	uint32_t    offset[2];
	uint32_t    default_value_offset; // if M_UINT32_MAX, default value is not present, otherwise offset into default-value-data.
	uint32_t    default_value_size;

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

	void SetBitFieldOffset( unsigned int bfoffset )
	{
		type = (dl_type_t)DL_INSERT_BITS( type, bfoffset, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED );
	}

	uint32_t inline_array_cnt() const
	{
		return DL_EXTRACT_BITS( type, DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT, DL_TYPE_INLINE_ARRAY_CNT_BITS_USED );
	}

	void set_inline_array_cnt( uint32_t bits )
	{
		type = (dl_type_t)DL_INSERT_BITS( type, bits, DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT, DL_TYPE_INLINE_ARRAY_CNT_BITS_USED );
	}
};

/**
 *
 */
enum dl_type_flags
{
	DL_TYPE_FLAG_HAS_SUBDATA = 1 << 0, ///< the type has subdata and need pointer patching.
	DL_TYPE_FLAG_IS_EXTERNAL = 1 << 1, ///< the type is marked as "external", this says that the type is not emitted in headers and expected to get defined by the user.

	DL_TYPE_FLAG_DEFAULT = 0,
};

struct dl_type_desc
{
	uint32_t name;
	uint32_t flags;
	uint32_t size[2];
	uint32_t alignment[2];
	uint32_t member_count;
	uint32_t member_start;
};

struct dl_enum_value_desc
{
	uint32_t main_alias;
	uint32_t value;
};

struct dl_enum_desc
{
	uint32_t name;
	uint32_t value_count;
	uint32_t value_start;
	uint32_t alias_count; /// number of aliases for this enum, always at least 1. Alias 0 is consider the "main name" of the value and need to be a valid c enum name.
	uint32_t alias_start; /// offset into alias list where aliases for this enum-value start.
};

struct dl_enum_alias_desc
{
	uint32_t name;
	uint32_t value_index; ///< index of the value this alias belong to.
};

struct dl_context
{
	dl_allocator alloc;

	dl_error_msg_handler error_msg_func;
	void*                error_msg_ctx;

	unsigned int type_count;
	unsigned int enum_count;
	unsigned int member_count;
	unsigned int enum_value_count;
	unsigned int enum_alias_count;

	size_t type_capacity;
	size_t enum_capacity;
	size_t member_capacity;
	size_t enum_value_capacity;
	size_t enum_alias_capacity;

	dl_typeid_t* type_ids; ///< list of all loaded typeid:s in the same order they appear in type_descs
	dl_typeid_t* enum_ids; ///< list of all loaded typeid:s for enums in the same order they appear in enum_descs

	dl_type_desc*       type_descs;    ///< list of all loaded descriptors for types.
	dl_member_desc*     member_descs; ///< list of all loaded descriptors for members in types.
	dl_enum_desc*       enum_descs;
	dl_enum_value_desc* enum_value_descs;
	dl_enum_alias_desc* enum_alias_descs;

	char*  typedata_strings;
	size_t typedata_strings_size;
	size_t typedata_strings_cap; // rename to capacity

	uint8_t* default_data;
	size_t   default_data_size;
};

#if defined( __GNUC__ )
inline void dl_log_error( dl_ctx_t dl_ctx, const char* fmt, ... ) __attribute__((format( printf, 2, 3 )));
#endif

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
inline unsigned int dl_bf_offset( dl_endian_t endian, unsigned int bf_size, unsigned int offset, unsigned int bits ) { return endian == DL_ENDIAN_LITTLE ? offset : ( bf_size * 8 ) - offset - bits; }

DL_FORCEINLINE dl_endian_t dl_other_endian( dl_endian_t endian ) { return endian == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE; }

static inline const dl_type_desc* dl_internal_find_type(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
	// linear search right now!
    for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
    	if( dl_ctx->type_ids[i] == type_id )
    		return &dl_ctx->type_descs[i];

    return 0x0;
}

static inline const char* dl_internal_type_name      ( dl_ctx_t ctx, const dl_type_desc*       type   ) { return &ctx->typedata_strings[type->name]; }
static inline const char* dl_internal_member_name    ( dl_ctx_t ctx, const dl_member_desc*     member ) { return &ctx->typedata_strings[member->name]; }
static inline const char* dl_internal_enum_name      ( dl_ctx_t ctx, const dl_enum_desc*       enum_  ) { return &ctx->typedata_strings[enum_->name]; }
static inline const char* dl_internal_enum_alias_name( dl_ctx_t ctx, const dl_enum_alias_desc* alias  ) { return &ctx->typedata_strings[alias->name]; }

static inline const dl_type_desc* dl_internal_find_type_by_name( dl_ctx_t dl_ctx, const char* name )
{
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
	{
		dl_type_desc* desc = &dl_ctx->type_descs[i];
		if( strcmp( name, dl_internal_type_name( dl_ctx, desc ) ) == 0 )
			return desc;
	}
	return 0x0;
}

static inline size_t dl_pod_size( dl_type_t type )
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
		default:                     return dl_pod_size( type );
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

static inline const dl_member_desc* dl_get_type_member( dl_ctx_t ctx, const dl_type_desc* type, unsigned int member_index )
{
	return &ctx->member_descs[ type->member_start + member_index ];
}

static inline const dl_enum_value_desc* dl_get_enum_value( dl_ctx_t ctx, const dl_enum_desc* e, unsigned int value_index )
{
	return ctx->enum_value_descs + e->value_start + value_index;
}

static inline const dl_enum_alias_desc* dl_get_enum_alias( dl_ctx_t ctx, const dl_enum_desc* e, unsigned int alias_index )
{
	return &ctx->enum_alias_descs[ e->alias_start + alias_index ];
}

static inline unsigned int dl_internal_find_member( dl_ctx_t ctx, const dl_type_desc* type, dl_typeid_t name_hash )
{
	for(unsigned int i = 0; i < type->member_count; ++i)
		if( dl_internal_hash_string( dl_internal_member_name( ctx, dl_get_type_member( ctx, type, i ) ) ) == name_hash )
			return i;

	return type->member_count + 1;
}

static inline bool dl_internal_find_enum_value( dl_ctx_t ctx, const dl_enum_desc* e, const char* name, size_t name_len, uint32_t* value )
{
	for( unsigned int j = 0; j < e->alias_count; ++j )
	{
		const dl_enum_alias_desc* a = dl_get_enum_alias( ctx, e, j );
		if( strncmp( dl_internal_enum_alias_name( ctx, a ), name, name_len ) == 0 )
		{
			*value = ctx->enum_value_descs[ a->value_index ].value;
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
			return dl_internal_enum_alias_name( dl_ctx, &dl_ctx->enum_alias_descs[v->main_alias] );
	}

	return "UnknownEnum!";
}

#endif // DL_DL_TYPES_H_INCLUDED
