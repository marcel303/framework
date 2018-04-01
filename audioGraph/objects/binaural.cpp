/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "binaural.h"
#include "delaunay/delaunay.h"
#include "fourier.h"
#include <stdarg.h>
#include <stdio.h>

#define USE_FRAMEWORK 1

#if USE_FRAMEWORK
	#include "audio.h"
	#include "framework.h"
	#include "Parse.h"
	#include "Timer.h"
#endif

#if BINAURAL_USE_SSE
	#include <xmmintrin.h>
	#include <immintrin.h>
#endif

#if BINAURAL_USE_SIMD == 0
	#if __SSE2__
		#warning "BINAURAL_USE_SSE is set to 0. is this intended?"
	#endif
#endif

/*

todo : quality :
- the audio seems to have a tin-can feel after being processed. todo : find out why. perhaps we need a low-pass filter ?

todo : performance :
- batch binaural object : most of the remaining cost is in performing the fourier transformations prior and after the convolution step is performed. for the source to frequency <-> time domain transformations, it's possible to do four source of them at once when we create a batch binauralizer object. this object would then perform binauralization on four streams at a time. convolution etc which operates on the left and right channels separately could be made to use 8-component AVX instructions, for additional gains
- optimize the time domain to frequency domain transformation by using a FFT optimized for working with real-values inputs. since the last 50% of the fourier transformation is a mirror of the first 50% (symmetry), the algortihm could be optimized to only compute the first half

*/

namespace binaural
{
	typedef Vector2<float> SampleLocation;
	typedef Triangle<float> SampleTriangle;
	
	//
	
#ifdef WIN32
	__declspec(thread) bool enableDebugLog;
#else
	__thread bool enableDebugLog = false;
#endif

	//

#ifdef WIN32

	inline __m128 operator+(__m128 a, __m128 b)
	{
		return _mm_add_ps(a, b);
	}

	inline __m128 operator-(__m128 a, __m128 b)
	{
		return _mm_sub_ps(a, b);
	}

	inline __m128 operator*(__m128 a, __m128 b)
	{
		return _mm_mul_ps(a, b);
	}

#endif

#if BINAURAL_USE_SSE
	// for SSE we do not need to define any compatibility functions
#elif BINAURAL_USE_GCC_VECTOR
	// translate SSE intrinsics into GCC vector code
	inline float4 _mm_set_ps(const float z, const float y, const float x, const float w)
	{
		return float4 { z, y, x, w };
	}
	
	inline float4 _mm_set1_ps(const float x)
	{
		return float4 { x, x, x, x };
	}

	inline float4 _mm_load1_ps(const float * _x)
	{
		const float x = *_x;

		return float4 { x, x, x, x };
	}
#endif

	//
	
	HRIRSampleGrid::Location operator-(const HRIRSampleGrid::Location & a, const HRIRSampleGrid::Location & b)
	{
		HRIRSampleGrid::Location result;
		result.elevation = a.elevation - b.elevation;
		result.azimuth = a.azimuth - b.azimuth;
		return result;
	}
	
	float dot(const HRIRSampleGrid::Location & a, const HRIRSampleGrid::Location & b)
	{
		return
			a.elevation * b.elevation +
			a.azimuth * b.azimuth;
	}
	
	// these indices are used to move data around in preparation for the FFT algorithm, which expects
	// the source data to be stored in a 'bit-reversed' order. since these indices are quite expensive
	// to calculate, we calculate them once and store them in a lookup table
	struct FftIndices
	{
		ALIGN16 int indices[HRTF_BUFFER_SIZE];

		FftIndices()
		{
			const int numBits = Fourier::integerLog2(HRTF_BUFFER_SIZE);
			
			for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
			{
				indices[i] = Fourier::reverseBits(i, numBits);
			}
		}
	};

	static FftIndices fftIndices;

	// functions

	bool convertSoundDataToHRIR(
		const SoundData & soundData,
		float * __restrict lSamples,
		float * __restrict rSamples)
	{
		if (soundData.numChannels != 2)
		{
			// sample data must be stereo as it must contain the impulse-response for both the left and right ear
			
			return false;
		}
		
		// ideally the number of samples in the sound data and the HRIR buffer would match
		// however, we clamp the number of samples in case the sound data contains more samples,
		// and pad it with zeroes in case it's less
		
		const int numSamplesToCopy = std::min(soundData.numSamples, HRIR_BUFFER_SIZE);
		
		if (soundData.sampleSize == 2)
		{
			// 16-bit signed integers. convert the sample data to [-1..+1] floating point and
			// de-interleave into left/right ear HRIR data
			
			const int16_t * __restrict sampleData = (const int16_t*)soundData.sampleData;
			
			for (int i = 0; i < numSamplesToCopy; ++i)
			{
				lSamples[i] = sampleData[i * 2 + 0] / float(1 << 15);
				rSamples[i] = sampleData[i * 2 + 1] / float(1 << 15);
			}
		}
		else if (soundData.sampleSize == 4)
		{
			// 32-bit floating point data. de-interleave into left/right ear HRIR data
			
			const float * __restrict sampleData = (const float*)soundData.sampleData;
			
			for (int i = 0; i < numSamplesToCopy; ++i)
			{
				lSamples[i] = sampleData[i * 2 + 0];
				rSamples[i] = sampleData[i * 2 + 1];
			}
		}
		else
		{
			// unknown sample format
			
			return false;
		}
		
		// pad the HRIR data with zeroes when necessary
		
		for (int i = numSamplesToCopy; i < HRIR_BUFFER_SIZE; ++i)
		{
			lSamples[i] = 0.f;
			rSamples[i] = 0.f;
		}
		
		return true;
	}

