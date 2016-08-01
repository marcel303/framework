#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "Random.h"

#if defined(WIN32) || defined(IPAD) || !USE_PSPVEC || defined(BBOS)
	#define CALC_HQ_SINCOS 1
#else
	#define CALC_HQ_SINCOS 0
#endif

#if defined(PSP) && USE_PSPVEC
#include <libfpu.h>
#include <libvfpu.h>
#else
#define USE_PSPVEC 0
#endif

// todo: fast angle 2 radians (?)

namespace Calc
{
	extern RNG::Random g_RandomHQ; // High-quality random
	extern RNG::Random g_RandomHS; // High-speed random
	
	const static float mPI = (float)M_PI;
	const static float m2PI = (float)(M_PI * 2.0);
	const static float m4PI = (float)(M_PI * 4.0);
	const static float mPI2 = (float)(M_PI / 2.0);
	const static float mPI4 = (float)(M_PI / 4.0);
	
	void Initialize();
	
#if USE_PSPVEC
	static inline float Sin_Fast(float angle)
	{
		return sceFpuSin(angle);
	}
	static inline float Cost_Fast(float angle)
	{
		return sceFpuCos(angle);
	}
	static inline void SinCos_Fast(float angle, float* out_Sin, float* out_Cos)
	{
		*out_Sin = sceFpuSin(angle);
		*out_Cos = sceFpuCos(angle);
	}
#elif CALC_HQ_SINCOS
	static inline float Sin_Fast(float angle)
	{
		return sinf(angle);
	}
	static inline float Cost_Fast(float angle)
	{
		return cosf(angle);
	}
	static inline void SinCos_Fast(float angle, float* out_Sin, float* out_Cos)
	{
		*out_Sin = sinf(angle);
		*out_Cos = cosf(angle);
	}
#else
	float Sin_Fast(float angle);
	float Cos_Fast(float angle);
	void SinCos_Fast(float angle, float* out_Sin, float* out_Cos);
#endif
	void RotAxis_Fast(float angle, float* out_Axis); // returns a 2x2 rotation matrix (xx, xy, yx, yy)
	void RotAxis(float angle, float* out_Axis); // returns a 2x2 rotation matrix (xx, xy, yx, yy)
	
	inline float Sqrt(float v)
	{
#if USE_PSPVEC
		return __builtin_allegrex_sqrt_s(v);
#else
		return sqrtf(v);
#endif
	}
	
	inline float Sqrt_Fast(float v)
	{
#if USE_PSPVEC
		return __builtin_allegrex_sqrt_s(v);
#else
		return sqrtf(v);
#endif
	}
	
	inline float Sqrt_Fast_0_1(float v); // note: v must be between 0..1 or results will become unpredictable
	
	inline float SqrtRcp(float v)
	{
#if USE_PSPVEC
		return sceFpuRsqrt(v);
#else
		return 1.0f / sqrtf(v);
#endif
	}
	
	inline float SqrtRcp_Fast(float v)
	{
#ifdef IPHONEOS
		float half = 0.5f * v;
		
		int i = *(int*)&v;         // store floating-point bits in integer
		i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
		
		v = *(float*)&i;               // convert new bits into float
		v = v * (1.5f - half * v * v);  // One round of Newton's method
		
		return v;
#elif USE_PSPVEC
		return sceFpuRsqrt(v);
#else
		return 1.0f / sqrtf(v);
#endif
	}
	
	inline int Min(int v1, int v2)
	{
#if USE_PSPVEC
		return __builtin_allegrex_min(v1, v2);
#else
		return v1 < v2 ? v1 : v2;
#endif
	}
	
	inline float Min(float v1, float v2)
	{
#if USE_PSPVEC
		return sceVfpuScalarMin(v1, v2);
#else
		return v1 < v2 ? v1 : v2;
#endif
	}
	
	inline double Min(double v1, double v2)
	{
#if USE_PSPVEC
		return sceVfpuScalarMin(v1, v2);
#else
		return v1 < v2 ? v1 : v2;
#endif
	}

	inline int Mid(int v, int min, int max)
	{
		if (v < min)
			return min;
		if (v > max)
			return max;

		return v;
	}

	inline float Mid(float v, float min, float max)
	{
#if USE_PSPVEC
		return sceVfpuScalarMax(sceVfpuScalarMin(v, max), min);
#else
		if (v < min)
			return min;
		if (v > max)
			return max;

		return v;
#endif
	}
	
	inline int Clamp(int v, int min, int max)
	{
		return Mid(v, min, max);
	}
	
	inline float Clamp(float v, float min, float max)
	{
		return Mid(v, min, max);
	}
	
	inline float Saturate(float v)
	{
		return Mid(v, 0.0f, 1.0f);
	}
	
	inline float Lerp(float v1, float v2, float t)
	{
		return v1 + (v2 - v1) * t;
	}
	
	inline float LerpSat(float v1, float v2, float t)
	{
		t = Saturate(t);
		
		return v1 + (v2 - v1) * t;
	}
	
	// random number generation
	
	inline uint32_t Random()
	{
		return g_RandomHQ.Next();
	}
	
