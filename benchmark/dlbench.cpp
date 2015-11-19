#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_typelib.h>

#include <vector>

#define STRINGIFY( ... ) #__VA_ARGS__
#define DL_ARRAY_LENGTH(Array) (uint32_t)(sizeof(Array)/sizeof(Array[0]))

#if defined( _MSC_VER )
	#include <windows.h>
	inline uint64_t cpu_tick()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return (uint64_t)t.QuadPart;
	}

	inline uint64_t cpu_freq()
	{
		static uint64_t freq = 0;
		if ( freq == 0 )
		{
			LARGE_INTEGER t;
			QueryPerformanceFrequency(&t);
			freq = (uint64_t) t.QuadPart;
		}
		return freq;
	}

	#define CURRENT_FUNCTION_STRING __FUNCSIG__
#else
	#include <time.h>
	#include <unistd.h>
	inline uint64_t cpu_tick()
	{
		timespec start;
		clock_gettime( CLOCK_MONOTONIC, &start );

		return (uint64_t)start.tv_sec * (uint64_t)1000000000 + (uint64_t)start.tv_nsec;
	}

	inline uint64_t cpu_freq() { return 1000000000; }

	#define CURRENT_FUNCTION_STRING __PRETTY_FUNCTION__
#endif

inline float cpu_ticks_to_ms( uint64_t ticks ) { return (float)ticks / (float)(cpu_freq() / 1000 ); }

#include "generated/dlbench.h"

const unsigned char TYPELIB_SRC[] = {
	#include "generated/dlbench.bin.h"
};

static dl_ctx_t dlbench_create_cxt( const unsigned char* tlsrc, size_t tlsrc_size )
{
	dl_ctx_t ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);

	if( dl_context_create( &ctx, &p ) != DL_ERROR_OK )
		return 0x0;

	if( dl_context_load_type_library( ctx, tlsrc, tlsrc_size ) != DL_ERROR_OK )
	{
		dl_context_destroy( ctx );
		return 0x0;
	}

	return ctx;
}

static char* dlbench_pack_instance_to_txt( dl_ctx_t ctx, uint32_t type, void* inst )
{
	size_t pack_size = 0;
	size_t txt_size  = 0;

	dl_error_t err;
	// ... pack ...
	err = dl_instance_store( ctx, type, inst, 0x0, 0, &pack_size );
	unsigned char* packed_instance = (unsigned char*)malloc( pack_size );
	err = dl_instance_store( ctx, type, inst, packed_instance, pack_size, 0x0 );

	err = dl_txt_unpack( ctx, type, packed_instance, pack_size, 0x0, 0, &txt_size );
	char* txt_instance = (char*)malloc( txt_size );
	err = dl_txt_unpack( ctx, type, packed_instance, pack_size, txt_instance, txt_size, 0x0 );

	free( packed_instance );

	// TODO: check err
	(void)err;

	return txt_instance;
}

#define DLBENCH_RUN_START( iters ) \
{ \
	const int _iters = (iters); \
	uint64_t total = 0; \
	uint64_t min_time = 0xFFFFFFFFFFFFFFFF; \
	uint64_t max_time = 0; \
	for( int _iter = 0; _iter < _iters; ++_iter ) \
	{ \
		uint64_t start = cpu_tick(); \

#define DLBENCH_RUN_END \
		uint64_t end = cpu_tick(); \
		uint64_t it_time = end - start; \
		min_time = it_time > min_time ? min_time : it_time; \
		max_time = it_time < max_time ? max_time : it_time; \
		total += it_time; \
	} \
	printf("%s - %d iterations in %f ms\n" \
			"  - avg %f ms\n" \
			"  - min %f ms\n" \
			"  - max %f ms\n" , CURRENT_FUNCTION_STRING, _iters, cpu_ticks_to_ms( total ), cpu_ticks_to_ms( total ) / (double)_iters, cpu_ticks_to_ms( min_time ), cpu_ticks_to_ms( max_time ) ); \
}

struct dlbench_txt_pack_bench
{
	dlbench_txt_pack_bench( uint32_t type, void* inst )
		: ctx(0x0)
		, txt_instance(0x0)
		, pack_buffer_size(0)
		, pack_buffer(0x0)
	{
		ctx = dlbench_create_cxt( TYPELIB_SRC, sizeof(TYPELIB_SRC) );
		txt_instance = dlbench_pack_instance_to_txt( ctx, type, inst );

		dl_error_t err = dl_txt_pack( ctx, txt_instance, 0x0, 0, &pack_buffer_size );
		pack_buffer = (unsigned char*)malloc( pack_buffer_size );
		(void)err; // TODO: check err
	}

	~dlbench_txt_pack_bench()
	{
		free( txt_instance );
		free( pack_buffer );
		dl_context_destroy( ctx );
	}

	dl_ctx_t ctx;
	char* txt_instance;
	size_t pack_buffer_size;
	unsigned char* pack_buffer;
};

static void dlbench_txt_pack_small_array_fp32()
{
	float data[] = { 1.0f, 2.0f, 3.0f };
	fp32_array inst = { { data, DL_ARRAY_LENGTH( data ) } };

	dlbench_txt_pack_bench _b( fp32_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

static void dlbench_txt_pack_big_array_fp32()
{
	std::vector<float>data( 10000 );
	fp32_array inst = { { &data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = (float)i;

	dlbench_txt_pack_bench _b( fp32_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

static void dlbench_txt_pack_big_array_fp32_zero()
{
	std::vector<float>data( 10000 );
	fp32_array inst = { { &data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = 0.0f;

	dlbench_txt_pack_bench _b( fp32_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

static void dlbench_txt_pack_big_array_array_fp32()
{
	std::vector<fp32_array>data( 10000 );
	fp32_array_array inst = { { &data[0], (uint32_t)data.size() } };

	for( size_t i = 0; i < data.size(); ++i )
	{
		static float data2[] = { 1.0f, 2.0f, 3.0f };
		inst.arr[i].arr.data = data2;
		inst.arr[i].arr.count = DL_ARRAY_LENGTH( data2 );
	}

	dlbench_txt_pack_bench _b( fp32_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

static void dlbench_txt_pack_big_array_str()
{
	std::vector<char*> data(10000);
	str_array inst = { { (const char**)&data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = "apa";

	dlbench_txt_pack_bench _b( str_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

static void dlbench_txt_pack_big_array_str_null()
{
	std::vector<char*> data(10000);
	str_array inst = { { (const char**)&data[0], (uint32_t)data.size() } };
	for( size_t i = 0; i < data.size(); ++i ) inst.arr[i] = 0x0;

	dlbench_txt_pack_bench _b( str_array_TYPE_ID, (void*)&inst );

	DLBENCH_RUN_START( 1000 )
		dl_txt_pack( _b.ctx, _b.txt_instance, _b.pack_buffer, _b.pack_buffer_size, 0x0 );
	DLBENCH_RUN_END
}

int main( int argc, const char** argv )
{
	(void)argc; (void)argv;

	// ... dl_txt_pack() benchmarks ...
	dlbench_txt_pack_small_array_fp32();
	dlbench_txt_pack_big_array_fp32();
	dlbench_txt_pack_big_array_fp32_zero();
	dlbench_txt_pack_big_array_array_fp32();
	dlbench_txt_pack_big_array_str();
	dlbench_txt_pack_big_array_str_null();

	return 0;
}
