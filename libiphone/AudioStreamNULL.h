#pragma once

class AudioStreamNULL
{
public:
	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			buffer[i].channel[0] = 0;
			buffer[i].channel[1] = 0;
		}
	}
};
