cmake_minimum_required(VERSION 2.8.4)
project(data_library)

# Force 64bit builds.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "Only 64-bit build is supported.")
	return()
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324") # structure was padded due to alignment specifier
endif()

# Common directories for compiler/linker path.
include_directories(src include tool/dlpack)

# Data Library shared library.
set(DATA_LIBRARY_SRCS
	src/dl.cpp
	src/dl_alloc.cpp
	src/dl_convert.cpp
	src/dl_patch_ptr.cpp
	src/dl_reflect.cpp
	src/dl_txt_pack.cpp
	src/dl_txt_read.cpp
	src/dl_txt_unpack.cpp
	src/dl_typelib_read_bin.cpp
	src/dl_typelib_read_txt.cpp
	src/dl_typelib_write_bin.cpp
	src/dl_typelib_write_c_header.cpp
	src/dl_typelib_write_txt.cpp
	src/dl_util.cpp
)

set(DATA_LIBRARY_HDRS
	include/dl/dl.h
	include/dl/dl_convert.h
	include/dl/dl_defines.h
	include/dl/dl_reflect.h
	include/dl/dl_txt.h
	include/dl/dl_typelib.h
	include/dl/dl_util.h

	src/dl_alloc.h
	src/dl_binary_writer.h
	src/dl_internal_util.h
	src/dl_patch_ptr.h
	src/dl_swap.h
	src/dl_txt_read.h
	src/dl_types.h
)

add_library(data_library SHARED ${DATA_LIBRARY_SRCS} ${DATA_LIBRARY_HDRS})

# Data Library Type Library Compiler (dltlc) - optional, defaulted to also build.
option(DATA_LIBRARY_TYPE_LIBRARY_COMPILER "Build the type library compiler (dltlc)." ON)
if (DATA_LIBRARY_TYPE_LIBRARY_COMPILER)
	set(DLTLC_SRCS
		tool/dltlc/dltlc.cpp
		tool/dlpack/getopt/getopt.c
	)

	add_executable(dltlc ${DLTLC_SRCS})
	target_link_libraries(dltlc PRIVATE data_library)
endif()

# Data Library Pack (dlpack) - optional, defaulted to also build.
option(DATA_LIBRARY_PACK "Build dlpack." ON)
if (DATA_LIBRARY_PACK)
	set(DLPACK_SRCS
		tool/dlpack/dlpack.cpp
		tool/dlpack/getopt/getopt.c
	)

	add_executable(dlpack ${DLPACK_SRCS})
	target_link_libraries(dlpack PRIVATE data_library)
endif()