	void blendHrirSamples_3(
		const HRIRSampleData & a, const float aWeight,
		const HRIRSampleData & b, const float bWeight,
		const HRIRSampleData & c, const float cWeight,
		HRIRSampleData & r)
	{
		debugTimerBegin("blendHrirSamples_3");
		
	#if BINAURAL_USE_SIMD
		const float4 * __restrict a_lSamples = (float4*)a.lSamples;
		const float4 * __restrict b_lSamples = (float4*)b.lSamples;
		const float4 * __restrict c_lSamples = (float4*)c.lSamples;

		const float4 * __restrict a_rSamples = (float4*)a.rSamples;
		const float4 * __restrict b_rSamples = (float4*)b.rSamples;
		const float4 * __restrict c_rSamples = (float4*)c.rSamples;

		const float4 aWeight4 = _mm_set1_ps(aWeight);
		const float4 bWeight4 = _mm_set1_ps(bWeight);
		const float4 cWeight4 = _mm_set1_ps(cWeight);

		float4 * __restrict r_lSamples = (float4*)r.lSamples;
		float4 * __restrict r_rSamples = (float4*)r.rSamples;

		for (int i = 0; i < HRIR_BUFFER_SIZE / 4; ++i)
		{
			r_lSamples[i] =
				a_lSamples[i] * aWeight4 +
				b_lSamples[i] * bWeight4 +
				c_lSamples[i] * cWeight4;

			r_rSamples[i] =
				a_rSamples[i] * aWeight4 +
				b_rSamples[i] * bWeight4 +
				c_rSamples[i] * cWeight4;
		}
	#else
		for (int i = 0; i < HRIR_BUFFER_SIZE; ++i)
		{
			r.lSamples[i] =
				a.lSamples[i] * aWeight +
				b.lSamples[i] * bWeight +
				c.lSamples[i] * cWeight;

			r.rSamples[i] =
				a.rSamples[i] * aWeight +
				b.rSamples[i] * bWeight +
				c.rSamples[i] * cWeight;
		}
	#endif
	
		debugTimerEnd("blendHrirSamples_3");
	}
	
	void blendHrirSamples_3(
		HRIRSampleData const * const * samples,
		const float * sampleWeights,
		HRIRSampleData & result)
	{
		blendHrirSamples_3(
			*samples[0], sampleWeights[0],
			*samples[1], sampleWeights[1],
			*samples[2], sampleWeights[2],
			result);
	}
	
