#ifdef __cplusplus
	#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include <gtest/gtest.h>
#include "dl_tests_base.h"
#include <dl/dl_txt.h>
#include <dl/dl_typelib.h>

TYPED_TEST(DLBase, pods)
{
	Pods P1Original = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,  P1.i8);
	EXPECT_EQ(P1Original.i16, P1.i16);
	EXPECT_EQ(P1Original.i32, P1.i32);
	EXPECT_EQ(P1Original.i64, P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	EXPECT_EQ(P1Original.f32, P1.f32);
	EXPECT_EQ(P1Original.f64, P1.f64);
}

TYPED_TEST(DLBase, pods_max)
{
	Pods P1Original = { INT8_MAX, INT16_MAX, INT32_MAX, INT64_MAX, UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX, FLT_MAX, DBL_MAX };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,  P1.i8);
	EXPECT_EQ(P1Original.i16, P1.i16);
	EXPECT_EQ(P1Original.i32, P1.i32);
	EXPECT_EQ(P1Original.i64, P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	EXPECT_FLOAT_EQ(P1Original.f32,   P1.f32);
	EXPECT_DOUBLE_EQ(P1Original.f64,   P1.f64);
}

TYPED_TEST(DLBase, pods_min)
{
	Pods P1Original = { INT8_MIN, INT16_MIN, INT32_MIN, INT64_MIN, 0, 0, 0, 0, FLT_MIN, DBL_MIN };
	Pods P1;
	memset( &P1, 0x0, sizeof(Pods) );

	this->do_the_round_about( Pods::TYPE_ID, &P1Original, &P1, sizeof(Pods) );

	EXPECT_EQ(P1Original.i8,    P1.i8);
	EXPECT_EQ(P1Original.i16,   P1.i16);
	EXPECT_EQ(P1Original.i32,   P1.i32);
	EXPECT_EQ(P1Original.i64,   P1.i64);
	EXPECT_EQ(P1Original.u8,    P1.u8);
	EXPECT_EQ(P1Original.u16,   P1.u16);
	EXPECT_EQ(P1Original.u32,   P1.u32);
	EXPECT_EQ(P1Original.u64,   P1.u64);
	EXPECT_NEAR(P1Original.f32, P1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.f64, P1.f64, 0.0000001f);
}

TYPED_TEST(DLBase, struct_in_struct)
{
	MorePods P1Original = { { 1, 2, 3, 4, 5, 6, 7, 8, 0.0f, 0}, { 9, 10, 11, 12, 13, 14, 15, 16, 0.0f, 0} };
	MorePods P1;
	memset( &P1, 0x0, sizeof(MorePods) );

	this->do_the_round_about( MorePods::TYPE_ID, &P1Original, &P1, sizeof(MorePods) );

	EXPECT_EQ(P1Original.Pods1.i8,    P1.Pods1.i8);
	EXPECT_EQ(P1Original.Pods1.i16,   P1.Pods1.i16);
	EXPECT_EQ(P1Original.Pods1.i32,   P1.Pods1.i32);
	EXPECT_EQ(P1Original.Pods1.i64,   P1.Pods1.i64);
	EXPECT_EQ(P1Original.Pods1.u8,    P1.Pods1.u8);
	EXPECT_EQ(P1Original.Pods1.u16,   P1.Pods1.u16);
	EXPECT_EQ(P1Original.Pods1.u32,   P1.Pods1.u32);
	EXPECT_EQ(P1Original.Pods1.u64,   P1.Pods1.u64);
	EXPECT_NEAR(P1Original.Pods1.f32, P1.Pods1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods1.f64, P1.Pods1.f64, 0.0000001f);

	EXPECT_EQ(P1Original.Pods2.i8,    P1.Pods2.i8);
	EXPECT_EQ(P1Original.Pods2.i16,   P1.Pods2.i16);
	EXPECT_EQ(P1Original.Pods2.i32,   P1.Pods2.i32);
	EXPECT_EQ(P1Original.Pods2.i64,   P1.Pods2.i64);
	EXPECT_EQ(P1Original.Pods2.u8,    P1.Pods2.u8);
	EXPECT_EQ(P1Original.Pods2.u16,   P1.Pods2.u16);
	EXPECT_EQ(P1Original.Pods2.u32,   P1.Pods2.u32);
	EXPECT_EQ(P1Original.Pods2.u64,   P1.Pods2.u64);
	EXPECT_NEAR(P1Original.Pods2.f32, P1.Pods2.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods2.f64, P1.Pods2.f64, 0.0000001f);
}

