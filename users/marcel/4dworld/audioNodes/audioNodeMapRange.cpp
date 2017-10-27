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

#include "audioNodeMapRange.h"

AUDIO_NODE_TYPE(mapRange, AudioNodeMapRange)
{
	typeName = "map.range";
	
	in("value", "audioValue");
	in("inMin", "audioValue");
	in("inMax", "audioValue", "1");
	in("outMin", "audioValue");
	in("outMax", "audioValue", "1");
	out("result", "audioValue");
}

AudioNodeMapRange::AudioNodeMapRange()
	: AudioNodeBase()
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_InMin, kAudioPlugType_FloatVec);
	addInput(kInput_InMax, kAudioPlugType_FloatVec);
	addInput(kInput_OutMin, kAudioPlugType_FloatVec);
	addInput(kInput_OutMax, kAudioPlugType_FloatVec);
	addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeMapRange::tick(const float dt)
{
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const AudioFloat * inMin = getInputAudioFloat(kInput_InMin, &AudioFloat::Zero);
	const AudioFloat * inMax = getInputAudioFloat(kInput_InMax, &AudioFloat::One);
	const AudioFloat * outMin = getInputAudioFloat(kInput_OutMin, &AudioFloat::Zero);
	const AudioFloat * outMax = getInputAudioFloat(kInput_OutMax, &AudioFloat::One);
	
	const bool scalarRange =
		inMin->isScalar &&
		inMax->isScalar &&
		outMin->isScalar &&
		outMax->isScalar;
	
	if (isPassthrough)
	{
		resultOutput.set(*value);
	}
	else if (scalarRange)
	{
		const float _inMin = inMin->getScalar();
		const float _inMax = inMax->getScalar();
		const float _outMin = outMin->getScalar();
		const float _outMax = outMax->getScalar();
		const float scale = (_inMin == _inMax) ? 0.f : 1.f / (_inMax - _inMin);
		
		if (value->isScalar)
		{
			const float t2 = (value->getScalar() - _inMin) * scale;
			const float t1 = 1.f - t2;
			const float result = t1 * _outMin + t2 * _outMax;
			
			resultOutput.setScalar(result);
		}
		else
		{
			resultOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				const float t2 = (value->samples[i] - _inMin) * scale;
				const float t1 = 1.f - t2;
				const float result = t1 * _outMin + t2 * _outMax;
				
				resultOutput.samples[i] = result;
			}
		}
	}
	else
	{
		resultOutput.setVector();
		
		value->expand();
		
		inMin->expand();
		inMax->expand();
		outMin->expand();
		outMax->expand();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float _inMin = inMin->samples[i];
			const float _inMax = inMax->samples[i];
			const float _outMin = outMin->samples[i];
			const float _outMax = outMax->samples[i];
			const float scale = (_inMin == _inMax) ? 0.f : 1.f / (_inMax - _inMin);
			const float _value = value->samples[i];
			
			const float t2 = (_value - _inMin) * scale;
			const float t1 = 1.f - t2;
			const float result = t1 * _outMin + t2 * _outMax;
			
			resultOutput.samples[i] = result;
		}
	}
}
