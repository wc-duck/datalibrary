/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_TYPES_H_INCLUDED
#define DL_DL_TYPES_H_INCLUDED

#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdint.h>

#include <dl/dl.h>
#include "dl_config.h"
#include "dl_alloc.h"
#include "dl_swap.h"

#include <stdarg.h> // for va_list
#include <new> // for inplace new

#define DL_BITMASK(_Bits)                   ( (1ULL << (_Bits)) - 1ULL )
#define DL_BITRANGE(_MinBit,_MaxBit)		( ((1ULL << (_MaxBit)) | ((1ULL << (_MaxBit))-1ULL)) ^ ((1ULL << (_MinBit))-1ULL) )

#define DL_ZERO_BITS(_Target, _Start, _Bits)         ( (_Target) & ~DL_BITRANGE(_Start, (_Start) + (_Bits) - 1ULL) )
#define DL_EXTRACT_BITS(_Val, _Start, _Bits)         ( (_Val >> (_Start)) & DL_BITMASK(_Bits) )
#define DL_INSERT_BITS(_Target, _Val, _Start, _Bits) ( DL_ZERO_BITS(_Target, _Start, _Bits) | ( (DL_BITMASK(_Bits) & (_Val) ) << (_Start)) )

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

static const uint32_t DL_UNUSED DL_TYPELIB_VERSION         = 5; // format version for type-libraries.
static const uint32_t DL_UNUSED DL_INSTANCE_VERSION        = 2; // format version for instances.
static const uint32_t DL_UNUSED DL_INSTANCE_VERSION_SWAPED = dl_swap_endian_uint32( DL_INSTANCE_VERSION );
static const uint32_t DL_UNUSED DL_TYPELIB_ID              = ('D'<< 24) | ('L' << 16) | ('T' << 8) | 'L';
static const uint32_t DL_UNUSED DL_TYPELIB_ID_SWAPED       = dl_swap_endian_uint32( DL_TYPELIB_ID );
static const uint32_t DL_UNUSED DL_INSTANCE_ID             = ('D'<< 24) | ('L' << 16) | ('D' << 8) | 'L';
static const uint32_t DL_UNUSED DL_INSTANCE_ID_SWAPED      = dl_swap_endian_uint32( DL_INSTANCE_ID );

#undef DL_UNUSED

typedef enum
{
	// Type-layout
	DL_TYPE_ATOM_MIN_BIT             = 0,
	DL_TYPE_ATOM_MAX_BIT             = 7,
	DL_TYPE_STORAGE_MIN_BIT          = 8,
	DL_TYPE_STORAGE_MAX_BIT          = 15,
	DL_TYPE_BITFIELD_SIZE_MIN_BIT    = 16,
	DL_TYPE_BITFIELD_SIZE_MAX_BIT    = 23,
	DL_TYPE_BITFIELD_OFFSET_MIN_BIT  = 24,
	DL_TYPE_BITFIELD_OFFSET_MAX_BIT  = 31,
	DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT = 16,
	DL_TYPE_INLINE_ARRAY_CNT_MAX_BIT = 31,

	// Masks
	DL_TYPE_ATOM_MASK             = DL_BITRANGE(DL_TYPE_ATOM_MIN_BIT,             DL_TYPE_ATOM_MAX_BIT),
	DL_TYPE_STORAGE_MASK          = DL_BITRANGE(DL_TYPE_STORAGE_MIN_BIT,          DL_TYPE_STORAGE_MAX_BIT),
	DL_TYPE_BITFIELD_SIZE_MASK    = DL_BITRANGE(DL_TYPE_BITFIELD_SIZE_MIN_BIT,    DL_TYPE_BITFIELD_SIZE_MAX_BIT),
	DL_TYPE_BITFIELD_OFFSET_MASK  = DL_BITRANGE(DL_TYPE_BITFIELD_OFFSET_MIN_BIT,  DL_TYPE_BITFIELD_OFFSET_MAX_BIT),
	DL_TYPE_INLINE_ARRAY_CNT_MASK = DL_BITRANGE(DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT, DL_TYPE_INLINE_ARRAY_CNT_MAX_BIT),

	DL_TYPE_FORCE_32_BIT = 0x7FFFFFFF
} dl_type_t;