	void hrirToHrtf(
		const float * __restrict lSamples,
		const float * __restrict rSamples,
		HRTFData & lFilter,
		HRTFData & rFilter)
	{
		debugTimerBegin("hrirToHrtf");
		
	#if ENABLE_FOURIER4
		// this will generate the HRTF from the HRIR samples
		
		float4 filterReal[HRTF_BUFFER_SIZE];
		float4 filterImag[HRTF_BUFFER_SIZE];
		
		interleaveAudioBuffersAndReverseIndices_4(lSamples, rSamples, lSamples, rSamples, filterReal);
		memset(filterImag, 0, sizeof(filterImag));
		
		Fourier::fft1D(filterReal, filterImag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
		
		deinterleaveAudioBuffers_4_to_2(filterReal, lFilter.real, rFilter.real);
		deinterleaveAudioBuffers_4_to_2(filterImag, lFilter.imag, rFilter.imag);
	#else
		// this will generate the HRTF from the HRIR samples
		
		memset(lFilter.imag, 0, sizeof(lFilter.imag));
		memset(rFilter.imag, 0, sizeof(rFilter.imag));
		
		float * __restrict lFilterReal = lFilter.real;
		float * __restrict rFilterReal = rFilter.real;
		
		for (int i = 0; i < HRIR_BUFFER_SIZE; ++i)
		{
			lFilterReal[fftIndices.indices[i]] = lSamples[i];
			rFilterReal[fftIndices.indices[i]] = rSamples[i];
		}
		
		Fourier::fft1D(lFilter.real, lFilter.imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
		Fourier::fft1D(rFilter.real, rFilter.imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
	#endif
	
		debugTimerEnd("hrirToHrtf");
	}
	
	void reverseSampleIndices(
		const float * __restrict src,
		float * __restrict dst)
	{
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			dst[fftIndices.indices[i]] = src[i];
		}
	}

	void convolveAudio(
		AudioBuffer & source,
		const HRTFData & lFilter,
		const HRTFData & rFilter,
		AudioBuffer & lResult,
		AudioBuffer & rResult)
	{
		// transform audio data from the time-domain into the frequency-domain
		
		source.transformToFrequencyDomain(true);
		
		// convolve audio data with impulse-response data in the frequency-domain
		
		source.convolveAndReverseIndices(lFilter, lResult);
		source.convolveAndReverseIndices(rFilter, rResult);
		
		// transform convolved audio data back to the time-domain
		
		lResult.transformToTimeDomain(true);
		rResult.transformToTimeDomain(true);
	}
	
	void convolveAudio_2(
		AudioBuffer & source,
		const HRTFData & lFilterOld,
		const HRTFData & rFilterOld,
		const HRTFData & lFilterNew,
		const HRTFData & rFilterNew,
		float * __restrict lResultOld,
		float * __restrict rResultOld,
		float * __restrict lResultNew,
		float * __restrict rResultNew)
	{
		debugTimerBegin("convolveAudio_2");
		
		// transform audio data from the time-domain into the frequency-domain
		
		source.transformToFrequencyDomain(true);
		
	#if ENABLE_FOURIER4
		float4 filterReal[HRTF_BUFFER_SIZE];
		float4 filterImag[HRTF_BUFFER_SIZE];
		
		interleaveAudioBuffers_4(lFilterOld.real, rFilterOld.real, lFilterNew.real, rFilterNew.real, filterReal);
		interleaveAudioBuffers_4(lFilterOld.imag, rFilterOld.imag, lFilterNew.imag, rFilterNew.imag, filterImag);
		
		// convolve audio data with impulse-response data in the frequency-domain
		
		float4 convolvedReal[HRTF_BUFFER_SIZE];
		float4 convolvedImag[HRTF_BUFFER_SIZE];
		
		source.convolveAndReverseIndices_4(filterReal, filterImag, convolvedReal, convolvedImag);
		
		// transform convolved audio data back to the time-domain
		
		Fourier::fft1D(convolvedReal, convolvedImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		
		deinterleaveAudioBuffers_4(convolvedReal, lResultOld, rResultOld, lResultNew, rResultNew);
	#else
		// convolve audio data with impulse-response data in the frequency-domain
		
		float lResultOldImag[HRTF_BUFFER_SIZE];
		float rResultOldImag[HRTF_BUFFER_SIZE];
		float lResultNewImag[HRTF_BUFFER_SIZE];
		float rResultNewImag[HRTF_BUFFER_SIZE];
		
		source.convolveAndReverseIndices(lFilterOld, lResultOld, lResultOldImag);
		source.convolveAndReverseIndices(rFilterOld, rResultOld, rResultOldImag);
		source.convolveAndReverseIndices(lFilterNew, lResultNew, lResultNewImag);
		source.convolveAndReverseIndices(rFilterNew, rResultNew, rResultNewImag);
		
		// transform convolved audio data back to the time-domain
		
		Fourier::fft1D(lResultOld, lResultOldImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		Fourier::fft1D(rResultOld, rResultOldImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		Fourier::fft1D(lResultNew, lResultNewImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		Fourier::fft1D(rResultNew, rResultNewImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
	#endif
	
		debugTimerEnd("convolveAudio_2");
	}
	
	void rampAudioBuffers(
		const float * __restrict a,
		const float * __restrict b,
		const int numSamples,
		float * __restrict r)
	{
		debugAssert((numSamples % 4) == 0);
		
	#if BINAURAL_USE_SIMD
		const float tStepScalar = 1.f / numSamples;
		const float4 tStep = _mm_set1_ps(8.f / numSamples);
		float4 t1 = _mm_set_ps(tStepScalar * 3.f, tStepScalar * 2.f, tStepScalar * 1.f, tStepScalar * 0.f);
		float4 t2 = _mm_set_ps(tStepScalar * 7.f, tStepScalar * 6.f, tStepScalar * 5.f, tStepScalar * 4.f);
		
		const float4 * __restrict a4 = (float4*)a;
		const float4 * __restrict b4 = (float4*)b;
		      float4 * __restrict r4 = (float4*)r;
		
		const float4 one = _mm_set1_ps(1.f);
		
		for (int i = 0; i < numSamples / 8; ++i)
		{
			const float4 aWeight1 = one - t1;
			const float4 bWeight1 = t1;
			
			const float4 aWeight2 = one - t2;
			const float4 bWeight2 = t2;
			
			r4[i * 2 + 0] = a4[i * 2 + 0] * aWeight1 + b4[i * 2 + 0] * bWeight1;
			r4[i * 2 + 1] = a4[i * 2 + 1] * aWeight2 + b4[i * 2 + 1] * bWeight2;
			
			t1 = t1 + tStep;
			t2 = t2 + tStep;
		}
		
	#else
		const float tStep = 1.f / numSamples;
		
		float t = 0.f;
		
		for (int i = 0; i < numSamples; ++i)
		{
			const float aWeight = 1.f - t;
			const float bWeight = t;
			
			r[i] = a[i] * aWeight + b[i] * bWeight;
			
			t += tStep;
		}
	#endif
	}
	
#if ENABLE_FOURIER4
	void interleaveAudioBuffers_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result)
	{
	 	// todo : include neon; requires transpose function
	#if BINAURAL_USE_SSE
		const float4 * __restrict array1_4 = (float4*)array1;
		const float4 * __restrict array2_4 = (float4*)array2;
		const float4 * __restrict array3_4 = (float4*)array3;
		const float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; ++i)
		{
			float4 value1a = array1_4[i * 2 + 0];
			float4 value2a = array2_4[i * 2 + 0];
			float4 value3a = array3_4[i * 2 + 0];
			float4 value4a = array4_4[i * 2 + 0];
			
			float4 value1b = array1_4[i * 2 + 1];
			float4 value2b = array2_4[i * 2 + 1];
			float4 value3b = array3_4[i * 2 + 1];
			float4 value4b = array4_4[i * 2 + 1];
			
		#if BINAURAL_USE_SSE
			_MM_TRANSPOSE4_PS(value1a, value2a, value3a, value4a);
			_MM_TRANSPOSE4_PS(value1b, value2b, value3b, value4b);
		#else
			#error
		#endif
			
			result[i * 8 + 0] = value1a;
			result[i * 8 + 1] = value2a;
			result[i * 8 + 2] = value3a;
			result[i * 8 + 3] = value4a;
			
			result[i * 8 + 4] = value1b;
			result[i * 8 + 5] = value2b;
			result[i * 8 + 6] = value3b;
			result[i * 8 + 7] = value4b;
		}
	#else
		float * __restrict resultScalar = (float*)result;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			const float value1 = array1[i];
			const float value2 = array2[i];
			const float value3 = array3[i];
			const float value4 = array4[i];
			
			resultScalar[i * 4 + 0] = value1;
			resultScalar[i * 4 + 1] = value2;
			resultScalar[i * 4 + 2] = value3;
			resultScalar[i * 4 + 3] = value4;
		}
	#endif
	}
	
	void interleaveAudioBuffersAndReverseIndices_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result)
	{
		// todo : include neon; requires transpose function
	#if BINAURAL_USE_SSE
		const float4 * __restrict array1_4 = (float4*)array1;
		const float4 * __restrict array2_4 = (float4*)array2;
		const float4 * __restrict array3_4 = (float4*)array3;
		const float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
			float4 value1 = array1_4[i];
			float4 value2 = array2_4[i];
			float4 value3 = array3_4[i];
			float4 value4 = array4_4[i];
			
		#if BINAURAL_USE_SSE
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
		#else
			#error
		#endif
			
			const int index1 = fftIndices.indices[i * 4 + 0];
			const int index2 = fftIndices.indices[i * 4 + 1];
			const int index3 = fftIndices.indices[i * 4 + 2];
			const int index4 = fftIndices.indices[i * 4 + 3];
			
			result[index1] = value1;
			result[index2] = value2;
			result[index3] = value3;
			result[index4] = value4;
		}
	#else
		float * __restrict resultScalar = (float*)result;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			const float value1 = array1[i];
			const float value2 = array2[i];
			const float value3 = array3[i];
			const float value4 = array4[i];
			
			const int index = fftIndices.indices[i];
			
			resultScalar[index * 4 + 0] = value1;
			resultScalar[index * 4 + 1] = value2;
			resultScalar[index * 4 + 2] = value3;
			resultScalar[index * 4 + 3] = value4;
		}
	#endif
	}
	
	void deinterleaveAudioBuffers_4(
		const float4 * __restrict interleaved,
		float * __restrict array1,
		float * __restrict array2,
		float * __restrict array3,
		float * __restrict array4)
	{
		// todo : include neon; requires transpose function
	#if BINAURAL_USE_SSE
		float4 * __restrict array1_4 = (float4*)array1;
		float4 * __restrict array2_4 = (float4*)array2;
		float4 * __restrict array3_4 = (float4*)array3;
		float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
			float4 value1 = interleaved[i * 4 + 0];
			float4 value2 = interleaved[i * 4 + 1];
			float4 value3 = interleaved[i * 4 + 2];
			float4 value4 = interleaved[i * 4 + 3];
			
		#if BINAURAL_USE_SSE
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
		#else
			#error
		#endif
			
			array1_4[i] = value1;
			array2_4[i] = value2;
			array3_4[i] = value3;
			array4_4[i] = value4;
		}
	#else
		const float * __restrict interleavedScalar = (float*)interleaved;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			array1[i] = interleavedScalar[i * 4 + 0];
			array2[i] = interleavedScalar[i * 4 + 1];
			array3[i] = interleavedScalar[i * 4 + 2];
			array4[i] = interleavedScalar[i * 4 + 3];
		}
	#endif
	}
	
	void deinterleaveAudioBuffers_4_to_2(
		const float4 * __restrict interleaved,
		float * __restrict array1,
		float * __restrict array2)
	{
	 	// todo : include neon; requires transpose function
	#if BINAURAL_USE_SSE
		float4 * __restrict array1_4 = (float4*)array1;
		float4 * __restrict array2_4 = (float4*)array2;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; ++i)
		{
			float4 value1a = interleaved[i * 8 + 0];
			float4 value2a = interleaved[i * 8 + 1];
			float4 value3a = interleaved[i * 8 + 2];
			float4 value4a = interleaved[i * 8 + 3];
			
			float4 value1b = interleaved[i * 8 + 4];
			float4 value2b = interleaved[i * 8 + 5];
			float4 value3b = interleaved[i * 8 + 6];
			float4 value4b = interleaved[i * 8 + 7];
			
		#if BINAURAL_USE_SSE
			_MM_TRANSPOSE4_PS(value1a, value2a, value3a, value4a);
			_MM_TRANSPOSE4_PS(value1b, value2b, value3b, value4b);
		#else
			#error
		#endif
			
			array1_4[i * 2 + 0] = value1a;
			array2_4[i * 2 + 0] = value2a;
			
			array1_4[i * 2 + 1] = value1b;
			array2_4[i * 2 + 1] = value2b;
		}
	#else
		const float * __restrict interleavedScalar = (float*)interleaved;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			array1[i] = interleavedScalar[i * 4 + 0];
			array2[i] = interleavedScalar[i * 4 + 1];
		}
	#endif
	}
