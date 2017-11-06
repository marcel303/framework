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

#include "editor_vfxTimeline.h"
#include "vfxGraph.h"
#include "vfxNodeTimeline.h"

#include "tinyxml2.h" // fixme

VFX_NODE_TYPE(VfxNodeTimeline)
{
	typeName = "timeline";
	
	resourceTypeName = "timeline";
	
	createResourceEditor = []() -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_VfxTimeline();
	};
	
	in("duration", "float");
	in("bpm", "float");
	in("loop", "bool", "1");
	in("autoPlay", "bool", "1");
	in("speed", "float", "1");
	in("play!", "trigger");
	in("pause!", "trigger");
	in("resume!", "trigger");
	in("time", "float");
	out("event!", "trigger");
	out("eventValue", "float");
	out("beat!", "trigger");
}

VfxNodeTimeline::VfxNodeTimeline()
	: VfxNodeBase()
	, time(0.0)
	, isPlaying(false)
	, timeline(nullptr)
	, eventValueOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Duration, kVfxPlugType_Float);
	addInput(kInput_Bpm, kVfxPlugType_Float);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_AutoPlay, kVfxPlugType_Bool);
	addInput(kInput_Speed, kVfxPlugType_Float);
	addInput(kInput_Play, kVfxPlugType_Trigger);
	addInput(kInput_Pause, kVfxPlugType_Trigger);
	addInput(kInput_Resume, kVfxPlugType_Trigger);
	addInput(kInput_Time, kVfxPlugType_Float);
	addOutput(kOutput_EventTrigger, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_EventValue, kVfxPlugType_Float, &eventValueOutput);
	addOutput(kOutput_BeatTrigger, kVfxPlugType_Trigger, nullptr);
}

VfxNodeTimeline::~VfxNodeTimeline()
{
	freeVfxNodeResource(timeline);
}

void VfxNodeTimeline::tick(const float dt)
{
	const double duration = getInputFloat(kInput_Duration, 0.f);
	const double bpm = getInputFloat(kInput_Bpm, 0.f);
	const bool loop = getInputBool(kInput_Loop, true);
	const double speed = getInputFloat(kInput_Speed, 1.f);
	
	if (isPlaying)
	{
		double oldTime = time;
		
		double newTime;
		
		if (tryGetInput(kInput_Time)->isConnected())
		{
			newTime = getInputFloat(kInput_Time, 0.f);
		}
		else
		{
			newTime = time + dt * speed;
		}
		
		if (tryGetInput(kInput_Duration)->isConnected() && duration > 0.0)
		{
			while (newTime < 0.0)
			{
				handleTimeSegment(oldTime, 0.0, bpm);
				
				if (loop)
				{
					newTime += duration;
					
					oldTime = duration;
				}
				else
				{
					newTime = 0.0;
					
					oldTime = 0.0;
				}
			}
			
			while (newTime > duration)
			{
				handleTimeSegment(oldTime, duration, bpm);
				
				if (loop)
				{
					newTime -= duration;
					
					oldTime = 0.0;
				}
				else
				{
					newTime = duration;
					
					oldTime = duration;
				}
			}
		}
		
		handleTimeSegment(oldTime, newTime, bpm);
		
		time = newTime;
	}
}

void VfxNodeTimeline::init(const GraphNode & node)
{
	createVfxNodeResource<VfxTimeline>(node, "timeline", "editorData", timeline);
	
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	
	if (autoPlay)
	{
		play();
	}
}

void VfxNodeTimeline::handleTrigger(const int srcSocketIndex)
{
	if (srcSocketIndex == kInput_Play)
	{
		play();
	}
	else if (srcSocketIndex == kInput_Pause)
	{
		pause();
	}
	else if (srcSocketIndex == kInput_Resume)
	{
		resume();
	}
}

void VfxNodeTimeline::getDescription(VfxNodeDescription & d)
{
	d.add("time: %.2fs, isPlaying: %d", time, isPlaying);
}

int VfxNodeTimeline::calculateMarkerIndex(const double time, const double _bpm)
{
	const double bpm = _bpm > 0.0 ? _bpm : 60.0;
	const double beat = time * bpm / 60.0;
	
	int index = -1;
	
	while (index + 1 < timeline->numKeys && beat > timeline->keys[index + 1].beat)
		index++;
	
	return index;
}

void VfxNodeTimeline::handleTimeSegment(const double oldTime, const double newTime, const double bpm)
{
	const int oldMarkerIndex = calculateMarkerIndex(oldTime, bpm);
	const int newMarkerIndex = calculateMarkerIndex(newTime, bpm);
	
	Assert(newMarkerIndex < timeline->numKeys);
	
	if (oldMarkerIndex <= newMarkerIndex)
	{
		for (int i = oldMarkerIndex; i < newMarkerIndex; ++i)
		{
			const VfxTimeline::Key & keyToTrigger = timeline->keys[i + 1];
			
			eventValueOutput = keyToTrigger.value;
			
			trigger(kOutput_EventTrigger);
		}
	}
	else
	{
		for (int i = oldMarkerIndex; i > newMarkerIndex; --i)
		{
			const VfxTimeline::Key & keyToTrigger = timeline->keys[i];
			
			eventValueOutput = keyToTrigger.value;
			
			trigger(kOutput_EventTrigger);
		}
	}
}

void VfxNodeTimeline::play()
{
	isPlaying = true;
}

void VfxNodeTimeline::pause()
{
	isPlaying = false;
}

void VfxNodeTimeline::resume()
{
	isPlaying = true;
}
