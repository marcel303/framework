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

#include "soundmix.h"

struct AudioSourcePcm : AudioSource
{
	const PcmData * pcmData;
	
	int samplePosition;
	
	bool isPlaying;
	bool loop;
	bool hasLooped;
	bool isDone;
	
	bool hasRange;
	int rangeBegin;
	int rangeEnd;
	
	int maxLoopCount;
	int loopCount;
	
	AudioSourcePcm();
	
	void setPcmData(const PcmData * pcmData, const int samplePosition = 0);
	void setLooping(const bool looping);
	
	void setRange(const int begin, const int length);
	void setRangeNorm(const float begin, const float length);
	void clearRange();
	
	void play();
	void stop();
	void pause();
	void resume();
	void resetSamplePosition();
	void setSamplePosition(const int position);
	void setSamplePositionNorm(const float position);
	void resetLoopCount();

	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override;
};