#endif
	
	//
	
	void elevationAndAzimuthToCartesian(const float elevation, const float azimuth, float & x, float & y, float & z)
	{
		const float degToRad = M_PI / 180.f;
		
	#if 1
		y = std::sin(elevation * degToRad);
		
		x = std::cos(azimuth * degToRad) * std::cos(elevation * degToRad);
		z = std::sin(azimuth * degToRad) * std::abs(std::cos(elevation * degToRad));
	#else
		Mat4x4 matA;
		Mat4x4 matE;
		
		matA.MakeRotationY(azimuth * degToRad);
		matE.MakeRotationZ(elevation * degToRad);
		
		Mat4x4 mat = matA * matE;
		
		Vec3 v = mat * Vec3(1.f, 0.f, 0.f);
		
		x = +v[0];
		y = -v[1];
		z = +v[2];
	#endif
	}
	
	void cartesianToElevationAndAzimuth(const float x, const float y, const float z, float & elevation, float & azimuth)
	{
		const float radToDeg = 180.f / M_PI;
		
	#if 1
		const float zxHypot = std::hypot(z, x);
		
		azimuth = std::atan2(z, x) * radToDeg;
		
		if (zxHypot == 0.f)
		{
			if (y < 0.f)
				elevation = -90.f;
			else
				elevation = +90.f;
		}
		else
		{
			elevation = std::atan(y / zxHypot) * radToDeg;
		}
	#else
		azimuth = std::atan2(z, x) * radToDeg;
		elevation = std::asin(y) * radToDeg;
	#endif
	}
	
	//
	
