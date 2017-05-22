#pragma once

#include "vfxNodeBase.h"

class AudioOutput_OpenAL;
class AudioStream_Vorbis;

struct VfxNodeSound : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_AutoPlay,
		kInput_Loop,
		kInput_BPM,
		kInput_Volume,
		kInput_Play,
		kInput_Pause,
		kInput_Resume,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Time,
		kOutput_Play,
		kOutput_Pause,
		kOutput_Beat,
		kOutput_COUNT
	};

	VfxTriggerData playTrigger;
	VfxTriggerData pauseTrigger;
	float timeOutput;
	VfxTriggerData beatTrigger;
	
	AudioOutput_OpenAL * audioOutput;
	AudioStream_Vorbis * audioStream;
	
	bool isPaused;

	VfxNodeSound();
	virtual ~VfxNodeSound() override;
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int inputSocketIndex, const VfxTriggerData & data) override;
};
