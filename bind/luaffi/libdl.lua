local ffi = require "ffi"

ffi.cdef[[
typedef struct dl_context* dl_ctx_t;

enum dl_error_t
{
	DL_ERROR_OK,
	DL_ERROR_MALFORMED_DATA,
	DL_ERROR_VERSION_MISMATCH,
	DL_ERROR_OUT_OF_LIBRARY_MEMORY,
	DL_ERROR_OUT_OF_INSTANCE_MEMORY,
	DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR,
	DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS,
	DL_ERROR_TYPE_MISMATCH,
	DL_ERROR_TYPE_NOT_FOUND,
	DL_ERROR_MEMBER_NOT_FOUND,
	DL_ERROR_BUFFER_TO_SMALL,
	DL_ERROR_ENDIAN_MISMATCH,
	DL_ERROR_BAD_ALIGNMENT,
	DL_ERROR_INVALID_PARAMETER,
	DL_ERROR_UNSUPPORTED_OPERATION,

	DL_ERROR_TXT_PARSE_ERROR,
	DL_ERROR_TXT_MEMBER_MISSING,
	DL_ERROR_TXT_MEMBER_SET_TWICE,
	DL_ERROR_TXT_INVALID_ENUM_VALUE,

	DL_ERROR_UTIL_FILE_NOT_FOUND,
	DL_ERROR_UTIL_FILE_TYPE_MISMATCH,

	DL_ERROR_INTERNAL_ERROR
};

typedef void* (*dl_alloc_func)( unsigned int size, unsigned int alignment, void* alloc_ctx );
typedef void  (*dl_free_func) ( void* ptr, void* alloc_ctx );
typedef void  (*dl_error_msg_handler)( const char* msg, void* userdata );

typedef struct dl_create_params
{
	dl_alloc_func alloc_func;
	dl_free_func  free_func;
	void*         alloc_ctx;

	dl_error_msg_handler error_msg_func;
	void*                error_msg_ctx;
} dl_create_params_t;

typedef unsigned int dl_typeid_t;

enum dl_error_t dl_context_create( dl_ctx_t* dl_ctx, dl_create_params_t* create_params );
enum dl_error_t dl_context_destroy( dl_ctx_t dl_ctx );
enum dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, unsigned int lib_data_size );
enum dl_error_t dl_instance_calc_size( dl_ctx_t dl_ctx, dl_typeid_t type, void* instance, unsigned int* out_size);
enum dl_error_t dl_instance_store( dl_ctx_t dl_ctx, dl_typeid_t type, void* instance,
										unsigned char* out_buffer, unsigned int out_buffer_size, unsigned int* produced_bytes );
char* dl_error_to_string( enum dl_error_t error );
enum dl_error_t dl_txt_unpack( dl_ctx_t dl_ctx, dl_typeid_t  type,
									const unsigned char* packed_instance,  unsigned int packed_instance_size,
									char*                out_txt_instance, unsigned int out_txt_instance_size,
									unsigned int*        produced_bytes );
enum dl_error_t dl_txt_unpack_calc_size( dl_ctx_t dl_ctx, dl_typeid_t  type,
                                                  const unsigned char* packed_instance, unsigned int packed_instance_size,
                                                  unsigned int* out_txt_instance_size );
]]

local function dl_error_check(dl_error)
	if dl_error ~= ffi.C.DL_ERROR_OK then
		local dl_err = dl_lib.dl_error_to_string(dl_error)
		local str = ffi.string(dl_err)
		print(str)
		return false
	end
	return true
end

local function _dl_initialize(dynamic_lib, type_library)
	local dl_lib = ffi.load(dynamic_lib)
	local dl_create_params = ffi.new("dl_create_params_t")
	local dl_context_ptr = ffi.new("dl_ctx_t[1]") -- ptr ptr
	local dl_error = dl_lib.dl_context_create(dl_context_ptr, dl_create_params)
	if dl_error_check(dl_error) == false then return end
	local dl_context = dl_context_ptr[0]
	local type_library_file = io.open(type_library, "rb")
	local lib_data = type_library_file:read("*all")
	local lib_data_size = #lib_data
	dl_error = dl_lib.dl_context_load_type_library(dl_context, lib_data, lib_data_size)
	if dl_error_check(dl_error) == false then return end
	
	local out_dl_context = {
		lib = dl_lib,
		context = dl_context
	}
	return out_dl_context
