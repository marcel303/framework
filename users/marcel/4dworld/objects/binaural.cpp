#include "binaural.h"
#include "../avgraph/vfxNodes/fourier.h"
#include <stdarg.h>
#include <xmmintrin.h>

#include "delaunay/delaunay.h"

#include "../avgraph/vfxNodes/fourier.cpp" // fixme

#define ENABLE_SSE 1
#define ENABLE_DEBUGGING 1
#define ENABLE_FOURIER4 1

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
	
	float dot(const HRIRSampleGrid::CellLocation & a, const HRIRSampleGrid::CellLocation & b)
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
		
		float temp[HRTF_BUFFER_SIZE];
		deinterleaveAudioBuffers_4(filterReal, lFilter.real, rFilter.real, temp, temp);
		deinterleaveAudioBuffers_4(filterImag, lFilter.imag, rFilter.imag, temp, temp);
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
		AudioBuffer & lResultOld,
		AudioBuffer & rResultOld,
		AudioBuffer & lResultNew,
		AudioBuffer & rResultNew)
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
		
		Fourier::fft1D((Fourier::float4*)convolvedReal, (Fourier::float4*)convolvedImag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		
		deinterleaveAudioBuffers_4(convolvedReal, lResultOld.real, rResultOld.real, lResultNew.real, rResultNew.real);
		deinterleaveAudioBuffers_4(convolvedImag, lResultOld.imag, rResultOld.imag, lResultNew.imag, rResultNew.imag);
	#else
		// convolve audio data with impulse-response data in the frequency-domain
		
		source.convolveAndReverseIndices(lFilterOld, lResultOld);
		source.convolveAndReverseIndices(rFilterOld, rResultOld);
		source.convolveAndReverseIndices(lFilterNew, lResultNew);
		source.convolveAndReverseIndices(rFilterNew, rResultNew);
		
		// transform convolved audio data back to the time-domain
		
		lResultOld.transformToTimeDomain(true);
		rResultOld.transformToTimeDomain(true);
		lResultNew.transformToTimeDomain(true);
		rResultNew.transformToTimeDomain(true);
	#endif
	
		debugTimerEnd("convolveAudio_2");
	}
	
	void rampAudioBuffers(
		const float * __restrict from,
		const float * __restrict to,
		float * __restrict result)
	{
	#if ENABLE_SSE && 0 // todo : test for correctness
		const float tStepScalar = 1.f / AUDIO_BUFFER_SIZE;
		
		const __m128 tStep = _mm_set1_ps(tStepScalar);
		__m128 t = _mm_set_ps(tStepScalar * 3.f, tStepScalar * 2.f, tStepScalar * 1.f, tStepScalar * 0.f);
		
		const __m128 * __restrict from4 = (__m128*)from;
		const __m128 * __restrict to4 = (__m128*)to;
		__m128 * __restrict result4 = (__m128*)result;
		
		const __m128 one = _mm_set1_ps(1.f);
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE / 4; ++i)
		{
			const __m128 fromWeight = one - t;
			const __m128 toWeight = t;
			
			result4[i] = from4[i] * fromWeight + to4[i] * toWeight;
			
			t += tStep;
		}
		
	#else
		const float tStep = 1.f / AUDIO_BUFFER_SIZE;
		
		float t = 0.f;
		
		for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
		{
			const float fromWeight = 1.f - t;
			const float toWeight = t;
			
			result[i] = from[i] * fromWeight + to[i] * toWeight;
			
			t += tStep;
		}
	#endif
	}
	
	void interleaveAudioBuffers_4(
		const float * __restrict array1,
		const float * __restrict array2,
		const float * __restrict array3,
		const float * __restrict array4,
		float4 * __restrict result)
	{
	#if ENABLE_SSE
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
			
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
			result[i * 4 + 0] = value1;
			result[i * 4 + 1] = value2;
			result[i * 4 + 2] = value3;
			result[i * 4 + 3] = value4;
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
	#if ENABLE_SSE
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
			
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
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
	#if ENABLE_SSE
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
			
			_MM_TRANSPOSE4_PS(value1, value2, value3, value4);
			
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
	
	//
	
	void elevationAndAzimuthToCartesian(const float elevation, const float azimuth, float & x, float & y, float & z)
	{
		const float degToRad = M_PI / 180.f;
		
		y = std::sin(elevation * degToRad);
		
		const float radius = std::sqrtf(1.f - y * y);
		
		x = std::cos(azimuth * degToRad) * radius;
		z = std::sin(azimuth * degToRad) * radius;
	}
	
	void cartesianToElevationAndAzimuth(const float x, const float y, const float z, float & elevation, float & azimuth)
	{
		const float radToDeg = 180.f / M_PI;
		
		azimuth = std::atan2(z, x) * radToDeg;
		elevation = std::asin(y) * radToDeg;
	}
	
	//
	
#if ENABLE_DEBUGGING && USE_FRAMEWORK
	static std::map<std::string, uint64_t> debugTimers;
	
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
	
	void AudioBuffer::convolveAndReverseIndices_4(
		const float4 * __restrict filterReal,
		const float4 * __restrict filterImag,
		float4 * __restrict outputReal,
		float4 * __restrict outputImag)
	{
		// todo : SSE optimize this code
		
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
				
				break;
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
			
			sampleWeights[0] = 1.f - baryU - baryV;
			sampleWeights[1] = baryV;
			sampleWeights[2] = baryU;
		
			result = true;
		}
		
		debugTimerEnd("lookup_hrir");
		
		return result;
	}
}
