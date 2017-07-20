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

#include "audioNodeLimiter.h"
#include "Noise.h"
#include <cmath>

AUDIO_NODE_TYPE(limiter, AudioNodeLimiter)
{
	typeName = "limiter";
	
	in("value", "audioValue");
	in("max", "float", "1");
	in("decay", "float", "0.5");
	out("result", "audioValue");
}

void AudioNodeLimiter::draw()
{
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const float max = getInputFloat(kInput_Max, 1.f);
	const double decayPerSecond = std::max(0.f, std::min(1.f, getInputFloat(kInput_Decay, .5f)));
	
	//

	if (value->isScalar)
	{
		const double dt = 1.0 / SAMPLE_RATE;
		const double retainPerTick = std::pow(1.0 - decayPerSecond, dt);

		//

		float currentValue = value->getScalar();

		measuredMax = std::max<double>(measuredMax, std::abs(currentValue));

		if (measuredMax > max)
		{
			currentValue = currentValue * max / measuredMax;
		}

		resultOutput.setScalar(currentValue);

		measuredMax = measuredMax * retainPerTick;
	}
	else
	{
		const double dt = 1.0 / SAMPLE_RATE;
		const double retainPerSample = std::pow(1.0 - decayPerSecond, dt);

		//

		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			float currentValue = value->samples[i];

			measuredMax = std::max<double>(measuredMax, std::abs(currentValue));

			if (measuredMax > max)
			{
				currentValue = currentValue * max / measuredMax;
			}

			resultOutput.samples[i] = currentValue;

			measuredMax = measuredMax * retainPerSample;
		}
	}
}
