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

#include "audioGraph.h"
#include "audioVoiceManager.h"
#include "vfxNodeBase.h"

struct AudioGraphInstance;

struct VfxNodeAudioGraph : VfxNodeBase
{
	struct VoiceMgr : AudioVoiceManager
	{
		std::set<AudioVoice*> voices;
		
		VoiceMgr()
			: AudioVoiceManager(kType_Basic)
		{
		}
		
		virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
		virtual void freeVoice(AudioVoice *& voice) override;
		
		virtual void generateAudio(float * __restrict samples, const int numSamples) override { }
	};
	
	enum Input
	{
		kInput_Filename,
		kInput_OutputMode,
		kInput_Limit,
		kInput_LimitPeak,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	enum OutputMode
	{
		kOutputMode_None = -1,
		kOutputMode_Mono,
		kOutputMode_Stereo
	};
	
	VoiceMgr voiceMgr;
	
	AudioGraphGlobals * globals;
	
	AudioGraphInstance * audioGraphInstance;
	
	std::string currentFilename;
	
	std::vector<std::string> currentControlValues;
	
	int currentNumSamples;
	int currentNumChannels;
	
	VfxChannelData channelData;
	VfxChannel * channelOutputs;
	
	VfxNodeAudioGraph();
	
	virtual ~VfxNodeAudioGraph() override;
	
	void updateDynamicInputs();
	void updateDynamicOutputs(const int numSamples, const int numChannels);
	
	virtual void tick(const float dt) override;
};
