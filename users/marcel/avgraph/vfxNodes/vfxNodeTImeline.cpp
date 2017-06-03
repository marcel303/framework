#include "vfxNodeTimeline.h"

#include "framework.h" // fixme : remove dep. needed for random(..) call

VfxNodeTimeline::VfxNodeTimeline()
	: VfxNodeBase()
	, markers()
	, time(0.0)
	, isPlaying(false)
	, eventTriggerData()
	, beatTriggerData()
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
	addOutput(kOutput_EventTrigger, kVfxPlugType_Trigger, &eventTriggerData);
	addOutput(kOutput_BeatTrigger, kVfxPlugType_Trigger, &beatTriggerData);
	
	// generate random markers; needed until we have the UI to edit markers
	
	for (int i = 0; i < 20; ++i)
	{
		Marker m;
		m.id = i;
		m.beat = random(0, 60 * 10);
		
		markers.push_back(m);
	}
	
	std::sort(markers.begin(), markers.end());
}

void VfxNodeTimeline::tick(const float dt)
{
	const double duration = getInputFloat(kInput_Duration, 0.f);
	const double bpm = getInputFloat(kInput_Bpm, 0.f);
	const bool loop = getInputBool(kInput_Loop, true);
	const double speed = getInputFloat(kInput_Speed, 1.f);
	
	// todo : add support for custom time. note: will have to remember old marker index as a member variable
	
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
		
		const int direction = newTime < oldTime ? -1 : +1;
		
		if (tryGetInput(kInput_Duration)->isConnected())
		{
			while (newTime < 0.0)
			{
				Assert(direction == -1);
				
				if (loop)
				{
					newTime += duration;
					
					handleTimeSegment(oldTime, 0.0, bpm);
					
					oldTime = duration;
				}
				else
				{
					newTime = 0.0;
					
					handleTimeSegment(newTime, oldTime, bpm);
					
					oldTime = newTime;
				}
			}
			
			// todo : should we use a for loop like above so we trigger events possibly multiple times per tick, if the speed/clock is incremented very, very quickly ?
			
			while (newTime >= duration)
			{
				Assert(direction == +1);
				
				if (loop)
				{
					newTime -= duration;
					
					handleTimeSegment(oldTime, duration, bpm);
					
					oldTime = 0.0;
				}
				else
				{
					newTime = duration;
					
					handleTimeSegment(oldTime, duration, bpm);
					
					oldTime = duration;
				}
			}
			
			handleTimeSegment(oldTime, newTime, bpm);
		}
		else
		{
			handleTimeSegment(oldTime, newTime, bpm);
		}
		
		time = newTime;
	}
}

void VfxNodeTimeline::init(const GraphNode & node)
{
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	
	if (autoPlay)
	{
		play();
	}
}

void VfxNodeTimeline::handleTrigger(const int srcSocketIndex, const VfxTriggerData & data)
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
	const double beat = time * bpm;
	
	int index = -1;
	
	while (index + 1 < markers.size() && beat > markers[index + 1].beat)
		index++;
	
	return index;
}

void VfxNodeTimeline::handleTimeSegment(const double oldTime, const double newTime, const double bpm)
{
	const int oldMarkerIndex = calculateMarkerIndex(oldTime, bpm);
	const int newMarkerIndex = calculateMarkerIndex(newTime, bpm);
	
	if (oldMarkerIndex <= newMarkerIndex)
	{
		for (int i = oldMarkerIndex; i < newMarkerIndex; ++i)
		{
			const Marker & markerToTrigger = markers[i + 1];
			
			eventTriggerData.setInt(markerToTrigger.id);
			
			trigger(kOutput_EventTrigger);
		}
	}
	else
	{
		for (int i = oldMarkerIndex; i > newMarkerIndex; --i)
		{
			const Marker & markerToTrigger = markers[i];
			
			eventTriggerData.setInt(markerToTrigger.id);
			
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
