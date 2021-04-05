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
#include <vector>

struct AudioGraphInstance;

//

#include "audioVoiceManager.h"
#include <atomic>

#include "soundmix.h" // todo : move to cpp // audioBufferSetZero

// todo : share VoiceMgr_VoiceGroup with vfxNodeAudioGraph, so it can have 'muted' mode too
struct VoiceMgr_VoiceGroup : AudioVoiceManager
{
	struct NullSource : AudioSource
	{
		virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override
		{
			audioBufferSetZero(samples, numSamples);
		}
	};
	
	struct VoiceInfo
	{
		AudioVoice * voice;
		AudioSource * source;
		
		VoiceInfo()
			: voice(nullptr)
			, source(nullptr)
		{
		}
	};
	
	AudioGraphManager * graphMgr;
	AudioVoiceManager * voiceMgr;
	
	std::vector<VoiceInfo> voiceInfos;
	
	std::atomic<bool> muted;
	
	NullSource nullSource;
	
	VoiceMgr_VoiceGroup()
		: AudioVoiceManager(kType_Basic)
		, graphMgr(nullptr)
		, voiceMgr(nullptr)
		, voiceInfos()
		, muted(false)
		, nullSource()
	{
	}
	
	virtual ~VoiceMgr_VoiceGroup() override
	{
		Assert(voiceInfos.empty());
	}
	
	void init(AudioGraphManager * in_graphMgr, AudioVoiceManager * in_voiceMgr)
	{
		graphMgr = in_graphMgr;
		voiceMgr = in_voiceMgr;
	}
	
	void setMuted(const bool in_muted)
	{
		if (in_muted != muted.load())
		{
			muted.store(in_muted);
			
			updateVoices();
		}
	}
	
	void updateVoices()
	{
		graphMgr->lockAudio();
		{
			if (muted)
			{
				for (auto & voiceInfo : voiceInfos)
				{
					voiceInfo.voice->source = &nullSource;
				}
			}
			else
			{
				for (auto & voiceInfo : voiceInfos)
				{
					voiceInfo.voice->source = voiceInfo.source;
				}
			}
		}
		graphMgr->unlockAudio();
	}
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	virtual int calculateNumVoices() const override;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples, const int numChannels) override { }
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
		kInput_OutputMode,
		kInput_Muted,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Voices,
		kOutput_COUNT
	};
	
	enum OutputMode
	{
		kOutputMode_Mono,
		kOutputMode_Stereo,
		kOutputMode_MultiChannel
	};
	
	VoiceMgr_VoiceGroup voiceMgr;
	
	AudioGraphContext * context;
	
	AudioGraphInstance * instances[kMaxInstances];
	
	std::string currentFilename;
	
	std::vector<std::string> currentControlValues;
	
	VfxChannelData voicesData;
	VfxChannel voicesOutput;
	
	// history of changes used for getDescription
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
