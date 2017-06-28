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

#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 44100
#define MAX_FILTERS 187

extern const int GFX_SX;
extern const int GFX_SY;

// filename : IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

static int fftBitReversedIndices[SAMPLE_BUFFER_SIZE];

struct AudioBuffer
{
	float r[SAMPLE_BUFFER_SIZE];
	float i[SAMPLE_BUFFER_SIZE];
	
	void transform(const bool inverse, const bool slow = true)
	{
		if (slow)
			Fourier::fft1D_slow(r, i, SAMPLE_BUFFER_SIZE, SAMPLE_BUFFER_SIZE, inverse, inverse);
		else
			Fourier::fft1D(r, i, SAMPLE_BUFFER_SIZE, SAMPLE_BUFFER_SIZE, inverse, inverse);
	}
	
	void convolve(const AudioBuffer & filter, AudioBuffer & output)
	{
		// todo : SSE optimize this code
		
		const float * __restrict sReal = r;
		const float * __restrict sImag = i;
		
		const float * __restrict fReal = filter.r;
		const float * __restrict fImag = filter.i;
		
		float * __restrict oReal = output.r;
		float * __restrict oImag = output.i;
		
		const float fi = fImag[0];
		const float si = sImag[0];
		
		*(float*)fImag = 0.f;
		*(float*)sImag = 0.f;
		
		for (int x = 0; x < SAMPLE_BUFFER_SIZE; ++x)
		{
			// complex multiply both arrays of complex values and store the result in output
			
			oReal[x] = sReal[x] * fReal[x] - sImag[x] * fImag[x];
			oImag[x] = sReal[x] * fImag[x] + sImag[x] * fReal[x];
		}
		
		oImag[0] = fi * si;
		
		*(float*)fImag = fi;
		*(float*)sImag = si;
	}
};

static void convolveAudio(AudioBuffer & source, const AudioBuffer & lFilter, const AudioBuffer & rFilter, AudioBuffer & lResult, AudioBuffer & rResult)
{
	// transform audio data from the time-domain into the frequency-domain
	
	source.transform(false, false);
	
	// convolve audio data with impulse-response data in the frequency-domain
	
#if 1
	source.convolve(lFilter, lResult);
	source.convolve(rFilter, rResult);
#else
	lResult = source;
	rResult = source;
#endif
	
	// transform convolved audio data back to the time-domain
	
	lResult.transform(true);
	rResult.transform(true);
}

struct HRTFFilter
{
	bool initialized;
	
	AudioBuffer sampleBufferL;
	AudioBuffer sampleBufferR;
	
	float elevation;
	float azimuth;
	
	float x;
	float y;
	float z;
	
	HRTFFilter()
		: initialized(false)
		, sampleBufferL()
		, sampleBufferR()
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

static float phase = 0.f;
static float phaseStep = 800.f / SAMPLE_RATE;

static HRTFFilter hrtfFilters[MAX_FILTERS];

static HRTFFilter * activeHrtfFilter = nullptr;

static SoundData * sound = nullptr;
static int soundPosition = 0;

static PaStream * stream = nullptr;

static AudioBuffer overlapBuffer;

static uint64_t hrtfTimeAvg = 0;

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	Assert(framesPerBuffer == SAMPLE_BUFFER_SIZE/2);
	
	float * __restrict buffer = (float*)outputBuffer;
	
	const HRTFFilter * hrtfFilter = activeHrtfFilter;
	
	if (hrtfFilter == nullptr)
	{
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			buffer[i * 2 + 0] = 0.f;
			buffer[i * 2 + 1] = 0.f;
		}
		
		return paContinue;
	}
	
#if 1
	for (int i = 0; i < SAMPLE_BUFFER_SIZE/2; ++i)
	{
		buffer[i] = std::sin(phase * 2.f * M_PI);
		
		phase = std::fmodf(phase + phaseStep, 1.f);
	}
#else
	for (int i = 0; i < SAMPLE_BUFFER_SIZE/2; ++i)
	{
		if (sound->channelSize == 2)
		{
			int16_t * sampleData = (int16_t*)sound->sampleData;
			
			buffer[i] = sampleData[soundPosition * sound->channelCount + 0] / float(1 << 15);
			
			soundPosition += 1;
			soundPosition %= sound->sampleCount;
		}
		else
		{
			buffer[i] = 0.f;
		}
	}
#endif

	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
#define DO_CONCAT 1

#if DO_CONCAT
	memcpy(overlapBuffer.r, overlapBuffer.r + SAMPLE_BUFFER_SIZE/2, SAMPLE_BUFFER_SIZE/2 * sizeof(float));
	memcpy(overlapBuffer.r + SAMPLE_BUFFER_SIZE/2, buffer, SAMPLE_BUFFER_SIZE/2 * sizeof(float));
#else
	memcpy(overlapBuffer.r, buffer, SAMPLE_BUFFER_SIZE/2 * sizeof(float));
	memset(overlapBuffer.r + SAMPLE_BUFFER_SIZE/2, 0, SAMPLE_BUFFER_SIZE/2 * sizeof(float));
#endif
	
	AudioBuffer source;
	
	for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
	{
		const int index = fftBitReversedIndices[i];
		
		source.r[index] = overlapBuffer.r[i];
	}
	
	memset(source.i, 0, sizeof(source.i));
	
	AudioBuffer lResult;
	AudioBuffer rResult;
	
	convolveAudio(source, hrtfFilter->sampleBufferL, hrtfFilter->sampleBufferR, lResult, rResult);
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	hrtfTimeAvg = (hrtfTimeAvg * 90 + (t2 - t1) * 10) / 100;
	
