/*
	Copyright (C) 2020 Marcel Smit
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
#include <algorithm>
#include <stdarg.h>
#include <stdio.h>
#include <string.h> // memset, memcpy

#define BINAURAL_USE_FRAMEWORK 1

#if BINAURAL_USE_FRAMEWORK
	#include "soundfile/SoundIO.h" // loadSound
	#include "Parse.h"
	#include "StringEx.h" // sprintf_s
	#include "Timer.h"
#ifdef WIN32
	#include <Windows.h>
	#undef min
	#undef max
#else
	#include <dirent.h>
#endif
#endif

#if BINAURAL_USE_SSE
	#include <xmmintrin.h>
	#include <immintrin.h>
#endif

#if BINAURAL_USE_SIMD == 0
	#ifdef __SSE2__
		#warning "BINAURAL_USE_SSE is set to 0. is this intended?"
	#endif
#endif

#if BINAURAL_ENABLE_WDL_FFT
	#include "WDL/fft.h"
#endif

#if BINAURAL_ENABLE_DEBUGGING
	#include <assert.h>
	#include <map>
#endif

#define WDL_REAL_FFT_TEST 0

/*

done : quality :
+ the audio seems to have a tin-can feel after being processed. find out why. perhaps we need a low-pass filter ?
	-> the reason is due to the way HRIR samples are combined. this creates a comb-like filter effect and colors the sound. the effect is not present when using the OpenAL soft HRTF filters, as they are properly processed to be able to blend them

todo : performance :
- batch binaural object : most of the remaining cost is in performing the fourier transformations prior and after the convolution step is performed. for the source to frequency <-> time domain transformations, it's possible to do four of them at once when we create a batch binauralizer object. this object would then perform binauralization on four streams at a time. convolution etc which operates on the left and right channels separately could be made to use 8-component AVX instructions, for additional gains
- optimize the time domain to frequency domain transformation by using a FFT optimized for working with real-values inputs. since the last 50% of the fourier transformation is a mirror of the first 50% (symmetry), the algortihm could be optimized to only compute the first half

*/

#if BINAURAL_USE_NEON
	#include "neon-transpose.h"
#endif

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

	inline __m128 operator*(__m128 a, float b)
	{
		return _mm_mul_ps(a, _mm_set1_ps(b));
	}

#endif

#if BINAURAL_USE_SSE
	// for SSE we do not need to define any compatibility functions
