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

#include "audio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "Calc.h"
#include "fourier.h"
#include "framework.h"
#include "paobject.h"
#include "Parse.h"
#include "Path.h"
#include "StringEx.h"
#include "testBase.h"
#include "Timer.h"
#include <complex>

// todo : use a longer test sound
// todo : make 3D view optional

#define HRTF_BUFFER_SIZE 512
#define AUDIO_BUFFER_SIZE 512
#define AUDIO_UPDATE_SIZE (AUDIO_BUFFER_SIZE/2)

#define SAMPLE_RATE 44100

#ifdef MACOS
	#define ALIGN16 __attribute__((aligned(16)))
#else
	#define ALIGN16
#endif

#define HACK2D 1

extern const int GFX_SX;
extern const int GFX_SY;

//

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

struct AudioBuffer;
struct HRIRSampleLocation;
struct HRTFData;

//

static void convolveAudio(
	AudioBuffer & source,
	const HRTFData & lFilter,
	const HRTFData & rFilter,
	AudioBuffer & lResult,
	AudioBuffer & rResult);

static void audioBufferMul(float * __restrict audioBuffer, const int numSamples, const float scale);
static void audioBufferAdd(const float * __restrict audioBuffer1, const float * __restrict audioBuffer2, const int numSamples, const float scale, float * __restrict destinationBuffer);

static void blendHrirSamples(HRIRSampleLocation const * const * samples, const float * sampleWeights, const int numSampleLocations, float * __restrict lResult, float * __restrict rResult);

static void azimuthElevationToXYZ(const float azimuth, const float elevation, float & x, float & y, float & z);

//

static int fftIndices[HRTF_BUFFER_SIZE];

//

struct HRIRSampleLocation
{
	ALIGN16 float lSamples[HRTF_BUFFER_SIZE];
	ALIGN16 float rSamples[HRTF_BUFFER_SIZE];
	
	float elevation;
	float azimuth;
	
	float x;
	float y;
	float z;
	
	void init(const float _elevation, const float _azimuth)
	{
		elevation = _elevation;
		azimuth = _azimuth;
		
		azimuthElevationToXYZ(azimuth, elevation, x, y, z);
	}
};

struct HRIRSampleLocationAndDistance
{
	HRIRSampleLocation * sampleLocation;
	float distance;
	
	bool operator<(const HRIRSampleLocationAndDistance & other) const
	{
		return distance < other.distance;
	}
};

struct HRIRSet
{
	std::vector<HRIRSampleLocation> sampleLocations;
	
	int findNearestSampleLocations(const float x, const float y, const float z, HRIRSampleLocationAndDistance * out, const int outSize);
	
	bool addHrirFromSoundData(const SoundData & soundData, const int elevation, const int azimuth, const bool swapLR);
	
	bool loadIrcamDatabase(const char * path);
	bool loadMitDatabase(const char * path);
};

static bool convertSoundDataToHRIR(const SoundData & soundData, float * __restrict lSamples, float * __restrict rSamples)
{
	if (soundData.channelCount != 2)
	{
		// sample data must be stereo as it must contain data for both the left and right ear impulse responses
		
		return false;
	}
	
	// ideally the number of samples in the sound data and HRTF buffer would match
	// however, clamp the number of samples in case the sound data contains more samples,
	// and pad with zeroes in case it's less
	
	const int numSamplesToCopy = std::min(soundData.sampleCount, HRTF_BUFFER_SIZE);
	
	if (soundData.channelSize == 2)
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
	else if (soundData.channelSize == 4)
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
	
	for (int i = numSamplesToCopy; i < HRTF_BUFFER_SIZE; ++i)
	{
		lSamples[i] = 0.f;
		rSamples[i] = 0.f;
	}
	
	return true;
}