// This check is not REALLY true, you can use static_assert on some compilers before this, but it is early enough
// for me to not have to write a big chunk of macro just to use static_assert ONCE!
#if __cplusplus >= 201103L
	static_assert(DL_INLINE_ARRAY_LENGTH_MAX == (1 << (DL_TYPE_INLINE_ARRAY_CNT_MAX_BIT - DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT + 1))-1, "bad value of DL_INLINE_ARRAY_LENGTH_MAX");
#endif

static const uintptr_t DL_NULL_PTR_OFFSET[2] =
{
	(uintptr_t)0, // DL_PTR_SIZE_32BIT
	(uintptr_t)0  // DL_PTR_SIZE_64BIT
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
	uint32_t c_includes_size;
	uint32_t metadatas_count;
};

struct dl_data_header
{
	uint32_t    id;
	uint32_t    version;
	dl_typeid_t root_instance_type;
	uint32_t    instance_size;
	uint8_t     is_64_bit_ptr; // currently uses uint8 instead of bitfield to be compiler-compliant.
	uint8_t     not_using_ptr_chain_patching; // currently uses uint8 instead of bitfield to be compiler-compliant.
	uint8_t     pad[2];
	uint32_t    first_pointer_to_patch;
};

enum dl_ptr_size_t
{
	DL_PTR_SIZE_32BIT = 0,
	DL_PTR_SIZE_64BIT = 1,

	DL_PTR_SIZE_HOST = sizeof(void*) == 4 ? DL_PTR_SIZE_32BIT : DL_PTR_SIZE_64BIT
};

/**
 *
 */
enum dl_member_flags
{
	DL_MEMBER_FLAG_IS_CONST                    = 1 << 0, ///< 
	DL_MEMBER_FLAG_VERIFY_EXTERNAL_SIZE_OFFSET = 1 << 1, ///< 

	DL_MEMBER_FLAG_DEFAULT = 0,
};

struct dl_member_desc
{
	uint32_t    name;
	uint32_t	comment;
	dl_type_t   type;
	dl_typeid_t type_id;
	uint32_t    size[2];
	uint32_t    alignment[2];
	uint32_t    offset[2];
	uint32_t    default_value_offset; // if M_UINT32_MAX, default value is not present, otherwise offset into default-value-data.
	uint32_t    default_value_size;
	uint32_t    flags;
	uint32_t    metadata_count;
	uint32_t    metadata_start;

	dl_type_atom_t    AtomType()        const { return dl_type_atom_t( (type & DL_TYPE_ATOM_MASK) >> DL_TYPE_ATOM_MIN_BIT); }
	dl_type_storage_t StorageType()     const { return dl_type_storage_t( (type & DL_TYPE_STORAGE_MASK) >> DL_TYPE_STORAGE_MIN_BIT); }
	uint16_t          bitfield_bits()   const { return uint16_t(( uint32_t(type) & DL_TYPE_BITFIELD_SIZE_MASK ) >> DL_TYPE_BITFIELD_SIZE_MIN_BIT); }
	uint16_t          bitfield_offset() const { return uint16_t(( uint32_t(type) & DL_TYPE_BITFIELD_OFFSET_MASK ) >> DL_TYPE_BITFIELD_OFFSET_MIN_BIT); }
	bool              IsSimplePod()     const
	{
		return StorageType() != DL_TYPE_STORAGE_STR &&
	           StorageType() != DL_TYPE_STORAGE_PTR &&
	           StorageType() != DL_TYPE_STORAGE_STRUCT;
	}

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

