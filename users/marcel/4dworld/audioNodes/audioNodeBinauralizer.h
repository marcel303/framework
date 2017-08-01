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
#include "binauralizer.h"
#include "binaural_cipic.h"

struct AudioNodeBinauralizer : AudioNodeBase
{
	struct Mutex : binaural::Mutex
	{
		virtual void lock() override { }
		virtual void unlock() override { }
	};
	
	enum Input
	{
		kInput_Audio,
		kInput_Elevation,
		kInput_Azimuth,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioL,
		kOutput_AudioR,
		kOutput_COUNT
	};
	
	binaural::HRIRSampleSet sampleSet;
	binaural::Binauralizer binauralizer;
	Mutex mutex;
	
	AudioFloat audioOutputL;
	AudioFloat audioOutputR;
	
	AudioNodeBinauralizer()
		: AudioNodeBase()
		, audioOutputL()
		, audioOutputR()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Audio, kAudioPlugType_FloatVec);
		addInput(kInput_Elevation, kAudioPlugType_FloatVec);
		addInput(kInput_Azimuth, kAudioPlugType_FloatVec);
		addOutput(kOutput_AudioL, kAudioPlugType_FloatVec, &audioOutputL);
		addOutput(kOutput_AudioR, kAudioPlugType_FloatVec, &audioOutputR);
		
		binaural::loadHRIRSampleSet_Cipic("binaural/CIPIC/subject147", sampleSet);
		
		sampleSet.finalize();
		
		binauralizer.init(&sampleSet, &mutex);
	}
	
	virtual void tick(const float dt) override;
};
