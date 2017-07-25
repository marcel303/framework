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

#include <string>
#include <vector>

#ifndef ALIGN16
	#ifdef MACOS
		#define ALIGN16 __attribute__((aligned(16)))
	#else
		#define ALIGN16
	#endif
#endif

#define AUDIO_BUFFER_SIZE 256
#define HRIR_BUFFER_SIZE 512
#define HRTF_BUFFER_SIZE 512

namespace binaural
{
	// forward declarations

	struct AudioBuffer;
	struct HRIRSample;
	struct HRIRSampleData;
	struct HRTF;
	struct HRTFData;
	struct SoundData;

	// structures

	struct AudioBuffer
	{
		ALIGN16 float real[AUDIO_BUFFER_SIZE];
		ALIGN16 float imag[AUDIO_BUFFER_SIZE];
		
		void transformToFrequencyDomain(const bool fast = false);
		void transformToTimeDomain(const bool fast = false);
		void convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output);
	};

	struct HRIRSampleData
	{
		ALIGN16 float lSamples[HRIR_BUFFER_SIZE];
		ALIGN16 float rSamples[HRIR_BUFFER_SIZE];
	};

	struct HRIRSample
	{
		HRIRSampleData sampleData;

		int elevation;
		int azimuth;

		HRIRSample()
			: sampleData()
			, elevation(0)
			, azimuth(0)
		{
		}
		
		void init(const int _elevation, const int _azimuth)
		{
			elevation = _elevation;
			azimuth = _azimuth;
		}
	};

	struct HRIRSampleGrid
	{
		struct CellLocation
		{
			int elevation;
			int azimuth;
		};
		
		struct CellVertex
		{
			CellLocation location;
			
			HRIRSampleData * sampleData;
		};
		
		struct Cell
		{
			CellVertex vertex[3];
		};
		
		std::vector<Cell> cells;
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
			const int elevation,
			const int azimuth,
			const bool swapLR);
		
		void finalize();
		
		bool lookup_3(const int elevation, const int azimuth, HRIRSampleData const * * samples, float * sampleWeights) const;
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

	void convolveAudio(
		AudioBuffer & source,
		const HRTFData & lFilter,
		const HRTFData & rFilter,
		AudioBuffer & lResult,
		AudioBuffer & rResult);
	
	//
	
	void debugAssert(const bool condition);
	void debugLog(const char * format, ...);
	void listFiles(const char * path, bool recurse, std::vector<std::string> & result);
	bool parseInt32(const std::string & text, int & result);
	SoundData * loadSound(const char * filename);
}