end

local function _dl_destroy(context)
	local dl_context = context.context
	local dl_lib = context.lib
	dl_lib.dl_context_destroy(dl_context)
end

local function _dl_instance_calcsize(context, instance)
	local type_id = instance.TYPE_ID
	local dl_lib = context.lib
	local instance_size = ffi.new("unsigned int[1]")
	local dl_error = dl_lib.dl_instance_calc_size(context.context, type_id, instance, instance_size)
	
	if dl_error_check(dl_error) == false then return end
	
	return instance_size[0]
end

local function _dl_instance_store(context, instance)
	local dl_context = context.context
	local dl_lib = context.lib

	local instance_size = _dl_instance_calcsize(context, instance)
	local type_id = instance.TYPE_ID
	local dl_context = context.context
	local out_size = ffi.new("unsigned int[1]")
	local out_data = ffi.new("char[?]", instance_size)
	local dl_error = dl_lib.dl_instance_store(dl_context, type_id, instance, out_data, instance_size, out_size)
	
	if dl_error_check(dl_error) == false then return end
	
	local out_string = ffi.string(out_data, instance_size)
	return out_string
end

local function _dl_txt_unpack_calc_size(context, instance, instance_size, packed_data)
	local type_id = instance.TYPE_ID
	local dl_lib = context.lib
	local dl_context = context.context
	local text_size = ffi.new("unsigned int[1]")
	local dl_error = dl_lib.dl_txt_unpack_calc_size(dl_context, type_id, packed_data, instance_size, text_size)
	
	if dl_error_check(dl_error) == false then return end
	
	return text_size[0]
end

local function _dl_txt_unpack(context, instance, instance_size, packed_data, text_size)
	local dl_lib = context.lib
	local dl_context = context.context
	
	local out_char_buff = ffi.new("char[?]", text_size)
	local packed_amount = ffi.new("unsigned int[1]")
	
	dl_error = dl_lib.dl_txt_unpack(dl_context, instance.TYPE_ID, packed_data, instance_size, out_char_buff, text_size, packed_amount)
	if dl_error_check(dl_error) == false then return end
	
	return out_char_buff, packed_amount
end

local function _dl_pack_to_txt(context, instance)
	local dl_context = context.context
	local dl_lib = context.lib
	-- Pack to instance.
	local instance_size = _dl_instance_calcsize(context, instance)
	local type_id = instance.TYPE_ID
	local dl_context = context.context
	local out_size = ffi.new("unsigned int[1]")
	local out_data = ffi.new("char[?]", instance_size)
	local dl_error = dl_lib.dl_instance_store(dl_context, type_id, instance, out_data, instance_size, out_size)
	if dl_error_check(dl_error) == false then return end
	-- Calc txt-size.
	local text_size = _dl_txt_unpack_calc_size(context, instance, instance_size, out_data)
	-- unpack into char-array.
	local packed_txt, packed_amount = _dl_txt_unpack(context, instance, instance_size, out_data, text_size)
	-- Turn into string.
	local out_string = ffi.string(packed_txt, text_size)
	return out_string
end

--- Simple datalibrary-wrapper
-- @author Simon Lundmark
-- @release A simple lua-ffi wrapper for DL.
libdl = {}
--- Initialize datalibrary.
-- @param dynamic_lib Full Path to the dynamic datalibrary library to initialize with.
-- @param type_library The full path to the type_library to start DL with.
-- @return A context-table if successful, nil if failed.
libdl.initialize = _dl_initialize
--- Shut down datalibrary.
-- @param context The dl-context to destroy (as returned by dl.initialize)
libdl.shut_down = _dl_destroy
--- Store a DL-instance to a binary array.
-- @param context The dl-context returned from dl.initialize.
-- @param instance The instance to store.
-- @return The packed instance in a binary array as a lua-string.
libdl.pack = _dl_instance_store
--- Store a DL-instance to a text-string.
-- @param context The dl-context returned from dl.initialize.
-- @param instance The instance to store.
-- @return The instance as a serializable text-string in a json-format.
libdl.pack_to_txt = _dl_pack_to_txt