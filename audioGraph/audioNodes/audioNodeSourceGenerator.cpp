/*
	Copyright (C) 2020 Marcel Smit
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

// note : sine, triangle and square appear to have a different 'phase' when looked
//        at through a visualizer. this is by design. by design the shape,
//        - is drawn just like the mathematical function sine(x) for sine,
//        - initially low (min) for the square and high (max) for phase > skew
//        - initially low (min) for the triangle and high (max) for phase == skew

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
	in("skew", "audioValue", "0.5");
	in("a", "audioValue", "0");
	in("b", "audioValue", "1");
	out("audio", "audioValue");
}

static const AudioFloat defaultSkew(.5f);

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
	addInput(kInput_Skew, kAudioPlugType_FloatVec);
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
	
	const float twoPi = float(2.f * M_PI);
	
	const float dt = 1.f / SAMPLE_RATE;
	
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
	const AudioFloat * skew = getInputAudioFloat(kInput_Skew, &defaultSkew);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::One);
	
	if (fine)
	{
		const float dt = 1.f / SAMPLE_RATE;
		
		audioOutput.setVector();
		
		frequency->expand();
		skew->expand();
		a->expand();
		b->expand();
		
		if (mode == kMode_BaseScale)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value;
				
				const float skew_i = skew->samples[i];
				
				if (phase < skew_i)
				{
					value = -1.f + 2.f * phase / skew_i;
				}
				else
				{
					value = +1.f - 2.f * (phase - skew_i) / (1.f - skew_i);
				}
				
				audioOutput.samples[i] = a->samples[i] + value * b->samples[i];
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
		else if (mode == kMode_MinMax)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value;
				
				const float skew_i = skew->samples[i];
				
				if (phase < skew_i)
				{
					value = phase / skew_i;
				}
				else
				{
					value = 1.f - (phase - skew_i) / (1.f - skew_i);
				}
				
				audioOutput.samples[i] = a->samples[i] + value * (b->samples[i] - a->samples[i]);
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
	}
	else
	{
		const float dt = AUDIO_UPDATE_SIZE / float(SAMPLE_RATE);
		
		const float frequency_scalar = frequency->getMean();
		const float a_scalar = a->getMean();
		const float b_scalar = b->getMean();
		
		if (mode == kMode_BaseScale)
		{
			float value;
			
			const float skew_i = skew->getMean();
			
			if (phase < skew_i)
			{
				value = -1.f + 2.f * phase / skew_i;
			}
			else
			{
				value = +1.f - 2.f * (phase - skew_i) / (1.f - skew_i);
			}
				
			audioOutput.setScalar(a_scalar + value * b_scalar);
				
			phase += dt * frequency_scalar;
			phase = phase - floorf(phase);
		}
		else if (mode == kMode_MinMax)
		{
			float value;
			
			const float skew_i = skew->getMean();
			
			if (phase < skew_i)
			{
				value = phase / skew_i;
			}
			else
			{
				value = 1.f - (phase - skew_i) / (1.f - skew_i);
			}
			
			audioOutput.setScalar(a_scalar + value * (b_scalar - a_scalar));
			
			phase += dt * frequency_scalar;
			phase = phase - floorf(phase);
		}
	}
}

void AudioNodeSourceSine::drawSquare()
{
	const Mode mode = (Mode)getInputInt(kInput_Mode, kMode_MinMax);
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::One);
	const AudioFloat * skew = getInputAudioFloat(kInput_Skew, &defaultSkew);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::One);
	
	if (fine)
	{
		const float dt = 1.f / SAMPLE_RATE;
		
		audioOutput.setVector();
		
		frequency->expand();
		skew->expand();
		a->expand();
		b->expand();
		
		if (mode == kMode_BaseScale)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				float value = a->samples[i];
				
				const float skew_i = skew->samples[i];
				
				if (phase < skew_i)
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
				const float skew_i = skew->samples[i];
				
				audioOutput.samples[i] =
					phase < skew_i
						? a->samples[i]
						: b->samples[i];
				
				phase += dt * frequency->samples[i];
				phase = phase - floorf(phase);
			}
		}
	}
	else
	{
		const float dt = AUDIO_UPDATE_SIZE / float(SAMPLE_RATE);
		
		if (mode == kMode_BaseScale)
		{
			float value = a->getMean();
			
			const float skew_i = skew->getMean();
			
			if (phase < skew_i)
				value -= b->getMean();
			else
				value += b->getMean();
			
			audioOutput.setScalar(value);
			
			phase += dt * frequency->getMean();
			phase = phase - floorf(phase);
		}
		else if (mode == kMode_MinMax)
		{
			const float skew_i = skew->getMean();
			
			const float value =
				phase < skew_i
					? a->getMean()
					: b->getMean();
			
			audioOutput.setScalar(value);
			
			phase += dt * frequency->getMean();
			phase = phase - floorf(phase);
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
