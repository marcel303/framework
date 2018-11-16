#pragma once

#include "audioNodeBase.h"

struct AllegroTimerApi;
struct AllegroVoiceApi;

struct JGMOD;
struct JGMOD_PLAYER;

struct AudioNodeJgmod : AudioNodeBase
{
	static const int kNumChannels = 16;
	
	enum Input
	{
		kInput_Filename,
		kInput_Speed,
		kInput_Pitch,
		kInput_Gain,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Channel1,
		kOutput_COUNT = kOutput_Channel1 + kNumChannels
	};
	
	AudioFloat output[kNumChannels];
	
	AllegroTimerApi * timerApi;
	AllegroVoiceApi * voiceApi;
	
	JGMOD * jgmod;
	JGMOD_PLAYER * jgmodPlayer;
	
	std::string currentFilename;
	
	AudioNodeJgmod();
	virtual ~AudioNodeJgmod() override;
	
	void updateJgmod();
	void freeJgmod();
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
};
