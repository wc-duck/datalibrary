/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_STORE_H_INCLUDED
#define DL_DL_STORE_H_INCLUDED

struct CDLBinStoreContext
{
	struct SString;

	CDLBinStoreContext( struct dl_binary_writer& writer_, dl_allocator alloc, CArrayStatic<uintptr_t, 256>& ptrs_, CArrayStatic<SString, 128>& strings_ )
	    : writer( writer_ )
	    , written_ptrs( alloc )
	    , ptrs( ptrs_ )
	    , strings( strings_ )
	{
	}

	uintptr_t FindWrittenPtr( const void* ptr )
	{
		if( ptr == 0 )
			return (uintptr_t)0;

		size_t len = written_ptrs.Len();
		for( size_t i = 0; i < len; ++i )
			if( written_ptrs[i].ptr == ptr )
				return written_ptrs[i].pos;

		return (uintptr_t)0;
	}

	void AddWrittenPtr( const void* ptr, uintptr_t pos )
	{
		written_ptrs.Add( { pos, ptr } );
	}

	uint32_t GetStringOffset( const char* str, int length, uint32_t hash )
	{
		for( size_t i = 0; i < strings.Len(); ++i )
			if( strings[i].hash == hash && strings[i].str.len == length && memcmp( str, strings[i].str.str, length ) == 0 )
				return strings[i].offset;

		return 0;
	}

	struct dl_binary_writer& writer;

	// Used to know where all pointer payloads are, so two identical pointers will refer to the same offset
	struct SWrittenPtr
	{
		uintptr_t pos;
		const void* ptr;
	};
	CArrayStatic<SWrittenPtr, 128> written_ptrs;

	// Used to know where all pointers are, to speed up pointer patching
	CArrayStatic<uintptr_t, 256>& ptrs;

	// Used to merge identical strings
	struct SString
	{
		dl_substr str;
		uint32_t hash;
		uint32_t offset;
	};
	CArrayStatic<SString, 128>& strings;
};

dl_error_t dl_internal_store_member( dl_ctx_t dl_ctx, const dl_member_desc* member, const uint8_t* instance, CDLBinStoreContext* store_ctx );

#endif // DL_DL_STORE_H_INCLUDED
