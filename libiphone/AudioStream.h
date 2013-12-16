#pragma once

typedef struct
{
	short channel[2];
} AudioSample;

class AudioStream
{
public:
	virtual ~AudioStream() { }
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) = 0;
};
