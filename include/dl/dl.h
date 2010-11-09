/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_H_INCLUDED
#define DL_DL_H_INCLUDED

/*
	File: dl.h
*/

#include <dl/dl_bit.h>
#include <dl/dl_defines.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	Handle: HDLContext
*/
typedef struct SDLContext* HDLContext;

/*
	Enum: EDLError
		Error-codes from DL

	DL_ERROR_OK                                            - All went ok!
	DL_ERROR_MALFORMED_DATA                                - The data was not valid DL-data.
	DL_ERROR_VERSION_MISMATCH                              - Data created with another version of DL.
	DL_ERROR_OUT_OF_LIBRARY_MEMORY                         - Out of memory.
	DL_ERROR_OUT_OF_INSTANCE_MEMORY                        - Out of instance memory.
	DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR  - DL would need to do a dynamic allocation but has now allocator set for that.
	DL_ERROR_TYPE_NOT_FOUND                                - Could not find a requested type. Is the correct type library loaded?
	DL_ERROR_MEMBER_NOT_FOUND                              - Could not find a requested member of a type.
	DL_ERROR_BUFFER_TO_SMALL                               - Provided buffer is to small.
	DL_ERROR_ENDIAN_ERROR                                  - Endianness of provided data is not the same as the platforms.

	DL_ERROR_TXT_PARSE_ERROR                               - Syntax error while parsing txt-file. Check log for details.
	DL_ERROR_TXT_MEMBER_MISSING                            - A member is missing in a struct and in do not have a default value.
	DL_ERROR_TXT_MEMBER_SET_TWICE                          - A member is set twice in one struct.

	DL_ERROR_UTIL_FILE_NOT_FOUND                           - A argument-file is not found.
	DL_ERROR_UTIL_FILE_TYPE_MISMATCH                       - File type specified to read do not match file content.

	DL_ERROR_INTERNAL_ERROR                                - Internal error, contact dev!
*/
enum EDLError
{
	DL_ERROR_OK,
	DL_ERROR_MALFORMED_DATA,
	DL_ERROR_VERSION_MISMATCH,
	DL_ERROR_OUT_OF_LIBRARY_MEMORY,
	DL_ERROR_OUT_OF_INSTANCE_MEMORY,
	DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR,
	DL_ERROR_TYPE_NOT_FOUND,
	DL_ERROR_MEMBER_NOT_FOUND,
	DL_ERROR_BUFFER_TO_SMALL,
	DL_ERROR_ENDIAN_ERROR,
	DL_ERROR_INVALID_PARAMETER,
	DL_ERROR_UNSUPORTED_OPERATION,

	DL_ERROR_TXT_PARSE_ERROR,
	DL_ERROR_TXT_MEMBER_MISSING,
	DL_ERROR_TXT_MEMBER_SET_TWICE,

	DL_ERROR_UTIL_FILE_NOT_FOUND,
	DL_ERROR_UTIL_FILE_TYPE_MISMATCH,

	DL_ERROR_INTERNAL_ERROR
};

/*
	Enum: EDLType
		Enumeration that describes a specific type in DL.
*/
enum EDLType
{
	// Type-layout
	DL_TYPE_ATOM_MIN_BIT            = 0,
	DL_TYPE_ATOM_MAX_BIT            = 7,
	DL_TYPE_STORAGE_MIN_BIT         = 8,
	DL_TYPE_STORAGE_MAX_BIT         = 15,
	DL_TYPE_BITFIELD_SIZE_MIN_BIT   = 16,
	DL_TYPE_BITFIELD_SIZE_MAX_BIT   = 23,
	DL_TYPE_BITFIELD_OFFSET_MIN_BIT = 24,
	DL_TYPE_BITFIELD_OFFSET_MAX_BIT = 31,

