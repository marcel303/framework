#include <chrono>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// define types

typedef float float4_value __attribute__ ((vector_size(16)));

struct float4
{
	float4_value value;
	
	float4()
	{
	}
	
	explicit float4(const int in_f)
	{
		const float f = (float)in_f;
		
		value = float4_value { f, f, f, f };
	}
	
	explicit float4(const float f)
	{
		value = float4_value { f, f, f, f };
	}
	
	explicit float4(const float f1, const float f2, const float f3, const float f4)
	{
		value = float4_value { f1, f2, f3, f4 };
	}
	
	explicit float4(const double in_f)
	{
		const float f = (float)in_f;
		
		value = float4_value { f, f, f, f };
	}
	
	explicit float4(float4_value in_value)
	{
		value = in_value;
	}
	
	explicit float4(const float * __restrict f)
	{
		value = float4_value { f[0], f[1], f[2], f[3] };
	}
	
	float4 operator+(const float4 other) const
	{
		return float4(value + other.value);
	}
	
	float4 operator-(const float4 other) const
	{
		return float4(value - other.value);
	}
	
	float4 operator*(const float4 other) const
	{
		return float4(value * other.value);
	}
	
	float4 operator/(const float4 other) const
	{
		return float4(value / other.value);
	}
	
	float4 operator*(const float other) const
	{
		return *this * float4(other);
	}
	
	float4 operator*(const int other) const
	{
		return *this * float4(other);
	}
};

typedef float float8_value __attribute__ ((vector_size(32)));

struct float8
{
	float8_value value;
	
	float8()
	{
	}
	
	explicit float8(const int in_f)
	{
		const float f = (float)in_f;
		
		value = float8_value { f, f, f, f, f, f, f, f };
	}
	
	explicit float8(const float f)
	{
		value = float8_value { f, f, f, f, f, f, f, f };
	}
	
	explicit float8(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8)
	{
		value = float8_value { f1, f2, f3, f4, f5, f6, f7, f8 };
	}
	
	explicit float8(const double in_f)
	{
		const float f = (float)in_f;
		
		value = float8_value { f, f, f, f, f, f, f, f };
	}
	
	explicit float8(float8_value in_value)
	{
		value = in_value;
	}
	
	explicit float8(const float * __restrict f)
	{
		value = float8_value { f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7] };
	}
	
	float8 operator+(const float8 other) const
	{
		return float8(value + other.value);
	}
	
	float8 operator-(const float8 other) const
	{
		return float8(value - other.value);
	}
	
	float8 operator*(const float8 other) const
	{
		return float8(value * other.value);
	}
	
	float8 operator/(const float8 other) const
	{
		return float8(value / other.value);
	}
	
	float8 operator*(const float other) const
	{
		return *this * float8(other);
	}
	
	float8 operator*(const int other) const
	{
		return *this * float8(other);
	}
};

namespace std
{
	float4 tan(const float4 value)
	{
		const float * valueElements = (float*)&value;
		
		return float4(
			tanf(valueElements[0]),
			tanf(valueElements[1]),
			tanf(valueElements[2]),
			tanf(valueElements[3]));
	}
	
	float8 tan(const float8 value)
	{
		const float * valueElements = (float*)&value;
		
		return float8(
			tanf(valueElements[0]),
			tanf(valueElements[1]),
			tanf(valueElements[2]),
			tanf(valueElements[3]),
			tanf(valueElements[4]),
			tanf(valueElements[5]),
			tanf(valueElements[6]),
			tanf(valueElements[7]));
	}
}

// templatized filtering routine. operating on a single input, multiple output basis

#include "biquad.h"

template <typename T>
static void applyBiquad(
	T Fc,                      // biquad central frequency
	T Q,                       // biquad Q factor
	T peakGainDB,              // biquad peak gain
	const float * inputBuffer, // input buffer of numSamples in size
	T * outputBuffer,          // output buffer of numSamples in size
	const int numSamples)      // the number of samples to process
{
	BiquadFilter<T> filter;
	filter.makeLowpass(Fc, Q, peakGainDB);
	
	for (int i = 0; i < numSamples; ++i)
	{
		const T inputValue(inputBuffer[i]);
		
		const T outputValue = filter.processSingle(inputValue);
		
		outputBuffer[i] = outputValue;
	}
}