TYPED_TEST(DLBase, struct_in_struct_in_struct)
{
	Pod2InStructInStruct original;
	Pod2InStructInStruct loaded;

	original.p2struct.Pod1.Int1 = 1337;
	original.p2struct.Pod1.Int2 = 7331;
	original.p2struct.Pod2.Int1 = 1234;
	original.p2struct.Pod2.Int2 = 4321;

	this->do_the_round_about( Pod2InStructInStruct::TYPE_ID, &original, &loaded, sizeof(Pod2InStructInStruct) );

	EXPECT_EQ(original.p2struct.Pod1.Int1, loaded.p2struct.Pod1.Int1);
	EXPECT_EQ(original.p2struct.Pod1.Int2, loaded.p2struct.Pod1.Int2);
	EXPECT_EQ(original.p2struct.Pod2.Int1, loaded.p2struct.Pod2.Int1);
	EXPECT_EQ(original.p2struct.Pod2.Int2, loaded.p2struct.Pod2.Int2);
}

static dl_error_t PatchPods4_PatchFunction( dl_ctx_t dl_ctx, void*, dl_patch_params* patch_params, dl_error_msg_handler error_f, void* error_ctx )
{
	(void)dl_ctx;
	(void)error_f;
	(void)error_ctx;
	if (patch_params->input_type == PodsDefaults_TYPE_ID && patch_params->wanted_type == PatchPods4_TYPE_ID)
	{
		// Do conversion here
		return DL_ERROR_OK;
	}
	return DL_ERROR_TYPE_NOT_FOUND;
}

TEST_F(DL, struct_patch)
{
	dl_patch_func patch_func;
	void* patch_ctx;
	dl_context_get_patch_func( Ctx, &patch_func, &patch_ctx );

	const char typelib[] = {
		#include "generated/unittest2.txt.h"
	};

	const char* text_data = STRINGIFY( { "PodsDefaults" : {} } );
	unsigned char instance[1024];
	EXPECT_DL_ERR_OK(dl_txt_pack(Ctx, text_data, instance, sizeof(instance), 0x0));

	dl_ctx_t target_ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	EXPECT_DL_ERR_OK(dl_context_create( &target_ctx, &p ));
	EXPECT_DL_ERR_OK(dl_context_load_txt_type_library( target_ctx, typelib, sizeof(typelib) - 1 ));

	size_t needed_size;
	uint8_t out_instance[1024];
	dl_patch_params patch_params{ &patch_func, &patch_ctx, 1, true, false, 0, 0, instance, out_instance, sizeof(out_instance), &needed_size };

	// Change order
	patch_params.wanted_type = PatchPods1_TYPE_ID;
	EXPECT_DL_ERR_OK(dl_instance_patch( target_ctx, &patch_params ));
	// Remove and add members
	//patch_params.wanted_type = PatchPods2_TYPE_ID;
	//EXPECT_DL_ERR_OK(dl_instance_patch( target_ctx, &patch_params ));
	// Change types
	patch_params.wanted_type = PatchPods3_TYPE_ID;
	EXPECT_DL_ERR_OK(dl_instance_patch( target_ctx, &patch_params ));
	// Advanced type change
	patch_params.wanted_type = PatchPods4_TYPE_ID;
	EXPECT_DL_ERR_EQ(DL_ERROR_TYPE_NOT_FOUND, dl_instance_patch( target_ctx, &patch_params ));

	patch_func = PatchPods4_PatchFunction;
	patch_params.wanted_type = PatchPods4_TYPE_ID;
	EXPECT_DL_ERR_OK(dl_instance_patch( target_ctx, &patch_params ));

	EXPECT_DL_ERR_OK(dl_context_destroy( target_ctx ));
}