	void set_offset( uint32_t bit32, uint32_t bit64 )
	{
		offset[ DL_PTR_SIZE_32BIT ] = bit32;
		offset[ DL_PTR_SIZE_64BIT ] = bit64;
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

	void set_storage( dl_type_storage_t storage )
	{
		type = (dl_type_t)( ( (unsigned int)type & ~DL_TYPE_STORAGE_MASK ) | ((unsigned int)storage << DL_TYPE_STORAGE_MIN_BIT) );
	}

	void set_bitfield_bits( unsigned int bits )
	{
		type = (dl_type_t)( ( (unsigned int)type & ~DL_TYPE_BITFIELD_SIZE_MASK ) | (bits << DL_TYPE_BITFIELD_SIZE_MIN_BIT) );
	}

	void set_bitfield_offset( unsigned int bfoffset )
	{
		type = (dl_type_t)( ( (unsigned int)type & ~DL_TYPE_BITFIELD_OFFSET_MASK ) | (bfoffset << DL_TYPE_BITFIELD_OFFSET_MIN_BIT) );
	}

	uint32_t inline_array_cnt() const
	{
		return (uint32_t)(type & DL_TYPE_INLINE_ARRAY_CNT_MASK) >> DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT;
	}

	void set_inline_array_cnt( uint32_t count )
	{
		DL_ASSERT(count <= DL_INLINE_ARRAY_LENGTH_MAX);
		type = (dl_type_t)( ( (unsigned int)type & ~DL_TYPE_INLINE_ARRAY_CNT_MASK ) | (count << DL_TYPE_INLINE_ARRAY_CNT_MIN_BIT) );
	}
};

/**
 *
 */
enum dl_type_flags
{
	DL_TYPE_FLAG_HAS_SUBDATA                = 1 << 0, ///< the type has subdata and need pointer patching.
	DL_TYPE_FLAG_IS_EXTERNAL                = 1 << 1, ///< the type is marked as "external", this says that the type is not emitted in headers and expected to get defined by the user.
	DL_TYPE_FLAG_IS_UNION                   = 1 << 2, ///< the type is a "union" type.
	DL_TYPE_FLAG_VERIFY_EXTERNAL_SIZE_ALIGN = 1 << 3, ///< the type is a "union" type.

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
	uint32_t comment;
	uint32_t metadata_count;
	uint32_t metadata_start;
};

struct dl_enum_value_desc
{
	uint32_t main_alias;
	uint32_t comment;
	uint64_t value;
	uint32_t metadata_count;
	uint32_t metadata_start;
};

struct dl_enum_desc
{
	uint32_t          name;
	uint32_t          flags;
	dl_type_storage_t storage;
	uint32_t          value_count;
	uint32_t          value_start;
	uint32_t          alias_count; /// number of aliases for this enum, always at least 1. Alias 0 is consider the "main name" of the value and need to be a valid c enum name.
	uint32_t          alias_start; /// offset into alias list where aliases for this enum-value start.
	uint32_t		  comment;
	uint32_t          metadata_count;
	uint32_t          metadata_start;
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
	unsigned int metadatas_count;

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
	size_t typedata_strings_cap;

	char*  c_includes;
	size_t c_includes_size;
	size_t c_includes_cap;

	void** metadatas;
	size_t metadatas_cap;
	const void** metadata_infos;
	dl_typeid_t* metadata_typeinfos;

	uint8_t* default_data;
	size_t   default_data_size;
};

struct dl_substr
{
	const char* str;
	int len;
};

#ifdef _MSC_VER
#  define DL_FORCEINLINE __forceinline
#else
#  define DL_FORCEINLINE inline __attribute__( ( always_inline ) )
#endif

// A growable array using a stack buffer while small. Use it to avoid dynamic allocations while the stack is big enough, but fall back to heap if it grows past the wanted stack size
template <typename T, int SIZE>
class CArrayStatic
{
private:
	inline void GrowIfNeeded()
	{
		if (m_nElements >= m_nCapacity)
		{
			size_t curr_size = sizeof(T) * m_nCapacity;
			if (m_Ptr != &m_Storage[0])
			{
				m_Ptr = reinterpret_cast<T*>(dl_realloc(&m_Allocator, m_Ptr, curr_size * 2, curr_size));
			}
			else
			{
				void* new_mem = dl_alloc(&m_Allocator, curr_size * 2);
				memcpy(new_mem, m_Storage, curr_size);
				m_Ptr = reinterpret_cast<T*>(new_mem);
			}
			m_nCapacity *= 2;
		}
	}

public:
	T* m_Ptr;
	T m_Storage[SIZE];
	size_t m_nElements;
	size_t m_nCapacity;
	dl_allocator m_Allocator;

