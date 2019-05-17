# Data Library

The data library, or DL for short, is a library for serialization of data c/c++.

---

| Linux GCC/Clang | Windows MSVC | Coverity | Code Coverage |
|-----------------|--------------|----------|---------------|
| [![Build Status](https://travis-ci.org/wc-duck/datalibrary.svg)](https://travis-ci.org/wc-duck/datalibrary) | [![Build status](https://ci.appveyor.com/api/projects/status/caoqg9y6c2vehtaw?svg=true)](https://ci.appveyor.com/project/wc-duck/datalibrary) | [![Build status](https://scan.coverity.com/projects/5151/badge.svg)](https://scan.coverity.com/projects/5151) | [![Coverage Status](https://coveralls.io/repos/github/wc-duck/datalibrary/badge.svg?branch=master)](https://coveralls.io/github/wc-duck/datalibrary?branch=master) |

---

## Goals of this Library

Provide a way to serialize data from a "json-like" format into binary and back where the binary format is binary-compatible with the structs generated by a standard c/c++ compiler. By doing, loading data from a binary blob is a single memcpy + patching pointers.

DL also provides ways to convert serialized blobs between platforms that differs in endianess and/or pointer-size.

Finally, by using a user-defined schema, here after referred to as a TLD, packing "json" to binary perform type-checks, range-checks etc on the input data.

## Building

The recommended way of using DL in your own code-base is to incorporate the source straight into your own codebase. All the cpp files under the src folder can be included directly into the source of your own project.

Optionally Bam or CMake can be used to build Data Library (more information on these options later).

Here is some additional information about the source code in this repository.

```
src/
  *.cpp - contains the bulk of the library that is used both runtime and by the compilers.
          Suggestion is to build src/*.cpp into a library.
tool\
  dltlc\
    dltlc.cpp - this is the source to the typelib-compiler, also called dltlc. Suggestion
	            is to build this as an exe and link to above built lib.
  dlpack\
    dlpack.cpp - this is a tool that wraps the lib built from src/ to perform json -> binary,
	             binary -> json and inspection of binary instances.
```

## The Worklow of Data Library

1. Define Type Library definitions for your structures (.tlc files). tlc files are used by Data Library load/store types.
2. Run dltlc (Data Library Type Library Compiler) to compile .tlc files. This step outputs all pieces necessary to load or store data at run-time.
3. During run-time the generated header and functions from dl.h or dl.utils.h can be used to perform load/store operations.

### The Various Parts of Data Library

* load: read an instance into memory
* store: write an instance to binary form
* pack: transform from run-time binary form to ready-for-disk binary form
* unpack: transform from ready-for-disk binary form to run-time form
* generated c/c++ headers from dltlc
* type library (tlc): the file format used by Data Library itself to load/store/pack/unpack types.
* c-library: data library itself, all the .cpp files in the src directory.
* dltlc: data library type-library-compiler
* dl_pack: tool to pack/unpack/convert instances.
* bindings: python, lua

## Suported pod-types in defined structs

| type          | c-storage                             | comment                                                        |
|---------------|---------------------------------------|----------------------------------------------------------------|
| signed ints   | int8_t, int16_t, int32_t, int64_t     |                                                                |
| unsigned ints | uint8_t, uint16_t, uint32_t, uint64_t |                                                                |
| bitfield      | uint32_t bf : 3                       | unsigned integer with specified amount of bits                 |
| fp32, fp64    | float/double                          |                                                                |
| string        | const char*                           | utf8 encoded, nul-terminated                                   |
| inline-array  | type[20]                              | fixed length, type can be any POD-type or userdefined struct   |
| array         | struct { type* data; uint32_t count } | dynamic length, type can be any POD-type or userdefined struct |
| pointer       | type*                                 | pointer to any user-defined type                               |

## TLD ( Type Library Definition ) format
"module"-section
"enums"-section
"types"-section
	"type"
		"members"-array

## Text data format

"type"-section
"data"-section
"subdata"-section

## Examples

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

## Other Build Options

### Building with Bam

Bam is officially supported by the original author of Data Library. If you want to build the library with bam, as is done during development and on autobuilders, these are the steps needed.

Run script/bootstrap.sh or script/bootstrap.bat depending on your platform, this will only have to be done once.
	- these scripts will expect there to be a compiler and git in your PATH ( 'gcc' or 'clang' on 'unix', 'msvc' on windows )
	- after this step there should be a local/bam/bam(.exe)

Run `local/bam/bam platform=<platform> config=<config> compiler=<compiler> -r sc`
	- this will build all target for the specified platform and configuration into `local/<platform>/<compiler>/<config>`

	- platforms:
		* linux_x86    - 32bit linux x86 build.
		* linux_x86_64 - 64bit linux x86 build.
		* win32        - 32bit windows build.
		* winx64       - 64bit windows build.

	- configs
		* debug        - non-optimized build
		* release      - optimized build
		* sanitizer    - sanitizer build
		* coverage     - special config used for code-coverage on autobuilders

	- compilers:
		* gcc          - build with gcc ( linux only as of now )
		* clang        - build with gcc ( linux only as of now )
		* msvs14       - build with msvc14 ( windows only )
		* msvs10       - build with msvc10 ( windows only )
		* msvs8        - build with msvc8 ( windows only )

> Note:
> msvc14/msvc10/msvc8 compiler will be replaced with only msvc later and use the cl.exe/link.exe found in PATH

You can also build specific 'targets' with `local/bam/bam platform=<platform> config=<config> compiler=<compiler> -r sc <target>`

	- valid targets
		* 'test'          - build unittests and all its dependencies + run tests
		* 'test_valgrind' - build unittests and all its dependencies + run tests under valgrind
		* 'test_gdb'      - build unittests and all its dependencies + run tests under gdb
		* 'benchmark'     - build benchmark and all its dependencies + run benchmarks

> when running any of the 'test*' targets you can also specify 'test_filter=***' to pass a filter on what tests to run to the executable.

### Building with CMake

A basic CMakelists.txt file is included to build a shared library, along with `dltlc` and `dlpack`. CMake is not *officially* supported by original Data Library author, but is included here by a contributor for convenience.

## License

```
  the data library
  version 0.1, september, 2010

  Copyright (C) 2010- Fredrik Kihlander

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Fredrik Kihlander
```