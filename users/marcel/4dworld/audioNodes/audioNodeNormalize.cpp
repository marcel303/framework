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

#include "audioNodeNormalize.h"
#include "Noise.h"
#include <cmath>

AUDIO_NODE_TYPE(normalize, AudioNodeNormalize)
{
	typeName = "normalize";
	
	in("value", "audioValue");
	in("level", "float", "1");
	in("maxAmp", "float", "10");
	in("decay", "float", "0.001");
	out("result", "audioValue");
}

void AudioNodeNormalize::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeNormalize);
	
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const float level = getInputFloat(kInput_Level, 1.f);
	const float maxAmp = getInputFloat(kInput_MaxAmplification, 10.f);
	const float decayPerMs = std::max(0.f, std::min(1.f, getInputFloat(kInput_DecayPerMillisecond, .001f)));
	
	//

	const float dtMs = 1000.f / SAMPLE_RATE;
	const float retainPerSample = std::pow(1.f - decayPerMs, dtMs);

	//
	
	if (isPassthrough)
	{
		resultOutput.set(*value);
	}
	else if (level <= 0.f || maxAmp <= 0.f)
	{
		resultOutput.setScalar(0.f);
	}
	else
	{
		value->expand();

		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float input = value->samples[i];

			const float inputMag = std::fmaxf(level / maxAmp, std::fabsf(input));

			if (inputMag > measuredMax)
			{
				measuredMax = inputMag;
			}

			const float output = input * level / measuredMax;

			resultOutput.samples[i] = output;

			measuredMax *= retainPerSample;
		}
	}
}
