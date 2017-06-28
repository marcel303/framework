#include "audio.h"
#include "Calc.h"
#include "framework.h"
#include "Parse.h"
#include "Path.h"
#include "portaudio/portaudio.h"
#include "StringEx.h"
#include "Timer.h"
#include "vfxNodes/fourier.h"
#include <complex>

#define HRTF_BUFFER_SIZE 512
#define AUDIO_BUFFER_SIZE 512
#define AUDIO_UPDATE_SIZE (AUDIO_BUFFER_SIZE/2)

#define SAMPLE_RATE 44100
#define MAX_FILTERS 187

extern const int GFX_SX;
extern const int GFX_SY;

//

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

struct AudioBuffer;
struct HRTFData;

//

static void convolveAudio(
	AudioBuffer & source,
	const HRTFData & lFilter,
	const HRTFData & rFilter,
	AudioBuffer & lResult,
	AudioBuffer & rResult);

static bool convertSoundDataToHRTF(
	const SoundData & soundData,
	HRTFData & lFilter,
	HRTFData & rFilter);

//

struct HRTFData
{
	float real[HRTF_BUFFER_SIZE];
	float imag[HRTF_BUFFER_SIZE];
	
	void transformToFrequencyDomain()
	{
		// this will generate the HRTF from the HRIR samples
		
		Fourier::fft1D_slow(real, imag, HRTF_BUFFER_SIZE, HRTF_BUFFER_SIZE, false, false);
	}
};

struct HRTFDataStereo
{
	HRTFData lFilter;
	HRTFData rFilter;
};

struct HRTFFilter
{
	bool initialized;
	
	HRTFDataStereo data;
	
	float elevation;
	float azimuth;
	
	float x;
	float y;
	float z;
	
	HRTFFilter()
		: initialized(false)
		, data()
		, elevation(0.f)
		, azimuth(0.f)
		, x(0.f)
		, y(0.f)
		, z(0.f)
	{
	}
	
	void init(const float _elevation, const float _azimuth)
	{
		initialized = true;
		
		elevation = _elevation;
		azimuth = _azimuth;
		
		x = -std::sin(Calc::DegToRad(azimuth));
		y = -std::cos(Calc::DegToRad(azimuth));
		z = std::sin(Calc::DegToRad(elevation));
	}
};

struct AudioBuffer
{
	float real[AUDIO_BUFFER_SIZE];
	float imag[AUDIO_BUFFER_SIZE];
	
	void transformToFrequencyDomain()
	{
		Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, false, false);
	}
	
	void transformToTimeDomain()
	{
		Fourier::fft1D_slow(real, imag, AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE, true, true);
	}
	
	void convolve(const HRTFData & filter, AudioBuffer & output)
	{
		// todo : SSE optimize this code
		
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
		
		for (int i = 0; i < HRTF_BUFFER_SIZE; ++i)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[i] = sReal[i] * fReal[i] - sImag[i] * fImag[i];
			oImag[i] = sReal[i] * fImag[i] + sImag[i] * fReal[i];
		}
		
		oImag[0] = fi * si;
		
		*(float*)fImag = fi;
		*(float*)sImag = si;
	}
};

static void convolveAudio(
	AudioBuffer & source,
	const HRTFData & lFilter,
	const HRTFData & rFilter,
	AudioBuffer & lResult,
	AudioBuffer & rResult)
{
	// transform audio data from the time-domain into the frequency-domain
	
	source.transformToFrequencyDomain();
	
	// convolve audio data with impulse-response data in the frequency-domain
	
	source.convolve(lFilter, lResult);
	source.convolve(rFilter, rResult);
	
	// transform convolved audio data back to the time-domain
	
	lResult.transformToTimeDomain();
	rResult.transformToTimeDomain();
}

static bool convertSoundDataToHRTF(
	const SoundData & soundData,
	HRTFData & lFilter,
	HRTFData & rFilter)
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
			lFilter.real[i] = sampleData[i * 2 + 0] / float(1 << 15);
			rFilter.real[i] = sampleData[i * 2 + 1] / float(1 << 15);
		}
	}
	else if (soundData.channelSize == 4)
	{
		// 32-bit floating point data. de-interleave into left/right ear HRIR data
		
		const float * __restrict sampleData = (const float*)soundData.sampleData;
		
		for (int i = 0; i < numSamplesToCopy; ++i)
		{
			lFilter.real[i] = sampleData[i * 2 + 0];
			rFilter.real[i] = sampleData[i * 2 + 1];
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
		lFilter.real[i] = 0.f;
		rFilter.real[i] = 0.f;
	}
	
	// initialize the imaginary components to zeroes
	
	memset(lFilter.imag, 0, sizeof(lFilter.imag));
	memset(rFilter.imag, 0, sizeof(rFilter.imag));
	
	// convert the HRIR (head-related impulse-response) to HRTF (head-related transfer function)
	
	lFilter.transformToFrequencyDomain();
	rFilter.transformToFrequencyDomain();
	
	return true;
}