static void blendHrirSamples(HRIRSampleLocation const * const * samples, const float * sampleWeights, const int numSampleLocations, float * __restrict lResult, float * __restrict rResult)
{
	memset(lResult, 0, HRTF_BUFFER_SIZE * sizeof(float));
	memset(rResult, 0, HRTF_BUFFER_SIZE * sizeof(float));
	
	for (int i = 0; i < numSampleLocations; ++i)
	{
		audioBufferAdd(lResult, samples[i]->lSamples, HRTF_BUFFER_SIZE, sampleWeights[i], lResult);
		audioBufferAdd(rResult, samples[i]->rSamples, HRTF_BUFFER_SIZE, sampleWeights[i], rResult);
	}
}

int HRIRSet::findNearestSampleLocations(const float x, const float y, const float z, HRIRSampleLocationAndDistance * out, const int outSize)
{
	std::vector<HRIRSampleLocationAndDistance> set;
	
	set.resize(sampleLocations.size());
	
	int index = 0;
	
	for (auto & s : sampleLocations)
	{
		const float dx = s.x - x;
		const float dy = s.y - y;
		const float dz = s.z - z;
		const float ds = sqrtf(dx * dx + dy * dy + dz * dz);
		
		HRIRSampleLocationAndDistance & sd = set[index++];
		
		sd.sampleLocation = &s;
		sd.distance = ds;
	}
	
	std::sort(set.begin(), set.end());
	
	const int numOut = std::min(outSize, (int)set.size());
	
	for (int i = 0; i < numOut; ++i)
	{
		out[i] = set[i];
	}
	
	return numOut;
}

bool HRIRSet::addHrirFromSoundData(const SoundData & soundData, const int elevation, const int azimuth, const bool swapLR)
{
	sampleLocations.resize(sampleLocations.size() + 1);
	
	HRIRSampleLocation & sampleLocation = sampleLocations.back();
	
	if (convertSoundDataToHRIR(soundData,
		swapLR == false ? sampleLocation.lSamples : sampleLocation.rSamples,
		swapLR == false ? sampleLocation.rSamples : sampleLocation.lSamples))
	{
		sampleLocation.init(elevation, azimuth);
		
		return true;
	}
	else
	{
		sampleLocations.resize(sampleLocations.size() - 1);
		
		return false;
	}
}

static bool parseIrcamFilename(const char * filename, int & subjectId, int & radius, int & elevation, int & azimuth)
{
	std::string name = Path::StripExtension(Path::GetBaseName(filename));
		
	// IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav;
	
	std::vector<std::string> parts;
	
	splitString(name, parts, '_');
	
	if (parts.size() != 6)
	{
		logDebug("error: filename doesn't match conversion: IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav");
		return false;
	}
	
	Assert(parts[0] == "IRC");
	if (parts[0] != "IRC")
		return false;
	
	subjectId = Parse::Int32(parts[1]);
	
	radius = parts[3][0] == 'R' ? Parse::Int32(String::SubString(parts[3], 1)) : -1;
	azimuth = parts[4][0] == 'T' ? Parse::Int32(String::SubString(parts[4], 1)) : -1;
	elevation = parts[5][0] == 'P' ? Parse::Int32(String::SubString(parts[5], 1)) : -1;
	
	if (radius < 0 || azimuth < 0 || elevation < 0)
		return false;
	
	return true;
}

bool HRIRSet::loadIrcamDatabase(const char * path)
{
	std::vector<std::string> files = listFiles(path, false);
	
	sampleLocations.reserve(sampleLocations.size() + files.size());
	
	int numAdded = 0;
	
	for (auto & filename : files)
	{
		int subjectId;
		int radius;
		int elevation;
		int azimuth;
		
		if (parseIrcamFilename(filename.c_str(), subjectId, radius, elevation, azimuth) == false)
		{
			continue;
		}
		
		if (elevation != 0)
		{
			continue;
		}
		
		logDebug("subjectId: %d, radius: %d, azimuth: %d, elevation: %d", subjectId, radius, azimuth, elevation);
		
		SoundData * soundData = loadSound(filename.c_str());
		
		if (soundData == nullptr)
		{
			logDebug("failed to load sound data");
		}
		else
		{
			if (addHrirFromSoundData(*soundData, elevation, azimuth, false))
			{
				numAdded++;
			}
			
			delete soundData;
			soundData = nullptr;
		}
	}
	
	return numAdded > 0;
}

