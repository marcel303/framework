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

#include "audioNodeCombFilter.h"
#include "vfxNodes/delayLine.h"

AUDIO_NODE_TYPE(filter_comb, AudioNodeCombFilter)
{
	typeName = "filter.comb";
	
	in("value", "audioValue");
	in("maxDelay", "float");
	in("delay", "audioValue");
	in("feedforward", "audioValue", "0.5");
	in("feedback", "audioValue", "0");
	out("value", "audioValue");
}

AudioNodeCombFilter::AudioNodeCombFilter()
	: AudioNodeBase()
	, valueOutput(0.f)
	, delayLine(nullptr)
{
	delayLine = new DelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_MaxDelay, kAudioPlugType_Float);
	addInput(kInput_Delay, kAudioPlugType_FloatVec);
	addInput(kInput_FeedForward, kAudioPlugType_FloatVec);
	addInput(kInput_FeedBack, kAudioPlugType_FloatVec);
	addOutput(kOutput_Value, kAudioPlugType_FloatVec, &valueOutput);
}

AudioNodeCombFilter::~AudioNodeCombFilter()
{
	delete delayLine;
	delayLine = nullptr;
}

void AudioNodeCombFilter::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeCombFilter);
	
	const float maxDelay = getInputFloat(kInput_MaxDelay, 0.f);
	
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const AudioFloat * delay = getInputAudioFloat(kInput_Delay, &AudioFloat::Zero);
	const AudioFloat * feedForward = getInputAudioFloat(kInput_FeedForward, &AudioFloat::Half);
	const AudioFloat * feedBack = getInputAudioFloat(kInput_FeedBack, &AudioFloat::Zero);
	
	{
		// set delay line length
		
		const int numSamples = maxDelay * SAMPLE_RATE;
		
		if (numSamples != delayLine->getLength())
		{
			delayLine->setLength(numSamples);
		}
	}
	
	if (isPassthrough)
	{
		valueOutput.set(*value);
	}
	else if (delayLine->getLength() > 0)
	{
		value->expand();
		delay->expand();
		feedForward->expand();
		feedBack->expand();

		valueOutput.setVector();

		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			//const float offset = std::min(float(delayLine->getLength() - 1),  ((maxDelay - delay->samples[i]) * SAMPLE_RATE));
			const int offset = std::min(delayLine->getLength() - 1,  int((maxDelay - delay->samples[i]) * SAMPLE_RATE));
			
			float input = value->samples[i];
			
			input += delayLine->read(offset) * feedBack->samples[i];

			delayLine->push(input);
			
			float output = input;

			output += delayLine->read(offset) * feedForward->samples[i];

			valueOutput.samples[i] = output;
		}
	}
	else
	{
		valueOutput.setScalar(0.f);
	}
}
