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
#include <xmmintrin.h>

AUDIO_ENUM_TYPE(smoothing_mode)
{
	elem("perSecond");
	elem("perMillisecond");
}

AUDIO_NODE_TYPE(smoothe, AudioNodeSmoothe)
{
	typeName = "smoothe";
	
	in("value", "audioValue");
	inEnum("smoothing", "smoothing_mode");
	in("smoothness", "audioValue", "0.5");
	out("result", "audioValue");
}

void AudioNodeSmoothe::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeSmoothe);
	
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const SmoothingUnit smoothingUnit = (SmoothingUnit)getInputInt(kInput_SmoothingUnit, 0);
	const AudioFloat * retain = getInputAudioFloat(kInput_Smoothness, &AudioFloat::Half);
	
	const double dt = (smoothingUnit == kSmoothingUnit_PerSecond ? 1.0 : 1000.0) / SAMPLE_RATE;
	
	if (isPassthrough)
	{
		resultOutput.set(*value);
	}
	else if (retain->isScalar)
	{
		value->expand();
		
		const double retainPerSecond = std::min(1.f, std::max(0.f, retain->getScalar()));
		const double retainPerSample = std::pow(retainPerSecond, dt);
		const double followPerSample = 1.0 - retainPerSample;
		
		resultOutput.setVector();
		
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = currentValue;
			
			currentValue = currentValue * retainPerSample + value->samples[i] * followPerSample;
		}
		
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
	}
	else
	{
		value->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const double retainPerSecond = std::min(1.f, std::max(0.f, retain->samples[i]));
			const double retainPerSample = std::pow(retainPerSecond, dt);
			const double followPerSample = 1.0 - retainPerSample;
		
			resultOutput.samples[i] = currentValue;
			
			currentValue = currentValue * retainPerSample + value->samples[i] * followPerSample;
		}
	}
}
