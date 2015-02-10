# THE DATA LIBRARY.

## About:
The data library, or DL for short, is a library for serialization of data.

## Goals:
load = memcpy + with plus-menu
guaratee complete data when built
error-checks
platform specific data ( convertion between platforms avaliable on all platforms )

## Parts:

generated headers:
	c/c++

type library ( tlc ):
	file used by dl itself to load/store/pack/unpack types.

c-library:
	heart of the library.

dltlc:
	type-library-compiler

dl_pack:
	tool to pack/unpack/convert instances.

bindings:
	python

## Suported types:
int8, int16, int32, int64     - signed integer 8 - 64 bits
uint8, uint16, uint32, uint64 - unsigned integer 8 - 64 bits
bitfield                      - unsigned integer with specified amount of bits ( uint32 example : 2; in c )
fp32, fp64                    - 32 bit and 64 bit floating point value ( float/double in c )
string                        - ascii string
inline-array                  - fixed size array of any type ( defined by dl ( int/uint etc ) or userdefined )
array                         - variable size array of any type ( defined by dl ( int/uint etc ) or userdefined )
pointer                       - pointer to any user-defined type 

planned for the future (maybe) :
utf8                          - utf8 encoded unicode-string
utf16                         - utf16 encoded unicode-string
utf32                         - utf32 encoded unicode-string

## TLD ( Type Library Definition ) format:
"module"-section
"enums"-section
"types"-section
	"type"
		"members"-array
						

## Text data format:
"type"-section
"data"-section
"subdata"-section

## Examples:

The easiest way to work with dl is through the util-functions exposed in dl_util.h.

These functions will solve all things like file-read/write, endianness- and ptr-size conversions etc for you.

### Preparation

structs are defined in a typelibrary like this:

```json
{
	"module" : "example",
	"types" : {
		"my_type" : {
			"members" : [
				{ "name" : "integer", "type" : "uint32" },
				{ "name" : "string",  "type" : "string" },
				{ "name" : "array",   "type" : "uint16[]" }
			]
		}
	}
}
```

Save above code to example.tld and run this through 'dltlc' to generate c-headers and
a typelibrary-definition-file, we will call it example.bin.

```
./dltlc -c example.h -o example.bin example.tld
```

### Create dl-context and load a type-library:

```c
#include <dl/dl.h>

dl_ctx_t create_dl_ctx()
{
	// create and load context
	dl_ctx_t dl_ctx;
	dl_create_params_t create_params;
	DL_CREATE_PARAMS_SET_DEFAULT( create_params );

	dl_context_create( &dl_ctx, &create_params );

	// load typelibrary
	unsigned char* lib_data = 0x0;
	unsigned int lib_data_size = 0;

	read_from_file( "example.bin", &lib_data, &lib_data_size );

	dl_context_load_type_library( dl_ctx, lib_data, lib_data_size );

	return dl_ctx;
}
```

### Store instance to file

```c
#include <dl/dl_util.h>
#include "example.h"

void store_me( dl_ctx_t dl_ctx )
{
	example e;
	e.integer = 1337;
	e.string  = "I like cowbells!";
	uint16 arr[] = { 1, 2, 3, 4, 5, 6 };
	e.array.data  = arr;
	e.array.count = 6;

	dl_util_store_to_file( dl_ctx,
						       example::TYPE_ID,         // type identifier for example-type
						       "path/to/store/to.file",  // file to store to
						       DL_UTIL_FILE_TYPE_BINARY, // store as binary file
						       DL_ENDIAN_HOST,           // store with endian of this system
						       sizeof(void*),            // store with pointer-size of this system
						       &e );                     // instance to store
}
```

### Load instance from file

```c
#include <dl/dl_util.h>
#include "example.h"

void load_me( dl_ctx_t dl_ctx )
{
	example* e;

	dl_util_load_from_file( dl_ctx,
							example::TYPE_ID,         // type identifier for example-type
							"path/to/read/from.file", // file to read
							DL_UTIL_FILE_TYPE_AUTO,   // autodetect if file is binary or text
							(void**)&e,               // instance will be returned here
							0x0 );

	printf( "e->integer = %u\n", e->integer );
	printf( "e->string  = %s\n", e->string );
	for( unsigned int i = 0; i < e->array.count; ++i )
		printf( "e->array[%u] = %u\n", i, e->array.data[i] );

	free( e ); // by default memory for e will be allocated by malloc
}
```

For more in deapth examples, see EXAMPLE-file
