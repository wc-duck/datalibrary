#include "dl_patch_ptr.h"
#include "dl_types.h"

static uintptr_t dl_internal_patch_ptr( uint8_t* ptrptr, uintptr_t patch_distance )
{
	union { uint8_t* src; uintptr_t* ptr; };
	src = ptrptr;
	if( *ptr != 0 )
		*ptr = *ptr + patch_distance;
	return *ptr;
}

static void dl_internal_patch_struct( dl_ctx_t            ctx,
									  const dl_type_desc* type,
									  uint8_t*            struct_data,
									  uintptr_t           base_address,
									  uintptr_t           patch_distance,
									  dl_patched_ptrs*    patched_ptrs,
									  dl_patched_ptrs*    patched_payloads );

static void dl_internal_patch_ptr_instance( dl_ctx_t            ctx,
		   	   	   	   	   	   	   	   	    const dl_type_desc* sub_type,
											uint8_t*            ptr_data,
											uintptr_t           base_address,
											uintptr_t           patch_distance,
											dl_patched_ptrs*    patched_ptrs,
											dl_patched_ptrs*    patched_payloads )
{
	uintptr_t offset = dl_internal_patch_ptr( ptr_data, patch_distance );
	if( offset == 0x0 )
		return;

	uintptr_t ptr = (uintptr_t)base_address + offset;
	if( patched_payloads->patched( ptr ) )
		return;

	patched_payloads->add( ptr );
	dl_internal_patch_struct( ctx, sub_type, (uint8_t*)ptr, base_address, patch_distance, patched_ptrs, patched_payloads );
}

static void dl_internal_patch_str_array( uint8_t* array_data, uint32_t count, uintptr_t patch_distance, uintptr_t base_address, dl_patched_ptrs* patched_ptrs )
{
	for( uint32_t index = 0; index < count; ++index )
		if( dl_internal_patch_ptr( array_data + index * sizeof( char* ), patch_distance ) && patched_ptrs )
			patched_ptrs->add( uintptr_t( array_data + index * sizeof( char* ) - base_address ) );
}

static void dl_internal_patch_ptr_array( dl_ctx_t            ctx,
								  	  	 uint8_t*            array_data,
										 uint32_t            count,
										 const dl_type_desc* sub_type,
										 uintptr_t           base_address,
										 uintptr_t           patch_distance,
										 dl_patched_ptrs*    patched_ptrs,
                                         dl_patched_ptrs*    patched_payloads )
{
	for( uint32_t index = 0; index < count; ++index )
		dl_internal_patch_ptr_instance( ctx, sub_type, array_data + index * sizeof(void*), base_address, patch_distance, patched_ptrs, patched_payloads );
}

static void dl_internal_patch_struct_array( dl_ctx_t            ctx,
									 	 	const dl_type_desc* type,
											uint8_t*            array_data,
											uint32_t            count,
											uintptr_t           base_address,
											uintptr_t           patch_distance,
											dl_patched_ptrs*    patched_ptrs,
											dl_patched_ptrs*    patched_payloads )
{
	uint32_t size = dl_internal_align_up( type->size[DL_PTR_SIZE_HOST], type->alignment[DL_PTR_SIZE_HOST] );
	for( uint32_t index = 0; index < count; ++index )
	{
		uint8_t* struct_data = array_data + index * size;
		dl_internal_patch_struct( ctx, type, struct_data, base_address, patch_distance, patched_ptrs, patched_payloads );
	}
}

