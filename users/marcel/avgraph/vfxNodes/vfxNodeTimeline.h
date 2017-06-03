#pragma once

#include "vfxNodeBase.h"
#include <vector>

struct VfxNodeTimeline : VfxNodeBase
{
	struct Marker
	{
		double beat;
		int id;
		
		bool operator<(const Marker & other) const
		{
			return beat < other.beat;
		}
	};

	enum Input
	{
		kInput_Duration,
		kInput_Bpm,
		kInput_Loop,
		kInput_AutoPlay,
		kInput_Speed,
		kInput_Play,
		kInput_Pause,
		kInput_Resume,
		kInput_Time,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_EventTrigger,
		kOutput_BeatTrigger,
		kOutput_COUNT
	};

	std::vector<Marker> markers;
	
	double time;
	
	bool isPlaying;
	
	VfxTriggerData eventTriggerData;
	VfxTriggerData beatTriggerData;

	VfxNodeTimeline();
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int srcSocketIndex, const VfxTriggerData & data) override;

	virtual void getDescription(VfxNodeDescription & d) override;
	
	int calculateMarkerIndex(const double time, const double bpm);
	
	void handleTimeSegment(const double oldTime, const double newTime, const double bpm);
	
	void play();
	void pause();
	void resume();
};
