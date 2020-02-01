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

#include "vfxNodeTriggerTimer.h"
#include <math.h>

VFX_NODE_TYPE(VfxNodeTriggerTimer)
{
	typeName = "trigger.timer";
	
	in("auto", "bool", "1");
	in("interval", "float");
	in("start!", "trigger");
	in("trigger!", "trigger");
	out("trigger!", "trigger");
	out("triggerCount", "float");
}

VfxNodeTriggerTimer::VfxNodeTriggerTimer()
	: VfxNodeBase()
	, isStarted(false)
	, timer(0.f)
	, triggerCount(0)
	, triggerCountOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Auto, kVfxPlugType_Bool);
	addInput(kInput_Interval, kVfxPlugType_Float);
	addInput(kInput_Start, kVfxPlugType_Trigger);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_TriggerCount, kVfxPlugType_Float, &triggerCountOutput);
}

void VfxNodeTriggerTimer::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTriggerTimer);
	
	const int kMaxFrequency = 10000;
	
	const bool startAuto = getInputBool(kInput_Auto, true);
	const float interval = getInputFloat(kInput_Interval, 0.f);
	
	if (isStarted == false && startAuto)
	{
		isStarted = true;
		timer = 0.f;
	}
	
	if (isPassthrough || interval <= 0.f || isStarted == false)
	{
		timer = 0.f;
	}
	else
	{
		timer += dt;
		
		// clamp interval to minimum value (= maximum frequency) to avoid getting stuck in the while loop below
		
		const float clampedInterval = fmaxf(interval, 1.f / kMaxFrequency);
		
		while (timer >= clampedInterval)
		{
			triggerCount++;
			triggerCountOutput = triggerCount;
			
			trigger(kOutput_Trigger);
			
			if (startAuto)
			{
				timer -= clampedInterval;
			}
			else
			{
				isStarted = false;
				break;
			}
		}
	}
}

void VfxNodeTriggerTimer::handleTrigger(const int socketIndex)
{
	if (socketIndex == kInput_Start)
	{
		isStarted = true;
		timer = 0.f;
	}
	else if (socketIndex == kInput_Trigger)
	{
		const bool startAuto = getInputBool(kInput_Auto, true);
		
		triggerCount++;
		triggerCountOutput = triggerCount;
	
		trigger(kOutput_Trigger);
		
		if (startAuto)
			timer = 0.f;
		else
			isStarted = false;
	}
}

void VfxNodeTriggerTimer::getDescription(VfxNodeDescription & d)
{
	if (isStarted)
	{
		const float interval = getInputFloat(kInput_Interval, 0.f);
		
		if (interval <= 0.f)
		{
			d.add("time remaining: infinity (interval <= 0)");
		}
		else
		{
			const float clampedInterval = fmaxf(interval, 1.f / kMaxFrequency);
			
			if (clampedInterval != interval)
				d.add("warning: frequency for interval exceeds maximum value of %dHz", kMaxFrequency);
			
			const float remaining = std::max(0.f, interval - timer);
			
			d.add("time remaining: %.2fs", remaining);
		}
	}
	else
	{
		d.add("not started");
	}
}