struct AudioSource
{
	virtual int getChannelCount() const = 0;
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) = 0;
};

struct AudioSource_Sine : AudioSource
{
	float phase = 0.f;
	float phaseStep = 800.f / SAMPLE_RATE;

	virtual int getChannelCount() const override
	{
		return 1;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex == 0);
		
		for (int i = 0; i < numSamples; ++i)
		{
			audioBuffer[i] = std::sin(phase * 2.f * M_PI);
			
			phase = std::fmodf(phase + phaseStep, 1.f);
		}
	}
};

struct AudioSource_Sound : AudioSource
{
	SoundData * sound;
	int position;
	
	AudioSource_Sound()
		: sound(nullptr)
		, position(0)
	{
	}
	
	~AudioSource_Sound()
	{
		delete sound;
		sound = nullptr;
		
		position = 0;
	}
	
	void load(const char * filename)
	{
		sound = loadSound(filename);
	}
	
	virtual int getChannelCount() const override
	{
		return sound->channelCount;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < sound->channelCount);
		
		if (sound == nullptr)
		{
			for (int i = 0; i < numSamples; ++i)
				audioBuffer[i] = 0.f;
			return;
		}
		
		for (int i = 0; i < numSamples; ++i)
		{
			if (sound->channelSize == 2)
			{
				const int16_t * __restrict sampleData = (const int16_t*)sound->sampleData;
				
				audioBuffer[i] = sampleData[position * sound->channelCount + channelIndex] / float(1 << 15);
				
				position += 1;
				position %= sound->sampleCount;
			}
			else
			{
				audioBuffer[i] = 0.f;
			}
		}
	}
};

static HRTFFilter hrtfFilters[MAX_FILTERS];

struct AudioSource_Binaural : AudioSource
{
	AudioBuffer overlapBuffer;
	
	AudioSource * source;
	
	AudioBuffer lResult;
	AudioBuffer rResult;
	
	const HRTFFilter * filterOld;
	const HRTFFilter * filterCur;
	
	uint64_t processTimeAvg;
	
	AudioSource_Binaural()
		: overlapBuffer()
		, source(nullptr)
		, filterOld(nullptr)
		, filterCur(nullptr)
		, processTimeAvg(0)
	{
		memset(&overlapBuffer, 0, sizeof(overlapBuffer));
	}
	
	void setActiveFilter(const HRTFFilter * filter)
	{
		filterOld = filterCur;
		filterCur = filter;
	}
	
	virtual int getChannelCount() const override
	{
		return 2;
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		
		const HRTFFilter * hrtfFilter = filterCur;
	
		if (hrtfFilter == nullptr || source == nullptr)
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
			
			memcpy(source.real, overlapBuffer.real, AUDIO_BUFFER_SIZE * sizeof(float));
			memset(source.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
			
			convolveAudio(source, hrtfFilter->data.lFilter, hrtfFilter->data.rFilter, lResult, rResult);
			
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

static AudioSource_Sine audioSource_Sine;
static AudioSource_Sound audioSource_Sound;
static AudioSource_Binaural audioSource_Binaural;

static PaStream * stream = nullptr;

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);
	Assert(AUDIO_BUFFER_SIZE == AUDIO_UPDATE_SIZE * 2);
	
	float channelL[AUDIO_UPDATE_SIZE];
	float channelR[AUDIO_UPDATE_SIZE];
	
	audioSource_Binaural.generate(0, channelL, AUDIO_UPDATE_SIZE);
	audioSource_Binaural.generate(1, channelR, AUDIO_UPDATE_SIZE);
	
	float * __restrict destinationBuffer = (float*)outputBuffer;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		destinationBuffer[i * 2 + 0] = channelL[i];
		destinationBuffer[i * 2 + 1] = channelR[i];
	}
	
	return paContinue;
}

static bool initAudioOutput()
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
#if 0
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
	}
#endif
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice)
	{
		logError("portaudio: failed to find output device");
		return false;
	}
	
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, AUDIO_UPDATE_SIZE, paDitherOff, portaudioCallback, nullptr)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

static bool shutAudioOutput()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

struct FilterAndDistance
{
	HRTFFilter * filter;
	float distance;
	
	bool operator<(const FilterAndDistance & other) const
	{
		return distance < other.distance;
	}
};

