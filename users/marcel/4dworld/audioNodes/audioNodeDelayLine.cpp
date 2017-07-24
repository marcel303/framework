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

#include "audioNodeDelayLine.h"
#include "vfxNodes/delayLine.h"

AUDIO_NODE_TYPE(sample_delay, AudioNodeDelayLine)
{
	typeName = "sample.delay";
	
	in("value", "audioValue");
	in("maxDelay", "float");
	in("delay1", "audioValue");
	in("delay2", "audioValue");
	in("delay3", "audioValue");
	in("delay4", "audioValue");
	out("value1", "audioValue");
	out("value2", "audioValue");
	out("value3", "audioValue");
	out("value4", "audioValue");
}

AudioNodeDelayLine::AudioNodeDelayLine()
	: AudioNodeBase()
	, outputValue()
	, dtRemaining(0.f)
	, delayLine(nullptr)
{
	delayLine = new DelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_MaxDelay, kAudioPlugType_Float);
	addInput(kInput_Delay1, kAudioPlugType_FloatVec);
	addInput(kInput_Delay2, kAudioPlugType_FloatVec);
	addInput(kInput_Delay3, kAudioPlugType_FloatVec);
	addInput(kInput_Delay4, kAudioPlugType_FloatVec);
	addOutput(kOutput_Value1, kAudioPlugType_FloatVec, &outputValue[0]);
	addOutput(kOutput_Value2, kAudioPlugType_FloatVec, &outputValue[1]);
	addOutput(kOutput_Value3, kAudioPlugType_FloatVec, &outputValue[2]);
	addOutput(kOutput_Value4, kAudioPlugType_FloatVec, &outputValue[3]);
}

AudioNodeDelayLine::~AudioNodeDelayLine()
{
	delete delayLine;
	delayLine = nullptr;
}

void AudioNodeDelayLine::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeDelayLine);
	
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	
	const float maxDelay = getInputFloat(kInput_MaxDelay, 0.f);
	
	const AudioFloat * delay1 = getInputAudioFloat(kInput_Delay1, &AudioFloat::Zero);
	const AudioFloat * delay2 = getInputAudioFloat(kInput_Delay2, &AudioFloat::Zero);
	const AudioFloat * delay3 = getInputAudioFloat(kInput_Delay3, &AudioFloat::Zero);
	const AudioFloat * delay4 = getInputAudioFloat(kInput_Delay4, &AudioFloat::Zero);
	
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
		outputValue[0].set(*value);
		outputValue[1].set(*value);
		outputValue[2].set(*value);
		outputValue[3].set(*value);
	}
	else if (delayLine->getLength() > 0)
	{
		int nextWriteIndex = delayLine->nextWriteIndex;
		
		const bool fixedDelay =
			delay1->isScalar &&
			delay2->isScalar &&
			delay3->isScalar &&
			delay4->isScalar;
		
		if (fixedDelay)
		{
			const int offset1 = std::min(delayLine->getLength() - 1, int((maxDelay - delay1->getScalar()) * SAMPLE_RATE));
			const int offset2 = std::min(delayLine->getLength() - 1, int((maxDelay - delay2->getScalar()) * SAMPLE_RATE));
			const int offset3 = std::min(delayLine->getLength() - 1, int((maxDelay - delay3->getScalar()) * SAMPLE_RATE));
			const int offset4 = std::min(delayLine->getLength() - 1, int((maxDelay - delay4->getScalar()) * SAMPLE_RATE));
			
			if (value->isScalar)
			{
				const float valueScalar = value->getScalar();
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
					nextWriteIndex = delayLine->pushEx(nextWriteIndex, valueScalar);
				
				outputValue[0].setScalar(delayLine->readEx(nextWriteIndex, offset1));
				outputValue[1].setScalar(delayLine->readEx(nextWriteIndex, offset2));
				outputValue[2].setScalar(delayLine->readEx(nextWriteIndex, offset3));
				outputValue[3].setScalar(delayLine->readEx(nextWriteIndex, offset4));
			}
			else
			{
				outputValue[0].setVector();
				outputValue[1].setVector();
				outputValue[2].setVector();
				outputValue[3].setVector();
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					nextWriteIndex = delayLine->pushEx(nextWriteIndex, value->samples[i]);
					
					outputValue[0].samples[i] = delayLine->readEx(nextWriteIndex, offset1);
					outputValue[1].samples[i] = delayLine->readEx(nextWriteIndex, offset2);
					outputValue[2].samples[i] = delayLine->readEx(nextWriteIndex, offset3);
					outputValue[3].samples[i] = delayLine->readEx(nextWriteIndex, offset4);
				}
			}
		}
		else
		{
			value->expand();
			
			delay1->expand();
			delay2->expand();
			delay3->expand();
			delay4->expand();
			
			outputValue[0].setVector();
			outputValue[1].setVector();
			outputValue[2].setVector();
			outputValue[3].setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				nextWriteIndex = delayLine->pushEx(nextWriteIndex, value->samples[i]);
				
				const int offset1 = std::min(delayLine->getLength() - 1, int((maxDelay - delay1->samples[i]) * SAMPLE_RATE));
				const int offset2 = std::min(delayLine->getLength() - 1, int((maxDelay - delay2->samples[i]) * SAMPLE_RATE));
				const int offset3 = std::min(delayLine->getLength() - 1, int((maxDelay - delay3->samples[i]) * SAMPLE_RATE));
				const int offset4 = std::min(delayLine->getLength() - 1, int((maxDelay - delay4->samples[i]) * SAMPLE_RATE));
				
				outputValue[0].samples[i] = delayLine->readEx(nextWriteIndex, offset1);
				outputValue[1].samples[i] = delayLine->readEx(nextWriteIndex, offset2);
				outputValue[2].samples[i] = delayLine->readEx(nextWriteIndex, offset3);
				outputValue[3].samples[i] = delayLine->readEx(nextWriteIndex, offset4);
			}
		}
		
		delayLine->nextWriteIndex = nextWriteIndex;
	}
	else
	{
		outputValue[0].setScalar(0.f);
		outputValue[1].setScalar(0.f);
		outputValue[2].setScalar(0.f);
		outputValue[3].setScalar(0.f);
	}
}
