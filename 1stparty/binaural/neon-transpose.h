#pragma once

/*

// 32 bit 4x4 vector transpose using NEON intrinsics
// source : http://tessy.org/wiki/index.php?NEON%A4%C732bit%A4%CE%C5%BE%C3%D6

float32x4_t _v0 = vcvtq_f32_u32(vld1_u8(src_v0));
float32x4_t _v1 = vcvtq_f32_u32(vld1_u8(src_v1));
float32x4_t _v2 = vcvtq_f32_u32(vld1_u8(src_v2));
float32x4_t _v3 = vcvtq_f32_u32(vld1_u8(src_v3));
//    |    3|    2|    1|    0|
// _v0|  0.1| 11.0|100.0|999.0|
// _v1|  0.2| 12.0|101.0|998.0|
// _v2|  0.3| 13.0|102.0|997.0|
// _v3|  0.4| 14.0|103.0|996.0|

float32x4x2_t v01 = vtrnq_f32(_v0, _v1);
float32x4x2_t v23 = vtrnq_f32(_v2, _v3);
//    |    3|    2|    1|    0|
// _v0| 12.0| 11.0|998.0|999.0|
// _v1|  0.2|  0.1|101.0|100.0|
// _v2| 14.0| 13.0|996.0|997.0|
// _v3|  0.4|  0.3|103.0|102.0|

float32x4_t _dst0 = vcombine_f32(vget_low_f32(v01.val[0]), vget_low_f32(v23.val[0]));
float32x4_t _dst1 = vcombine_f32(vget_low_f32(v01.val[1]), vget_low_f32(v23.val[1]));
float32x4_t _dst2 = vcombine_f32(vget_high_f32(v01.val[0]), vget_high_f32(v23.val[0]));
float32x4_t _dst3 = vcombine_f32(vget_high_f32(v01.val[1]), vget_high_f32(v23.val[1]));
//    |    3|    2|    1|    0|
// _v0|996.0|997.0|998.0|999.0|
// _v1|103.0|102.0|101.0|100.0|
// _v2| 14.0| 13.0| 12.0| 11.0|
// _v3|  0.4|  0.3|  0.2|  0.1|

*/

#include <arm_neon.h>

#define _MM_TRANSPOSE4_PS(_v0, _v1, _v2, _v3) \
	{ \
		float32x4x2_t v01 = vtrnq_f32(_v0, _v1); \
		float32x4x2_t v23 = vtrnq_f32(_v2, _v3); \
		_v0 = vcombine_f32(vget_low_f32(v01.val[0]), vget_low_f32(v23.val[0])); \
		_v1 = vcombine_f32(vget_low_f32(v01.val[1]), vget_low_f32(v23.val[1])); \
		_v2 = vcombine_f32(vget_high_f32(v01.val[0]), vget_high_f32(v23.val[0])); \
		_v3 = vcombine_f32(vget_high_f32(v01.val[1]), vget_high_f32(v23.val[1])); \
	}
