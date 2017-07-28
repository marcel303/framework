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
#include <xmmintrin.h>

#ifndef ALIGN16
	#ifdef MACOS
		#define ALIGN16 __attribute__((aligned(16)))
	#else
		#define ALIGN16
	#endif
#endif

#define HRIR_BUFFER_SIZE 512
#define HRTF_BUFFER_SIZE HRIR_BUFFER_SIZE
#define AUDIO_BUFFER_SIZE HRIR_BUFFER_SIZE

namespace binaural
{
	// forward declarations

	struct AudioBuffer;
	struct HRIRSample;
	struct HRIRSampleData;
	struct HRTF;
	struct HRTFData;
	struct SoundData;
	
	// value types
	
	typedef __m128 float4;
	
	// structures

	struct AudioBuffer
	{
		ALIGN16 float real[AUDIO_BUFFER_SIZE];
		ALIGN16 float imag[AUDIO_BUFFER_SIZE];
		
		void transformToFrequencyDomain(const bool fast = false);
		void transformToTimeDomain(const bool fast = false);
		void convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output);
		void convolveAndReverseIndices_4(
			const float4 * __restrict filterReal,
			const float4 * __restrict filterImag,
			float4 * __restrict outputReal,
			float4 * __restrict outputImag);
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
			
			HRIRSampleData * sampleData;
		};
		
		struct Triangle
		{
			Vertex vertex[3];
		};
		
		struct Cell
		{
			std::vector<Triangle*> triangles;
		};
		
		std::list<Triangle> triangles;
		
		Cell cells[kGridSx][kGridSy];
		
		HRIRSampleGrid()
			: triangles()
			, cells()
		{
		}
		
		const Cell * lookupCell(const float elevation, const float azimuth) const;
		
		const Triangle * lookupTriangle(const float elevation, const float azimuth, float & baryU, float & baryV) const;
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
			free(sampleData);
			sampleData = nullptr;
		}
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
		AudioBuffer & lResultOld,
		AudioBuffer & rResultOld,
		AudioBuffer & lResultNew,
		AudioBuffer & rResultNew);
	
	void rampAudioBuffers(
		const float * __restrict from,
		const float * __restrict to,
		const int numSamples,
		float * __restrict result);
	
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
	
	// @see http://blackpawn.com/texts/pointinpoly/
	template <typename Vector>
	inline bool baryPointInTriangle(
		const Vector & A,
		const Vector & B,
		const Vector & C,
		const Vector & P,
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
		if ((u >= 0.f) && (v >= 0.f) && (u + v < 1.f))
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
	
	void debugAssert(const bool condition);
	void debugLog(const char * format, ...);
	void debugTimerBegin(const char * name);
	void debugTimerEnd(const char * name);
	
	void listFiles(const char * path, bool recurse, std::vector<std::string> & result);
	bool parseInt32(const std::string & text, int & result);
	SoundData * loadSound(const char * filename);
}
