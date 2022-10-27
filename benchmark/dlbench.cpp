#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_typelib.h>

#include <vector>

#include "ubench.h"

#define DL_ARRAY_LENGTH(arr) (uint32_t)(sizeof(arr)/sizeof(arr[0]))

#include "generated/dlbench.h"

const unsigned char TYPELIB_SRC[] = {
	#include "generated/dlbench.bin.h"
};

struct dlbench
{
	dl_ctx_t ctx;
};

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4834) // discarding return value of function with 'nodiscard' attribute
#endif

UBENCH_F_SETUP(dlbench)
{
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	dl_context_create( &ubench_fixture->ctx, &p );
	dl_context_load_type_library( ubench_fixture->ctx, TYPELIB_SRC, sizeof(TYPELIB_SRC) );
}

UBENCH_F_TEARDOWN(dlbench)
{
	free(ubench_fixture->ctx);
}

/**
 * Helper class to store a scoped txt-instance. 
 */
struct dlbench_txt_instance
{
	template<typename T>
	dlbench_txt_instance(dl_ctx_t ctx, T* inst)
	{
		size_t pack_size = 0;
		size_t txt_size  = 0;

		dl_error_t err;
		// ... pack ...
		err = dl_instance_store( ctx, T::TYPE_ID, inst, 0x0, 0, &pack_size );
		unsigned char* packed_instance = (unsigned char*)malloc( pack_size );
		err = dl_instance_store( ctx, T::TYPE_ID, inst, packed_instance, pack_size, 0x0 );

		err = dl_txt_unpack( ctx, T::TYPE_ID, packed_instance, pack_size, 0x0, 0, &txt_size );
		txt = (char*)malloc( txt_size );
		err = dl_txt_unpack( ctx, T::TYPE_ID, packed_instance, pack_size, txt, txt_size, 0x0 );

		free( packed_instance );

		// TODO: check err
		(void)err;
	}

	~dlbench_txt_instance()
	{
		free(txt);
	}

	char* txt;
};

/**
 * Helper class to allocate a scoped buffer with size to store a text-instance.
 */
struct dlbench_pack_buffer
{
	dlbench_pack_buffer(dl_ctx_t ctx, const char* txt_inst)
	{
		dl_error_t err = dl_txt_pack( ctx, txt_inst, 0x0, 0, &size );
		buffer = (unsigned char*)malloc( size );
		(void)err; // TODO: check err
	}

	~dlbench_pack_buffer()
	{
		free(buffer);
	}

	size_t         size;
	unsigned char* buffer;
};

// testing perf packing a text-instance with a small array of floats
UBENCH_EX_F(dlbench, txt_pack_small_array_fp32)
{
	float data[] = { 1.0f, 2.0f, 3.0f };
	fp32_array inst = { { data, DL_ARRAY_LENGTH( data ) } };

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

// testing perf packing a text-instance with a big array of floats
UBENCH_EX_F(dlbench, txt_pack_big_array_fp32)
{
	std::vector<float>data( 10000 );
	fp32_array inst = { { &data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = (float)i;

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

// testing perf packing a text-instance with a big array of floats that are all 0.0f
UBENCH_EX_F(dlbench, txt_pack_big_array_fp32_zero)
{
	std::vector<float>data( 10000 );
	fp32_array inst = { { &data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = 0.0f;

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

// testing perf packing a text-instance with a big array of small arrays of floats
UBENCH_EX_F(dlbench, txt_pack_big_array_array_fp32)
{
	std::vector<fp32_array>data( 10000 );
	fp32_array_array inst = { { &data[0], (uint32_t)data.size() } };

	for( size_t i = 0; i < data.size(); ++i )
	{
		static float data2[] = { 1.0f, 2.0f, 3.0f };
		inst.arr[i].arr.data = data2;
		inst.arr[i].arr.count = DL_ARRAY_LENGTH(data2);
	}

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

// testing perf packing a text-instance with a big array of strings
UBENCH_EX_F(dlbench, txt_pack_big_array_str)
{
	std::vector<char*> data(10000);
	str_array inst = { { (const char**)&data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = "apa";

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

// testing perf packing a text-instance with a big array of null-strings
UBENCH_EX_F(dlbench, txt_pack_big_array_str_null)
{
	std::vector<char*> data(10000);
	str_array inst = { { (const char**)&data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = 0x0;

	dlbench& f = *ubench_fixture;
	dlbench_txt_instance t( f.ctx, &inst );
	dlbench_pack_buffer  b( f.ctx, t.txt );

	UBENCH_DO_BENCHMARK()
	{
		dl_txt_pack( f.ctx, t.txt, b.buffer, b.size, 0x0 );
	}
}

UBENCH_MAIN();

#ifdef _MSC_VER
# pragma warning(pop)
#endif
