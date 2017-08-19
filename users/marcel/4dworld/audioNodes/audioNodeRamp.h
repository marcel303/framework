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

#include "audioNodeBase.h"

struct AudioNodeRamp : AudioNodeBase
{
	enum Input
	{
		kInput_StartRamped,
		kInput_Value,
		kInput_RampTime,
		kInput_RampUp,
		kInput_RampDown,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_RampedUp,
		kOutput_RampedDown,
		kOutput_COUNT
	};
	
	bool ramp;
	double value;
	
	AudioFloat valueOutput;
	
	AudioNodeRamp()
		: AudioNodeBase()
		, ramp(false)
		, value(0.0)
		, valueOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_StartRamped, kAudioPlugType_Bool);
		addInput(kInput_Value, kAudioPlugType_FloatVec);
		addInput(kInput_RampTime, kAudioPlugType_FloatVec);
		addInput(kInput_RampUp, kAudioPlugType_Trigger);
		addInput(kInput_RampDown, kAudioPlugType_Trigger);
		addOutput(kOutput_Value, kAudioPlugType_FloatVec, &valueOutput);
		addOutput(kOutput_RampedUp, kAudioPlugType_Trigger, nullptr);
		addOutput(kOutput_RampedDown, kAudioPlugType_Trigger, nullptr);
	}
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;

	virtual void handleTrigger(const int inputSocketIndex) override;
};
