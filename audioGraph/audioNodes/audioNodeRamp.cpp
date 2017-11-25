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

#include "audioNodeRamp.h"
#include <algorithm>

AUDIO_NODE_TYPE(ramp, AudioNodeRamp)
{
	typeName = "ramp";
	
	in("startRamped", "bool", "1");
	in("value", "audioValue", "1");
	in("rampTime", "audioValue", "1");
	in("rampUp!", "trigger");
	in("rampDown!", "trigger");
	out("value", "audioValue");
	out("rampedUp!", "trigger");
	out("rampedDown!", "trigger");
}

void AudioNodeRamp::tick(const float dt)
{
	const AudioFloat * input = getInputAudioFloat(kInput_Value, &AudioFloat::One);
	const AudioFloat * rampTime = getInputAudioFloat(kInput_RampTime, &AudioFloat::One);
	
	if (isPassthrough)
	{
		valueOutput.setScalar(0.f);
	}
	else
	{
		input->expand();
		rampTime->expand();
		
		//
		
		if (ramp == true && value == 1.0)
			valueOutput.set(*input);
		else if (ramp == false && value == 0.0)
			valueOutput.setZero();
		else
		{
			valueOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				const double step = 1.0 / (double(SAMPLE_RATE) * rampTime->samples[i]);
				
				if (ramp)
					value = std::min(1.0, value + step);
				else
					value = std::max(0.0, value - step);
				
				valueOutput.samples[i] = input->samples[i] * value;
			}
			
			if (ramp == true && value == 1.0)
				trigger(kOutput_RampedUp);
			else if (ramp == false && value == 0.0)
				trigger(kOutput_RampedDown);
		}
	}
}

void AudioNodeRamp::init(const GraphNode & node)
{
	const bool startRamped = getInputBool(kInput_StartRamped, true);
	
	if (startRamped)
	{
		ramp = true;
		value = 1.0;
	}
	else
	{
		ramp = false;
		value = 0.0;
	}
}

void AudioNodeRamp::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_RampUp)
	{
		ramp = true;
	}
	else if (inputSocketIndex == kInput_RampDown)
	{
		ramp = false;
	}
}