#elif BINAURAL_USE_NEON
	// use an external dependency to translate SSE intrinsics to NEON intrinsics
	#include "sse2neon.h"
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
		const HRIRSample & a, const float aWeight,
		const HRIRSample & b, const float bWeight,
		const HRIRSample & c, const float cWeight,
		HRIRSample & r)
	{
		debugTimerBegin("blendHrirSamples_3");
		debugAssert(aWeight >= -1e-3f && aWeight <= 1.f + 1e-3f);
		debugAssert(bWeight >= -1e-3f && bWeight <= 1.f + 1e-3f);
		debugAssert(cWeight >= -1e-3f && cWeight <= 1.f + 1e-3f);
		debugAssert(fabsf(aWeight + bWeight + cWeight - 1.f) <= 1e-3f);
		
	#if BINAURAL_USE_SIMD
		const float4 * __restrict a_lSamples = (float4*)a.sampleData.lSamples;
		const float4 * __restrict b_lSamples = (float4*)b.sampleData.lSamples;
		const float4 * __restrict c_lSamples = (float4*)c.sampleData.lSamples;

		const float4 * __restrict a_rSamples = (float4*)a.sampleData.rSamples;
		const float4 * __restrict b_rSamples = (float4*)b.sampleData.rSamples;
		const float4 * __restrict c_rSamples = (float4*)c.sampleData.rSamples;

		const float4 aWeight4 = _mm_set1_ps(aWeight);
		const float4 bWeight4 = _mm_set1_ps(bWeight);
		const float4 cWeight4 = _mm_set1_ps(cWeight);

		float4 * __restrict r_lSamples = (float4*)r.sampleData.lSamples;
		float4 * __restrict r_rSamples = (float4*)r.sampleData.rSamples;

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
			r.sampleData.lSamples[i] =
				a.sampleData.lSamples[i] * aWeight +
				b.sampleData.lSamples[i] * bWeight +
				c.sampleData.lSamples[i] * cWeight;

			r.sampleData.rSamples[i] =
				a.sampleData.rSamples[i] * aWeight +
				b.sampleData.rSamples[i] * bWeight +
				c.sampleData.rSamples[i] * cWeight;
		}
	#endif
	
		r.sampleDelayL =
			a.sampleDelayL * aWeight +
			b.sampleDelayL * bWeight +
			c.sampleDelayL * cWeight;
		
		r.sampleDelayR =
			a.sampleDelayR * aWeight +
			b.sampleDelayR * bWeight +
			c.sampleDelayR * cWeight;
	
		debugTimerEnd("blendHrirSamples_3");
	}
	
	void blendHrirSamples_3(
		HRIRSample const * const * samples,
		const float * sampleWeights,
		HRIRSample & result)
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
		
	#if BINAURAL_ENABLE_FOURIER4
		// this will generate the HRTF from the HRIR samples
		
		float4 filterReal[HRTF_BUFFER_SIZE];
		float4 filterImag[HRTF_BUFFER_SIZE];
		
		interleaveAudioBuffersAndReverseIndices_4(lSamples, rSamples, lSamples, rSamples, filterReal);
		memset(filterImag, 0, sizeof(filterImag));
		
		Fourier::fft1D(filterReal, filterImag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
		
		deinterleaveAudioBuffers_4_to_2(filterReal, lFilter.real, rFilter.real);
		deinterleaveAudioBuffers_4_to_2(filterImag, lFilter.imag, rFilter.imag);
	#elif BINAURAL_ENABLE_WDL_FFT
		static bool isInit = false;
		
		if (isInit == false)
		{
			isInit = true;
			
			WDL_fft_init();
		}
		
		WDL_FFT4_COMPLEX samples_wdl[HRTF_BUFFER_SIZE];
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			samples_wdl[i].re.v = _mm_set_ps(0.f, 0.f, rSamples[i], lSamples[i]);
			samples_wdl[i].im.v = _mm_setzero_ps();
		}
		
		WDL_fft4(samples_wdl, HRTF_BUFFER_SIZE, false);
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			lFilter.real[i] = samples_wdl[i].re.elem(0);
			lFilter.imag[i] = samples_wdl[i].im.elem(0);
			
			rFilter.real[i] = samples_wdl[i].re.elem(1);
			rFilter.imag[i] = samples_wdl[i].im.elem(1);
		}
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
	#if !BINAURAL_ENABLE_FOURIER4 && BINAURAL_ENABLE_WDL_FFT
		memcpy(dst, src, AUDIO_BUFFER_SIZE * sizeof(float));
	#else
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			dst[fftIndices.indices[i]] = src[i];
		}
	#endif
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
		
	#if BINAURAL_ENABLE_FOURIER4
		// transform audio data from the time-domain into the frequency-domain
		
		source.transformToFrequencyDomain(true);
		
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
	#elif BINAURAL_ENABLE_WDL_FFT
		static bool isInit = false;
		
		if (isInit == false)
		{
			isInit = true;
			
			WDL_fft_init();
		}
		
		// transform audio data from the time-domain into the frequency-domain
		
		WDL_FFT_COMPLEX source_wdl[AUDIO_BUFFER_SIZE];
		
	#if WDL_REAL_FFT_TEST
	// todo : use WDL_real_fft since the source data has only zeroes for the imaginary part
		memset(source_wdl, 0, sizeof(source_wdl));
		memcpy(source_wdl, source.real, AUDIO_BUFFER_SIZE * sizeof(float));
		WDL_real_fft((WDL_FFT_REAL*)source_wdl, AUDIO_BUFFER_SIZE, false);
		
		static int n = 0;
		n++;
		
		if (n >= 10000)
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
			printf("R: %03d : %.3f , %.3f\n", i, source_wdl[i].re/2.f, source_wdl[i].im/2.f);
	#endif
	
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			source_wdl[i].re = source.real[i];
			source_wdl[i].im = source.imag[i];
		}
		
		WDL_fft(source_wdl, AUDIO_BUFFER_SIZE, false);
		
	#if WDL_REAL_FFT_TEST
		if (n >= 10000)
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
			printf("C: %03d : %.3f , %.3f\n", i, source_wdl[i].re, source_wdl[i].im);
	#endif
		
		WDL_FFT4_COMPLEX filter_wdl[HRTF_BUFFER_SIZE]; // lOld, rOld, lNew, rNew

	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		// 0.113ms -> 0.075ms for 100x
		const float4 * __restrict lFilterOld_re_4 = (const float4*)lFilterOld.real;
		const float4 * __restrict rFilterOld_re_4 = (const float4*)rFilterOld.real;
		const float4 * __restrict lFilterNew_re_4 = (const float4*)lFilterNew.real;
		const float4 * __restrict rFilterNew_re_4 = (const float4*)rFilterNew.real;
		
		const float4 * __restrict lFilterOld_im_4 = (const float4*)lFilterOld.imag;
		const float4 * __restrict rFilterOld_im_4 = (const float4*)rFilterOld.imag;
		const float4 * __restrict lFilterNew_im_4 = (const float4*)lFilterNew.imag;
		const float4 * __restrict rFilterNew_im_4 = (const float4*)rFilterNew.imag;
		
		for (int i = 0; i < HRTF_BUFFER_SIZE / 4; ++i)
		{
			float4 re1 = lFilterOld_re_4[i];
			float4 re2 = rFilterOld_re_4[i];
			float4 re3 = lFilterNew_re_4[i];
			float4 re4 = rFilterNew_re_4[i];
		
			_MM_TRANSPOSE4_PS(re1, re2, re3, re4);
			
			filter_wdl[i * 4 + 0].re.v = re1;
			filter_wdl[i * 4 + 1].re.v = re2;
			filter_wdl[i * 4 + 2].re.v = re3;
			filter_wdl[i * 4 + 3].re.v = re4;
			
			float4 im1 = lFilterOld_im_4[i];
			float4 im2 = rFilterOld_im_4[i];
			float4 im3 = lFilterNew_im_4[i];
			float4 im4 = rFilterNew_im_4[i];
			
			_MM_TRANSPOSE4_PS(im1, im2, im3, im4);
			
			filter_wdl[i * 4 + 0].im.v = im1;
			filter_wdl[i * 4 + 1].im.v = im2;
			filter_wdl[i * 4 + 2].im.v = im3;
			filter_wdl[i * 4 + 3].im.v = im4;
		}
	#else
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			filter_wdl[i].re.v = _mm_set_ps(
				rFilterNew.real[i],
				lFilterNew.real[i],
				rFilterOld.real[i],
				lFilterOld.real[i]);
			filter_wdl[i].im.v = _mm_set_ps(
				rFilterNew.imag[i],
				lFilterNew.imag[i],
				rFilterOld.imag[i],
				lFilterOld.imag[i]);
		}
	#endif
		
		// convolve audio data with impulse-response data in the frequency-domain
		
		WDL_fft4_complexmul1(filter_wdl, source_wdl, AUDIO_BUFFER_SIZE);
		
		// transform convolved audio data back to the time-domain
		
		WDL_fft4(filter_wdl, AUDIO_BUFFER_SIZE, true);
		
		const float scale = 1.f / AUDIO_BUFFER_SIZE;
		
	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		float4 * __restrict lResultOld_4 = (float4*)lResultOld;
		float4 * __restrict rResultOld_4 = (float4*)rResultOld;
		float4 * __restrict lResultNew_4 = (float4*)lResultNew;
		float4 * __restrict rResultNew_4 = (float4*)rResultNew;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
			const int sortedIndex = i;
			
		#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
			float4 value1 = filter_wdl[i * 4 + 0].re.v;
			float4 value2 = filter_wdl[i * 4 + 1].re.v;
			float4 value3 = filter_wdl[i * 4 + 2].re.v;
			float4 value4 = filter_wdl[i * 4 + 3].re.v;

			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
			lResultOld_4[sortedIndex] = value1 * scale;
			rResultOld_4[sortedIndex] = value2 * scale;
			lResultNew_4[sortedIndex] = value3 * scale;
			rResultNew_4[sortedIndex] = value4 * scale;
		#else
			#error
		#endif
		}
	#else
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			const int sortedIndex = i;

			const WDL_FFT4_REAL value = filter_wdl[i].re * scale;
			
			lResultOld[sortedIndex] = value.elem(0);
			rResultOld[sortedIndex] = value.elem(1);
			lResultNew[sortedIndex] = value.elem(2);
			rResultNew[sortedIndex] = value.elem(3);
		}
	#endif
	#else
		// transform audio data from the time-domain into the frequency-domain
		
		source.transformToFrequencyDomain(true);
		
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
		
	#if BINAURAL_ENABLE_DEBUGGING
		float * t1_array = (float*)&t1;
		float * t2_array = (float*)&t2;
		debugAssert(t2[3] == tStepScalar * 7.f);
	#endif

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
	