static bool parseMitFilename(const char * filename, int & elevation, int & azimuth)
{
	std::string name = Path::StripExtension(Path::GetBaseName(filename));
	
	int offset = 0;
	
	if (offset == name.size())
		return false;
	if (name[offset] != 'H')
		return false;
	offset++;
	
	int begin = offset;
	while (offset < name.size() && name[offset] != 'e')
		offset++;
	if (offset == name.size())
		return false;
	elevation = Parse::Int32(name.substr(begin, offset - begin));
	offset++;
	
	begin = offset;
	while (offset < name.size() && name[offset] != 'a')
		offset++;
	if (offset == name.size())
		return false;
	azimuth = Parse::Int32(name.substr(begin, offset - begin));
	offset++;
	
	return true;
}

bool HRIRSet::loadMitDatabase(const char * path)
{
	std::vector<std::string> files = listFiles(path, true);
	
	sampleLocations.reserve(sampleLocations.size() + files.size());
	
	int numAdded = 0;
	
	for (auto & filename : files)
	{
		// "H0e070a.wav"
		
		int elevation;
		int azimuth;
		
		if (parseMitFilename(filename.c_str(), elevation, azimuth) == false)
		{
			continue;
		}
	
	#if HACK2D
		if (elevation != 0)
		{
			continue;
		}
	#endif
		
		SoundData * soundData = loadSound(filename.c_str());
		
		if (soundData == nullptr)
		{
			logDebug("failed to load sound data");
		}
		else
		{
			if (addHrirFromSoundData(*soundData, elevation, -azimuth, false))
			{
				numAdded++;
			}
			
			if (azimuth != 0 && azimuth != 180)
			{
				if (addHrirFromSoundData(*soundData, elevation, +azimuth, true))
				{
					numAdded++;
				}
			}
			else
			{
				logDebug("skipping HRIR mirroring for %s", filename.c_str());
			}
			
			delete soundData;
			soundData = nullptr;
		}
	}
	
	return numAdded > 0;
}

//

struct HRTFData
{
	ALIGN16 float real[HRTF_BUFFER_SIZE];
	ALIGN16 float imag[HRTF_BUFFER_SIZE];
	
	void transformToFrequencyDomain()
	{
		// this will generate the HRTF from the HRIR samples
		
		Fourier::fft1D_slow(real, imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
	}
};

struct HRTF
{
	HRTFData lFilter;
	HRTFData rFilter;
};

struct AudioBuffer
{
	ALIGN16 float real[AUDIO_BUFFER_SIZE];
	ALIGN16 float imag[AUDIO_BUFFER_SIZE];
	
	void transformToFrequencyDomain(const bool fast = false)
	{
		if (fast)
			Fourier::fft1D(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, false, false);
		else
			Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, false, false);
	}
	
	void transformToTimeDomain(const bool fast = false)
	{
		if (fast)
			Fourier::fft1D(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
		else
			Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
	}
	
	void convolve(const HRTFData & filter, AudioBuffer & output)
	{
		const float * __restrict sReal = real;
		const float * __restrict sImag = imag;
		
		const float * __restrict fReal = filter.real;
		const float * __restrict fImag = filter.imag;
		
		float * __restrict oReal = output.real;
		float * __restrict oImag = output.imag;
		
		const float fi = fImag[0];
		const float si = sImag[0];
		
		*(float*)fImag = 0.f;
		*(float*)sImag = 0.f;
		
	#if 0
		const int size4 = HRTF_BUFFER_SIZE/4;
		
		const __m128 * __restrict vsReal = (__m128*)sReal;
		const __m128 * __restrict vsImag = (__m128*)sImag;
		
		const __m128 * __restrict vfReal = (__m128*)fReal;
		const __m128 * __restrict vfImag = (__m128*)fImag;
		
		__m128 * __restrict voReal = (__m128*)oReal;
		__m128 * __restrict voImag = (__m128*)oImag;
		
		for (int i = 0; i < size4; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			voReal[i] = vsReal[i] * vfReal[i] - vsImag[i] * vfImag[i];
			voImag[i] = vsReal[i] * vfImag[i] + vsImag[i] * vfReal[i];
		}
	#else
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[i] = sReal[i] * fReal[i] - sImag[i] * fImag[i];
			oImag[i] = sReal[i] * fImag[i] + sImag[i] * fReal[i];
		}
	#endif
		
		oImag[0] = fi * si;
		
		*(float*)fImag = fi;
		*(float*)sImag = si;
	}
	
