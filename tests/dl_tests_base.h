#ifndef DL_TESTS_DL_TESTS_BASE_H
#define DL_TESTS_DL_TESTS_BASE_H

#include "dl_test_common.h"

#define EXPECT_INSTANCE_INFO( inst, size, exp_ptr_size, exp_endian, exp_type ) \
{ \
	dl_instance_info_t inst_info; \
	EXPECT_DL_ERR_OK( dl_instance_get_info( inst, size, &inst_info ) ); \
	EXPECT_EQ( exp_ptr_size, inst_info.ptrsize ); \
	EXPECT_EQ( exp_endian,   inst_info.endian ); \
	EXPECT_EQ( exp_type,     inst_info.root_type ); \
}

const unsigned int TEST_REPS       = 1; // number of times to repeate tests internally, use for profiling etc.

struct pack_text_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char** out_buffer,   size_t*     out_size );
};

struct inplace_load_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char** out_buffer,   size_t*     out_size );
};

void convert_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
						 unsigned char* store_buffer,  size_t      store_size,
						 unsigned char** out_buffer,    size_t*     out_size,
						 unsigned int   conv_ptr_size, dl_endian_t conv_endian );

template<unsigned int conv_ptr_size, dl_endian_t conv_endian>
struct convert_test
{

	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
					   unsigned char* store_buffer, size_t      store_size,
					   unsigned char** out_buffer,   size_t*     out_size )
	{
		convert_test_do_it( dl_ctx, type, store_buffer, store_size, out_buffer, out_size, conv_ptr_size, conv_endian );
	}
};

void convert_inplace_test_do_it( dl_ctx_t       dl_ctx,        dl_typeid_t type,
        						 unsigned char* store_buffer,  size_t      store_size,
								 unsigned char** out_buffer,    size_t*     out_size,
								 unsigned int   conv_ptr_size, dl_endian_t conv_endian );

template<unsigned int conv_ptr_size, dl_endian_t conv_endian>
struct convert_inplace_test
{
	static void do_it( dl_ctx_t       dl_ctx,       dl_typeid_t type,
			           unsigned char* store_buffer, size_t      store_size,
			           unsigned char** out_buffer,   size_t*     out_size )
	{
		convert_inplace_test_do_it( dl_ctx, type, store_buffer, store_size, out_buffer, out_size, conv_ptr_size, conv_endian );
	}
};

template <typename T>
struct DLBase : public DL
{
	size_t calculate_unpack_size(dl_typeid_t type, void *pack_me) {
		size_t unpack_me_size = 0;
		dl_instance_calc_size(this->Ctx, type, pack_me, &unpack_me_size);
		return unpack_me_size;
	}

	void do_the_round_about( dl_typeid_t type, void* pack_me, void* unpack_me, size_t unpack_me_size )
	{
		dl_ctx_t dl_ctx = this->Ctx;
		// calc size of stored instance
		size_t store_size = 0;
		EXPECT_DL_ERR_OK(dl_instance_calc_size(dl_ctx, type, pack_me, &store_size));
		unsigned char* store_buffer = (unsigned char*)malloc(store_size+1);
		for( unsigned int i = 0; i < TEST_REPS; ++i )
		{
			memset(store_buffer, 0xFE, store_size+1);
			// store instance to binary
			EXPECT_DL_ERR_OK( dl_instance_store( dl_ctx, type, pack_me, store_buffer, store_size, 0x0 ) );
			EXPECT_INSTANCE_INFO( store_buffer, store_size, sizeof(void*), DL_ENDIAN_HOST, type );
			EXPECT_EQ( 0xFE, store_buffer[store_size] ); // no overwrite on the calculated size plox!

			unsigned char *out_buffer = 0x0;
			size_t out_size;

			// Moved memset out-buffer to inside do_it();, which allocates bytes + 1 and memsets it to 0xfe
			T::do_it( dl_ctx, type, store_buffer, store_size, &out_buffer, &out_size );

			EXPECT_EQ( (unsigned char)0xFE, out_buffer[out_size] ); // no overwrite when packing text plox!
			
			// out instance should have correct format
			EXPECT_INSTANCE_INFO( out_buffer, out_size, sizeof(void*), DL_ENDIAN_HOST, type );

			size_t consumed = 0;
			EXPECT_DL_ERR_OK( dl_instance_load( dl_ctx, type, unpack_me, unpack_me_size, out_buffer, out_size, &consumed ) );

			EXPECT_EQ( out_size, consumed );

			free(out_buffer);
		}
		free(store_buffer);
	}
};

typedef ::testing::Types<
	 pack_text_test
	,inplace_load_test
	,convert_test<4, DL_ENDIAN_LITTLE>
	,convert_test<8, DL_ENDIAN_LITTLE>
	,convert_test<4, DL_ENDIAN_BIG>
	,convert_test<8, DL_ENDIAN_BIG>
	,convert_inplace_test<4, DL_ENDIAN_LITTLE>
	,convert_inplace_test<8, DL_ENDIAN_LITTLE>
	,convert_inplace_test<4, DL_ENDIAN_BIG>
	,convert_inplace_test<8, DL_ENDIAN_BIG>
> DLBaseTypes;
TYPED_TEST_CASE(DLBase, DLBaseTypes);

#endif // DL_TESTS_DL_TESTS_BASE_H
