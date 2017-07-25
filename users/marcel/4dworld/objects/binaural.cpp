#include "binaural.h"
#include "../avgraph/vfxNodes/fourier.h"
#include <stdarg.h>
#include <xmmintrin.h>

#include "delaunay/delaunay.h"

#include "../avgraph/vfxNodes/fourier.cpp" // fixme

#define ENABLE_SSE 1
#define ENABLE_DEBUGGING 1

#define USE_FRAMEWORK 1

#if USE_FRAMEWORK
	#include "audio.h"
	#include "framework.h"
	#include "Parse.h"
	#include "Timer.h"
#endif

namespace binaural
{
	typedef Vector2<float> SampleLocation;
	typedef Triangle<float> SampleTriangle;
	
	HRIRSampleGrid::CellLocation operator-(const HRIRSampleGrid::CellLocation & a, const HRIRSampleGrid::CellLocation & b)
	{
		HRIRSampleGrid::CellLocation result;
		result.elevation = a.elevation - b.elevation;
		result.azimuth = a.azimuth - b.azimuth;
		return result;
	}
	
	int dot(const HRIRSampleGrid::CellLocation & a, const HRIRSampleGrid::CellLocation & b)
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
	#if ENABLE_SSE
		const int bufferSize4 = HRIR_BUFFER_SIZE / 4;

		const __m128 * __restrict a_lSamples = (__m128*)a.lSamples;
		const __m128 * __restrict b_lSamples = (__m128*)b.lSamples;
		const __m128 * __restrict c_lSamples = (__m128*)c.lSamples;

		const __m128 * __restrict a_rSamples = (__m128*)a.rSamples;
		const __m128 * __restrict b_rSamples = (__m128*)b.rSamples;
		const __m128 * __restrict c_rSamples = (__m128*)c.rSamples;

		const __m128 aWeight4 = _mm_set1_ps(aWeight);
		const __m128 bWeight4 = _mm_set1_ps(bWeight);
		const __m128 cWeight4 = _mm_set1_ps(cWeight);

		__m128 * __restrict r_lSamples = (__m128*)r.lSamples;
		__m128 * __restrict r_rSamples = (__m128*)r.rSamples;

		for (int i = 0; i < bufferSize4; ++i)
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
		// this will generate the HRTF from the HRIR samples
		
		memset(lFilter.imag, 0, sizeof(lFilter.imag));
		memset(rFilter.imag, 0, sizeof(rFilter.imag));
		
		for (int i = 0; i < HRIR_BUFFER_SIZE; ++i)
		{
			lFilter.real[fftIndices.indices[i]] = lSamples[i];
			rFilter.real[fftIndices.indices[i]] = rSamples[i];
		}
		