	void convolveAndReverseIndices(const HRTFData & filter, AudioBuffer & output)
	{
		const float * __restrict sReal = real;
		const float * __restrict sImag = imag;
		
		const float * __restrict fReal = filter.real;
		const float * __restrict fImag = filter.imag;
		
		float * __restrict oReal = output.real;
		float * __restrict oImag = output.imag;
		
		//const float fi = fImag[0];
		//const float si = sImag[0];
		
		//*(float*)fImag = 0.f;
		//*(float*)sImag = 0.f;
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[fftIndices[i]] = sReal[i] * fReal[i] - sImag[i] * fImag[i];
			oImag[fftIndices[i]] = sReal[i] * fImag[i] + sImag[i] * fReal[i];
		}
		
		//oImag[0] = fi * si;
		
		//*(float*)fImag = fi;
		//*(float*)sImag = si;
	}
};

static void convolveAudio(
	AudioBuffer & source,
	const HRTFData & lFilter,
	const HRTFData & rFilter,
	AudioBuffer & lResult,
	AudioBuffer & rResult)
{
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	// transform audio data from the time-domain into the frequency-domain
	
	source.transformToFrequencyDomain(true);
	
	// convolve audio data with impulse-response data in the frequency-domain
	
	source.convolveAndReverseIndices(lFilter, lResult);
	source.convolveAndReverseIndices(rFilter, rResult);
	
	// transform convolved audio data back to the time-domain
	
	lResult.transformToTimeDomain(true);
	rResult.transformToTimeDomain(true);
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	printf("convolveAudio took %gms\n", (t2 - t1) / 1000.0);
}

//

static void audioBufferMul(float * __restrict audioBuffer, const int numSamples, const float scale)
{
	for (int i = 0; i < numSamples; ++i)
	{
		audioBuffer[i] *= scale;
	}
}


static void audioBufferAdd(const float * __restrict audioBuffer1, const float * __restrict audioBuffer2, const int numSamples, const float scale, float * __restrict destinationBuffer)
{
	for (int i = 0; i < numSamples; ++i)
	{
		destinationBuffer[i] = audioBuffer1[i] + audioBuffer2[i] * scale;
	}
}

//

struct AudioSource
{
	virtual int getChannelCount() const = 0;
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) = 0;
};

struct AudioSource_Mix : AudioSource
{
	struct Input
	{
		AudioSource * source;
		float gain;
	};
	
	std::vector<Input> inputs;
	
	bool normalizeGain;
	
	AudioSource_Mix()
		: inputs()
		, normalizeGain(false)
	{
	}
	
	void add(AudioSource * source, const float gain)
	{
		Input input;
		input.source = source;
		input.gain = gain;
		
		inputs.push_back(input);
	}
	
	virtual int getChannelCount() const
	{
		return 2;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples)
	{
		Assert(channelIndex < getChannelCount());
		
		if (inputs.empty())
		{
			for (int i = 0; i < numSamples; ++i)
				audioBuffer[i] = 0.f;
			return;
		}
		
		bool isFirst = true;
		
		float gainScale = 1.f;
		
		if (normalizeGain)
		{
			float totalGain = 0.f;
			
			for (auto & input : inputs)
			{
				totalGain += input.gain;
			}
			
			if (totalGain > 0.f)
			{
				gainScale = 1.f / totalGain;
			}
		}
		
		for (auto & input : inputs)
		{
			if (channelIndex < input.source->getChannelCount())
			{
				if (isFirst)
				{
					isFirst = false;
					
					input.source->generate(channelIndex, audioBuffer, numSamples);
					
					const float gain = input.gain * gainScale;
					
					if (gain != 1.f)
					{
						audioBufferMul(audioBuffer, numSamples, gain);
					}
				}
				else
				{
					ALIGN16 float tempBuffer[AUDIO_UPDATE_SIZE];
					
					input.source->generate(channelIndex, tempBuffer, numSamples);
					
					const float gain = input.gain * gainScale;
					
					audioBufferAdd(audioBuffer, tempBuffer, numSamples, gain, audioBuffer);
				}
			}
		}
	}
};

