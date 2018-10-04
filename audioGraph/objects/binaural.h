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

#pragma once

/*

Binaural DSP routines
marcel303@gmail.com

Basic binauralization steps:

At startup,
- Load impulse-response measurement files from sound files. These files are usually uncompressed PCM/WAVE data.
- Convert stereo PCM/WAVE data to left/right HRIRs. HRIR being the head-realted impulse-response. One for each ear.
- Put the HRIR data for the left and right ear in a database, storing the elevation and azimuth along with it.

During runtime,
- Find the nearest HRIR sample points based on the direction of the sound source.
- Interpolate between the HRIR samples.
- Convert the interpolated HRIR from the time-domain into the frequency domain. The results are the HRTFs for the left and right ear.
- Convert the incoming audio data from the time-domain into the frequency domain.
- Convolve the HRTFs with the audio data in the frequency domain, for both the left and the right ear independently.
- Convert the result of the convolution back to the time domain.
- To avoid issues with the phase being offset between separate updates, either use windowing or glue blocks of audio data together and perform the convolution on the combined blocks.

For finding the nearest HRIR sample points, a Delaunay triangulation is used on the polar (azimuth, evevation)
vertices. The lookup results in the triangle composed of the three sample points that enclose the current
sampling point. The three HRIRs corresponding to the vertices are interpolated based on the barycentric coordinates
within the triangle.

*/

#include <list>
#include <string>
#include <vector>

#ifndef ALIGN16
	#if defined(MACOS) || defined(LINUX)
		#define ALIGN16 __attribute__((aligned(16)))
	#else
		#define ALIGN16 __declspec(align(16))
	#endif
#endif

#if !defined(BINAURAL_USE_SSE)
	#if __SSE2__
		#define BINAURAL_USE_SSE 1
	#else
		#define BINAURAL_USE_SSE 0 // do not alter
	#endif
#endif

#if !defined(BINAURAL_USE_NEON)
	#if defined(__arm__) || defined(__aarch64__)
		#define BINAURAL_USE_NEON 1
	#else
		#define BINAURAL_USE_NEON 0 // do not alter
	#endif
#endif

#if !defined(BINAURAL_USE_GCC_VECTOR)
	#if defined(__GNUC__)
		#define BINAURAL_USE_GCC_VECTOR 1
	#else
		#define BINAURAL_USE_GCC_VECTOR 0 // do not alter
	#endif
#endif

#if BINAURAL_USE_SSE || BINAURAL_USE_NEON || BINAURAL_USE_GCC_VECTOR
	#define BINAURAL_USE_SIMD 1
#else
	#define BINAURAL_USE_SIMD 0 // do not alter
#endif

#define ENABLE_DEBUGGING 0
#define ENABLE_FOURIER4 (BINAURAL_USE_SIMD && 1)

#if BINAURAL_USE_SSE
	#include <xmmintrin.h>
#endif

namespace binaural
{
	static const int HRIR_BUFFER_SIZE = 512;
	static const int HRTF_BUFFER_SIZE = HRIR_BUFFER_SIZE;
	static const int AUDIO_BUFFER_SIZE = HRIR_BUFFER_SIZE;

	// forward declarations

	struct AudioBuffer;
	struct HRIRSample;
	struct HRIRSampleData;
	struct HRTF;
	struct HRTFData;
	struct SoundData;
	
	// values
	
#ifdef WIN32
	extern __declspec(thread) bool enableDebugLog;
#else
	extern __thread bool enableDebugLog;
#endif
	
	// value types
	
#if BINAURAL_USE_SSE
	typedef __m128 float4;
#elif BINAURAL_USE_GCC_VECTOR
	typedef float float4 __attribute__ ((vector_size(16)));
#endif
	
	// structures

	struct AudioBuffer
	{
		ALIGN16 float real[AUDIO_BUFFER_SIZE];
		ALIGN16 float imag[AUDIO_BUFFER_SIZE];
		
