#include "dl_types.h"
#include "dl_swap.h"
#include "dl_binary_writer.h"
#include "dl_patch_ptr.h"

#include <dl/dl.h>

dl_error_t dl_context_create( dl_ctx_t* dl_ctx, dl_create_params_t* create_params )
{
	dl_allocator alloc;
	dl_allocator_initialize( &alloc, create_params->alloc_func, create_params->realloc_func, create_params->free_func, create_params->alloc_ctx );

	dl_context* ctx = (dl_context*)dl_alloc( &alloc, sizeof( dl_context ) );

	if(ctx == 0x0)
		return DL_ERROR_OUT_OF_LIBRARY_MEMORY;

	memset(ctx, 0x0, sizeof(dl_context));
	memcpy(&ctx->alloc, &alloc, sizeof( dl_allocator ) );

	ctx->error_msg_func = create_params->error_msg_func;
	ctx->error_msg_ctx  = create_params->error_msg_ctx;

	*dl_ctx = ctx;

	return DL_ERROR_OK;
}

dl_error_t dl_context_destroy(dl_ctx_t dl_ctx)
{
	dl_free( &dl_ctx->alloc, dl_ctx->type_ids );
	dl_free( &dl_ctx->alloc, dl_ctx->type_descs );
	dl_free( &dl_ctx->alloc, dl_ctx->enum_ids );
	dl_free( &dl_ctx->alloc, dl_ctx->enum_descs );
	dl_free( &dl_ctx->alloc, dl_ctx->member_descs );
	dl_free( &dl_ctx->alloc, dl_ctx->enum_value_descs );
	dl_free( &dl_ctx->alloc, dl_ctx->enum_alias_descs );
	dl_free( &dl_ctx->alloc, dl_ctx->typedata_strings );
	dl_free( &dl_ctx->alloc, dl_ctx->default_data );
	dl_free( &dl_ctx->alloc, dl_ctx->c_includes );
	for( size_t i = 0; i < dl_ctx->metadatas_count; ++i)
		dl_free( &dl_ctx->alloc, dl_ctx->metadatas[i] );
	dl_free( &dl_ctx->alloc, dl_ctx->metadatas );
	dl_free( &dl_ctx->alloc, dl_ctx->metadata_infos );
	dl_free( &dl_ctx->alloc, dl_ctx->metadata_typeinfos );
	dl_free( &dl_ctx->alloc, dl_ctx );
	return DL_ERROR_OK;
}

dl_error_t dl_instance_load( dl_ctx_t             dl_ctx,          dl_typeid_t  type_id,
							 void*                instance,        size_t instance_size,
							 const unsigned char* packed_instance, size_t packed_instance_size,
							 size_t*              consumed )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )           return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                  return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )        return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type_id )         return DL_ERROR_TYPE_MISMATCH;
	if( header->instance_size > instance_size )         return DL_ERROR_BUFFER_TOO_SMALL;

	const dl_type_desc* root_type = dl_internal_find_type( dl_ctx, header->root_instance_type );
	if( root_type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	// TODO: Temporary disabled due to CL doing some magic stuff!!!
	// Structs allocated on qstack seems to be unaligned!!!
	// if( !dl_internal_is_align( instance, pType->m_Alignment[DL_PTR_SIZE_HOST] ) )
	//	return DL_ERROR_BAD_ALIGNMENT;

	DL_ASSERT( (uint8_t*)instance + header->instance_size > packed_instance || (uint8_t*)instance < packed_instance + header->instance_size );
	memcpy( instance, packed_instance + sizeof(dl_data_header), header->instance_size );

	dl_internal_patch_instance( dl_ctx, root_type, (uint8_t*)instance, 0x0, (uintptr_t)instance );

	if( consumed )
		*consumed = (size_t)header->instance_size + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_instance_load_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type_id,
												   unsigned char* packed_instance, size_t      packed_instance_size,
												   void**         loaded_instance, size_t*     consumed)
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )           return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                  return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION )        return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type_id )         return DL_ERROR_TYPE_MISMATCH;

	const dl_type_desc* type = dl_internal_find_type(dl_ctx, header->root_instance_type);
	if( type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	uint8_t* instance_ptr = packed_instance + sizeof(dl_data_header);
	dl_internal_patch_instance( dl_ctx, type, instance_ptr, 0x0, (uintptr_t)instance_ptr );

	*loaded_instance = instance_ptr;

	if( consumed )
		*consumed = header->instance_size + sizeof(dl_data_header);

	return DL_ERROR_OK;
}