#if ENABLE_DEBUGGING && USE_FRAMEWORK
	static std::map<std::string, uint64_t> debugTimers;
	
	void debugAssert(const bool condition)
	{
		fassert(condition);
		
		if (condition == false)
			debugLog("assert failed!");
	}
	
	void debugLog(const char * format, ...)
	{
		if (enableDebugLog == false)
			return;
			
		char text[1024];
		va_list args;
		va_start(args, format);
	#ifdef WIN32
		vsprintf_s(text, sizeof(text), format, args);
	#else
		vsnprintf(text, sizeof(text), format, args);
	#endif
		va_end(args);
		
		//logDebug(text);
		printf("%s\n", text);
	}
	
	void debugTimerBegin(const char * name)
	{
		auto & debugTimer = debugTimers[name];
		
		debugTimer = 0;
		debugTimer -= g_TimerRT.TimeUS_get();
	}
	
	void debugTimerEnd(const char * name)
	{
		auto & debugTimer = debugTimers[name];
		
		debugTimer += g_TimerRT.TimeUS_get();
		
		debugLog("timer %s took %.3fms", name, debugTimer / 1000.f);
	}
#endif
	
#if USE_FRAMEWORK
	void listFiles(const char * path, bool recurse, std::vector<std::string> & result)
	{
		debugTimerBegin("list_files");
		
		result = ::listFiles(path, recurse);
		
		debugTimerEnd("list_files");
	}
	
	bool parseInt32(const std::string & text, int & result)
	{
		// todo : return false on parse error
		
		result = Parse::Int32(text);
		
		return true;
	}
	
	SoundData * loadSound(const char * filename)
	{
		::SoundData * temp = ::loadSound(filename);
		
		if (temp == nullptr)
		{
			return nullptr;
		}
		else
		{
			SoundData * result = new SoundData();
			
			result->numChannels = temp->channelCount;
			result->sampleSize = temp->channelSize;
			result->numSamples = temp->sampleCount;
			result->sampleData = temp->sampleData;
			
			// steal sample data so it won't get freed
			
			temp->sampleData = nullptr;
			
			delete temp;
			temp = nullptr;
			
			//
			
			return result;
		}
	}
