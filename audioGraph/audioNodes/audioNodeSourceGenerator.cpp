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

#include "audioNodeSourceGenerator.h"
#include <math.h>

AUDIO_ENUM_TYPE(audioSineType)
{
	elem("sine");
	elem("triangle");
	elem("square");
}

AUDIO_ENUM_TYPE(audioSineMode)
{
	elem("baseScale");
	elem("minMax");
}

AUDIO_NODE_TYPE(AudioNodeSourceSine)
{
	typeName = "audio.sine";
	
	in("fine", "bool", "1");
	inEnum("type", "audioSineType");
	inEnum("mode", "audioSineMode", "1");
	in("frequency", "audioValue", "1");
	in("skew", "float", "0.5");
	in("a", "audioValue", "0");
	in("b", "audioValue", "1");
	out("audio", "audioValue");
}

AudioNodeSourceSine::AudioNodeSourceSine()
	: AudioNodeBase()
	, audioOutput()
	, phase(0.0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Fine, kAudioPlugType_Bool);
	addInput(kInput_Type, kAudioPlugType_Int);
	addInput(kInput_Mode, kAudioPlugType_Int);
	addInput(kInput_Frequency, kAudioPlugType_FloatVec);
	addInput(kInput_Skew, kAudioPlugType_Float); // todo : change into float vec
	addInput(kInput_A, kAudioPlugType_FloatVec);
	addInput(kInput_B, kAudioPlugType_FloatVec);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
}

void AudioNodeSourceSine::drawSine()
{
	const Mode mode = (Mode)getInputInt(kInput_Mode, kMode_MinMax);
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::One);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::One);
	
	const float twoPi = 2.f * M_PI;
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	if (fine)
	{
		audioOutput.setVector();
		
		frequency->expand();
		a->expand();
		b->expand();
		
		if (mode == kMode_BaseScale)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				audioOutput.samples[i] = a->samples[i] + sinf(phase * twoPi) * b->samples[i];
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
		else if (mode == kMode_MinMax)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				audioOutput.samples[i] = a->samples[i] + (.5f + .5f * sinf(phase * twoPi)) * (b->samples[i] - a->samples[i]);
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
	}
	else
	{
		if (mode == kMode_BaseScale)
		{
			const float value = a->getMean() + sinf(phase * twoPi) * b->getMean();
			
			audioOutput.setScalar(value);
			
			phase += dt * frequency->getMean() * AUDIO_UPDATE_SIZE;
			phase = fmodf(phase, 1.f);
		}
		else if (mode == kMode_MinMax)
		{
			const float aMean = a->getMean();
			const float bMean = b->getMean();
			
			const float value = aMean + (.5f + .5f * sinf(phase * twoPi)) * (bMean - aMean);
			
			audioOutput.setScalar(value);
			
			phase += dt * frequency->getMean() * AUDIO_UPDATE_SIZE;
			phase = fmodf(phase, 1.f);
		}
	}
}

void AudioNodeSourceSine::drawTriangle()
{
	const Mode mode = (Mode)getInputInt(kInput_Mode, kMode_MinMax);
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::One);
	const float skew = getInputFloat(kInput_Skew, .5f);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::One);
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	if (fine || true)
	{
		audioOutput.setVector();
		
		frequency->expand();
		a->expand();
		b->expand();
		
		if (mode == kMode_BaseScale)
		{
			const float mulA = 1.f / skew;
			const float mulB = 2.f / (1.f - skew);
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value;
				
				if (phase < skew)
				{
					value = -1.f + 2.f * phase * mulA;
				}
				else
				{
					value = +1.f - (phase - skew) * mulB;
				}
				
				audioOutput.samples[i] = a->samples[i] + value * b->samples[i];
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
		else if (mode == kMode_MinMax)
		{
			const float mulA = 1.f / skew;
			const float mulB = 1.f / (1.f - skew);
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value;
				
				if (phase < skew)
				{
					value = phase * mulA;
				}
				else
				{
					value = 1.f - (phase - skew) * mulB;
				}
				
				audioOutput.samples[i] = a->samples[i] + value * (b->samples[i] - a->samples[i]);
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
	}
}

void AudioNodeSourceSine::drawSquare()
{
	const Mode mode = (Mode)getInputInt(kInput_Mode, kMode_MinMax);
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::One);
	const float skew = getInputFloat(kInput_Skew, .5f);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::One);
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	if (fine || true)
	{
		audioOutput.setVector();
		
		frequency->expand();
		a->expand();
		b->expand();
		
		if (mode == kMode_BaseScale)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value = a->samples[i];
				
				if (phase < skew)
					value -= b->samples[i];
				else
					value += b->samples[i];
				
				audioOutput.samples[i] = value;
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
		else if (mode == kMode_MinMax)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				audioOutput.samples[i] = phase < skew ? a->samples[i] : b->samples[i];
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
	}
}

void AudioNodeSourceSine::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, 0);
	
	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
	}
	else if (type == kType_Sine)
	{
		drawSine();
	}
	else if (type == kType_Triangle)
	{
		drawTriangle();
	}
	else if (type == kType_Square)
	{
		drawSquare();
	}
	else
	{
		Assert(false);
		
		audioOutput.setScalar(0.f);
	}
}