#if BINAURAL_ENABLE_FOURIER4
	void interleaveAudioBuffers_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result)
	{
	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		const float4 * __restrict array1_4 = (float4*)array1;
		const float4 * __restrict array2_4 = (float4*)array2;
		const float4 * __restrict array3_4 = (float4*)array3;
		const float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; ++i)
		{
		#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
			float4 value1a = array1_4[i * 2 + 0];
			float4 value2a = array2_4[i * 2 + 0];
			float4 value3a = array3_4[i * 2 + 0];
			float4 value4a = array4_4[i * 2 + 0];
			
			float4 value1b = array1_4[i * 2 + 1];
			float4 value2b = array2_4[i * 2 + 1];
			float4 value3b = array3_4[i * 2 + 1];
			float4 value4b = array4_4[i * 2 + 1];
			
			_MM_TRANSPOSE4_PS(value1a, value2a, value3a, value4a);
			_MM_TRANSPOSE4_PS(value1b, value2b, value3b, value4b);
			
			result[i * 8 + 0] = value1a;
			result[i * 8 + 1] = value2a;
			result[i * 8 + 2] = value3a;
			result[i * 8 + 3] = value4a;
			
			result[i * 8 + 4] = value1b;
			result[i * 8 + 5] = value2b;
			result[i * 8 + 6] = value3b;
			result[i * 8 + 7] = value4b;
		#else
			#error
		#endif
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
	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		const float4 * __restrict array1_4 = (float4*)array1;
		const float4 * __restrict array2_4 = (float4*)array2;
		const float4 * __restrict array3_4 = (float4*)array3;
		const float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
			const int index1 = fftIndices.indices[i * 4 + 0];
			const int index2 = fftIndices.indices[i * 4 + 1];
			const int index3 = fftIndices.indices[i * 4 + 2];
			const int index4 = fftIndices.indices[i * 4 + 3];
			
		#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
			float4 value1 = array1_4[i];
			float4 value2 = array2_4[i];
			float4 value3 = array3_4[i];
			float4 value4 = array4_4[i];
			
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
			result[index1] = value1;
			result[index2] = value2;
			result[index3] = value3;
			result[index4] = value4;
		#else
			#error
		#endif
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
	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		float4 * __restrict array1_4 = (float4*)array1;
		float4 * __restrict array2_4 = (float4*)array2;
		float4 * __restrict array3_4 = (float4*)array3;
		float4 * __restrict array4_4 = (float4*)array4;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
		#if BINAURAL_USE_SSE
			float4 value1 = interleaved[i * 4 + 0];
			float4 value2 = interleaved[i * 4 + 1];
			float4 value3 = interleaved[i * 4 + 2];
			float4 value4 = interleaved[i * 4 + 3];
			
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
			array1_4[i] = value1;
			array2_4[i] = value2;
			array3_4[i] = value3;
			array4_4[i] = value4;
		#elif BINAURAL_USE_NEON
			// note : vld4q_f32 is a spread-load instruction, which
			//        deinterleaves ('transposes') the values for us!
			const float32x4x4_t value = vld4q_f32(&interleaved[i * 4]);
			
			array1_4[i] = value.val[0];
			array2_4[i] = value.val[1];
			array3_4[i] = value.val[2];
			array4_4[i] = value.val[3];
		#else
			#error
		#endif
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
	#if BINAURAL_USE_SSE || BINAURAL_USE_NEON
		float4 * __restrict array1_4 = (float4*)array1;
		float4 * __restrict array2_4 = (float4*)array2;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; ++i)
		{
		#if BINAURAL_USE_SSE
			float4 value1a = interleaved[i * 8 + 0];
			float4 value2a = interleaved[i * 8 + 1];
			float4 value3a = interleaved[i * 8 + 2];
			float4 value4a = interleaved[i * 8 + 3];
			
			float4 value1b = interleaved[i * 8 + 4];
			float4 value2b = interleaved[i * 8 + 5];
			float4 value3b = interleaved[i * 8 + 6];
			float4 value4b = interleaved[i * 8 + 7];
			
			_MM_TRANSPOSE4_PS(value1a, value2a, value3a, value4a);
			_MM_TRANSPOSE4_PS(value1b, value2b, value3b, value4b);
			
			array1_4[i * 2 + 0] = value1a;
			array2_4[i * 2 + 0] = value2a;
			
			array1_4[i * 2 + 1] = value1b;
			array2_4[i * 2 + 1] = value2b;
		#elif BINAURAL_USE_NEON
			// note : vld4q_f32 is a spread-load instruction, which
			//        deinterleaves ('transposes') the values for us!
			const float32x4x4_t value_a = vld4q_f32(&interleaved[i * 8 + 0]);
			const float32x4x4_t value_b = vld4q_f32(&interleaved[i * 8 + 4]);
			
			array1_4[i * 2 + 0] = value_a.val[0];
			array2_4[i * 2 + 0] = value_a.val[1];
			
			array1_4[i * 2 + 1] = value_b.val[0];
			array2_4[i * 2 + 1] = value_b.val[1];
		#else
			#error
		#endif
		}
	#else
		const float * __restrict interleavedScalar = (float*)interleaved;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			array1[i] = interleavedScalar[i * 2 + 0];
			array2[i] = interleavedScalar[i * 2 + 1];
		}
	#endif
	}