struct CDLBinStoreContext
{
	CDLBinStoreContext( uint8_t* out_data, size_t out_data_size, bool is_dummy, dl_allocator alloc )
		: written_ptrs(alloc)
		, strings(alloc)
	{
		dl_binary_writer_init( &writer, out_data, out_data_size, is_dummy, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );
	}

	uintptr_t FindWrittenPtr( const uint8_t* ptr )
	{
		for (size_t i = 0; i < written_ptrs.Len(); ++i)
			if( written_ptrs[i].ptr == ptr )
				return written_ptrs[i].pos;

		return (uintptr_t)-1;
	}

	void AddWrittenPtr( const uint8_t* ptr, uintptr_t pos )
	{
		written_ptrs.Add( { pos, ptr } );
	}

	uint32_t GetStringOffset(const char* str, uint32_t length, uint32_t hash)
	{
		for (size_t i = 0; i < strings.Len(); ++i)
			if (strings[i].hash == hash && strings[i].length == length && strcmp(str, strings[i].str) == 0)
				return strings[i].offset;

		return 0;
	}

	dl_binary_writer writer;

	struct SWrittenPtr
	{
		uintptr_t      pos;
		const uint8_t* ptr;
	};
	CArrayStatic<SWrittenPtr, 128> written_ptrs;

	struct SString
	{
		const char* str;
		uint32_t length;
		uint32_t hash;
		uint32_t offset;
	};
	CArrayStatic<SString, 128> strings;
};

static void dl_internal_store_string( const uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	char* str = *(char**)instance;
	if( str == 0x0 )
	{
		dl_binary_writer_write( &store_ctx->writer, &DL_NULL_PTR_OFFSET[ DL_PTR_SIZE_HOST ], sizeof(uintptr_t) );
		return;
	}
	uint32_t hash = dl_internal_hash_string(str);
	uint32_t length = (uint32_t) strlen(str);
	uintptr_t offset = store_ctx->GetStringOffset(str, length, hash);
	if (offset == 0) // Merge identical strings
	{
		uintptr_t pos = dl_binary_writer_tell(&store_ctx->writer);
		dl_binary_writer_seek_end(&store_ctx->writer);
		offset = dl_binary_writer_tell(&store_ctx->writer);
		dl_binary_writer_write(&store_ctx->writer, str, length + 1);
		store_ctx->strings.Add( {str, length, hash, (uint32_t) offset} );
		dl_binary_writer_seek_set(&store_ctx->writer, pos);
	}
	dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );
}

static dl_error_t dl_internal_instance_store( dl_ctx_t dl_ctx, const dl_type_desc* type, const uint8_t* instance, CDLBinStoreContext* store_ctx );

static dl_error_t dl_internal_store_ptr( dl_ctx_t dl_ctx, const uint8_t* instance, const dl_type_desc* sub_type, CDLBinStoreContext* store_ctx )
{
	const uint8_t* data = *(const uint8_t* const*)instance;
	uintptr_t offset = store_ctx->FindWrittenPtr( data );

	if( data == 0x0 ) // Null-pointer, store pint(-1) to signal to patching!
	{
		DL_ASSERT(offset == (uintptr_t)-1 && "This pointer should not have been found among the written ptrs!");
		// keep the -1 in Offset and store it to ptr.
	}
	else if( offset == (uintptr_t)-1 ) // has not been written yet!
	{
		uintptr_t pos = dl_binary_writer_tell( &store_ctx->writer );
		dl_binary_writer_seek_end( &store_ctx->writer );

		// const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
		uintptr_t size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
		dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );

		offset = dl_binary_writer_tell( &store_ctx->writer );

		// write data!
		dl_binary_writer_reserve( &store_ctx->writer, size ); // reserve space for ptr so subdata is placed correctly

		store_ctx->AddWrittenPtr(data, offset);

		dl_error_t err = dl_internal_instance_store(dl_ctx, sub_type, data, store_ctx);
		if (DL_ERROR_OK != err)
			return err;
		dl_binary_writer_seek_set( &store_ctx->writer, pos );
	}

	dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );
	return DL_ERROR_OK;
}