	explicit CArrayStatic(dl_allocator allocator)
	{
		m_nElements = 0;
		m_nCapacity = SIZE;
		m_Ptr = &m_Storage[0];
		m_Allocator = allocator;
	}

	~CArrayStatic()
	{
		if (m_Ptr != &m_Storage[0])
		{
			dl_free(&m_Allocator, m_Ptr);
		}
	}

	inline size_t Len()
	{
		return m_nElements;
	}

	void Add(const T& _Element)
	{
		GrowIfNeeded();
		new (&m_Ptr[m_nElements]) T(_Element);
		m_nElements++;
	}

	void Add(T&& _Element)
	{
		GrowIfNeeded();
		new (&m_Ptr[m_nElements]) T(std::move(_Element));
		m_nElements++;
	}

	DL_FORCEINLINE T& operator[]( size_t _iEl )
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");
		return m_Ptr[_iEl];
	}
	DL_FORCEINLINE const T& operator[]( size_t _iEl ) const
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");
		return m_Ptr[_iEl];
	}
};

#if defined( __GNUC__ )
#	define _Printf_format_string_
inline void dl_log_error( dl_ctx_t dl_ctx, const char* fmt, ... ) __attribute__((format( printf, 2, 3 )));
#endif

inline void dl_log_error( dl_ctx_t dl_ctx, _Printf_format_string_ const char* fmt, ... )
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
static inline T    dl_internal_align_up( const T value,   size_t alignment ) { return T( ((size_t)value + alignment - 1) & ~(alignment - 1) ); }
static inline bool dl_internal_is_align( const void* ptr, size_t alignment ) { return ((size_t)ptr & (alignment - 1)) == 0; }

/*
	return a bitfield offset on a particular platform (currently endian-ness is used to set them apart, that might break ;) )
	the bitfield offset are counted from least significant bit or most significant bit on different platforms.
*/
inline unsigned int dl_bf_offset( dl_endian_t endian, unsigned int bf_size, unsigned int offset, unsigned int bits ) { return endian == DL_ENDIAN_LITTLE ? offset : ( bf_size * 8 ) - offset - bits; }

static inline dl_endian_t dl_other_endian( dl_endian_t endian ) { return endian == DL_ENDIAN_LITTLE ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE; }

static inline const dl_type_desc* dl_internal_find_type(dl_ctx_t dl_ctx, dl_typeid_t type_id)
{
	// linear search right now!
    for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
    	if( dl_ctx->type_ids[i] == type_id )
    		return &dl_ctx->type_descs[i];

    return 0x0;
}

static inline const char* dl_internal_type_name         ( dl_ctx_t ctx, const dl_type_desc*       type   ) { return &ctx->typedata_strings[type->name]; }
static inline const char* dl_internal_type_comment      ( dl_ctx_t ctx, const dl_type_desc*       type   ) { return type->comment != UINT32_MAX ? &ctx->typedata_strings[type->comment] : 0x0; }
static inline const char* dl_internal_member_name       ( dl_ctx_t ctx, const dl_member_desc*     member ) { return &ctx->typedata_strings[member->name]; }
static inline const char* dl_internal_member_comment    ( dl_ctx_t ctx, const dl_member_desc*     member ) { return member->comment != UINT32_MAX ? &ctx->typedata_strings[member->comment] : 0x0; }
static inline const char* dl_internal_enum_name         ( dl_ctx_t ctx, const dl_enum_desc*       enum_  ) { return &ctx->typedata_strings[enum_->name]; }
static inline const char* dl_internal_enum_comment      ( dl_ctx_t ctx, const dl_enum_desc*       enum_  ) { return enum_->comment != UINT32_MAX ? &ctx->typedata_strings[enum_->comment] : 0x0; }
static inline const char* dl_internal_enum_alias_name   ( dl_ctx_t ctx, const dl_enum_alias_desc* alias  ) { return &ctx->typedata_strings[alias->name]; }
static inline const char* dl_internal_enum_value_comment( dl_ctx_t ctx, const dl_enum_value_desc* value  ) { return value->comment != UINT32_MAX ? &ctx->typedata_strings[value->comment] : 0x0; }

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

