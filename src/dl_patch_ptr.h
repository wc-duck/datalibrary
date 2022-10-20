#ifndef DL_PATCH_PTR_H_INCLUDED
#define DL_PATCH_PTR_H_INCLUDED

#include "dl_types.h"

struct dl_patched_ptrs
{
	CArrayStatic<uintptr_t, 256> addresses;

	explicit dl_patched_ptrs( dl_allocator alloc )
		: addresses( alloc )
	{
	}

	void add( uintptr_t addr )
	{
		DL_ASSERT( !patched( addr ) );
		addresses.Add( addr );
	}

	bool patched( uintptr_t addr )
	{
		for( size_t i = 0; i < addresses.Len(); ++i )
			if( addr == addresses[i] )
				return true;
		return false;
	}
};

/**
 * Patch all pointers in an instance.
 *
 * @param ctx dl-context containing all types used in type.
 * @param type type desc of instance to patch.
 * @param instance pointer to instance to patch.
 * @param base_address base address to patch the pointers against.
 * @param patch_distance distance in bytes to patch all pointers.
 */
void dl_internal_patch_instance( dl_ctx_t            ctx,
								 const dl_type_desc* type,
								 uint8_t*            instance,
								 uintptr_t           base_address,
								 uintptr_t           patch_distance );

/**
 * Patch all pointers in a member.
 *
 * @param ctx dl-context containing all types used in type.
 * @param member member desc of member to patch.
 * @param member_data pointer to member to patch.
 * @param base_address base address to patch the pointers against.
 * @param patch_distance distance in bytes to patch all pointers.
 * @param patched_ptrs keeps a record of all patched ptrs. Can be nullptr
 */
void dl_internal_patch_member( dl_ctx_t              ctx,
							   const dl_member_desc* member,
							   uint8_t*              member_data,
							   uintptr_t             base_address,
							   uintptr_t             patch_distance,
							   dl_patched_ptrs*      patched_ptrs );

#endif // DL_PATCH_PTR_H_INCLUDED