static dl_error_t dl_internal_store_array( dl_ctx_t dl_ctx, dl_type_storage_t storage_type, const dl_type_desc* sub_type, const uint8_t* instance, uint32_t count, uintptr_t size, CDLBinStoreContext* store_ctx )
{
	switch( storage_type )
	{
		case DL_TYPE_STORAGE_STRUCT:
		{
			uintptr_t size_ = sub_type->size[DL_PTR_SIZE_HOST];
			if( sub_type->flags & DL_TYPE_FLAG_HAS_SUBDATA )
			{
				for (uint32_t elem = 0; elem < count; ++elem)
				{
					dl_error_t err = dl_internal_instance_store(dl_ctx, sub_type, instance + (elem * size_), store_ctx);
					if (DL_ERROR_OK != err)
						return err;
				}
			}
			else
				dl_binary_writer_write( &store_ctx->writer, instance, count * size_ );
		}
		break;
		case DL_TYPE_STORAGE_STR:
			for( uint32_t elem = 0; elem < count; ++elem )
				dl_internal_store_string( instance + (elem * sizeof(char*)), store_ctx );
			break;
		case DL_TYPE_STORAGE_PTR:
			for( uint32_t elem = 0; elem < count; ++elem )
			{
				dl_error_t err = dl_internal_store_ptr( dl_ctx, instance + ( elem * sizeof( void* ) ), sub_type, store_ctx );
				if( DL_ERROR_OK != err )
					return err;
			}
			break;
		case DL_TYPE_STORAGE_ANY_POINTER:
			for( uint32_t elem = 0; elem < count; ++elem )
			{
				dl_typeid_t tid = *reinterpret_cast<const dl_typeid_t*>( instance + ( ( elem * 2 + 1 ) * sizeof( void* ) ) );
				sub_type = dl_internal_find_type( dl_ctx, tid );
				dl_error_t err = dl_internal_store_ptr( dl_ctx, instance + ( elem * sizeof( void* ) * 2 ), sub_type, store_ctx );
				if( DL_ERROR_OK != err )
					return err;
				dl_binary_writer_write_uint32( &store_ctx->writer, tid );
				if (sizeof(uintptr_t) == 8)
					dl_binary_writer_write_zero( &store_ctx->writer, 4 );
			}
			break;
		default: // default is a standard pod-type
			dl_binary_writer_write( &store_ctx->writer, instance, count * size );
			break;
	}
	return DL_ERROR_OK;
}

static dl_error_t dl_internal_store_anyarray( dl_ctx_t dl_ctx, const uint8_t* data_ptr, uint32_t count, CDLBinStoreContext* store_ctx )
{
	for (uint32_t i = 0; i < count; ++i)
	{
		uintptr_t sub_pos = dl_binary_writer_tell( &store_ctx->writer );
		dl_binary_writer_seek_end( &store_ctx->writer );

		const uint8_t* sub_data_ptr = data_ptr + i * 3 * sizeof(void*);
		const uint8_t* sub_data = *(const uint8_t* const*)sub_data_ptr;
		uint32_t sub_count  = *(const uint32_t*)( sub_data_ptr + sizeof(void*) );
		dl_typeid_t sub_type_id = *(const dl_typeid_t*)( sub_data_ptr + 2 * sizeof(void*) );
		const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, sub_type_id );
		size_t sub_size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
		dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );

		uintptr_t sub_offset = dl_binary_writer_tell( &store_ctx->writer );

		// write data!
		dl_binary_writer_reserve( &store_ctx->writer, sub_count * sub_size ); // reserve space for array so subdata is placed correctly

		dl_error_t err = dl_internal_store_array(dl_ctx, DL_TYPE_STORAGE_STRUCT, sub_type, sub_data, sub_count, sub_size, store_ctx);
		if (DL_ERROR_OK != err)
			return err;
		dl_binary_writer_seek_set( &store_ctx->writer, sub_pos );
								
		// use when fast_loading is merged 
		//if( !store_ctx->writer.dummy )
		//	store_ctx->ptrs.Add( dl_binary_writer_tell( &store_ctx->writer ) );

		// make room for ptr
		dl_binary_writer_write( &store_ctx->writer, &sub_offset, sizeof(uintptr_t) );

		// write count
		dl_binary_writer_write( &store_ctx->writer, &sub_count, sizeof(uint32_t) );
		if (sizeof(uintptr_t) == 8)
			dl_binary_writer_write_zero( &store_ctx->writer, sizeof(uint32_t) );

		// write type id
		dl_binary_writer_write( &store_ctx->writer, &sub_type_id, sizeof(uint32_t) );
		if (sizeof(uintptr_t) == 8)
			dl_binary_writer_write_zero( &store_ctx->writer, sizeof(uint32_t) );
	}
	return DL_ERROR_OK;
}