	inline uint32_t Random(int upperExclusive)
	{
		return g_RandomHQ.Next() % upperExclusive;
	}
	
	inline float RandomMin0Max1(uint32_t v)
	{
#if defined(IPHONEOS) && 0
		v = (v & 0x007fffff) | 0x40000000;

		return (*((float*)&v) - 2.0f) * 0.5f;
#else
		return (v & 65535) / 65535.0f;
#endif
	}
	
	inline float RandomMin0Max1()
	{
#if defined(IPHONEOS) && 0
		uint32_t v = (Random() & 0x007fffff) | 0x40000000;

		return (*((float*)&v) - 2.0f) * 0.5f;
#elif USE_PSPVEC
		return sceVfpuRandFloat(1.0f);
#else
		return RandomMin0Max1(Random());
#endif
	}
	
	inline float RandomMin1Max1(uint32_t v)
	{
#if defined(IPHONEOS) && 0
		v = (v & 0x007fffff) | 0x40000000;

		return *((float*)&v) - 3.0f;
#else
		return ((v & 65535) / 65535.0f - 0.5f) * 2.0f;
#endif
	}
	
	inline float RandomMin1Max1()
	{
#if defined(IPHONEOS) && 0
		uint32_t v = (Random() & 0x007fffff) | 0x40000000;

		return *((float*)&v) - 3.0f;
#elif USE_PSPVEC
		return sceVfpuRandFloat(2.0f) - 1.0f;
#else
		return RandomMin1Max1(Random());
#endif
	}
	
	inline float Random(float magnitude)
	{
#if USE_PSPVEC
		return sceVfpuRandFloat(magnitude);
#else
		return RandomMin0Max1() * magnitude;
#endif
	}
	
	inline float Random_Scaled(float magnitude)
	{
#if USE_PSPVEC
		return sceVfpuRandFloat(magnitude * 2.0f) - magnitude;
#else
		return RandomMin1Max1() * magnitude;
#endif
	}
	
	inline int Random(int min, int max)
	{
		return min + Random(max - min + 1);
	}
	
	inline float Random(float min, float max)
	{
#if USE_PSPVEC
		return min + sceVfpuRandFloat(max - min);
#else
		return min + RandomMin0Max1() * (max - min);
#endif
	}
	
	// rounding
	
	inline float RoundUp(float v)
	{
#if USE_PSPVEC
		return __builtin_allegrex_ceil_w_s(v);
#else
		return ceilf(v);
#endif
	}
	
	inline float RoundDown(float v)
	{
#if USE_PSPVEC
		return __builtin_allegrex_floor_w_s(v);
#else
		return floorf(v);
#endif
	}
	
	inline float RoundNearest(float v)
	{
#if USE_PSPVEC
		return __builtin_allegrex_round_w_s(v);
#else
		const float base = RoundDown(v);
		const float rest = v - base;

		if (rest > 0.5f)
			return base + 1.0f;
		else
			return base;
#endif
	}
	
	// misc number operations
	
	inline int Sign(int32_t v)
	{
		// sign extended negative would yield 0xFFFFFFFF. or'd with 1 that's -1
		
		return 1 | (v >> 31);
	}
	
	inline float Sign(float v)
	{
		if (v < 0.0f)
			return -1;
		else
			return +1;
	}

	inline int Abs(int32_t v)
	{
		return (v ^ (v >> 31)) - (v >> 31);
	}
	
	inline float Abs(float v)
	{
#if USE_PSPVEC
		return sceFpuAbs(v);
#else
		if (v < 0.0f)
			return -v;
		else
			return v;
#endif
	}
	
	inline bool IsPowerOf2(int v)
	{
		return !(v & (v - 1));
	}
	
	inline int DivideUp(int v1, int v2) // integer divide with upward rounding
	{
		int result = v1 / v2;
		
		if (result * v2 != v1)
			result++;
		
		return result;
	}
	
	inline float DivideUp(float v1, float v2)
	{
		return RoundUp(v1 / v2);
	}
	
	template <class T>
	inline T Max(T v1, T v2)
	{
		if (v1 > v2)
			return v1;
		else
			return v2;
	}

#if USE_PSPVEC
	inline int Max(int v1, int v2)
	{
		return __builtin_allegrex_max(v1, v2);
	}
#endif

#if USE_PSPVEC
	inline float Max(float v1, float v2)
	{
		return sceVfpuScalarMax(v1, v2);
	}
#endif

	inline float Scale(float t, float min, float max)
	{
		return min + (max - min) * t;
	}

	// angles
	
	const static float deg2rad = 1.745329251994e-02f;
	const static float rad2deg = 57.295779513082322f;
	
	inline float DegToRad(float angle)
	{
		return angle * deg2rad;
	}
	
	inline float RadToDeg(float rad)
	{
		return rad * rad2deg;
	}
	
	// shuffling
	
	template <class T>
	void Shuffle(T* array, int arraySize)
	{
		for (int i = 0; i < arraySize; ++i)
		{
			int newIndex = Random(arraySize);
			
			T temp = array[i];
			array[i] = array[newIndex];
			array[newIndex] = temp;
		}
	}
}
