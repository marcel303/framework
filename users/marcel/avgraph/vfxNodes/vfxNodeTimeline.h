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

#include "vfxNodeBase.h"

#include "vfxTypes.h"

struct VfxNodeTimeline : VfxNodeBase
{
	enum Input
	{
		kInput_Timeline,
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
	
	double time;
	
	bool isPlaying;
	
	VfxTriggerData eventTriggerData;
	VfxTriggerData beatTriggerData;
	
	VfxTimeline timeline;

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