		void transformToFrequencyDomain(const bool fast = false);
		void transformToTimeDomain(const bool fast = false);
		void convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output) const;
		void convolveAndReverseIndices(
			const HRTFData & filter,
			float * __restrict outputReal,
			float * __restrict outputImag) const;
	#if ENABLE_FOURIER4
		void convolveAndReverseIndices_4(
			const float4 * __restrict filterReal,
			const float4 * __restrict filterImag,
			float4 * __restrict outputReal,
			float4 * __restrict outputImag) const;
	#endif
	};
	
	struct AudioBuffer_Real
	{
		ALIGN16 float samples[AUDIO_BUFFER_SIZE];
	};

	struct HRIRSampleData
	{
		ALIGN16 float lSamples[HRIR_BUFFER_SIZE];
		ALIGN16 float rSamples[HRIR_BUFFER_SIZE];
	};

	struct HRIRSample
	{
		HRIRSampleData sampleData;

		float elevation;
		float azimuth;

		HRIRSample()
			: sampleData()
			, elevation(0)
			, azimuth(0)
		{
		}
		
		void init(const float _elevation, const float _azimuth)
		{
			elevation = _elevation;
			azimuth = _azimuth;
		}

	#if BINAURAL_USE_SSE
		void * operator new(size_t size)
		{
			return _mm_malloc(size, 32);
		}

		void operator delete(void * mem)
		{
			_mm_free(mem);
		}
	#endif
	};

	struct HRIRSampleGrid
	{
		static const int kGridSx = 20;
		static const int kGridSy = 40;
		
		struct Location
		{
			float elevation;
			float azimuth;
		};
		
		struct Vertex
		{
			Location location;
			
			int sampleIndex;
		};
		
		struct Triangle
		{
			Vertex vertex[3];
		};
		
		struct Cell
		{
			std::vector<int> triangleIndices;
		};
		
		std::vector<Triangle> triangles;
		
		Cell cells[kGridSx][kGridSy];
		
		HRIRSampleGrid()
			: triangles()
			, cells()
		{
		}
		
		const Cell * lookupCell(const float elevation, const float azimuth) const;
		
		const Triangle * lookupTriangle(const float elevation, const float azimuth, float & baryU, float & baryV) const;
		
		bool save(FILE * file) const;
		bool load(FILE * file);
	};
	
	struct HRIRSampleSet
	{
		std::vector<HRIRSample*> samples;
		
		HRIRSampleGrid sampleGrid;
		
		HRIRSampleSet()
			: samples()
			, sampleGrid()
		{
		}
		
		~HRIRSampleSet()
		{
			for (auto & sample : samples)
			{
				delete sample;
				sample = nullptr;
			}
			
			samples.clear();
		}
		
		bool addHrirSampleFromSoundData(
			const SoundData & soundData,
			const float elevation,
			const float azimuth,
			const bool swapLR);
		
		void finalize();
		
		bool lookup_3(const float elevation, const float azimuth, HRIRSampleData const * * samples, float * sampleWeights) const;
		
		bool save(FILE * file) const;
		bool load(FILE * file);
	};

	struct HRTFData
	{
		ALIGN16 float real[HRTF_BUFFER_SIZE];
		ALIGN16 float imag[HRTF_BUFFER_SIZE];
		
		void transformToFrequencyDomain();
	};

	struct HRTF
	{
		HRTFData lFilter;
		HRTFData rFilter;
	};

	struct SoundData
	{
		int numChannels; // 1 = mono, 2 = stereo
		int sampleSize; // bytes per sample. 1 = char, 2 = short, 4 = float
		int numSamples;
		void * sampleData;
		
		SoundData()
			: numChannels(0)
			, sampleSize(0)
			, numSamples(0)
			, sampleData(nullptr)
		{
		}
		
		~SoundData()
		{
			if (sampleData != nullptr)
			{
				free(sampleData);
				sampleData = nullptr;
			}
		}
	};
	
	struct Mutex
	{
		virtual ~Mutex()
		{
		}
		
		virtual void lock() = 0;
		virtual void unlock() = 0;
	};

	// functions

	bool convertSoundDataToHRIR(
		const SoundData & soundData,
		float * __restrict lSamples,
		float * __restrict rSamples);

	void blendHrirSamples_3(
		const HRIRSampleData & a, const float weightA,
		const HRIRSampleData & b, const float weightB,
		const HRIRSampleData & c, const float weightC,
		HRIRSampleData & result);
	
	void blendHrirSamples_3(
		HRIRSampleData const * const * samples,
		const float * sampleWeights,
		HRIRSampleData & result);
	
	void hrirToHrtf(
		const float * __restrict lSamples,
		const float * __restrict rSamples,
		HRTFData & lFilter,
		HRTFData & rFilter);
	
	void reverseSampleIndices(
		const float * __restrict src,
		float * __restrict dst);
		
	void convolveAudio(
		AudioBuffer & source,
		const HRTFData & lFilter,
		const HRTFData & rFilter,
		AudioBuffer & lResult,
		AudioBuffer & rResult);
	
	void convolveAudio_2(
		AudioBuffer & source,
		const HRTFData & lFilterOld,
		const HRTFData & rFilterOld,
		const HRTFData & lFilterNew,
		const HRTFData & rFilterNew,
		float * __restrict lResultOld,
		float * __restrict rResultOld,
		float * __restrict lResultNew,
		float * __restrict rResultNew);
	
	void rampAudioBuffers(
		const float * __restrict from,
		const float * __restrict to,
		const int numSamples,
		float * __restrict result);
	
