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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant
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

# Data Library Tests (dltests) - optional, defaulted to also build.
option(DATA_LIBRARY_TESTS "Build dltests." ON)
if (DATA_LIBRARY_TESTS)
	set(GTEST_SRCS
		external/gtest/src/gtest-death-test.cc
		external/gtest/src/gtest-filepath.cc
		external/gtest/src/gtest-matchers.cc
		external/gtest/src/gtest-port.cc
		external/gtest/src/gtest-printers.cc
		external/gtest/src/gtest-test-part.cc
		external/gtest/src/gtest-typed-test.cc
		external/gtest/src/gtest.cc
		external/gtest/src/gtest_main.cc)

	set(GTEST_HDRS
		external/gtest/src/gtest-internal-inl.h
		external/gtest/include/gtest/gtest-death-test.h
		external/gtest/include/gtest/gtest-matchers.h
		external/gtest/include/gtest/gtest-message.h
		external/gtest/include/gtest/gtest-param-test.h
		external/gtest/include/gtest/gtest-printers.h
		external/gtest/include/gtest/gtest-spi.h
		external/gtest/include/gtest/gtest-test-part.h
		external/gtest/include/gtest/gtest-typed-test.h
		external/gtest/include/gtest/gtest.h
		external/gtest/include/gtest/gtest_pred_impl.h
		external/gtest/include/gtest/gtest_prod.h
		external/gtest/include/gtest/internal/gtest-death-test-internal.h
		external/gtest/include/gtest/internal/gtest-filepath.h
		external/gtest/include/gtest/internal/gtest-internal.h
		external/gtest/include/gtest/internal/gtest-param-util.h
		external/gtest/include/gtest/internal/gtest-port-arch.h
		external/gtest/include/gtest/internal/gtest-port.h
		external/gtest/include/gtest/internal/gtest-string.h
		external/gtest/include/gtest/internal/gtest-type-util.h)

	add_library(gtest STATIC ${GTEST_SRCS} ${GTEST_HDRS})
	include_directories(src include tool/dlpack external/gtest/include external/gtest/)

	set(DLTESTS_SRCS
		tests/dl_tests_array.cpp
		tests/dl_tests_base.cpp
		tests/dl_tests_bitfield.cpp
		tests/dl_tests_dl.cpp
		tests/dl_tests_enum.cpp
		tests/dl_tests_error.cpp
		tests/dl_tests_inline_array.cpp
		tests/dl_tests_ptr.cpp
		tests/dl_tests_reflect.cpp
		tests/dl_tests_string.cpp
		tests/dl_tests_struct.cpp
		tests/dl_tests_txt.cpp
		tests/dl_tests_typelib.cpp
		tests/dl_tests_union.cpp
		tests/dl_tests_util.cpp	
	)

	add_executable(dltests ${DLTESTS_SRCS})
	target_link_libraries(dltests PRIVATE gtest data_library)
	include_directories(src include tool/dlpack external/gtest/include local)
endif()


# Data Library Tests (dltests) - optional, defaulted to also build.
option(DATA_LIBRARY_BENCHMARK "Build dlbench." ON)
if (DATA_LIBRARY_BENCHMARK)
	set(DLBENCH_SRCS
		benchmark/dlbench.cpp
	)

	add_executable(dlbench ${DLBENCH_SRCS})
	target_link_libraries(dlbench PRIVATE data_library)
	include_directories(src include build/local)
endif()
