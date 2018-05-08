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

#pragma once

#include "audioNodeBase.h"
#include "soundmix.h" // AudioSource

struct AudioGraph;

struct AudioNodeSatellites : AudioNodeBase
{
	static const int kNumChannels = 3;

	struct AudioSourceSatellitesNode : AudioSource
	{
		AudioNodeSatellites * satellitesNode = nullptr;
		int index = -1;
		
		virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples);
	};

	enum Input
	{
		kInput_Channel1,
		kInput_Channel2,
		kInput_Channel3,
		kInput_Audio1,
		kInput_Audio2,
		kInput_Audio3,
		kInput_Gain1,
		kInput_Gain2,
		kInput_Gain3,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioSourceSatellitesNode sources[kNumChannels];
	AudioVoice * voices[kNumChannels];
	
	AudioGraph * audioGraph;
	
	AudioNodeSatellites();
	virtual ~AudioNodeSatellites() override;
	
	virtual void tick(const float dt) override;
};
