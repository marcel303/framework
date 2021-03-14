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

#include "audioGraph.h"
#include "vfxNodeBase.h"
#include <set>

struct AudioGraphInstance;

//

#include "audioVoiceManager.h"

struct VoiceMgr_VoiceGroup : AudioVoiceManager
{
	enum Mode
	{
		kMode_Passthrough,
		kMode_DownmixMono,
		kMode_Disabled
	};
	
	AudioVoiceManager * parentVoiceMgr;
	
	std::set<AudioVoice*> voices;
	
	Mode currentMode;
	
	VoiceMgr_VoiceGroup()
		: AudioVoiceManager(kType_Basic)
		, parentVoiceMgr(nullptr)
		, voices()
		, currentMode(kMode_Passthrough)
	{
	}
	
	virtual ~VoiceMgr_VoiceGroup() override
	{
		Assert(voices.empty());
	}
	
	void init(AudioVoiceManager * in_parentVoiceMgr)
	{
		parentVoiceMgr = in_parentVoiceMgr;
	}
	
	void setMode(const Mode mode)
	{
		if (mode != currentMode)
		{
			// todo : remove all voices
			
			// todo : if mode is passthrough -> add all voices
			// todo : if mode is downmix to mono -> add special voice to perform downmix based on the set of voices we manage here
			// todo : if mode is muted -> don't add any voices
		}
	}
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	virtual int calculateNumVoices() const override;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples, const int numChannels) override
	{
		// todo : implement
	}
};

struct VfxNodeAudioGraphPoly : VfxNodeBase
{
	static const int kMaxInstances = 128;
	static const int kMaxHistory = 16;
	
	enum HistoryType
	{
		kHistoryType_InstanceCreate,
		kHistoryType_InstanceFree
	};
	
	struct HistoryElem
	{
		HistoryType type;
		float value;
	};
	
	enum Input
	{
		kInput_Filename,
		kInput_Volume,
		kInput_MaxInstances,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Voices,
		kOutput_COUNT
	};
	
	VoiceMgr_VoiceGroup voiceMgr;
	
	AudioGraphContext * context;
	
	AudioGraphInstance * instances[kMaxInstances];
	
	std::string currentFilename;
	
	std::vector<std::string> currentControlValues;
	
	VfxChannelData voicesData;
	VfxChannel voicesOutput;
	
	HistoryElem history[kMaxHistory];
	int historyWritePos;
	int historySize;
	
	VfxNodeAudioGraphPoly();
	virtual ~VfxNodeAudioGraphPoly() override;
	
	virtual void init(const GraphNode & node) override;
	
	void updateDynamicInputs();
	void listChannels(const VfxChannel ** channels, const VfxChannel * volume, int & numChannels, const int maxChannels);
	void addHistoryElem(const HistoryType type, const float value);
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