#if ENABLE_FOURIER4
	void interleaveAudioBuffers_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result);
	
	void interleaveAudioBuffersAndReverseIndices_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result);

	void deinterleaveAudioBuffers_4(
		const float4 * __restrict interleaved,
		float * __restrict array1,
		float * __restrict array2,
		float * __restrict array3,
		float * __restrict array4);
	
	void deinterleaveAudioBuffers_4_to_2(
		const float4 * __restrict interleaved,
		float * __restrict array1,
		float * __restrict array2);
#endif

	// @see http://blackpawn.com/texts/pointinpoly/
	template <typename Vector>
	inline bool baryPointInTriangle(
		const Vector & A,
		const Vector & B,
		const Vector & C,
		const Vector & P,
		const float eps,
		float & baryU, float & baryV)
	{
		// Compute vectors
		const Vector v0 = C - A;
		const Vector v1 = B - A;
		const Vector v2 = P - A;

		// Compute dot products
		const auto dot00 = dot(v0, v0);
		const auto dot01 = dot(v0, v1);
		const auto dot02 = dot(v0, v2);
		const auto dot11 = dot(v1, v1);
		const auto dot12 = dot(v1, v2);

		// Compute barycentric coordinates
		const float invDenom = 1.f / float(dot00 * dot11 - dot01 * dot01);
		const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		// Check if point is in triangle
		if ((u >= 0.f - eps) && (v >= 0.f - eps) && (u + v < 1.f + eps))
		{
			baryU = u;
			baryV = v;
			
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void elevationAndAzimuthToCartesian(const float elevation, const float azimuth, float & x, float & y, float & z);
	void cartesianToElevationAndAzimuth(const float x, const float y, const float z, float & elevation, float & azimuth);
	
	//
	
#if ENABLE_DEBUGGING
	void debugAssert(const bool condition);
	void debugLog(const char * format, ...);
	void debugTimerBegin(const char * name);
	void debugTimerEnd(const char * name);
#else
	inline void debugAssert(const bool condition)
	{
	}
	
	inline void debugLog(const char * format, ...)
	{
	}
	
	inline void debugTimerBegin(const char * name)
	{
	}
	
	inline void debugTimerEnd(const char * name)
	{
	}
#endif
	
	void listFiles(const char * path, bool recurse, std::vector<std::string> & result);
	bool parseInt32(const std::string & text, int & result);
	SoundData * loadSound(const char * filename);
}
