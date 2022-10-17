# Data Library

The data library, or DL for short, is a library for serialization of data C/C++

---

| Linux GCC/Clang | Windows MSVC | Coverity | Code Coverage |
|-----------------|--------------|----------|---------------|
| [![Build Status](https://travis-ci.org/wc-duck/datalibrary.svg)](https://travis-ci.org/wc-duck/datalibrary) | [![Build status](https://ci.appveyor.com/api/projects/status/caoqg9y6c2vehtaw?svg=true)](https://ci.appveyor.com/project/wc-duck/datalibrary) | [![Build status](https://scan.coverity.com/projects/5151/badge.svg)](https://scan.coverity.com/projects/5151) | [![Coverage Status](https://coveralls.io/repos/github/wc-duck/datalibrary/badge.svg?branch=master)](https://coveralls.io/github/wc-duck/datalibrary?branch=master) |

---

## Goals of this Library

Provide a way to serialize data from a "json-like" format into binary and back where the binary format is binary-compatible with the structs generated by a standard C/C++ compiler. By doing, loading data from a binary blob is a single memcpy + patching pointers.

DL also provides ways to convert serialized blobs between platforms that differ in endianness and/or pointer-size.

Finally, by using a user-defined schema, hereafter referred to as a TLD, packing "json" to binary perform type-checks, range-checks etc on the input data.

## Building

The recommended way of using DL in your own code-base is to incorporate the source straight into your own codebase. All the cpp files under the src folder can be included directly into the source of your own project.

Optionally Bam or CMake can be used to build Data Library (more information on these options later).

Here is some additional information about the source code in this repository.

```
src/
  *.cpp        - Contains the bulk of the library that is used both runtime and by the compilers.
                 Suggestion is to build src/*.cpp into a library.
tool\
  dltlc\
    dltlc.cpp  - This is the source to the typelib-compiler, also called dltlc. Suggestion
                 is to build this as an exe and link to the above built lib.
  dlpack\
    dlpack.cpp - This is a tool that wraps the lib built from src/ to perform json -> binary,
                 binary -> json and inspection of binary instances.
```

## The Workflow of Data Library

1. Define Type Library definitions for your structures (.tlc files). tlc files are used by Data Library load/store types.
2. Run dltlc (Data Library Type Library Compiler) to compile .tlc files. This step outputs all pieces necessary to load or store data at run-time.
3. During run-time the generated header and functions from dl.h or dl.utils.h can be used to perform load/store operations.

### The Various Parts of Data Library

* **load**: read an instance from packed form into memory
* **store**: write an instance to packed form from memory
* **pack**: pack txt/json instance to packed format
* **unpack**: unpack from packed format to txt/json
* generated C/C++ headers from dltlc
* **type library (tlc)**: the file format used by Data Library itself to load/store/pack/unpack types.
* **C-library**: data library itself, all the .cpp files in the src directory.
* **dltlc**: data library type-library-compiler
* **dl_pack**: tool to pack/unpack/convert instances to/from JSON.
* **bindings**: python, lua

## Supported POD-Types in Defined Structs

| name           | tld-type                     | C-storage                             | comment                                                         |
|----------------|------------------------------|---------------------------------------|-----------------------------------------------------------------|
| signed int     | int8, int16, int32, int64    | int8_t, int16_t, int32_t, int64_t     |                                                                 |
| unsigned int   | int8, uint16, uint32, uint64 | uint8_t, uint16_t, uint32_t, uint64_t |                                                                 |
| floating point | fp32, fp64                   | float/double                          |                                                                 |
| bitfield       | bitfield:<bits>              | uint32_t bf : 3                       | unsigned integer with specified amount of bits                  |
| string         | string                       | const char*                           | utf8 encoded, null-terminated                                   |
| inline_array   | int8[20], some_type[20]      | type[20]                              | fixed length, type can be any POD-type or user defined struct   |
| array          | int8[], some_type[]          | struct { type* data; uint32_t count } | dynamic length, type can be any POD-type or user defined struct |
| pointer        | some_type*                   | type*                                 | pointer to any user-defined type                                |

## DL-JSON

All text-data in DL, type-libraries and instances, is simply JSON (Javascript Notation Object), with some small nuances. This is called 
DL-JSON (or dl-json, standing for Data Library JSON).

* Valid JSON IS valid DL-JSON.
* DL-JSON support comments // and /**/.
* DL JSON can use '' or "" for strings/keys.
* DL-JSON '' or "" is optional on keys.
* Keys need to be valid c-identifiers.
* Comma `,` is accepted for the last item in JSON-array and JSON-map.
* New-line in strings are valid.

### Integers

Integers in DL-JSON can be specified in a few more ways than what is valid in ordinary json. All these formats are valid for all sized/signedness of ints
and DL will always error out if the value does not fit the type.

* normal integers, such as 1, -2, 1337 etc.
* hexadecimal integers, such as 0x0, 0xDEADBEEF, -0x123
* binary literals, such as 0b1001 and -0b001101
* constants `min` and `max` whose value depend on size/signedness of integer.
* constants `true` and `false` resulting in 0 and 1

### Floating point values.

Floating point values in DL-JSON can be specified in a few more ways than what is valid in ordinary json. All these formats are valid for both 32 and 64-bit floats 
and DL will always error out if the value do not fit the type.

* normal floats, such as 1.0, -2.0, 123.456
* scientific float notation, such as 3.2e+12, 1.2e-5
* constants `inf`, `infinity` and `nan` ( ignoring case! )

## TLD (Type Library Definition) Format

Type-libs in text-format follows this format.

```javascript
{
    // "module" is unused and deprecated.
    "module" : "unit_test",

    // "c_includes" is a list of includes to output to the generated c-header. DL will not do anything more that save and
    // write these to the header as '#include "../../tests/dl_test_included.h"' or '#include <some/system/header.h>'
    // if the specified string starts with '<'.
    "c_includes" : ["../../tests/dl_test_included.h", 
                    "<some/system/header.h>"],

    // A typelibrary can have any number of "enums" sections. In each of these sections enum-types are specified.
    // An enum is just the same thing as in c/c++ a collection of integer-values.
    "enums" : {
        // each key in "enums" declare an enum with that name, in this case an enum called "my_enum"
        "my_enum" : {
            // an enum can be declared "extern", declaring a type extern will make it NOT be generated in .h-files and
            // the actual definition is expected to be found in "other ways" ( from other includes most likely ).
            // dl will however generate static_assert():s to check that all values are set correctly and exist.
            // Defaults to `false` if not set
            "extern" : true,

            // "type" specifies the storage-type of the generated enum. The storage type can be any signed or unsigned 
            // integer type supported by dl.
            // Defaults to `uint32` if not set.
            "type"   : "int64",

            "comment": "my_enum is an externally declared enumeration and have 7 values",,

            // "metadata" has a list of dl instances which are attached to the enumeration
            "metadata": [ { "my_type" : { "member": 1 } } ]

            // "values" are the actual values of the enum-type as a dict.
            "values" : {
                // ... a value can be specified as a simple integer ...
                "MY_VALUE" : 3,

                // ... or it can be specified in hex ...
                "MY_HEX_VALUE" : 0x123,

                // ... or as a binary literal, maybe use more of them ...
                "BIT1"       : 0b000001,
                "BIT2"       : 0b000010,
                "BIT3"       : 0b000100,
                "BIT1_AND_2" : 0b000011,

                // ... or like this in special cases where you want to use "aliases".
                "MY_ALIASED_VALUE" : {
                    "value"   : 5,

                    // these aliases + the enum-name will map to this enum value when parsed in the text-format.
                    // In this case writing "MY_ALIASED_VALUE", "apa" or "kossa" would all result in a
                    // MY_ALIASED_VALUE when packed to binary.
                    // it was added as to shorten the text-format and still keep more descriptive names
                    // in the generated headers.
                    "aliases" : ["apa", "kossa"],
                    
                    "comment": "This enumerator has aliases",

                    // "metadata" has a list of dl instances which are attached to the enumerator
                    "metadata": [ { "my_type" : { "member": 1 } } ]
                }
            }
        }
    },

    // A typelibrary can have any number of "types" sections. In each of these sections types are specified.
    // A type will result in a `struct` in the generated .h-files and is the top-most building-block in dl.
    "types" : {
        // each key in "types" declare a type with that name, in this case an enum called "my_type"
        "my_type" : {
            // a type can be declared "extern", declaring a type extern will make it NOT be generated in .h-files and
            // the actual definition is expected to be found in "other ways" ( from other includes most likely ).
            // dl will however generate static_assert():s to check that sizeof(), alignment, member-names, member-offsets
            // etc matches.
            // Defaults to `false` if not set
            "extern"  : false,

            // if a type was specified as extern you can omit the generated static_assert() by setting "verify" to true.
            // this is mostly useful in cases where we can't generate all checks. Such as when using namespaces, having
            // private data-members etc.
            // Defaults to `false` if not set
            "verify"  : false,

            // "align" can be used to force the alignment of a type.
            // Defaults to the types "natural alignment" if not set.
            "align"   : 128

            // "comment" will be output in the .h-files for this specified type.
            // If not set, no comment will be written to .h
            "comment" : "this type is a nice type!",

            // "metadata" has a list of dl instances which are attached to the type
            "metadata": [ { "my_type" : { "member": 1 } } ],

            // "members" is a list of all members of this type.
            "members" : [
                // one {} per member and the members will be ordered as they are in this list.
                {
                    // "name" of the member, will be used to generate member-name in .h, used in txt-format, used by reflection etc.
                    // Required field.
                    "name"    : "member",

                    // "type" of the member, this can be any of the types listed in "Supported POD-Types in Defined Structs" under
                    // "tld-type".
                    // Required field.
                    "type"    : "int32",

                    // "default" is a value to use, while packing text-data, if this member was not specified. Works with all types
                    // including arrays. However pointers can only be defaulted to `null`
                    // If not set the member will be a required field when packing text.
                    "default" : 0,

                    // "verify" works in the same way as on the type, but only for this member.
                    "verify"  : false,

                    // "comment" works in the same way as on the type, but only for this member.
                    "comment" : "only used in unittests to check for errors",

                    // "const" adds the const type modifier to this member in the generated header file
                    "const" : true,

                    // "metadata" has a list of dl instances which are attached to the member
                    "metadata": [ { "my_type" : { "member": 1 } } ]
                }
            ]
        }
    },

    // A typelibrary can have any number of "types" sections. In each of these sections unions are specified.
    // A union-types is a bit special as in how they are packed. The use the same format as "types" to specify members etc
    // but will generate a struct containing one .type-member and one .value that is a union where .type specifies what
    // union-value is currently valid. See separate section on 'union'-types.
    "unions" : {
        // see doc for "types" as they are the same.
        "my_union" : {
            "members" : [
                { "name" : "an_int",  "type" : "int32", },
                { "name" : "a_float", "type" : "fp32", }
            ]
        }
    },
}
```

## Instance Text Format

Instances in text-format follow this format.

```javascript
{
    // root level always contains one key that is the type of the root object of the data.
    // In this case we would have to have a type called 'my_type' in the typelibrary used
    // to work with this data.
    "my_type" : {
        // each member is represented by a key with the name of the member in 'my_type'.

        "int_member1"  : 123,        // integers are written as integers ...
        "int_member2"  : 0xFE,       // ... or in hex ...
        "int_member2"  : 0b11001,    // ... or as a binary literal ...
        "int_member3"  : min,        // ... or as a constant, supported are (+-)min/max which value depends on the member-type.

        "float_member1" : 1.2,       // fp32/fp64 are written as floats ...
        "float_member2" : infinity,  // ... or as a constant, supported are (+-)infinity/nan which value depends on the member-type.

        "bitfield_member" : 234,     // bitfields follow the same rules as ints.

        // strings are just strings ...
        "string_member1" : "some string"
        "string_member2" : "where new
lines are
left untouched!",
        "string_member3" : "strings also support escapes like \n\r\t\b\"",
        "string_member3" : 'and you can invert the char used to use " directly!',

        // inline arrays are just arrays with the correct type ...
        "inline_array_member1"  : [1, 2, 3],

        // ... it could be a struct with x and y members.
        "inline_array_member2"  : [
            {
                "x" : 1,
                "y" : 2
            },
            {
                "x" : 3,
                "y" : 4
            },
        ],

        // arrays have the exact same rules and format as inline array.
        "array_member" : [1, 2, 3],

        // pointers use name to refer into a sub-section of the data and will be turned into a pointer after load.
        "pointer_member" : "my_pointer1",

        // a member that is struct of another type is just parsed as the root-instance...
        "my_struct_member1" : {
            "x" : 1,
            "y" : 2
        },

        // struct can also be parsed from arrays if it is following some specific rules. This was added to make
        // simple structs, such as vec3 and matrix easier to write.
        // The rules says that members will be parsed in the order that they were defined in the type if members
        // are left out they must be at the end of the struct and have a default-value.
        "my_struct_member2" : [ 1, 2 ],
    },

    // __subdata is where the contents of structs pointed to by "pointers" are stored.
    "__subdata" : {
        // this is the instance pointed to by "my_pointer1", it is parsed in the same way as the root-instance and the type is deduced from the
        // member pointing to it.
        "my_pointer1" : {
            "some_int"       : 1,
            "my_sub_pointer" : "another_sub_data"
        },

        "another_sub_data" : {
            "some_float" : 1.23
        }
    }
}
```

## Examples

The easiest way to work with dl is through the util-functions exposed in dl_util.h.

These functions will solve all things like file-read/write, endianness- and ptr-size conversions etc for you.

### Preparation

structs are defined in a typelibrary like this:

```json
{
	"types" : {
		"data_t" : {
			"members" : [
				{ "name" : "id", "type" : "string" },
				{ "name" : "x", "type" : "fp32" },
				{ "name" : "y", "type" : "fp32" },
				{ "name" : "array", "type" : "uint32[]" }
			]
		}
	}
}
```

Save above code to example.tld and run this through `dltlc` to generate an optional C-header and
a `tld` file called example_tld.bin. `dltlc` must be run twice, like so.

1. `dltlc -o example_tld.bin example.tld`
2. `dltlc -c -o example_dl.h example.tld`

### Create DL Context and Load a Type Library

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

	read_from_file( "example_tld.bin", &lib_data, &lib_data_size );

	dl_context_load_type_library( dl_ctx, lib_data, lib_data_size );

	return dl_ctx;
}
```

### Store Instance to File

```c
#include <dl/dl_util.h>
#include "example_tld.h"

void store_me( dl_ctx_t dl_ctx )
{
	uint16 arr[] = { 1, 2, 3, 4, 5, 6 };

	data_t instance_data;
	instance_data.id = "Data Identifier.";
	instance_data.x = 10.0f;
	instance_data.y = 15.0f;
	instance_data.array.data = arr;
	instance_data.array.count = 6;

	dl_util_store_to_file( dl_ctx,
	                       data_t::TYPE_ID,          // type identifier for example-type
	                       "path/to/store/to.file",  // file to store to
	                       DL_UTIL_FILE_TYPE_BINARY, // store as binary file
	                       DL_ENDIAN_HOST,           // store with endian of this system
	                       sizeof(void*),            // store with pointer-size of this system
	                       &instance_data );         // instance to store
}
```

### Load instance from file

```c
#include <dl/dl_util.h>
#include "example_tld.h"

void load_me( dl_ctx_t dl_ctx )
{
	data_t* instance_data;

	dl_util_load_from_file( dl_ctx,
	                        data_t::TYPE_ID,          // type identifier for example-type
	                        "path/to/read/from.file", // file to read
	                        DL_UTIL_FILE_TYPE_AUTO,   // autodetect if file is binary or text
	                        (void**)&instance_data,   // instance will be returned here
	                        0x0 );

	printf( "instance_data->id  = %s\n", instance_data->id );
	printf( "instance_data->x = %f\n", instance_data->x );
	printf( "instance_data->x = %f\n", instance_data->y );
	for( unsigned int i = 0; i < instance_data->array.count; ++i )
		printf( "instance_data->array[%u] = %u\n", i, instance_data->array.data[i] );

	// Prints:
	// instance_data->id = Data Identifier
	// instance_data->x = 10.000000
	// instance_data->y = 15.000000
	// instance_data->array[%u] = 1
	// instance_data->array[%u] = 2
	// instance_data->array[%u] = 3
	// instance_data->array[%u] = 4
	// instance_data->array[%u] = 5
	// instance_data->array[%u] = 6

	free( instance_data ); // by default memory for instance_data will be allocated by malloc
}
```

## Overriding default behavior

DL supports some customization of its internal by defining your own config-file that override any of the 
defaults that DL provides.

This is picked up in DL by including `DL_USER_CONFIG`. This can be defined either in code or via the
command-line.

```c
#define DL_USER_CONFIG "my_dl_config.h"
```

> Note: It's really important that all build-targets using dl is compiled with the same DL_USER_CONFIG,
>       at least IF overriding the hash-functions as hashes must be consistent between compile and load.

This allows the user to configure how DL handles:

### Asserts

DL exposes 2 macros that can be overridden for asserts, DL_ASSERT(expr) and DL_ASSERT_MSG(expr, fmt, ...)
where the first one is a bog-standard assert with the same api as assert() and DL_ASSERT_MSG support
assert-messages with printf-style string and arguments.
As a user you can opt in to override none, one or both of these.
If you implement none of them the standard assert() from assert.h will be used. If only one is implemented
the other one will be implemented with the other.

```c
#define DL_ASSERT(expr) MY_SUPER_ASSERT(expr)

#define DL_ASSERT_MSG(expr, fmt, ...) if(expr) { printf("assert was hit: " fmt, ##__VA_ARGS__ ); DEBUG_BREAKPOINT; }
```

### Hash-function

DL also allows the user to override what hash-function is used internally. This might be useful if you
want to use the same one throughout your code, track what hashes are generated and maybe other things.

Two functions are exposed for the user to override, DL_HASH_BUFFER(buffer, byte_count) and 
DL_HASH_STRING(str). You are allowed to override None, only DL_HASH_BUFFER or both.
If DL_HASH_STRING is not defined it will be implemented via DL_HASH_BUFFER + strlen().

Important to note here is that DL_HASH_BUFFER(str, strlen(str)) is required to be the same as 
DL_HASH_STRING().

The 2 macros must expand to something converting to an uint32_t in the end.

If none of the macros are defined, dl will use a fairly simple default hash-function.

```c
// override with murmurhash3

namespace mmh3 {
    #include "MurmurHash3.inc"
}

static inline my_murmur(void* buffer, unsigned int len)
{
	uint32_t res;
	mmh3::MurmurHash3_x86_32( buffer, len, 1234, &res );
	return res;
}

#define DL_HASH_BUFFER(buf, len) my_murmur(buf, len)
#define DL_HASH_STRING(str)      my_murmur(str, strlen(str))
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

A basic CMakelists.txt file is included to build a shared library, along with `dltlc` and `dlpack`. CMake is not *officially* supported by the original Data Library author, but is included here by a contributor for convenience.

See `run_cmake_msvs_2017.bat` for an example of running CMake on Windows for MSVS2017.

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