#if DO_CONCAT
	for (int i = 0; i < SAMPLE_BUFFER_SIZE/2; ++i)
	{
		buffer[i * 2 + 0] = lResult.r[i + SAMPLE_BUFFER_SIZE/2];
		buffer[i * 2 + 1] = rResult.r[i + SAMPLE_BUFFER_SIZE/2];
	}
#else
	for (int i = 0; i < SAMPLE_BUFFER_SIZE/2; ++i)
	{
		buffer[i * 2 + 0] = lResult.r[i];
		buffer[i * 2 + 1] = rResult.r[i];
	}
#endif
	
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
	
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, SAMPLE_BUFFER_SIZE/2, paDitherOff, portaudioCallback, nullptr)) != paNoError)
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

static bool convertSoundDataToLRFilter(const SoundData & soundData, AudioBuffer & lFilter, AudioBuffer & rFilter)
{
	if (soundData.channelCount != 2)
		return false;
	
	const int numSamplesToCopy = std::min(soundData.sampleCount, SAMPLE_BUFFER_SIZE);
	
	if (soundData.channelSize == 2)
	{
		int16_t * sampleData = (int16_t*)soundData.sampleData;
		
		for (int i = 0; i < numSamplesToCopy; ++i)
		{
			lFilter.r[i] = sampleData[i * 2 + 0] / float(1 << 15);
			rFilter.r[i] = sampleData[i * 2 + 1] / float(1 << 15);
		}
	}
	else if (soundData.channelSize == 4)
	{
		float * sampleData = (float*)soundData.sampleData;
		
		for (int i = 0; i < numSamplesToCopy; ++i)
		{
			lFilter.r[i] = sampleData[i * 2 + 0];
			rFilter.r[i] = sampleData[i * 2 + 1];
		}
	}
	else
	{
		return false;
	}
	
	for (int i = numSamplesToCopy; i < SAMPLE_BUFFER_SIZE; ++i)
	{
		lFilter.r[i] = 0.f;
		rFilter.r[i] = 0.f;
	}
	
	memset(lFilter.i, 0, sizeof(lFilter.i));
	memset(rFilter.i, 0, sizeof(rFilter.i));
	
	lFilter.transform(false);
	rFilter.transform(false);
	
	return true;
}

void testHrtf()
{
	const int numBits = Fourier::integerLog2(SAMPLE_BUFFER_SIZE);
	
	for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
	{
		fftBitReversedIndices[i] = Fourier::reverseBits(i, numBits);
	}
	
	// load impulse-response audio files
	
	std::vector<std::string> files = listFiles("hrtf/IRC_1057", false);
	
	int nextFilterIndex = 0;
	
	for (auto & filename : files)
	{
		std::string name = Path::StripExtension(Path::GetBaseName(filename));
		
		// IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav;
		
		std::vector<std::string> parts;
		
		splitString(name, parts, '_');
		
		if (parts.size() != 6)
		{
			logDebug("error: filename doesn't match conversion: IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav");
			continue;
		}
		
		Assert(parts[0] == "IRC");
		const int subjectId = Parse::Int32(parts[1]);
		
		const int radius = parts[3][0] == 'R' ? Parse::Int32(String::SubString(parts[3], 1)) : -1;
		const int azimuth = parts[4][0] == 'T' ? Parse::Int32(String::SubString(parts[4], 1)) : -1;
		const int elevation = parts[5][0] == 'P' ? Parse::Int32(String::SubString(parts[5], 1)) : -1;
		
		if (elevation != 0)
		{
			continue;
		}
		
		logDebug("subjectId: %d, radius: %d, azimuth: %d, elevation: %d", subjectId, radius, azimuth, elevation);
		
		SoundData * soundData = loadSound(filename.c_str());
		
		if (soundData == nullptr)
		{
			logDebug("failed to load sound data");
			continue;
		}
		
		HRTFFilter & filter = hrtfFilters[nextFilterIndex++];
		
		filter.init(elevation, azimuth);
		
		if (convertSoundDataToLRFilter(*soundData, filter.sampleBufferL, filter.sampleBufferR) == true)
		{
			logDebug("converted HRIR to HRTF!");
		}
		else
		{
			filter = HRTFFilter();
		}
		
		/*
		for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
		{
			const float l = std::hypotf(lFilter.r[i], lFilter.i[i]);
			const float r = std::hypotf(rFilter.r[i], rFilter.i[i]);
			
			logDebug("transfer %03d: %.2f <-> %.2f", i, l, r);
		}
		*/
		
		delete soundData;
		soundData = nullptr;
	}
	
	if (false)
	{
		SoundData * mitSoundData = loadSound("hrtf/mit-H0e070a.wav");
		
		if (mitSoundData != nullptr)
		{
			if (convertSoundDataToLRFilter(
				*mitSoundData,
				hrtfFilters[0].sampleBufferL,
				hrtfFilters[0].sampleBufferR) == true)
			{
				logDebug("converted MIT HRIR to HRTF!");
			}
			
			delete mitSoundData;
			mitSoundData = nullptr;
		}
	}
	
	sound = loadSound("hrtf/music.ogg");
	
	memset(overlapBuffer.r, 0, sizeof(overlapBuffer.r));
	memset(overlapBuffer.i, 0, sizeof(overlapBuffer.i));
	
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
			activeHrtfFilter = nullptr;
		}
		else
		{
			activeHrtfFilter = &hrtfFilters[closestIndex];
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
			
			drawText(10, 10, 24, 1, 1, "time: %.4fms", hrtfTimeAvg / 1000.0);
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	if (shutAudioOutput() == false)
	{
		logError("failed to shut down audio output");
	}
	
	delete sound;
	sound = nullptr;
}
