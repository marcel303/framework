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

#include "audioNodePhase.h"

AUDIO_NODE_TYPE(phase, AudioNodePhase)
{
	typeName = "phase";
	
	in("fine", "bool", "1");
	in("frequency", "audioValue");
	in("offset", "audioValue");
	out("phase", "audioValue");
}

AudioNodePhase::AudioNodePhase()
	: AudioNodeBase()
	, phase(0.0)
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_FineGrained, kAudioPlugType_Bool);
	addInput(kInput_Frequency, kAudioPlugType_FloatVec);
	addInput(kInput_PhaseOffset, kAudioPlugType_FloatVec);
	addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodePhase::tick(const float dt)
{
	const bool fineGrained = getInputBool(kInput_FineGrained, true);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::Zero);
	const AudioFloat * phaseOffset = getInputAudioFloat(kInput_PhaseOffset, &AudioFloat::Zero);
	
	const bool scalarInputs = frequency->isScalar && phaseOffset->isScalar;
	
	if (isPassthrough)
	{
		resultOutput.setScalar(0.f);
	}
	else if (fineGrained)
	{
		resultOutput.setVector();
		
		if (scalarInputs)
		{
			const float _frequency = frequency->getScalar();
			const float _phaseOffset = phaseOffset->getScalar();
			const float phaseStep = _frequency / SAMPLE_RATE;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				resultOutput.samples[i] = phase + _phaseOffset;
				
				phase += phaseStep;
				phase = phase - std::floorf(phase);
			}
		}
		else
		{
			frequency->expand();
			phaseOffset->expand();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				const float _frequency = frequency->samples[i];
				const float _phaseOffset = phaseOffset->samples[i];
				
				resultOutput.samples[i] = phase + _phaseOffset;
				
				phase += _frequency / SAMPLE_RATE;
				phase = phase - std::floorf(phase);
			}
		}
	}
	else
	{
		const float _frequency = frequency->getScalar();
		const float _phaseOffset = phaseOffset->getScalar();
		
		resultOutput.setScalar(phase + _phaseOffset);
		
		phase += _frequency / SAMPLE_RATE * AUDIO_UPDATE_SIZE;
		phase = phase - std::floorf(phase);
	}
}
