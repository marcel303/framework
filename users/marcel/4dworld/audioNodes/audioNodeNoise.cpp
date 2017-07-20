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

AUDIO_NODE_TYPE(noise, AudioNodeNoise)
{
	typeName = "noise.1d";
	
	in("octaves", "int", "6");
	in("sample.rate", "int", "100");
	in("scale", "audioValue", "1");
	in("persistence", "audioValue", "0.5");
	in("min", "audioValue", "0");
	in("max", "audioValue", "1");
	in("x", "audioValue");
	out("result", "audioValue");
}

void AudioNodeNoise::draw()
{
	const int numOctaves = getInputInt(kInput_NumOctaves, 6);
	const int sampleRate = std::max(1, getInputInt(kInput_SampleRate, 100));
	const AudioFloat * scale = getInputAudioFloat(kInput_Scale, &AudioFloat::One);
	const AudioFloat * persistence = getInputAudioFloat(kInput_Persistence, &AudioFloat::Half);
	const AudioFloat * min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero);
	const AudioFloat * max = getInputAudioFloat(kInput_Max, &AudioFloat::One);
	const AudioFloat * x = getInputAudioFloat(kInput_X, &AudioFloat::Zero);
	
	const double numSamplesPerSecond = sampleRate;
	const double numSamplesPerTick = AUDIO_UPDATE_SIZE / double(SAMPLE_RATE) * numSamplesPerSecond;

	const int nearestSampleCount = int(std::round(numSamplesPerTick));

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
		const int nearestSamplingInterval = AUDIO_UPDATE_SIZE / nearestSampleCount;
		
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