#endif
	
	//
	
	void elevationAndAzimuthToCartesian(const float elevation, const float azimuth, float & x, float & y, float & z)
	{
		const float degToRad = float(M_PI) / 180.f;
		
		y = std::sin(elevation * degToRad);
		
		x = std::cos(azimuth * degToRad) * std::cos(elevation * degToRad);
		z = std::sin(azimuth * degToRad) * std::abs(std::cos(elevation * degToRad));
	}
	
	void cartesianToElevationAndAzimuth(const float x, const float y, const float z, float & elevation, float & azimuth)
	{
		const float radToDeg = 180.f / float(M_PI);
		
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
	}
	
	//
	
#if BINAURAL_ENABLE_DEBUGGING && BINAURAL_USE_FRAMEWORK
	static std::map<std::string, uint64_t> debugTimers;
	
	void debugAssert(const bool condition)
	{
		assert(condition);
		
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
	
#if BINAURAL_USE_FRAMEWORK
	void listFiles(const char * path, bool recurse, std::vector<std::string> & result)
	{
		debugTimerBegin("list_files");
		
	#ifdef WIN32
		WIN32_FIND_DATAA ffd;
		char wildcard[MAX_PATH];
		sprintf_s(wildcard, sizeof(wildcard), "%s\\*", path);
		HANDLE find = FindFirstFileA(wildcard, &ffd);
		if (find != INVALID_HANDLE_VALUE)
		{
			do
			{
				char fullPath[MAX_PATH];
				if (strcmp(path, "."))
					sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ffd.cFileName);
				else
					strcpy_s(fullPath, sizeof(fullPath), ffd.cFileName);

				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (recurse && strcmp(ffd.cFileName, ".") && strcmp(ffd.cFileName, ".."))
					{
						listFiles(fullPath, recurse, result);
					}
				}
				else
				{
					result.push_back(fullPath);
				}
			} while (FindNextFileA(find, &ffd) != 0);

			FindClose(find);
		}
	#else
		std::vector<DIR*> dirs;
		{
			DIR * dir = opendir(path);
			if (dir)
				dirs.push_back(dir);
		}
		
		while (!dirs.empty())
		{
			DIR * dir = dirs.back();
			dirs.pop_back();
			
			dirent * ent;
			
			while ((ent = readdir(dir)) != 0)
			{
				char fullPath[PATH_MAX];
				if (strcmp(path, "."))
					sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ent->d_name);
				else
					strcpy_s(fullPath, sizeof(fullPath), ent->d_name);
				
				if (ent->d_type == DT_DIR)
				{
					if (recurse && strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
					{
						listFiles(fullPath, recurse, result);
					}
				}
				else
				{
					result.push_back(fullPath);
				}
			}
			
			closedir(dir);
		}
	#endif
		
		debugTimerEnd("list_files");
	}
	
	bool parseInt32(const std::string & text, int & result)
	{
		return Parse::Int32(text, result);
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
	
#if BINAURAL_ENABLE_FOURIER4 || BINAURAL_ENABLE_WDL_FFT
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
			sample->init(elevation, azimuth, 0.f, 0.f);
			
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
						// duplicates are expected, as triangles share vertices. so either the sample should be
						// invalid, or set to the same sample index we're about to assign
						debugAssert(triangleSamples[i] == -1 || triangleSamples[i] == sampleIndex);
						
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
	
	bool HRIRSampleSet::lookup_3(const float elevation, const float azimuth, HRIRSample const * * samples, float * sampleWeights) const
	{
		debugTimerBegin("lookup_hrir");
		
		// lookup HRIR sample points using triangulation result
		
		bool result = false;
		
		float baryU;
		float baryV;
		
		auto triangle = sampleGrid.lookupTriangle(elevation, azimuth, baryU, baryV);
		
		if (triangle != nullptr)
		{
			samples[0] = this->samples[triangle->vertex[0].sampleIndex];
			samples[1] = this->samples[triangle->vertex[1].sampleIndex];
			samples[2] = this->samples[triangle->vertex[2].sampleIndex];
			
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