	// Field sizes
	DL_TYPE_BITFIELD_SIZE_BITS_USED   = DL_TYPE_BITFIELD_SIZE_MAX_BIT + 1   - DL_TYPE_BITFIELD_SIZE_MIN_BIT,
	DL_TYPE_BITFIELD_OFFSET_BITS_USED = DL_TYPE_BITFIELD_OFFSET_MAX_BIT + 1 - DL_TYPE_BITFIELD_OFFSET_MIN_BIT,

	// Masks
	DL_TYPE_ATOM_MASK            = DL_BITRANGE(DL_TYPE_ATOM_MIN_BIT,            DL_TYPE_ATOM_MAX_BIT),
	DL_TYPE_STORAGE_MASK         = DL_BITRANGE(DL_TYPE_STORAGE_MIN_BIT,         DL_TYPE_STORAGE_MAX_BIT),
	DL_TYPE_BITFIELD_SIZE_MASK   = DL_BITRANGE(DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_MAX_BIT),
	DL_TYPE_BITFIELD_OFFSET_MASK = DL_BITRANGE(DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_MAX_BIT),

	// Atomic types
	DL_TYPE_ATOM_POD          = DL_INSERT_BITS(0x00000000, 1, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1),
	DL_TYPE_ATOM_ARRAY        = DL_INSERT_BITS(0x00000000, 2, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1),
	DL_TYPE_ATOM_INLINE_ARRAY = DL_INSERT_BITS(0x00000000, 3, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1),
	DL_TYPE_ATOM_BITFIELD     = DL_INSERT_BITS(0x00000000, 4, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1),

