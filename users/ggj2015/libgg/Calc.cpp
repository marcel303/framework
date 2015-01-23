#include <cmath>
#include <time.h>
#include "Calc.h"
#include "Debugging.h"

//#define SINCOS_TABLE_SIZE 256
//#define SINCOS_TABLE_SIZE 512
#define SINCOS_TABLE_SIZE 1024

namespace Calc
{
	RNG::Random g_RandomHQ;
	RNG::Random g_RandomHS;
	
	// todo: create a better sin/cos table implementation
	
#if !CALC_HQ_SINCOS
	static float g_SinCosTable[SINCOS_TABLE_SIZE * 2];
	
	static inline int AngleToIndex(float angle)
	{
		const static float radToIndex = SINCOS_TABLE_SIZE / m2PI;
		
		int index = (int)(angle * radToIndex);
		
		index &= SINCOS_TABLE_SIZE - 1;
		
		Assert(index >= 0 && index < SINCOS_TABLE_SIZE);

		return index << 1;
	}
#endif
	
	void Initialize()
	{
		g_RandomHQ = Rand_Create(RNG::RandomType_MT);
		g_RandomHS = Rand_Create(RNG::RandomType_XORSHIFT);
		
		time_t t;
		time(&t);
		srand((unsigned int)t);
		
		g_RandomHQ.Initialize(rand());
		g_RandomHS.Initialize(rand());
		
#if !CALC_HQ_SINCOS
		for (int i = 0; i < SINCOS_TABLE_SIZE; ++i)
		{
			const float angle = m2PI / SINCOS_TABLE_SIZE * i;
			
			g_SinCosTable[i * 2 + 0] = sinf(angle);
			g_SinCosTable[i * 2 + 1] = cosf(angle);
		}
#endif
	}
	
#if !CALC_HQ_SINCOS
	float Sin_Fast(float angle)
	{
		const int index = AngleToIndex(angle);
		
		return g_SinCosTable[index];
	}
	
	float Cos_Fast(float angle)
	{
		const int index = AngleToIndex(angle);
		
		return g_SinCosTable[index + 1];
	}
	
	// todo: tell compiler there is no aliasing between out_Sin and out_Cos (?)
	
	void SinCos_Fast(float angle, float* out_Sin, float* out_Cos)
	{
		const int index = AngleToIndex(angle);
		
		*out_Sin = g_SinCosTable[index];
		*out_Cos = g_SinCosTable[index + 1];
	}
#endif
	
	void RotAxis_Fast(float angle, float* out_Axis)
	{
		angle *= -1.0f;
		
#if 1
		SinCos_Fast(angle, out_Axis + 1, out_Axis + 0);
		SinCos_Fast(angle + mPI2, out_Axis + 3, out_Axis + 2);
#else
		out_Axis[0] = fcosf(angle);
		out_Axis[1] = fsinf(angle);
		out_Axis[2] = fcosf(angle + mPI2);
		out_Axis[3] = fsinf(angle + mPI2);
#endif
	}
	
	void RotAxis(float angle, float* out_Axis)
	{
#if USE_PSPVEC
		/*out_Axis[0] = cosf(-angle);
		out_Axis[1] = sinf(-angle);
		out_Axis[2] = cosf(-angle + mPI2);
		out_Axis[3] = sinf(-angle + mPI2);*/
		out_Axis[0] = sceFpuCos(-angle);
		out_Axis[1] = sceFpuSin(-angle);
		out_Axis[2] = sceFpuCos(-angle + mPI2);
		out_Axis[3] = sceFpuSin(-angle + mPI2);
#else
		out_Axis[0] = cosf(-angle);
		out_Axis[1] = sinf(-angle);
		out_Axis[2] = cosf(-angle + mPI2);
		out_Axis[3] = sinf(-angle + mPI2);
#endif
	}
};
