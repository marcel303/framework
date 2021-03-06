/*
	Copyright (C) 2020 Marcel Smit
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

#pragma once

#include "audioTypes.h"

#if LINUX || WINDOWS
	#define SAMPLE_ALIGN16
#else
	#define SAMPLE_ALIGN16 ALIGN16
#endif

struct PcmData
{
	float * samples;
	int numSamples;
	bool ownData;

	PcmData();
	~PcmData();

	void free();
	void alloc(const int numSamples);
	
	void set(float * samples, const int numSamples);
	void reset();

	bool load(const char * filename, const int channel, const bool createCache);
};

//

struct AudioSource
{
	virtual ~AudioSource()
	{
	}
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) = 0;
};

struct AudioSourceSine : AudioSource
{
	float phase;
	float phaseStep;
	
	AudioSourceSine();
	
	void init(const float phase, const float frequency);
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override;
};

//

extern void audioBufferSetZero(
	float * __restrict audioBuffer,
	const int numSamples);

extern void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale);

extern void audioBufferMul(
	float * __restrict audioBuffer,
	const int numSamples,
	const float * __restrict scale);

extern void audioBufferRamp(
	float * __restrict audioBuffer,
	const int numSamples,
	const float scale1,
	const float scale2);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float scale);

extern void audioBufferAdd(
	      float * __restrict audioBufferDst,
	const float * __restrict audioBufferSrc,
	const int numSamples,
	const float * __restrict scale);

extern void audioBufferAdd(
	const float * __restrict audioBuffer1,
	const float * __restrict audioBuffer2,
	const int numSamples,
	const float scale,
	float * __restrict destinationBuffer);

extern void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float * __restrict wetnessBuffer);

extern void audioBufferDryWet(
	float * dstBuffer,
	const float * __restrict dryBuffer,
	const float * __restrict wetBuffer,
	const int numSamples,
	const float wetness);

extern float audioBufferSum(
	const float * __restrict audioBuffer,
	const int numSamples);

extern void audioBufferClip_Hard(
	float * __restrict audioBuffer,
	const int numSamples);
extern void audioBufferClip_SigmoidSqrt(
	float * __restrict audioBuffer,
	const int numSamples);
extern void audioBufferClip_SigmoidFast(
	float * __restrict audioBuffer,
	const int numSamples);