#include <xmmintrin.h>

#define test1 1
#define test4 1
#define test8 1

static void testBiquad(const bool showResults)
{
	const int kNumFilters = 8;
	const int kNumSamples = 1024;
	
	// initialize constants for the eight lowpass filters
	float Fc[kNumFilters];
	float Q[kNumFilters];
	float peakGainDB[kNumFilters];
	
	for (int i = 0; i < kNumFilters; ++i)
	{
		Fc[i] = .5f / (i + 1);
		Q[i] = sqrtf(2.f) / (i + 1);
		peakGainDB[i] = 20.f / (i + 1);
	}
	
	// input buffer just contains some noise
	static float inputBuffer[kNumSamples];
	static bool inoutBufferIsInitialized = false;
	if (!inoutBufferIsInitialized)
	{
		inoutBufferIsInitialized = true;
		for (int i = 0; i < kNumSamples; ++i)
			inputBuffer[i] = rand() / float(RAND_MAX) - .5f;
	}
	
	// temp buffers will store the output of the eight lowpass filters
	// output buffers will store the mixdown of the eight lowpass filters
	
	float tempBuffer_float[8][kNumSamples];
	float outputBuffer_float[kNumSamples];
	
	float4 tempBuffer_float4[2][kNumSamples];
	float outputBuffer_float4[kNumSamples];
	
	float8 tempBuffer_float8[1][kNumSamples];
	float outputBuffer_float8[kNumSamples];

#if test1
// -----------------------------------------------------------------------------------------------------------------------------
// apply filter. float version needs to run eight times

	const auto float_begin = std::chrono::system_clock::now();
	
	for (int i = 0; i < 8; ++i)
	{
		applyBiquad<float>(
			Fc[i],
			Q[i],
			peakGainDB[i],
			inputBuffer,
			tempBuffer_float[i], kNumSamples);
	}
	
	const auto float_end = std::chrono::system_clock::now();
	
	// mixdown
	for (int i = 0; i < kNumSamples; ++i)
	{
		const float value1 = tempBuffer_float[0][i];
		const float value2 = tempBuffer_float[1][i];
		const float value3 = tempBuffer_float[2][i];
		const float value4 = tempBuffer_float[3][i];
		const float value5 = tempBuffer_float[4][i];
		const float value6 = tempBuffer_float[5][i];
		const float value7 = tempBuffer_float[6][i];
		const float value8 = tempBuffer_float[7][i];
		
		const float sum =
			(
				(value1 + value2) +
				(value3 + value4)
			) +
			(
				(value5 + value6) +
				(value7 + value8)
			);
		
		outputBuffer_float[i] = sum;
	}
	
	const auto float_endmix = std::chrono::system_clock::now();
#endif

#if test4
// -----------------------------------------------------------------------------------------------------------------------------
// apply filter. float4 version needs to run two times

	const auto float4_begin = std::chrono::system_clock::now();
	
	for (int i = 0; i < 2; ++i)
	{
		applyBiquad<float4>(
			float4(Fc + i * 4),
			float4(Q + i * 4),
			float4(peakGainDB + i * 4),
			inputBuffer,
			tempBuffer_float4[i], kNumSamples);
	}
	
	const auto float4_end = std::chrono::system_clock::now();
	
	// mixdown
	for (int i = 0; i < kNumSamples; ++i)
	{
		const float4 & value1 = tempBuffer_float4[0][i];
		const float4 & value2 = tempBuffer_float4[1][i];
		
		const float4 value = value1 + value2;
		
		const float * valueElements = (float*)&value;
		
		const float sum =
			(valueElements[0] + valueElements[1]) +
			(valueElements[2] + valueElements[3]);
		
		outputBuffer_float4[i] = sum;
	}
	
	const auto float4_endmix = std::chrono::system_clock::now();
#endif

#if test8
// -----------------------------------------------------------------------------------------------------------------------------
// apply filter. float8 version needs to run one time
	
	const auto float8_begin = std::chrono::system_clock::now();
	
	for (int i = 0; i < 1; ++i)
	{
		applyBiquad<float8>(
			float8(Fc + i * 8),
			float8(Q + i * 8),
			float8(peakGainDB + i * 8),
			inputBuffer,
			tempBuffer_float8[i], kNumSamples);
	}
	
	const auto float8_end = std::chrono::system_clock::now();
	
	// mixdown
#if 1
	const __m128 * __restrict src = (__m128*)tempBuffer_float8[0];
	      __m128 * __restrict dst = (__m128*)outputBuffer_float8;
	
	for (int i = 0; i < kNumSamples / 4; ++i, src += 8)
	{
		__m128 value11 = src[0];
		__m128 value12 = src[2];
		__m128 value13 = src[4];
		__m128 value14 = src[6];
		
		__m128 value21 = src[1];
		__m128 value22 = src[3];
		__m128 value23 = src[5];
		__m128 value24 = src[7];
		
		_MM_TRANSPOSE4_PS(value11, value12, value13, value14);
		_MM_TRANSPOSE4_PS(value21, value22, value23, value24);
		
		const __m128 sum1 = _mm_add_ps(_mm_add_ps(value11, value12), _mm_add_ps(value13, value14));
		const __m128 sum2 = _mm_add_ps(_mm_add_ps(value21, value22), _mm_add_ps(value23, value24));
		
		const __m128 sum = _mm_add_ps(sum1, sum2);
		
		dst[i] = sum;
	}
#else
	for (int i = 0; i < kNumSamples; ++i)
	{
		const float8 & value = tempBuffer_float8[0][i];
		
		const float * valueElements = (float*)&value;
		
		const float value1 = valueElements[0];
		const float value2 = valueElements[1];
		const float value3 = valueElements[2];
		const float value4 = valueElements[3];
		const float value5 = valueElements[4];
		const float value6 = valueElements[5];
		const float value7 = valueElements[6];
		const float value8 = valueElements[7];
		
		const float sum =
			(
				(value1 + value2) +
				(value3 + value4)
			) +
			(
				(value5 + value6) +
				(value7 + value8)
			);
		
		outputBuffer_float8[i] = sum;
	}
#endif
	
	const auto float8_endmix = std::chrono::system_clock::now();
#endif

	if (showResults)
	{
		// output the results of the different vector sizes. these should all be exactly equal!
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			printf("output[%03d]: %+.2f, %+.2f, %+.2f\n",
				i,
				outputBuffer_float[i],
				outputBuffer_float4[i],
				outputBuffer_float8[i]);
		}
		
		// show the elapsed times
		
		printf("elapsed time (us):\n");
		
	#if test1
		const auto elapsed_float = std::chrono::duration_cast<std::chrono::microseconds>(float_end - float_begin).count();
		const auto elapsed_floatmix = std::chrono::duration_cast<std::chrono::microseconds>(float_endmix - float_begin).count();
		printf("float: %lld, float +mix: %lld\n", elapsed_float, elapsed_floatmix);
	#endif
	
	#if test4
		const auto elapsed_float4 = std::chrono::duration_cast<std::chrono::microseconds>(float4_end - float4_begin).count();
		const auto elapsed_float4mix = std::chrono::duration_cast<std::chrono::microseconds>(float4_endmix - float4_begin).count();
		printf("float4: %lld, float4+mix: %lld\n", elapsed_float4, elapsed_float4mix);
	#endif
	
	#if test8
		const auto elapsed_float8 = std::chrono::duration_cast<std::chrono::microseconds>(float8_end - float8_begin).count();
		const auto elapsed_float8mix = std::chrono::duration_cast<std::chrono::microseconds>(float8_endmix - float8_begin).count();
		printf("float8: %lld, float8+mix: %lld\n", elapsed_float8, elapsed_float8mix);
	#endif
	}
}

int main(int argc, char * argv[])
{
	const auto loop_begin = std::chrono::system_clock::now();
	
	for (int i = 0; i < 10000; ++i)
		testBiquad(false);
	
	const auto loop_end = std::chrono::system_clock::now();
	
	const auto loop_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(loop_end - loop_begin).count();
	
	printf("loop time (us): %lld\n", loop_elapsed);
	
	testBiquad(true);

	return 0;
}
