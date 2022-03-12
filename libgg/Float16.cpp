#include "Float16.h"

void toFloat16_slow(const float * __restrict values, const int numValues, Float16 * __restrict out_values)
{
	for (int i = 0; i < numValues; ++i)
	{
		out_values[i] = toFloat16(values[i]);
	}
}

#if defined(__aarch64__)

#include <arm_neon.h>

void toFloat16(const float * __restrict values, const int numValues, Float16 * __restrict out_values)
{
	const int numValues_4 = numValues >> 2;
	
	const float32x4_t * __restrict     values_4 = (const float32x4_t*)    values;
	      float16x4_t * __restrict out_values_4 = (      float16x4_t*)out_values;
	
	int i;
	
	for (i = 0; i < numValues_4; ++i)
	{
		out_values_4[i] = vcvt_f16_f32(values_4[i]);
	}
	
	i <<= 2;
	
	if (i != numValues)
	{
		toFloat16_slow(values + i, numValues - i, out_values + i);
	}
}

#else

void toFloat16(const float * __restrict values, const int numValues, Float16 * __restrict out_values)
{
	toFloat16_slow(values, numValues, out_values);
}

#endif