static void dl_internal_patch_member( dl_ctx_t              ctx,
								      const dl_member_desc* member,
								      uint8_t*              member_data,
								      uintptr_t             base_address,
								      uintptr_t             patch_distance,
								      dl_patched_ptrs*      patched_ptrs,
								      dl_patched_ptrs*      patched_payloads )
{
	dl_type_atom_t    atom_type    = member->AtomType();
	dl_type_storage_t storage_type = member->StorageType();

	switch( atom_type )
	{
		case DL_TYPE_ATOM_POD:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STR:
					if( dl_internal_patch_ptr( member_data, patch_distance ) && patched_ptrs )
						patched_ptrs->add( uintptr_t(member_data - base_address) );
				break;
				case DL_TYPE_STORAGE_PTR:
					dl_internal_patch_ptr_instance( ctx,
													dl_internal_find_type( ctx, member->type_id ),
													member_data,
													base_address,
													patch_distance,
													patched_ptrs,
													patched_payloads );
				break;
				case DL_TYPE_STORAGE_STRUCT:
					dl_internal_patch_struct( ctx,
											  dl_internal_find_type( ctx, member->type_id ),
											  member_data,
											  base_address,
											  patch_distance,
											  patched_ptrs,
											  patched_payloads );
				break;
				default:
					break;
			}
		}
		break;

		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STR:
					dl_internal_patch_str_array( member_data, member->inline_array_cnt(), patch_distance, base_address, patched_ptrs );
				break;
				case DL_TYPE_STORAGE_PTR:
					dl_internal_patch_ptr_array( ctx,
												 member_data,
												 member->inline_array_cnt(),
												 dl_internal_find_type( ctx, member->type_id ),
												 base_address,
												 patch_distance,
												 patched_ptrs,
												 patched_payloads );
				break;
				case DL_TYPE_STORAGE_STRUCT:
					dl_internal_patch_struct_array( ctx,
													dl_internal_find_type( ctx, member->type_id ),
													member_data,
													member->inline_array_cnt(),
													base_address,
													patch_distance,
													patched_ptrs,
													patched_payloads );
				break;
				default:
					break;
			}
		}
		break;

		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t offset = dl_internal_patch_ptr( member_data, patch_distance );
			if( offset && patched_ptrs )
				patched_ptrs->add( uintptr_t( member_data - base_address ) );

			uint8_t* src   = member_data + sizeof( void* );
			uint32_t count = *(uint32_t*)src;

			if( count != 0 )
			{
				uint8_t* array_data = (uint8_t*)base_address + offset;
				switch( storage_type )
				{
					case DL_TYPE_STORAGE_STR:
						dl_internal_patch_str_array( array_data, count, patch_distance, base_address, patched_ptrs );
					break;
					case DL_TYPE_STORAGE_PTR:
						dl_internal_patch_ptr_array( ctx,
													 array_data,
													 count,
													 dl_internal_find_type( ctx, member->type_id ),
													 base_address,
													 patch_distance,
													 patched_ptrs,
													 patched_payloads );
					break;
					case DL_TYPE_STORAGE_STRUCT:
						dl_internal_patch_struct_array( ctx,
														dl_internal_find_type( ctx, member->type_id ),
														array_data,
														count,
														base_address,
														patch_distance,
														patched_ptrs,
														patched_payloads );
					break;
					default:
						break;
				}
			}
		}
		break;
		default:
			break;
	}
}

static void dl_internal_patch_union( dl_ctx_t            ctx,
									 const dl_type_desc* type,
									 uint8_t*            union_data,
									 uintptr_t           base_address,
									 uintptr_t           patch_distance,
									 dl_patched_ptrs*    patched_ptrs,
									 dl_patched_ptrs*    patched_payloads )
{
	DL_ASSERT(type->flags & DL_TYPE_FLAG_IS_UNION);
	size_t type_offset = dl_internal_union_type_offset( ctx, type, DL_PTR_SIZE_HOST );

	// find member index from union type ...
	uint32_t union_type = *((uint32_t*)(union_data + type_offset));
	const dl_member_desc* member = dl_internal_union_type_to_member(ctx, type, union_type);
	DL_ASSERT(member->offset[DL_PTR_SIZE_HOST] == 0);
	dl_internal_patch_member( ctx, member, union_data, base_address, patch_distance, patched_ptrs, patched_payloads );
}

static void dl_internal_patch_struct( dl_ctx_t            ctx,
									  const dl_type_desc* type,
									  uint8_t*            struct_data,
									  uintptr_t           base_address,
									  uintptr_t           patch_distance,
									  dl_patched_ptrs*    patched_ptrs,
									  dl_patched_ptrs*    patched_payloads )
{
	if( type->flags & DL_TYPE_FLAG_HAS_SUBDATA )
	{
		if( type->flags & DL_TYPE_FLAG_IS_UNION )
		{
			dl_internal_patch_union( ctx, type, struct_data, base_address, patch_distance, patched_ptrs, patched_payloads );
		}
		else
		{
			for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
			{
				const dl_member_desc* member = dl_get_type_member( ctx, type, member_index );
				dl_internal_patch_member( ctx, member, struct_data + member->offset[DL_PTR_SIZE_HOST], base_address, patch_distance, patched_ptrs, patched_payloads );
			}
		}
	}
}

void dl_internal_patch_member( dl_ctx_t              ctx,
							   const dl_member_desc* member,
							   uint8_t*              member_data,
							   uintptr_t             base_address,
							   uintptr_t             patch_distance,
							   dl_patched_ptrs*      patched_ptrs )
{
	dl_patched_ptrs patched(ctx->alloc);
	dl_internal_patch_member( ctx, member, member_data, base_address, patch_distance, patched_ptrs, &patched );
}

void dl_internal_patch_instance( dl_ctx_t            ctx,
								 const dl_type_desc* type,
								 uint8_t*            instance,
								 uintptr_t           base_address,
								 uintptr_t           patch_distance )
{
	dl_patched_ptrs patched(ctx->alloc);
	patched.add( (uintptr_t)instance );

	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		dl_internal_patch_union( ctx, type, instance, base_address, patch_distance, 0, &patched );
	}
	else
	{
		for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
		{
			const dl_member_desc* member = dl_get_type_member( ctx, type, member_index );
			uint8_t*   member_data = instance + member->offset[DL_PTR_SIZE_HOST];

			dl_internal_patch_member( ctx, member, member_data, base_address, patch_distance, 0, &patched );
		}
	}
}