dl_error_t dl_internal_store_member( dl_ctx_t dl_ctx, const dl_member_desc* member, const uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	dl_type_atom_t    atom_type    = member->AtomType();
	dl_type_storage_t storage_type = member->StorageType();

	if (storage_type == DL_TYPE_STORAGE_ANY_ARRAY)
		atom_type = DL_TYPE_ATOM_ARRAY; // To avoid code duplification and simplify implementation
	switch ( atom_type )
	{
		case DL_TYPE_ATOM_POD:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
					if( sub_type == 0x0 )
					{
						dl_log_error( dl_ctx, "Could not find struct subtype %X for member %s", member->type_id, dl_internal_member_name( dl_ctx, member ) );
						return DL_ERROR_TYPE_NOT_FOUND;
					}
					dl_error_t err = dl_internal_instance_store(dl_ctx, sub_type, instance, store_ctx);
					if (DL_ERROR_OK != err)
						return err;
				}
				break;
				case DL_TYPE_STORAGE_STR:
					dl_internal_store_string( instance, store_ctx );
					break;
				case DL_TYPE_STORAGE_ANY_POINTER:
				{
					dl_typeid_t tid = *reinterpret_cast<const dl_typeid_t*>( instance + sizeof( void* ) );
					const dl_type_desc* sub_type = nullptr;
					if( *reinterpret_cast<const void* const*>( instance ) )
					{
						sub_type = dl_internal_find_type( dl_ctx, tid );
						if( sub_type == 0x0 )
						{
							dl_log_error( dl_ctx, "Could not find any_pointer subtype %X for member %s", tid, dl_internal_member_name( dl_ctx, member ) );
							return DL_ERROR_TYPE_NOT_FOUND;
						}
					}
					dl_error_t err = dl_internal_store_ptr( dl_ctx, instance, sub_type, store_ctx );
					if( DL_ERROR_OK != err )
						return err;
					dl_binary_writer_write( &store_ctx->writer, &tid, sizeof(dl_typeid_t) );
				}
				break;
				case DL_TYPE_STORAGE_PTR:
				{
					const dl_type_desc* sub_type = dl_internal_find_type( dl_ctx, member->type_id );
					if( sub_type == 0x0 )
					{
						dl_log_error( dl_ctx, "Could not find pointer subtype %X for member %s", member->type_id, dl_internal_member_name( dl_ctx, member ) );
						return DL_ERROR_TYPE_NOT_FOUND;
					}
					dl_error_t err = dl_internal_store_ptr( dl_ctx, instance, sub_type, store_ctx );
					if (DL_ERROR_OK != err)
						return err;
				}
				break;
				default: // default is a standard pod-type
					DL_ASSERT( member->IsSimplePod() );
					dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] );
					break;
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			const dl_type_desc* sub_type = 0x0;
			uint32_t count = member->inline_array_cnt();
			if( storage_type == DL_TYPE_STORAGE_STRUCT ||
				storage_type == DL_TYPE_STORAGE_PTR )
			{
				sub_type = dl_internal_find_type(dl_ctx, member->type_id);
				if (sub_type == 0x0)
				{
					dl_log_error(dl_ctx, "Could not find subtype for member %s", dl_internal_member_name(dl_ctx, member));
					return DL_ERROR_TYPE_NOT_FOUND;
				}
			}
			else if( storage_type != DL_TYPE_STORAGE_STR && storage_type != DL_TYPE_STORAGE_ANY_POINTER)
				count = member->size[DL_PTR_SIZE_HOST];

			return dl_internal_store_array( dl_ctx, storage_type, sub_type, instance, count, 1, store_ctx );
		}

		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t size = 0;
			const dl_type_desc* sub_type = 0x0;

			const uint8_t* data_ptr = instance;
			uint32_t count    = *(uint32_t*)( data_ptr + sizeof(void*) );

			uintptr_t offset = 0;

			if( count == 0 )
				offset = DL_NULL_PTR_OFFSET[ DL_PTR_SIZE_HOST ];
			else
			{
				uintptr_t pos = dl_binary_writer_tell( &store_ctx->writer );
				dl_binary_writer_seek_end( &store_ctx->writer );

				switch(storage_type)
				{
					case DL_TYPE_STORAGE_STRUCT:
						sub_type = dl_internal_find_type( dl_ctx, member->type_id );
						size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
						dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );
						break;
					case DL_TYPE_STORAGE_ANY_ARRAY:
						storage_type = DL_TYPE_STORAGE_STRUCT;
						if (member->AtomType() == DL_TYPE_ATOM_POD)
						{
							dl_typeid_t type_id = *(dl_typeid_t*)( data_ptr + 2 * sizeof(void*) );
							sub_type = dl_internal_find_type( dl_ctx, type_id );
							size = dl_internal_align_up( sub_type->size[DL_PTR_SIZE_HOST], sub_type->alignment[DL_PTR_SIZE_HOST] );
							dl_binary_writer_align( &store_ctx->writer, sub_type->alignment[DL_PTR_SIZE_HOST] );
						}
						else if( member->AtomType() == DL_TYPE_ATOM_INLINE_ARRAY )
						{
							dl_binary_writer_seek_set( &store_ctx->writer, pos );
							return dl_internal_store_anyarray( dl_ctx, data_ptr, member->inline_array_cnt(), store_ctx );
						}
						else if( member->AtomType() == DL_TYPE_ATOM_ARRAY )
						{
							dl_binary_writer_align( &store_ctx->writer, sizeof(void*) );
							offset = dl_binary_writer_tell( &store_ctx->writer );
							dl_binary_writer_reserve( &store_ctx->writer, count * 3 * sizeof(void*) ); // reserve space for array so subdata is placed correctly
							dl_error_t err = dl_internal_store_anyarray( dl_ctx, *(const uint8_t* const*)data_ptr, count, store_ctx );
							if (DL_ERROR_OK != err) return err;
						
							dl_binary_writer_seek_set( &store_ctx->writer, pos );

							// use when fast_loading is merged 
							//if( !store_ctx->writer.dummy )
							//	store_ctx->ptrs.Add( dl_binary_writer_tell( &store_ctx->writer ) );

							// make room for ptr
							dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );

							// write count
							dl_binary_writer_write( &store_ctx->writer, &count, sizeof(uint32_t) );
							if (sizeof(uintptr_t) == 8)
								dl_binary_writer_write_zero( &store_ctx->writer, sizeof(uint32_t) );
							return DL_ERROR_OK;
						}
						else
							DL_ASSERT(false && "Invalid ATOM-type!");
						break;
					case DL_TYPE_STORAGE_PTR:
						sub_type = dl_internal_find_type( dl_ctx, member->type_id );
						/*fallthrough*/
					default:
						size = dl_pod_size( member->StorageType() );
						dl_binary_writer_align( &store_ctx->writer, size );
				}

				offset = dl_binary_writer_tell( &store_ctx->writer );

				// write data!
				dl_binary_writer_reserve( &store_ctx->writer, count * size ); // reserve space for array so subdata is placed correctly

				uint8_t* data = *(uint8_t**)data_ptr;

				dl_error_t err = dl_internal_store_array(dl_ctx, storage_type, sub_type, data, count, size, store_ctx);
				if (DL_ERROR_OK != err)
					return err;
				dl_binary_writer_seek_set( &store_ctx->writer, pos );
			}

			// make room for ptr
			dl_binary_writer_write( &store_ctx->writer, &offset, sizeof(uintptr_t) );

			// write count
			dl_binary_writer_write( &store_ctx->writer, &count, sizeof(uint32_t) );
			if (sizeof(uintptr_t) == 8)
				dl_binary_writer_write_zero( &store_ctx->writer, sizeof(uint32_t) );

			if (member->StorageType() == DL_TYPE_STORAGE_ANY_ARRAY)
			{
				// write type id
				dl_binary_writer_write( &store_ctx->writer, (data_ptr + 2 * sizeof(void*)), sizeof(uint32_t) );
				if (sizeof(uintptr_t) == 8)
					dl_binary_writer_write_zero( &store_ctx->writer, sizeof(uint32_t) );
			}
		}
		return DL_ERROR_OK;

		case DL_TYPE_ATOM_BITFIELD:
			dl_binary_writer_write( &store_ctx->writer, instance, member->size[DL_PTR_SIZE_HOST] );
		break;

		default:
			DL_ASSERT(false && "Invalid ATOM-type!");
			break;
	}

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_instance_store( dl_ctx_t dl_ctx, const dl_type_desc* type, const uint8_t* instance, CDLBinStoreContext* store_ctx )
{
	bool last_was_bitfield = false;

	dl_binary_writer_align( &store_ctx->writer, type->alignment[DL_PTR_SIZE_HOST] );

	uintptr_t instance_pos = dl_binary_writer_tell( &store_ctx->writer );
	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		size_t type_offset = dl_internal_union_type_offset( dl_ctx, type, DL_PTR_SIZE_HOST );

		// find member index from union type ...
		uint32_t union_type = *((uint32_t*)(instance + type_offset));
		const dl_member_desc* member = dl_internal_union_type_to_member(dl_ctx, type, union_type);
		if (member == nullptr)
		{
			dl_log_error(dl_ctx, "Could not find union type %X for type %s", union_type, dl_internal_type_name(dl_ctx, type));
			return DL_ERROR_MALFORMED_DATA;
		}

		dl_error_t err = dl_internal_store_member( dl_ctx, member, instance + member->offset[DL_PTR_SIZE_HOST], store_ctx );
		if( err != DL_ERROR_OK )
			return err;

		dl_binary_writer_seek_set( &store_ctx->writer, instance_pos + type_offset );
		dl_binary_writer_write_uint32( &store_ctx->writer, union_type );
	}
	else
	{
		for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
		{
			const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );

			if( !last_was_bitfield || member->AtomType() != DL_TYPE_ATOM_BITFIELD )
			{
				dl_binary_writer_seek_set( &store_ctx->writer, instance_pos + member->offset[DL_PTR_SIZE_HOST] );
				dl_error_t err = dl_internal_store_member( dl_ctx, member, instance + member->offset[DL_PTR_SIZE_HOST], store_ctx );
				if( err != DL_ERROR_OK )
					return err;
			}

			last_was_bitfield = member->AtomType() == DL_TYPE_ATOM_BITFIELD;
		}
	}

	return DL_ERROR_OK;
}

