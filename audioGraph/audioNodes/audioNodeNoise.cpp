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

#include "audioNodeNoise.h"
#include "Noise.h"
#include <algorithm>
#include <cmath>

AUDIO_ENUM_TYPE(noiseType)
{
	elem("octave");
	elem("white");
	elem("pink");
	elem("brown");
}

AUDIO_NODE_TYPE(noise, AudioNodeNoise)
{
	typeName = "noise.1d";
	
	inEnum("type", "noiseType");
	in("fine", "bool", "1");
	in("octaves", "int", "6");
	in("sample.rate", "int", "100");
	in("scale", "audioValue", "1");
	in("persistence", "audioValue", "0.5");
	in("min", "audioValue", "0");
	in("max", "audioValue", "1");
	in("x", "audioValue");
	out("result", "audioValue");
}

void AudioNodeNoise::drawOctave()
{
	const int numOctaves = getInputInt(kInput_NumOctaves, 6);
	const int sampleRate = std::max(1, getInputInt(kInput_SampleRate, 100));
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
	const AudioFloat * persistence = getInputAudioFloat(kInput_Persistence, &AudioFloat::Half);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	const AudioFloat * x = getInputAudioFloat(kInput_X, &AudioFloat::Zero);
	
	const double numSamplesPerSecond = sampleRate;
	const double numSamplesPerTick = AUDIO_UPDATE_SIZE / double(SAMPLE_RATE) * numSamplesPerSecond;

	const int nearestSampleCount = fine ? int(std::round(numSamplesPerTick)) : 1;

	if (nearestSampleCount <= 1)
	{
		const int iMid = AUDIO_UPDATE_SIZE/2;

		const float value = scaled_octave_noise_1d(
			numOctaves,
			persistence->isScalar ? persistence->getScalar() : persistence->samples[iMid],
			scale->isScalar ? scale->getScalar() : scale->samples[iMid],
			min->isScalar ? min->getScalar() : min->samples[iMid],
			max->isScalar ? max->getScalar() : max->samples[iMid],
			x->isScalar ? x->getScalar() : x->samples[iMid]);
		
		resultOutput.setScalar(value);
	}
	else
	{
		const int nearestSamplingInterval = std::max(1, AUDIO_UPDATE_SIZE / nearestSampleCount);
		
		scale->expand();
		persistence->expand();
		min->expand();
		max->expand();
		x->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; i += nearestSamplingInterval)
		{
			const int i1 = i;
			const int i2 = std::min(AUDIO_UPDATE_SIZE, i1 + nearestSamplingInterval);

			const int iMid = (i1 + i2) / 2;

			const float value = scaled_octave_noise_1d(
				numOctaves,
				persistence->samples[iMid],
				scale->samples[iMid],
				min->samples[iMid],
				max->samples[iMid],
				x->samples[iMid]);
			
			for (int j = i1; j < i2; ++j)
			{
				resultOutput.samples[j] = value;
			}
		}
	}
}

void AudioNodeNoise::drawWhite()
{
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	auto & rng = pinkNumber.rng;
	
	if (fine == false)
	{
		const float minValue = min->getMean();
		const float maxValue = max->getMean();
		
		resultOutput.setScalar(rng.nextf(minValue, maxValue));
	}
	else if (min->isScalar && max->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar();
		const float maxValue = max->getScalar();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = rng.nextf(minValue, maxValue);
		}
	}
	else
	{
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i];
			const float maxValue = max->samples[i];
			
			resultOutput.samples[i] = rng.nextf(minValue, maxValue);
		}
	}
}

void AudioNodeNoise::drawPink()
{
	// pink noise (-3dB/octave) using Voss' method of a sample-and-hold random number generator
	
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	const float scale = 1.f / float(1 << 16);
	
	if (fine == false)
	{
		const float minValue = min->getMean();
		const float maxValue = max->getMean();
		
		const float t = pinkNumber.next() * scale;
		
		resultOutput.setScalar(minValue + (maxValue - minValue) * t);
	}
	else if (min->isScalar && max->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar();
		const float maxValue = max->getScalar();
		
		ALIGN16 int values[AUDIO_UPDATE_SIZE];
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			values[i] = pinkNumber.next();
		}
		
		// todo : write (SSE) optimized routine to convert, scale and map values from integer to floating point
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float t = values[i] * scale;
			
			resultOutput.samples[i] = minValue + (maxValue - minValue) * t;
		}
	}
	else
	{
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i];
			const float maxValue = max->samples[i];
			
			const float t = pinkNumber.next() / float(1 << 16);
			
			resultOutput.samples[i] = minValue + (maxValue - minValue) * t;
		}
	}
}

void AudioNodeNoise::drawBrown()
{
	// brown noise (-6dB/octave) by integrating brownian motion, using a leaky integrator to ensure the signal doesn't drift off too much from zero
	
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	const double falloffPerSample = 0.98;
	const double wanderPerSample = 0.1;
	
	auto & rng = pinkNumber.rng;
	
	if (fine == false)
	{
		const float minValue = min->getMean();
		const float maxValue = max->getMean();
		
		resultOutput.setScalar(((minValue + maxValue) + brownValue * (maxValue - minValue)) * .5f);
		
		brownValue *= falloffPerSample;
		brownValue += rng.nextf(-wanderPerSample, +wanderPerSample);
	}
	else if (min->isScalar && max->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar();
		const float maxValue = max->getScalar();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = ((minValue + maxValue) + brownValue * (maxValue - minValue)) * .5f;

			brownValue *= falloffPerSample;
			brownValue += rng.nextf(-wanderPerSample, +wanderPerSample);
		}
	}
	else
	{
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i];
			const float maxValue = max->samples[i];
			
			resultOutput.samples[i] = ((minValue + maxValue) + brownValue * (maxValue - minValue)) * .5f;

			brownValue *= falloffPerSample;
			brownValue += rng.nextf(-wanderPerSample, +wanderPerSample);
		}
	}
}

void AudioNodeNoise::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, 0);
	
	if (isPassthrough)
	{
		resultOutput.setScalar(0.f);
	}
	else if (type == kType_Octave)
	{
		drawOctave();
	}
	else if (type == kType_White)
	{
		drawWhite();
	}
	else if (type == kType_Pink)
	{
		drawPink();
	}
	else if (type == kType_Brown)
	{
		drawBrown();
	}
	else
	{
		resultOutput.setScalar(0.f);
	}
}
