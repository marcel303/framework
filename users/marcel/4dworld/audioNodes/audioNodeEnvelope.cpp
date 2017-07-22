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

#include "audioNodeEnvelope.h"

AUDIO_NODE_TYPE(envelope, AudioNodeEnvelope)
{
	typeName = "envelope";
	
	in("signal", "audioValue");
	in("attack", "audioValue");
	in("decay", "audioValue");
	in("sustain", "audioValue");
	in("release", "audioValue");
	out("result", "audioValue");
}

AudioNodeEnvelope::AudioNodeEnvelope()
	: AudioNodeBase()
	, state(kState_Idle)
	, hasSignal(false)
	, value(0.0)
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Signal, kAudioPlugType_FloatVec);
	addInput(kInput_Attack, kAudioPlugType_FloatVec);
	addInput(kInput_Decay, kAudioPlugType_FloatVec);
	addInput(kInput_Sustain, kAudioPlugType_FloatVec);
	addInput(kInput_Release, kAudioPlugType_FloatVec);
	addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeEnvelope::tick(const float dt)
{
	const AudioFloat * signal = getInputAudioFloat(kInput_Signal, &AudioFloat::Zero);
	const AudioFloat * attack = getInputAudioFloat(kInput_Attack, &AudioFloat::Zero);
	const AudioFloat * decay = getInputAudioFloat(kInput_Decay, &AudioFloat::Zero);
	const AudioFloat * sustain = getInputAudioFloat(kInput_Sustain, &AudioFloat::Zero);
	const AudioFloat * release = getInputAudioFloat(kInput_Release, &AudioFloat::Zero);
	
	signal->expand();
	
	const bool scalarInputs =
		attack->isScalar &&
		decay->isScalar &&
		sustain->isScalar &&
		release->isScalar;
	
	if (scalarInputs)
	{
		resultOutput.setVector();
		
		const double dt = 1.0 / SAMPLE_RATE;
		
		const double attackPerSample = dt / attack->getScalar();
		const double decayPerSample = dt / decay->getScalar();
		const double releasePerSample = dt / release->getScalar();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const bool newSignal = signal->samples[i] != 0.f;
			
			if (newSignal != hasSignal)
			{
				hasSignal = newSignal;
				
				if (hasSignal)
					state = kState_Attack;
				else
					state = kState_Release;
			}
			
			switch (state)
			{
			case kState_Idle:
				break;
				
			case kState_Attack:
				{
					value += attackPerSample;
					
					if (value >= 1.0)
					{
						value = 1.0;
						
						state = kState_Decay;
						break;
					}
				}
				break;
				
			case kState_Decay:
				{
					value -= decayPerSample;
					
					if (value < sustain->samples[i])
					{
						value = sustain->samples[i];
						
						state = kState_Sustain;
						break;
					}
				}
				break;
				
			case kState_Sustain:
				break;
				
			case kState_Release:
				{
					value -= releasePerSample;
					
					if (value < 0.0)
					{
						value = 0.0;
						
						state = kState_Idle;
						break;
					}
				}
				break;
			}
			
			resultOutput.samples[i] = value;
		}
	}
	else
	{
		attack->expand();
		decay->expand();
		sustain->expand();
		release->expand();
		
		//
		
		resultOutput.setVector();
		
		const float dt = 1.f / SAMPLE_RATE;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const bool newSignal = signal->samples[i] != 0.f;
			
			if (newSignal != hasSignal)
			{
				hasSignal = newSignal;
				
				if (hasSignal)
					state = kState_Attack;
				else
					state = kState_Release;
			}
			
			switch (state)
			{
			case kState_Idle:
				break;
				
			case kState_Attack:
				{
					value += dt / attack->samples[i];
					
					if (value >= 1.0)
					{
						value = 1.0;
						
						state = kState_Decay;
						break;
					}
				}
				break;
				
			case kState_Decay:
				{
					value -= dt / decay->samples[i];
					
					if (value < sustain->samples[i])
					{
						value = sustain->samples[i];
						
						state = kState_Sustain;
						break;
					}
				}
				break;
				
			case kState_Sustain:
				break;
				
			case kState_Release:
				{
					value -= dt / release->samples[i];
					
					if (value < 0.0)
					{
						value = 0.0;
						
						state = kState_Idle;
						break;
					}
				}
				break;
			}
			
			resultOutput.samples[i] = value;
		}
	}
}