	// Storage type
	DL_TYPE_STORAGE_INT8   = DL_INSERT_BITS(0x00000000,  1, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_INT16  = DL_INSERT_BITS(0x00000000,  2, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_INT32  = DL_INSERT_BITS(0x00000000,  3, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_INT64  = DL_INSERT_BITS(0x00000000,  4, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_UINT8  = DL_INSERT_BITS(0x00000000,  5, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_UINT16 = DL_INSERT_BITS(0x00000000,  6, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_UINT32 = DL_INSERT_BITS(0x00000000,  7, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_UINT64 = DL_INSERT_BITS(0x00000000,  8, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_FP32   = DL_INSERT_BITS(0x00000000,  9, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_FP64   = DL_INSERT_BITS(0x00000000, 10, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_STR    = DL_INSERT_BITS(0x00000000, 11, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_PTR    = DL_INSERT_BITS(0x00000000, 12, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_STRUCT = DL_INSERT_BITS(0x00000000, 13, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),
	DL_TYPE_STORAGE_ENUM   = DL_INSERT_BITS(0x00000000, 14, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1),

	DL_TYPE_FORCE_32_BIT = 0x7FFFFFFF
};

enum EDLCpuEndian
{
	DL_ENDIAN_BIG,
	DL_ENDIAN_LITTLE,
};

DL_FORCEINLINE EDLCpuEndian dl_endian_host()
{
	union { unsigned char c[4]; unsigned int  i; } test;
	test.i = 0xAABBCCDD;
	return test.c[0] == 0xAA ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE;
}

#define DL_ENDIAN_HOST dl_endian_host()

/*
	Struct: SDLAllocFunctions
*/
struct SDLAllocFunctions
{
	void* (*m_Alloc)(unsigned int _Size, unsigned int _Alignment);
	void  (*m_Free) (void* _pPtr);
};

/*
	Group: Context
*/

/*
	Function: dl_context_create
		Creates a context.

	Parameters:
		_pContext            - Ptr to instance to create.
		_pDLAllocFuncs       - Allocation functions that will be used when allocating context and loaded type-libraries when using this context. 
		                       This memory will be freed by DL.
							   If NULL, malloc/free will be used by default!
		_pInstanceAllocFuncs - Allocation functions that will be used when allocating instance data when using this context. Could be NULL. 
		                       This memory will NOT be freed by DL.
*/
EDLError DL_DLL_EXPORT dl_context_create( HDLContext*        _pContext,
                                          SDLAllocFunctions* _pDLAllocFuncs,
                                          SDLAllocFunctions* _pInstanceAllocFuncs );

/*
	Function: dl_context_destroy
		Destroys a context and free all memory allocated with the DLAllocFuncs-functions.
*/
EDLError DL_DLL_EXPORT dl_context_destroy( HDLContext _Context );

/*
	Function: dl_context_load_type_library
		Load a type-library from bin-data into the context for use.
		One context can have multiple type libraries loaded and reference types within the different ones.

	Parameters:
		_Context  - Context to load type-library into.
		_pData    - Pointer to binary-data with type-library.
		_DataSize - Size of _pData.
*/
EDLError DL_DLL_EXPORT dl_context_load_type_library( HDLContext           _Context,
                                                     const unsigned char* _pData,
                                                     unsigned int         _DataSize );


/*
	Group: Load
*/

/*
	Function: dl_instance_load
		Load instances inplace from a binary blob of data.
		Loads the instance to the memory area pointed to by _pInstance. Will return error if type is not constant size!

	Parameters:
		_Context        - Context to load type-library into.
		_pInstance      - Ptr where to load the instance to.
		_pData          - Ptr to binary data to load from.
		_DataSize       - Size of _pData.

	Note:
		Packed instance to load is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
EDLError DL_DLL_EXPORT dl_instance_load( HDLContext           _Context,
                                         void*                _pInstance,
                                         const unsigned char* _pData,
                                         unsigned int         _DataSize );

/*
	Group: Store
*/
/*
	Function: dl_instace_calc_size
		Calculate size needed to store instance.

	Parameters:
		_Context        - Context to load type-library into.
		_TypeHash       - Type hash for type to store.
		_pInstance      - Ptr to instance to calculate size of.
		_pDataSize      - Ptr where to store the amount of bytes needed to store the instances.
*/
EDLError DL_DLL_EXPORT dl_instace_calc_size( HDLContext    _Context,
                                             StrHash       _TypeHash,
                                             void*         _pInstance,
                                             unsigned int* _pDataSize);

/*
	Function: dl_instace_store
		Store the instances.

	Parameters:
		_Context        - Context to load type-library into.
		_TypeHash       - Type hash for type to store.
		_pInstance      - Ptr to instance to store.
		_pOutBuffer     - Ptr to memory-area where to store the instances.
		_OutBufferSize  - Size of _pData.

	Note:
		The instance after pack will be in current platform endian.
*/
EDLError DL_DLL_EXPORT dl_instace_store( HDLContext     _Context,
                                         StrHash        _TypeHash,
                                         void*          _pInstance,
                                         unsigned char* _pOutBuffer,
                                         unsigned int   _OutBufferSize );


/*
	Group: Util
*/

/*
	Function: dl_error_to_string
		Converts EDLError to string.
*/
DL_DLL_EXPORT const char* dl_error_to_string( EDLError _Err );

/*
	Function: dl_instance_ptr_size
		Return the ptr-size of the instance stored in _pData.

	Parameters:
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.

	Returns:
		The ptr size of stored instance (4 or 8) or 0 on error.
*/
unsigned int dl_instance_ptr_size( const unsigned char* _pData, unsigned int _DataSize );

/*
	Function: dl_instance_endian
		Return the endianness of the instance stored in _pData.

	Parameters:
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.
*/
EDLCpuEndian dl_instance_endian( const unsigned char* _pData, unsigned int _DataSize );

/*
	Function: dl_instance_root_type
		Return the type of the instance stored in _pData.

	Note:
		0 is returned if the data is malformed.

	Parameters:
		_pData       - Ptr to memory-area where packed instance is to be found.
		_DataSize    - Size of _pData.
*/
StrHash dl_instance_root_type( const unsigned char* _pData, unsigned int _DataSize );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_H_INCLUDED