struct AudioSource_StreamOgg : AudioSource
{
	AudioStream_Vorbis stream;
	
	AudioSource_StreamOgg()
		: stream()
	{
	}
	
	void load(const char * filename)
	{
		stream.Open(filename, true);
	}
	
	virtual int getChannelCount() const override
	{
		return 1;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		AudioSample * samples = (AudioSample*)alloca(sizeof(AudioSample) * numSamples);
		
		const int numProvided = stream.Provide(numSamples, samples);
		
		for (int i = 0; i < numProvided; ++i)
			audioBuffer[i] = (int(samples[i].channel[0]) + int(samples[i].channel[1])) / float(1 << 16);
		
		for (int i = numProvided; i < numSamples; ++i)
			audioBuffer[i] = 0.f;
	}
};

struct AudioSource_Binaural : AudioSource
{
	AudioBuffer overlapBuffer;
	
	AudioSource * source;
	
	AudioBuffer lResult;
	AudioBuffer rResult;
	
	HRTF hrtf;
	
	uint64_t processTimeAvg;
	
	SDL_mutex * mutex;
	
	AudioSource_Binaural()
		: overlapBuffer()
		, source(nullptr)
		, hrtf()
		, processTimeAvg(0)
		, mutex(nullptr)
	{
		memset(&overlapBuffer, 0, sizeof(overlapBuffer));
		
		memset(&hrtf, 0, sizeof(hrtf));
		
		mutex = SDL_CreateMutex();
	}
	
	~AudioSource_Binaural()
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}
	
	void setHrtf(const HRTF & _hrtf, const bool threadSafe)
	{
		if (threadSafe)
			SDL_LockMutex(mutex);
		
		hrtf = _hrtf;
		
		if (threadSafe)
			SDL_UnlockMutex(mutex);
	}
	
	virtual int getChannelCount() const override
	{
		return 2;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		
		if (source == nullptr)
		{
			for (int i = 0; i < numSamples; ++i)
			{
				audioBuffer[i] = 0.f;
			}
			
			return;
		}
		
		if (channelIndex == 0)
		{
			// copy the previous source data to the lower part of the overlap buffer
			
			memcpy(overlapBuffer.real, overlapBuffer.real + AUDIO_UPDATE_SIZE, AUDIO_UPDATE_SIZE * sizeof(float));
			
			// generate new source data into the upper part of the overlap buffer
			
			float * __restrict sourceBuffer = overlapBuffer.real + AUDIO_UPDATE_SIZE;
			
			source->generate(0, sourceBuffer, AUDIO_UPDATE_SIZE);

			const uint64_t t1 = g_TimerRT.TimeUS_get();
			
			AudioBuffer source;
			
			for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
			{
				source.real[fftIndices[i]] = overlapBuffer.real[i];
			}
			
			memset(source.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
			
			SDL_LockMutex(mutex);
			{
				convolveAudio(source, hrtf.lFilter, hrtf.rFilter, lResult, rResult);
			}
			SDL_UnlockMutex(mutex);
			
			const uint64_t t2 = g_TimerRT.TimeUS_get();
			
			processTimeAvg = (processTimeAvg * 90 + (t2 - t1) * 10) / 100;
		}
		
		if (channelIndex == 0)
		{
			memcpy(audioBuffer, lResult.real + AUDIO_UPDATE_SIZE, numSamples * sizeof(float));
		}
		else
		{
			memcpy(audioBuffer, rResult.real + AUDIO_UPDATE_SIZE, numSamples * sizeof(float));
		}
	}
};

//

struct MyPortAudioHandler : PortAudioHandler
{
	AudioSource * audioSource;
	