		Fourier::fft1D(lFilter.real, lFilter.imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
		Fourier::fft1D(rFilter.real, rFilter.imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
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
	
	//
	
#if ENABLE_DEBUGGING && USE_FRAMEWORK
	void debugAssert(const bool condition)
	{
		fassert(condition);
	}
	
	void debugLog(const char * format, ...)
	{
		char text[1024];
		va_list args;
		va_start(args, format);
	#ifdef WIN32
		vsprintf_s(text, sizeof(text), format, args);
	#else
		vsnprintf(text, sizeof(text), format, args);
	#endif
		va_end(args);
		
		logDebug(text);
	}
	
	static std::map<std::string, uint64_t> debugTimers;
	
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
		
		debugLog("timer %s took %.2fms", name, debugTimer / 1000.f);
	}
#else
	void debugAssert(const bool condition)
	{
	}
	
	void debugLog(const char * format, ...)
	{
	}
	
	void debugTimerBegin(const char * name)
	{
	}
	
	void debugTimerEnd(const char * name)
	{
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

	void AudioBuffer::convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output)
	{
		// todo : SSE optimize this code
		
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
	
	const HRIRSampleGrid::Cell * HRIRSampleGrid::lookup(const float elevation, const float azimuth, float & baryU, float & baryV) const
	{
		CellLocation sampleLocation;
		sampleLocation.elevation = elevation;
		sampleLocation.azimuth = azimuth;
		
		const Cell * result = nullptr;
		
		for (auto & cell : cells)
		{
			if (baryPointInTriangle(
				cell.vertex[0].location,
				cell.vertex[1].location,
				cell.vertex[2].location,
				sampleLocation,
				baryU, baryV))
			{
				result = &cell;
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
		
	#if 0
		std::vector<SampleLocation> sampleLocations;
		sampleLocations.resize(samples.size() * 4);
		
		int offset[4][2] =
		{
			{   0,   0 },
			{ 360,   0 },
			{ 360, 180 },
			{   0, 180 },
		};
		
		for (auto & sample : samples)
		{
			sample->elevation = (sample->elevation + 180) % 180;
			sample->azimuth   = (sample->azimuth   + 360) % 360;
			
			for (int i = 0; i < 4; ++i)
			{
				SampleLocation & sampleLocation = sampleLocations[index++];
				
				sampleLocation.x = (sample->elevation + offset[i][1]) % 180;
				sampleLocation.y = (sample->azimuth   + offset[i][0]) % 360;
			}
		}
	#else
		std::vector<SampleLocation> sampleLocations;
		sampleLocations.resize(samples.size());
		
		int index = 0;
		
		for (auto & sample : samples)
		{
			SampleLocation & sampleLocation = sampleLocations[index++];
			
			sampleLocation.x = sample->elevation;
			sampleLocation.y = sample->azimuth;
		}
	#endif
	
		Delaunay<float> delaunay;
		const std::vector<SampleTriangle> triangles = delaunay.triangulate(sampleLocations);
		
		debugTimerEnd("triangulation");
		
		debugLog("got a bunch of triangles back from the triangulator! numTriangles=%d", triangles.size());
		
		debugTimerBegin("grid_insertion");
		
		// now we've got the triangulation.. store the triangles into a grid for fast lookups
		
		for (auto & triangle : triangles)
		{
			// find matching HRIR samples
			
			// todo : use a faster lookup mechanism here
			
			const SampleLocation triangleLocations[3] =
			{
				triangle.p1,
				triangle.p2,
				triangle.p3,
			};
			
			HRIRSample * triangleSamples[3];
			
			for (int i = 0; i < 3; ++i)
			{
				auto & triangleLocation = triangleLocations[i];
				
				triangleSamples[i] = nullptr;
				
				for (auto & sample : samples)
				{
					if (sample->elevation == triangleLocation.x && sample->azimuth == triangleLocation.y)
					{
						debugAssert(triangleSamples[i] == nullptr);
						
						triangleSamples[i] = sample;
					}
				}
				
				debugAssert(triangleSamples[i] != nullptr);
			}
			
			if (triangleSamples[0] == nullptr ||
				triangleSamples[1] == nullptr ||
				triangleSamples[2] == nullptr)
			{
				debugLog("failed to localize samples!");
			}
			else
			{
				HRIRSampleGrid::Cell cell;
				
				for (int i = 0; i < 3; ++i)
				{
					cell.vertex[i].location.elevation = triangleSamples[i]->elevation;
					cell.vertex[i].location.azimuth = triangleSamples[i]->azimuth;
					cell.vertex[i].sampleData = &triangleSamples[i]->sampleData;
				}
				
				sampleGrid.cells.push_back(cell);
			}
		}
		
		debugTimerEnd("grid_insertion");
	}
	
	bool HRIRSampleSet::lookup_3(const float elevation, const float azimuth, HRIRSampleData const * * samples, float * sampleWeights) const
	{
		debugTimerBegin("lookup_hrir");
		
		// todo : lookup HRIR sample points using triangulation result
		
		bool result = false;
		
		float baryU;
		float baryV;
		
		auto cell = sampleGrid.lookup(elevation, azimuth, baryU, baryV);
		
		if (cell != nullptr)
		{
			samples[0] = cell->vertex[0].sampleData;
			samples[1] = cell->vertex[1].sampleData;
			samples[2] = cell->vertex[2].sampleData;
			
			sampleWeights[0] = baryU;
			sampleWeights[1] = baryV;
			sampleWeights[2] = 1.f - baryU - baryV;
		
			result = true;
		}
		
		debugTimerEnd("lookup_hrir");
		
		return result;
	}
}
