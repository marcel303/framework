#pragma once

#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamVorbis.h"
#include "Calc.h"

class MyAudioStream : public AudioStream
{
public:
	const static int numStreams = 12;

	MyAudioStream()
	{
		for (int i = 0; i < numStreams; ++i)
		{
			volume[i] = 0.f;
			targetVolume[i] = 1.f;
		}

		step = 1.f / 48000.f / 3.f / 4.f;
	}

	AudioStream_Vorbis source[numStreams];
	float targetVolume[numStreams];
	float volume[numStreams];
	float step;

	void Open(const char * musicFilename, const char * choirFilename, const char * ambFilename)
	{
		for (int i = 0; i < 4; ++i)
		{
			char s[1024];
			sprintf(s, musicFilename, i + 1);

			source[i].Open(s, true);
		}

		for (int i = 0; i < 4; ++i)
		{
			char s[1024];
			sprintf(s, choirFilename, i + 1);

			source[i + 4].Open(s, true);
		}

		for (int i = 0; i < 4; ++i)
		{
			char s[1024];
			sprintf(s, ambFilename, i + 1);

			source[i + 8].Open(s, true);
		}
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		float data[1 << 8][2];
		memset(data, 0, sizeof(data));

		int result = numSamples;

		for (int c = 0; c < numStreams; ++c)
		{
			if (!source[c].IsOpen_get())
				continue;

			int s = source[c].Provide(numSamples, buffer);
			if (s < result)
				result = s;

			for (int i = 0; i < numSamples; ++i)
			{
				data[i][0] += buffer[i].channel[0] * volume[c];
				data[i][1] += buffer[i].channel[1] * volume[c];

				if (volume[c] < targetVolume[c])
				{
					volume[c] += step;
					if (volume[c] > targetVolume[c])
						volume[c] = targetVolume[c];
				}
				else
				{
					volume[c] -= step;
					if (volume[c] < targetVolume[c])
						volume[c] = targetVolume[c];
				}
			}
		}

		for (int i = 0; i < result; ++i)
		{
			buffer[i].channel[0] = data[i][0];
			buffer[i].channel[1] = data[i][1];
		}

		return result;
	}
};

// FFT

static const int kFFTBufferSize = 4096;
static const int kFFTSize = 1024;
static const int kFFTComplexSize = 513; // n/2+1
static const int kFFTBucketCount = 32;

extern float s_fftInputBuffer[4096];
extern float s_fftInput[kFFTSize];
extern float s_fftReal[kFFTComplexSize];
extern float s_fftImaginary[kFFTComplexSize];
extern float s_fftBuckets[kFFTBucketCount];

extern float s_fftProvideTime;

void fftInit();
float fftPowerValue(int i);
void fftProcess(float time);

class AudioStream_Capture : public AudioStream
{
public:
	AudioStream_Capture()
		: mSource(0)
		, mTime(0.f)
	{
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		if (mSource)
		{
			const int result = mSource->Provide(numSamples, buffer);
			const int copySize = Calc::Min(result, kFFTBufferSize);
			const float scale = 2.f / 65536.f;

			for (int i = 0; i < copySize; ++i)
				s_fftInputBuffer[i] = buffer[i].channel[0] * scale;
			for (int i = copySize; i < kFFTBufferSize; ++i)
				s_fftInputBuffer[i] = 0.f;

			s_fftProvideTime = mTime;

			return result;
		}
		else
		{
			return 0;
		}
	}

	AudioStream * mSource;
	float mTime;
};
