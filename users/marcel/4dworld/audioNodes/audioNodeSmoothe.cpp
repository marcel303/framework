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

#include "audioNodeSmoothe.h"
#include "Noise.h"

AUDIO_NODE_TYPE(smoothe, AudioNodeSmoothe)
{
	typeName = "smoothe";
	
	in("value", "audioValue");
	in("smoothness", "audioValue", "0.5");
	out("result", "audioValue");
}

void AudioNodeSmoothe::draw()
{
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const AudioFloat * decay = getInputAudioFloat(kInput_Decay, &AudioFloat::Half);
	
	//
	
	value->expand();
	
	//
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	if (decay->isScalar)
	{
		const double decayPerSecond = std::min(1.f, std::max(0.f, decay->getScalar()));
		const double decayPerSample = std::pow(decayPerSecond, dt);
		const double followPerSample = 1.0 - decayPerSample;
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = currentValue;
			
			currentValue = currentValue * decayPerSample + value->samples[i] * followPerSample;
		}
	}
	else
	{
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const double decayPerSecond = std::min(1.f, std::max(0.f, decay->samples[i]));
			const double decayPerSample = std::pow(decayPerSecond, dt);
			const double followPerSample = 1.0 - decayPerSample;
		
			resultOutput.samples[i] = currentValue;
			
			currentValue = currentValue * decayPerSample + value->samples[i] * followPerSample;
		}
	}
}
