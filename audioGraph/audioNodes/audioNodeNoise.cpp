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

AUDIO_NODE_TYPE(AudioNodeNoise)
{
	typeName = "noise.1d";
	
	inEnum("type", "noiseType");
	in("fine", "bool", "1");
	in("octaves", "int", "6");
	in("sample.rate", "int", "100");
	in("scale", "audioValue", "1", "gain");
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
	const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	auto & rng = pinkNumber.rng;
	
	if (fine == false)
	{
		const float minValue = min->getMean() * scale->getMean();
		const float maxValue = max->getMean() * scale->getMean();
		
		resultOutput.setScalar(rng.nextf(minValue, maxValue));
	}
	else if (min->isScalar && max->isScalar && scale->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar() * scale->getScalar();
		const float maxValue = max->getScalar() * scale->getScalar();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = rng.nextf(minValue, maxValue);
		}
	}
	else
	{
		scale->expand();
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i] * scale->samples[i];
			const float maxValue = max->samples[i] * scale->samples[i];
			
			resultOutput.samples[i] = rng.nextf(minValue, maxValue);
		}
	}
}

void AudioNodeNoise::drawPink()
{
	// pink noise (-3dB/octave) using Voss' method of a sample-and-hold random number generator
	
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	const float pinkScale = 1.f / float(1 << 16);
	
	if (fine == false)
	{
		const float minValue = min->getMean() * scale->getMean();
		const float maxValue = max->getMean() * scale->getMean();
		
		const float t = pinkNumber.next() * pinkScale;
		
		resultOutput.setScalar(minValue + (maxValue - minValue) * t);
	}
	else if (min->isScalar && max->isScalar && scale->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar() * scale->getScalar();
		const float maxValue = max->getScalar() * scale->getScalar();
		
		ALIGN16 float values[AUDIO_UPDATE_SIZE];
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			values[i] = pinkNumber.next() * pinkScale;
		}
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float t = values[i];
			
			resultOutput.samples[i] = minValue + (maxValue - minValue) * t;
		}
	}
	else
	{
		scale->expand();
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i] * scale->samples[i];
			const float maxValue = max->samples[i] * scale->samples[i];
			
			const float t = pinkNumber.next() * pinkScale;
			
			resultOutput.samples[i] = minValue + (maxValue - minValue) * t;
		}
	}
}

void AudioNodeNoise::drawBrown()
{
	// brown noise (-6dB/octave) by integrating brownian motion, using a leaky integrator to ensure the signal doesn't drift off too much from zero
	
	const bool fine = getInputBool(kInput_Fine, true);
	const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	
	const double falloffPerSample = 0.98;
	const float wanderPerSample = .1f;
	
	auto & rng = pinkNumber.rng;
	
	if (fine == false)
	{
		const float minValue = min->getMean() * scale->getMean();
		const float maxValue = max->getMean() * scale->getMean();
		
		resultOutput.setScalar(((minValue + maxValue) + brownValue * (maxValue - minValue)) * .5f);
		
		brownValue *= falloffPerSample;
		brownValue += rng.nextf(-wanderPerSample, +wanderPerSample);
	}
	else if (min->isScalar && max->isScalar && scale->isScalar)
	{
		resultOutput.setVector();
		
		const float minValue = min->getScalar() * scale->getScalar();
		const float maxValue = max->getScalar() * scale->getScalar();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = ((minValue + maxValue) + (float)brownValue * (maxValue - minValue)) * .5f;

			brownValue *= falloffPerSample;
			brownValue += rng.nextf(-wanderPerSample, +wanderPerSample);
		}
	}
	else
	{
		scale->expand();
		min->expand();
		max->expand();
		
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float minValue = min->samples[i] * scale->samples[i];
			const float maxValue = max->samples[i] * scale->samples[i];
			
			resultOutput.samples[i] = ((minValue + maxValue) + (float)brownValue * (maxValue - minValue)) * .5f;

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
