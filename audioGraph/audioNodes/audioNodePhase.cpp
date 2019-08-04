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
#include <math.h>

AUDIO_NODE_TYPE(AudioNodePhase)
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
			const float _phaseOffset = fmodf(phaseOffset->getScalar(), 1.f);
			
			// todo : handle case where frequency < 0
			
		#if 0
			const float phaseStep = fmodf(_frequency / SAMPLE_RATE, 1.f);
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				resultOutput.samples[i] = phase + _phaseOffset;
				
				phase += phaseStep;
				
				if (phase >= 1.f)
					phase -= 1.f;
			}
		#else
			const float phaseStep1 = fmodf(_frequency / SAMPLE_RATE * 1.f, 1.f);
			const float phaseStep2 = fmodf(_frequency / SAMPLE_RATE * 2.f, 1.f);
			const float phaseStep3 = fmodf(_frequency / SAMPLE_RATE * 3.f, 1.f);
			const float phaseStep4 = fmodf(_frequency / SAMPLE_RATE * 4.f, 1.f);
			
			float _phase = phase + _phaseOffset;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; i += 4)
			{
				float value1 = _phase;
				float value2 = _phase + phaseStep1;
				float value3 = _phase + phaseStep2;
				float value4 = _phase + phaseStep3;
				
				if (value2 >= 1.f) value2 -= 1.f;
				if (value3 >= 1.f) value3 -= 1.f;
				if (value4 >= 1.f) value4 -= 1.f;
				
				resultOutput.samples[i + 0] = value1;
				resultOutput.samples[i + 1] = value2;
				resultOutput.samples[i + 2] = value3;
				resultOutput.samples[i + 3] = value4;
				
				_phase += phaseStep4;
				
				if (_phase >= 1.f)
					_phase -= 1.f;
			}
			
			_phase -= _phaseOffset;
			
			if (_phase < 0.f)
				_phase += 1.f;
			
			phase = _phase;
		#endif
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
				phase = phase - floorf(phase);
			}
		}
	}
	else
	{
		const float _frequency = frequency->getScalar();
		const float _phaseOffset = phaseOffset->getScalar();
		
		resultOutput.setScalar(phase + _phaseOffset);
		
		phase += _frequency / SAMPLE_RATE * AUDIO_UPDATE_SIZE;
		phase = phase - floorf(phase);
	}
}