	MyPortAudioHandler(AudioSource * _audioSource)
		: PortAudioHandler()
		, audioSource(_audioSource)
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		audioSource->generate(0, channelL, AUDIO_UPDATE_SIZE);
		audioSource->generate(1, channelR, AUDIO_UPDATE_SIZE);
		
		float * __restrict destinationBuffer = (float*)outputBuffer;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

//

static void azimuthElevationToXYZ(const float azimuth, const float elevation, float & x, float & y, float & z)
{
	z = std::sin(Calc::DegToRad(elevation));
	
	const float radius = sqrtf(1.f - z * z);
	
	x = std::cos(Calc::DegToRad(azimuth)) * radius;
	y = std::sin(Calc::DegToRad(azimuth)) * radius;
	
	//printf("z = %.2f, radius = %.2f\n", z, radius);
}

static float mapFloat(const float value, const float minInput, const float maxInput, const float minOutput, const float maxOutput)
{
	if (minOutput == maxOutput)
		return minOutput;
	if (minInput == maxInput)
		return (minOutput + maxOutput) * .5f;
	else
	{
		const float t2 = std::max(0.f, std::min(1.f, (value - minInput) / (maxInput - minInput)));
		const float t1 = 1.f - t2;
		
		return minOutput * t1 + maxOutput * t2;
	}
}

void testHrtf()
{
	setAbout("-- Please Use Headphones -- This example demonstrates the use of the HRTF binauralizer object. The binauralizer makes it possible to spatialize audio, giving a mono sound with a virtual XYZ-position the appearence of having a spatial orientation in relationship to the listener. HRTF stands for Head-Related Transfer Function, and is a technique which uses a series of orientation-dependent audio filters captured by measuring the impulse-response of sound as it propagates through the ears (pinnae) of human test subjects. The result of applying these filters is that the spectrum of the audio arriving in the left and right ears is slightly different, which the brain interprets as the result of the attenuation due to the pinnae.");
	
	const int numBits = Fourier::integerLog2(HRTF_BUFFER_SIZE);
	
	for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
	{
		fftIndices[i] = Fourier::reverseBits(i, numBits);
	}

	// load impulse-response audio files
	
	HRIRSet hrirSet;
	hrirSet.loadMitDatabase("binaural/MIT-HRTF-DIFFUSE");
	
	AudioSource_StreamOgg sound;
	sound.load("menuselect.ogg");
	
	AudioSource_Binaural binaural;
	binaural.source = &sound;
	
	AudioSource_Mix mix;
	mix.normalizeGain = true;
	mix.add(&binaural, 1.f);
	
	MyPortAudioHandler audioUpdateHandler(&mix);
	
	PortAudioObject pa;
	
	if (pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler) == false)
	{
		logError("failed to initialize audio output");
	}
	
	framework.process();
	
