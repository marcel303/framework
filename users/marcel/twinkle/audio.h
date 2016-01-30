#pragma once

#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamVorbis.h"

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
		float data[1 << 16][2];
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