static inline const dl_type_desc* dl_internal_find_type_by_name( dl_ctx_t dl_ctx, const dl_substr* name )
{
	for(unsigned int i = 0; i < dl_ctx->type_count; ++i)
	{
		dl_type_desc* desc = &dl_ctx->type_descs[i];
		const char* t_name = dl_internal_type_name( dl_ctx, desc );
		if( strncmp( name->str, t_name, (size_t)name->len ) == 0 )
			if(t_name[name->len] == '\0')
				return desc;
	}
	return 0x0;
}


static inline size_t dl_pod_size( dl_type_storage_t storage )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT8:
		case DL_TYPE_STORAGE_UINT8:
		case DL_TYPE_STORAGE_ENUM_INT8:
		case DL_TYPE_STORAGE_ENUM_UINT8:
			return 1;

		case DL_TYPE_STORAGE_INT16:
		case DL_TYPE_STORAGE_UINT16:
		case DL_TYPE_STORAGE_ENUM_INT16:
		case DL_TYPE_STORAGE_ENUM_UINT16:
			return 2;

		case DL_TYPE_STORAGE_INT32:
		case DL_TYPE_STORAGE_UINT32:
		case DL_TYPE_STORAGE_FP32:
		case DL_TYPE_STORAGE_ENUM_INT32:
		case DL_TYPE_STORAGE_ENUM_UINT32:
			return 4;

		case DL_TYPE_STORAGE_INT64:
		case DL_TYPE_STORAGE_UINT64:
		case DL_TYPE_STORAGE_FP64:
		case DL_TYPE_STORAGE_ENUM_INT64:
		case DL_TYPE_STORAGE_ENUM_UINT64:
			return 8;

		case DL_TYPE_STORAGE_STR:
		case DL_TYPE_STORAGE_PTR:
			return sizeof(void*);

		default:
			DL_ASSERT(false && "This should not happen!");
			return 0;
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
	return member_index >= ctx->member_count ? nullptr : &ctx->member_descs[ type->member_start + member_index ];
}

static inline const dl_member_desc* dl_internal_union_type_to_member( dl_ctx_t dl_ctx, const dl_type_desc* type, uint32_t union_type )
{
	// type is typeid + member_index + 1, + one to separate the member-id from the type-id.
	return dl_get_type_member(dl_ctx, type, union_type - dl_internal_typeid_of(dl_ctx, type) - 1);
}

static inline uint32_t dl_internal_union_type_offset(dl_ctx_t ctx, const dl_type_desc* type, dl_ptr_size_t ptr_size)
{
	uint32_t max_member_size = 0; // TODO: calc and store this in type?
	uint32_t max_member_alignment = 0; // TODO: calc and store this in type?
	for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
	{
		const dl_member_desc* member = dl_get_type_member( ctx, type, member_index );
		max_member_size = member->size[ptr_size] > max_member_size ? member->size[ptr_size] : max_member_size;
		max_member_alignment = member->alignment[ptr_size] > max_member_alignment ? member->alignment[ptr_size] : max_member_alignment;
	}

	return dl_internal_align_up(max_member_size, max_member_alignment);
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

static inline bool dl_internal_find_enum_value( dl_ctx_t ctx, const dl_enum_desc* e, const char* name, size_t name_len, uint64_t* value )
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

#endif // DL_DL_TYPES_H_INCLUDED