	do
	{
		framework.process();
		
		//
		
		const Mat4x4 worldToView =
			Mat4x4(true).
			Translate(0, 0, 200).
			Scale(100, 100, 100).
			RotateZ(Calc::DegToRad(90)).
			RotateX(Calc::DegToRad(180));
		
	#if HACK2D
		const Mat4x4 object = Mat4x4(true);
	#else
		const Mat4x4 object =
			Mat4x4(true).
			RotateZ(Calc::DegToRad(framework.time * 9.01f)).
			RotateY(Calc::DegToRad(framework.time * 7.89f)).
			RotateX(Calc::DegToRad(framework.time * 4.56f));
	#endif
	
		const Mat4x4 objectToView =
			worldToView *
			object;
		
		const Mat4x4 viewToObject = objectToView.Invert();
		
		//
		
		const float mouseViewX = mapFloat(mouse.x, 0.f, GFX_SX, -100.f, +100.f);
		const float mouseViewY = mapFloat(mouse.y, 0.f, GFX_SY, +100.f, -100.f);
		
		const Vec3 mousePosition = viewToObject * Vec3(mouseViewX, mouseViewY, +200.f);
		const Vec3 mousePositionNorm = mousePosition.CalcNormalized();
		
		//
		
	#if HACK2D
		const int kMaxSampleLocations = 2;
	#else
		const int kMaxSampleLocations = 3;
	#endif
		
		HRIRSampleLocationAndDistance sampleLocations[kMaxSampleLocations];

		const int numSampleLocations = hrirSet.findNearestSampleLocations(
			mousePositionNorm[0],
			mousePositionNorm[1],
			mousePositionNorm[2],
			sampleLocations, kMaxSampleLocations);
		
		if (numSampleLocations > 0)
		{
			float totalDistance = 0.f;
			
			for (int i = 0; i < numSampleLocations; ++i)
			{
				totalDistance += sampleLocations[i].distance;
			}
			
			float weights[kMaxSampleLocations];
			
			const float distanceScale = totalDistance > 0.f ? 1.f / totalDistance : 1.f;
			
			float totalWeight = 0.f;
			
			for (int i = 0; i < numSampleLocations; ++i)
			{
				const float weight = 1.f - sampleLocations[i].distance * distanceScale;
				
				weights[i] = weight;
				
				totalWeight += weight;
			}
			
			if (totalWeight > 0.f)
			{
				for (int i = 0; i < numSampleLocations; ++i)
				{
					weights[i] /= totalWeight;
				}
			}
			
			HRIRSampleLocation * samples[kMaxSampleLocations];
			for (int i = 0; i < numSampleLocations; ++i)
				samples[i] = sampleLocations->sampleLocation;
			
			//logDebug("HRIR weights: %.2f, %.2f", weights[0], weights[1]);
			
			HRTF hrtf;
			
			memset(&hrtf, 0, sizeof(hrtf));
			
			float * lSamples = hrtf.lFilter.real;
			float * rSamples = hrtf.rFilter.real;
			
			blendHrirSamples(samples, weights, numSampleLocations, lSamples, rSamples);
			
			hrtf.lFilter.transformToFrequencyDomain();
			hrtf.rFilter.transformToFrequencyDomain();
			
			binaural.setHrtf(hrtf, true);
		}
			
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			Mat4x4 projectionMatrix;
			projectionMatrix.MakePerspectiveLH(Calc::DegToRad(90.f), GFX_SY / float(GFX_SX), .01f, 1000.f);
			gxMatrixMode(GL_PROJECTION);
			gxPushMatrix();
			gxLoadMatrixf(projectionMatrix.m_v);
			gxMatrixMode(GL_MODELVIEW);
			gxPushMatrix();
			gxLoadIdentity();
			
			gxPushMatrix();
			{
				gxMultMatrixf(objectToView.m_v);
				
				gxBegin(GL_LINES);
				{
					setColor(colorRed);
					gxVertex3f(-1.f, 0.f, 0.f);
					gxVertex3f(+1.f, 0.f, 0.f);
					
					setColor(colorGreen);
					gxVertex3f(0.f, -1.f, 0.f);
					gxVertex3f(0.f, +1.f, 0.f);
					
					setColor(colorBlue);
					gxVertex3f(0.f, 0.f, -1.f);
					gxVertex3f(0.f, 0.f, +1.f);
				}
				gxEnd();
				
				glPointSize(5.f);
				gxBegin(GL_POINTS);
				{
					for (auto & s : hrirSet.sampleLocations)
					{
						setColor(100, 100, 100);
						gxVertex3f(s.x, s.y, s.z);
					}
					
					for (int i = 0; i < numSampleLocations; ++i)
					{
						auto s = sampleLocations[i].sampleLocation;
						
						setColor(colorGreen);
						gxVertex3f(s->x, s->y, s->z);
					}
					
					setColor(colorGreen);
					gxVertex3f(mousePosition[0], mousePosition[1], mousePosition[2]);
				}
				gxEnd();
			}
			gxPopMatrix();
			
			gxMatrixMode(GL_PROJECTION);
			gxPopMatrix();
			gxMatrixMode(GL_MODELVIEW);
			gxPopMatrix();
			
			setFont("calibri.ttf");
			setColor(200, 200, 200);
			drawText(10, 10, 14, 1, 1, "time: %.4fms", binaural.processTimeAvg / 1000.0);
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
	
	pa.shut();
	
	//exit(0);
}