static int findNearestFilters(const float x, const float y, const float z, FilterAndDistance * out, const int outSize)
{
	std::vector<FilterAndDistance> set;
	
	set.reserve(MAX_FILTERS);
	
	for (int i = 0; i < MAX_FILTERS; ++i)
	{
		auto & f = hrtfFilters[i];
		
		if (f.initialized == false)
			continue;
		
		const float dx = f.x - x;
		const float dy = f.y - y;
		const float ds = std::hypotf(dx, dy);
		
		set.resize(set.size() + 1);
		
		FilterAndDistance & fd = set.back();
		
		fd.filter = &f;
		fd.distance = ds;
	}
	
	std::sort(set.begin(), set.end());
	
	const int numOut = std::min(outSize, (int)set.size());
	
	for (int i = 0; i < numOut; ++i)
	{
		out[i] = set[i];
	}
	
	return numOut;
}

static int nextAllocIndex = 0;

static bool addHrtfFromSoundData(const SoundData & soundData, const int elevation, const int azimuth, const bool swapLR)
{
	HRTFFilter & filter = hrtfFilters[nextAllocIndex];
	
	if (convertSoundDataToHRTF(soundData,
		swapLR == false ? filter.data.lFilter : filter.data.rFilter,
		swapLR == false ? filter.data.rFilter : filter.data.lFilter))
	{
		filter.init(elevation, azimuth);
		
		nextAllocIndex++;
		
		return true;
	}
	else
	{
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

static void loadIrcamDatabase(const char * path)
{
	std::vector<std::string> files = listFiles(path, false);
	
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
			addHrtfFromSoundData(*soundData, elevation, azimuth, false);
			
			delete soundData;
			soundData = nullptr;
		}
	}
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

static void loadMitDatabase(const char * path)
{
	std::vector<std::string> files = listFiles(path, false);
	
	for (auto & filename : files)
	{
		// "hrtf/MIT/mit-H0e070a.wav"
		
		int elevation;
		int azimuth;
		
		if (parseMitFilename(filename.c_str(), elevation, azimuth) == false)
		{
			continue;
		}
		
		SoundData * soundData = loadSound(filename.c_str());
		
		if (soundData == nullptr)
		{
			logDebug("failed to load sound data");
		}
		else
		{
			addHrtfFromSoundData(*soundData, elevation, -azimuth, false);
			addHrtfFromSoundData(*soundData, elevation, +azimuth, true);
			
			delete soundData;
			soundData = nullptr;
		}
	}
}

void testHrtf()
{
	// load impulse-response audio files
	
	//loadIrcamDatabase("hrtf/IRC_1057");
	
	loadMitDatabase("hrtf/MIT");
	
	audioSource_Sound.load("hrtf/music.ogg");
	
	audioSource_Binaural.source = &audioSource_Sound;
	
	if (initAudioOutput() == false)
	{
		logError("failed to initialize audio output");
	}
	
	do
	{
		framework.process();
		
		//
		
		const Mat4x4 worldToView = Mat4x4(true).Translate(GFX_SX/2, GFX_SY/2, 0).Scale(100, 100, 1);
		const Mat4x4 viewToWorld = worldToView.Invert();
		
		//
		
		const Vec2 mousePosition = viewToWorld * Vec2(mouse.x, mouse.y);
		
		// calculate closest HRTF filter
		
		int closestIndex = -1;
		float closestDistance = std::numeric_limits<float>::max();
		
		for (int i = 0; i < MAX_FILTERS; ++i)
		{
			auto & f = hrtfFilters[i];
			
			if (f.initialized == false)
				continue;
			
			const float dx = f.x - mousePosition[0];
			const float dy = f.y - mousePosition[1];
			const float ds = std::hypotf(dx, dy);
			
			if (ds < closestDistance)
			{
				closestIndex = i;
				closestDistance = ds;
			}
		}
		
		if (closestIndex < 0)
		{
			audioSource_Binaural.setActiveFilter(nullptr);
		}
		else
		{
			audioSource_Binaural.setActiveFilter(&hrtfFilters[closestIndex]);
		}
		
		FilterAndDistance fd[1];
		
		if (findNearestFilters(mousePosition[0], mousePosition[1], 0, fd, 1) == 1)
		{
			// todo : interpolate filters
			
			Assert(fd[0].filter == audioSource_Binaural.filterCur);
		}
			
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// todo : show source and head position
			
			gxPushMatrix();
			{
				gxMultMatrixf(worldToView.m_v);
				
				for (int i = 0; i < MAX_FILTERS; ++i)
				{
					auto & f = hrtfFilters[i];
					
					if (f.initialized == false)
						continue;
					
					const bool isActive = i == closestIndex;
					
					if (isActive)
						setColor(colorYellow);
					else
						setColor(colorWhite);
					
					drawCircle(f.x, f.y, .1f, 10);
				}
				
				setColor(colorGreen);
				fillCircle(mousePosition[0], mousePosition[1], .1f, 10);
			}
			gxPopMatrix();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			
			drawText(10, 10, 24, 1, 1, "time: %.4fms", audioSource_Binaural.processTimeAvg / 1000.0);
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	if (shutAudioOutput() == false)
	{
		logError("failed to shut down audio output");
	}
	
	exit(0);
}