dl_error_t dl_instance_store( dl_ctx_t       dl_ctx,     dl_typeid_t type_id,         const void* instance,
							  unsigned char* out_buffer, size_t      out_buffer_size, size_t*     produced_bytes )
{
	if( out_buffer_size > 0 && out_buffer_size <= sizeof(dl_data_header) )
		return DL_ERROR_BUFFER_TOO_SMALL;

	const dl_type_desc* type = dl_internal_find_type( dl_ctx, type_id );
	if( type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	bool store_ctx_is_dummy = out_buffer_size == 0;
	CDLBinStoreContext store_context(out_buffer + sizeof(dl_data_header), out_buffer_size - sizeof(dl_data_header), store_ctx_is_dummy, dl_ctx->alloc);

	if( out_buffer_size > 0 )
	{
		if( instance == out_buffer + sizeof(dl_data_header) )
			memset( out_buffer, 0, sizeof(dl_data_header) );
		else
			memset( out_buffer, 0, out_buffer_size );

		dl_data_header* header = (dl_data_header*)out_buffer;
		header->id                 = DL_INSTANCE_ID;
		header->version            = DL_INSTANCE_VERSION;
		header->root_instance_type = type_id;
		header->is_64_bit_ptr      = sizeof( void* ) == 8 ? 1 : 0;
	}

	dl_binary_writer_reserve( &store_context.writer, type->size[DL_PTR_SIZE_HOST] );
	store_context.AddWrittenPtr((const uint8_t*)instance, 0); // if pointer refere to root-node, it can be found at offset 0

	dl_error_t err = dl_internal_instance_store( dl_ctx, type, (const uint8_t*)instance, &store_context );

	// write instance size!
	dl_data_header* out_header = (dl_data_header*)out_buffer;
	dl_binary_writer_seek_end( &store_context.writer );
	if( out_buffer )
		out_header->instance_size = (uint32_t)dl_binary_writer_tell( &store_context.writer );

	if( produced_bytes )
		*produced_bytes = (uint32_t)dl_binary_writer_tell( &store_context.writer ) + sizeof(dl_data_header);

	if( out_buffer_size > 0 && dl_binary_writer_tell( &store_context.writer ) > out_buffer_size )
		return DL_ERROR_BUFFER_TOO_SMALL;

	return err;
}

dl_error_t dl_instance_calc_size( dl_ctx_t dl_ctx, dl_typeid_t type, const void* instance, size_t* out_size )
{
	return dl_instance_store( dl_ctx, type, instance, 0x0, 0, out_size );
}

const char* dl_error_to_string( dl_error_t error )
{
#define DL_ERR_TO_STR(ERR) case ERR: return #ERR
	switch(error)
	{
		DL_ERR_TO_STR(DL_ERROR_OK);
		DL_ERR_TO_STR(DL_ERROR_MALFORMED_DATA);
		DL_ERR_TO_STR(DL_ERROR_VERSION_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_LIBRARY_MEMORY);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_INSTANCE_MEMORY);
		DL_ERR_TO_STR(DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR);
		DL_ERR_TO_STR(DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS);
		DL_ERR_TO_STR(DL_ERROR_TYPE_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_TYPE_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_BUFFER_TOO_SMALL);
		DL_ERR_TO_STR(DL_ERROR_ENDIAN_MISMATCH);
		DL_ERR_TO_STR(DL_ERROR_BAD_ALIGNMENT);
		DL_ERR_TO_STR(DL_ERROR_INVALID_PARAMETER);
		DL_ERR_TO_STR(DL_ERROR_INVALID_DEFAULT_VALUE);
		DL_ERR_TO_STR(DL_ERROR_UNSUPPORTED_OPERATION);

		DL_ERR_TO_STR(DL_ERROR_TXT_PARSE_ERROR);
		DL_ERR_TO_STR(DL_ERROR_TXT_MISSING_MEMBER);
		DL_ERR_TO_STR(DL_ERROR_TXT_MEMBER_SET_TWICE);
		DL_ERR_TO_STR(DL_ERROR_TXT_INVALID_MEMBER);
		DL_ERR_TO_STR(DL_ERROR_TXT_RANGE_ERROR);
		DL_ERR_TO_STR(DL_ERROR_TXT_INVALID_MEMBER_TYPE);
		DL_ERR_TO_STR(DL_ERROR_TXT_INVALID_ENUM_VALUE);
		DL_ERR_TO_STR(DL_ERROR_TXT_MISSING_SECTION);
		DL_ERR_TO_STR(DL_ERROR_TXT_MULTIPLE_MEMBERS_IN_UNION_SET);

		DL_ERR_TO_STR(DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE);

		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_NOT_FOUND);
		DL_ERR_TO_STR(DL_ERROR_UTIL_FILE_TYPE_MISMATCH);

		DL_ERR_TO_STR(DL_ERROR_INTERNAL_ERROR);
		default: return "Unknown error!";
	}
#undef DL_ERR_TO_STR
}

dl_error_t dl_instance_get_info( const unsigned char* packed_instance, size_t packed_instance_size, dl_instance_info_t* out_info )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) && header->id != DL_INSTANCE_ID_SWAPED && header->id != DL_INSTANCE_ID )
		return DL_ERROR_MALFORMED_DATA;
	if(header->version != DL_INSTANCE_VERSION && header->version != DL_INSTANCE_VERSION_SWAPED)
		return DL_ERROR_VERSION_MISMATCH;

	out_info->ptrsize   = header->is_64_bit_ptr ? 8 : 4;
	if( header->id == DL_INSTANCE_ID )
	{
		out_info->load_size = header->instance_size;
		out_info->endian    = DL_ENDIAN_HOST;
		out_info->root_type = header->root_instance_type ;
	}
	else
	{
		out_info->load_size = dl_swap_endian_uint32( header->instance_size );
		out_info->endian    = dl_other_endian( DL_ENDIAN_HOST );
		out_info->root_type = dl_swap_endian_uint32( header->root_instance_type );
	}

	return DL_ERROR_OK;
}