#else
#endif

	//

	void AudioBuffer::transformToFrequencyDomain(const bool fast)
	{
		if (fast)
			Fourier::fft1D(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, false, false);
		else
			Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, false, false);
	}

	void AudioBuffer::transformToTimeDomain(const bool fast)
	{
		if (fast)
			Fourier::fft1D(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		else
			Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
	}

	void AudioBuffer::convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output) const
	{
		const float * __restrict sReal = real;
		const float * __restrict sImag = imag;
		
		const float * __restrict fReal = filter.real;
		const float * __restrict fImag = filter.imag;
		
		float * __restrict oReal = output.real;
		float * __restrict oImag = output.imag;
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[fftIndices.indices[i]] = sReal[i] * fReal[i] - sImag[i] * fImag[i];
			oImag[fftIndices.indices[i]] = sReal[i] * fImag[i] + sImag[i] * fReal[i];
		}
	}
	
	void AudioBuffer::convolveAndReverseIndices(
		const HRTFData & filter,
		float * __restrict outputReal,
		float * __restrict outputImag) const
	{
		const float * __restrict sReal = real;
		const float * __restrict sImag = imag;
		
		const float * __restrict fReal = filter.real;
		const float * __restrict fImag = filter.imag;
		
		float * __restrict oReal = outputReal;
		float * __restrict oImag = outputImag;
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[fftIndices.indices[i]] = sReal[i] * fReal[i] - sImag[i] * fImag[i];
			oImag[fftIndices.indices[i]] = sReal[i] * fImag[i] + sImag[i] * fReal[i];
		}
	}
	
#if ENABLE_FOURIER4
	void AudioBuffer::convolveAndReverseIndices_4(
		const float4 * __restrict filterReal,
		const float4 * __restrict filterImag,
		float4 * __restrict outputReal,
		float4 * __restrict outputImag) const
	{
		const float * __restrict sReal = real;
		const float * __restrict sImag = imag;
		
		const float4 * __restrict fReal = filterReal;
		const float4 * __restrict fImag = filterImag;
		
		float4 * __restrict oReal = outputReal;
		float4 * __restrict oImag = outputImag;
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			float4 sReal4 = _mm_load1_ps(&sReal[i]);
			float4 sImag4 = _mm_load1_ps(&sImag[i]);
			
			oReal[fftIndices.indices[i]] = sReal4 * fReal[i] - sImag4 * fImag[i];
			oImag[fftIndices.indices[i]] = sReal4 * fImag[i] + sImag4 * fReal[i];
		}
	}
