#pragma once

// Copyright (C) 2013 Grannies Games - All rights reserved

struct AudioSample
{
	short channel[2];
};

class AudioStream
{
public:
	virtual ~AudioStream() { }
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) = 0;
};