#endif

	const HRIRSampleGrid::Cell * HRIRSampleGrid::lookupCell(const float elevation, const float azimuth) const
	{
		const int cellX = (int(std::floor(elevation / 180.f * HRIRSampleGrid::kGridSx)) + HRIRSampleGrid::kGridSx) % HRIRSampleGrid::kGridSx;
		const int cellY = (int(std::floor(azimuth   / 360.f * HRIRSampleGrid::kGridSy)) + HRIRSampleGrid::kGridSy) % HRIRSampleGrid::kGridSy;
		
		//debugAssert(cellX >= 0 && cellX < HRIRSampleGrid::kGridSx);
		//debugAssert(cellY >= 0 && cellY < HRIRSampleGrid::kGridSy);
		
		if (cellX < 0 || cellY < 0)
			return nullptr;
		else
			return &cells[cellX][cellY];
	}
	
	const HRIRSampleGrid::Triangle * HRIRSampleGrid::lookupTriangle(const float elevation, const float azimuth, float & baryU, float & baryV) const
	{
		const float eps = .001f;
		
		auto cell = lookupCell(elevation, azimuth);
		
		if (cell == nullptr)
		{
			return nullptr;
		}
		
		Location sampleLocation;
		sampleLocation.elevation = elevation;
		sampleLocation.azimuth = azimuth;
		
		const Triangle * result = nullptr;
		
		for (auto triangleIndex : cell->triangleIndices)
		{
			auto triangle = &triangles[triangleIndex];
			
			if (baryPointInTriangle(
				triangle->vertex[0].location,
				triangle->vertex[1].location,
				triangle->vertex[2].location,
				sampleLocation,
				eps,
				baryU, baryV))
			{
				result = triangle;
				
				break;
			}
		}
		
		return result;
	}
	
	static bool saveInt(FILE * file, const int value)
	{
		return fwrite(&value, sizeof(value), 1, file) == 1;
	}
	
	static bool saveFloat(FILE * file, const float value)
	{
		return fwrite(&value, sizeof(value), 1, file) == 1;
	}
	
	static bool loadInt(FILE * file, int & value)
	{
		return fread(&value, sizeof(value), 1, file) == 1;
	}
	
	static bool loadFloat(FILE * file, float & value)
	{
		return fread(&value, sizeof(value), 1, file) == 1;
	}
	
	bool HRIRSampleGrid::save(FILE * file) const
	{
		bool result = true;
		
		// write triangles
		
		result &= saveInt(file, triangles.size());
		
		for (auto & triangle : triangles)
		{
			for (auto & vertex : triangle.vertex)
			{
				result &= saveFloat(file, vertex.location.elevation);
				result &= saveFloat(file, vertex.location.azimuth);
				
				result &= saveInt(file, vertex.sampleIndex);
			}
		}
		
		// write cells
		
		result &= saveInt(file, kGridSx);
		result &= saveInt(file, kGridSy);
		
		for (int x = 0; x < kGridSx; ++x)
		{
			for (int y = 0; y < kGridSy; ++y)
			{
				result &= saveInt(file, cells[x][y].triangleIndices.size());
				
				for (auto triangleIndex : cells[x][y].triangleIndices)
				{
					result &= saveInt(file, triangleIndex);
				}
			}
		}
		
		return result;
	}
	
	bool HRIRSampleGrid::load(FILE * file)
	{
		debugAssert(triangles.empty());
		
		bool result = true;
		
		// read triangles
		
		int numTriangles = 0;
		
		result &= loadInt(file, numTriangles);
		
		if (numTriangles < 0)
		{
			result = false;
		}
		else
		{
			triangles.resize(numTriangles);
			
			for (auto & triangle : triangles)
			{
				for (auto & vertex : triangle.vertex)
				{
					result &= loadFloat(file, vertex.location.elevation);
					result &= loadFloat(file, vertex.location.azimuth);
					
					result &= loadInt(file, vertex.sampleIndex);
				}
			}
		}
		
		// read cells
		
		int gridSx = 0;
		int gridSy = 0;
		
		result &= loadInt(file, gridSx);
		result &= loadInt(file, gridSy);
		
		if (gridSx != kGridSx || gridSy != kGridSy)
		{
			result &= false;
		}
		else
		{
			for (int x = 0; x < kGridSx; ++x)
			{
				for (int y = 0; y < kGridSy; ++y)
				{
					int numTriangleIndices = 0;
					
					result &= loadInt(file, numTriangleIndices);
					
					if (numTriangleIndices < 0)
					{
						result &= false;
					}
					else
					{
						cells[x][y].triangleIndices.resize(numTriangleIndices);
						
						for (auto & triangleIndex : cells[x][y].triangleIndices)
						{
							result &= loadInt(file, triangleIndex);
						}
					}
				}
			}
		}
		
		return result;
	}
	
	bool HRIRSampleSet::addHrirSampleFromSoundData(const SoundData & soundData, const float elevation, const float azimuth, const bool swapLR)
	{
		HRIRSample * sample = new HRIRSample();
		
		if (convertSoundDataToHRIR(
			soundData,
			swapLR == false ? sample->sampleData.lSamples : sample->sampleData.rSamples,
			swapLR == false ? sample->sampleData.rSamples : sample->sampleData.lSamples))
		{
			sample->init(elevation, azimuth);
			
			samples.push_back(sample);
			
			return true;
		}
		else
		{
			delete sample;
			sample = nullptr;
			
			return false;
		}
	}
	
	void HRIRSampleSet::finalize()
	{
		// perform Delaunay triangulation
		
		debugTimerBegin("triangulation");
		
		std::vector<SampleLocation> sampleLocations;
		sampleLocations.resize(samples.size());
		
		int index = 0;
		
		for (auto & sample : samples)
		{
			SampleLocation & sampleLocation = sampleLocations[index++];
			
			sampleLocation.x = sample->elevation;
			sampleLocation.y = sample->azimuth;
		}
	
		Delaunay<float> delaunay;
		const std::vector<SampleTriangle> triangles = delaunay.triangulate(sampleLocations);
		
		debugTimerEnd("triangulation");
		
		debugLog("got a bunch of triangles back from the triangulator! numTriangles=%d", triangles.size());
		
		debugTimerBegin("grid_insertion");
		
		// now we've got the triangulation.. store the triangles into a grid for fast lookups
		
		for (auto & triangle : triangles)
		{
			// find matching HRIR samples
			
			const SampleLocation triangleLocations[3] =
			{
				triangle.p1,
				triangle.p2,
				triangle.p3,
			};
			
			int triangleSamples[3];
			
			for (int i = 0; i < 3; ++i)
			{
				auto & triangleLocation = triangleLocations[i];
				
				triangleSamples[i] = -1;
				
				int sampleIndex = 0;
				
				for (auto & sample : samples)
				{
					if (sample->elevation == triangleLocation.x && sample->azimuth == triangleLocation.y)
					{
						debugAssert(triangleSamples[i] == -1);
						
						triangleSamples[i] = sampleIndex;
					}
					
					sampleIndex++;
				}
				
				debugAssert(triangleSamples[i] != -1);
			}
			
			if (triangleSamples[0] == -1 ||
				triangleSamples[1] == -1 ||
				triangleSamples[2] == -1)
			{
				debugLog("failed to localize samples!");
			}
			else
			{
				HRIRSampleGrid::Triangle triangle;
				
				for (int i = 0; i < 3; ++i)
				{
					const int sampleIndex = triangleSamples[i];
					const HRIRSample * sample = samples[sampleIndex];
					
					triangle.vertex[i].location.elevation = sample->elevation;
					triangle.vertex[i].location.azimuth = sample->azimuth;
					triangle.vertex[i].sampleIndex = sampleIndex;
				}
				
				const int triangleIndex = sampleGrid.triangles.size();
				
				sampleGrid.triangles.push_back(triangle);
				
				float minE = triangle.vertex[0].location.elevation;
				float minA = triangle.vertex[0].location.azimuth;
				
				float maxE = triangle.vertex[0].location.elevation;
				float maxA = triangle.vertex[0].location.azimuth;
				
				for (int i = 1; i < 3; ++i)
				{
					minE = std::min(minE, triangle.vertex[i].location.elevation);
					minA = std::min(minA, triangle.vertex[i].location.azimuth);
					
					maxE = std::max(maxE, triangle.vertex[i].location.elevation);
					maxA = std::max(maxA, triangle.vertex[i].location.azimuth);
				}
				
				const int cellX1 = int(std::floor(minE / 180.f * HRIRSampleGrid::kGridSx));
				const int cellY1 = int(std::floor(minA / 360.f * HRIRSampleGrid::kGridSy));
				
				const int cellX2 = int(std::floor(maxE / 180.f * HRIRSampleGrid::kGridSx));
				const int cellY2 = int(std::floor(maxA / 360.f * HRIRSampleGrid::kGridSy));
				
				debugAssert(cellX1 <= cellX2);
				debugAssert(cellY1 <= cellY2);
				
			#if 0
				debugLog("insert triangle into cells: (%d, %d) -> (%d, %d)",
					cellX1, cellY1,
					cellX2, cellY2);
			#endif
				
				for (int x = cellX1; x <= cellX2; ++x)
				{
					for (int y = cellY1; y <= cellY2; ++y)
					{
						const int xi = (x + HRIRSampleGrid::kGridSx) % HRIRSampleGrid::kGridSx;
						const int yi = (y + HRIRSampleGrid::kGridSy) % HRIRSampleGrid::kGridSy;
						
						debugAssert(xi >= 0 && xi < HRIRSampleGrid::kGridSx);
						debugAssert(yi >= 0 && yi < HRIRSampleGrid::kGridSy);
						
						HRIRSampleGrid::Cell & cell = sampleGrid.cells[xi][yi];
						
						cell.triangleIndices.push_back(triangleIndex);
					}
				}
			}
		}
		
		debugTimerEnd("grid_insertion");
	}
	
	bool HRIRSampleSet::lookup_3(const float elevation, const float azimuth, HRIRSampleData const * * samples, float * sampleWeights) const
	{
		debugTimerBegin("lookup_hrir");
		
		// lookup HRIR sample points using triangulation result
		
		bool result = false;
		
		float baryU;
		float baryV;
		
		auto triangle = sampleGrid.lookupTriangle(elevation, azimuth, baryU, baryV);
		
		if (triangle != nullptr)
		{
			samples[0] = &this->samples[triangle->vertex[0].sampleIndex]->sampleData;
			samples[1] = &this->samples[triangle->vertex[1].sampleIndex]->sampleData;
			samples[2] = &this->samples[triangle->vertex[2].sampleIndex]->sampleData;
			
			sampleWeights[0] = 1.f - baryU - baryV;
			sampleWeights[1] = baryV;
			sampleWeights[2] = baryU;
		
			result = true;
		}
		
		debugTimerEnd("lookup_hrir");
		
		return result;
	}
	
	bool HRIRSampleSet::save(FILE * file) const
	{
		bool result = true;
		
		result &= saveInt(file, samples.size());
		
		for (auto sample : samples)
		{
			result &= fwrite(sample, sizeof(*sample), 1, file) == 1;
		}
		
		return result;
	}
	
	bool HRIRSampleSet::load(FILE * file)
	{
		debugAssert(samples.empty());
		
		bool result = true;
		
		int numSamples = 0;
		
		result &= loadInt(file, numSamples);
		
		if (numSamples < 0)
		{
			result &= false;
		}
		else
		{
			samples.resize(numSamples);
			
			for (auto & sample : samples)
			{
				sample = new HRIRSample();
				
				result &= fread(sample, sizeof(*sample), 1, file) == 1;
			}
		}
		
		return result;
	}
}
