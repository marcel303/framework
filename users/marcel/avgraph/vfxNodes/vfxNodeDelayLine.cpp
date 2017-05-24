#include "vfxNodeDelayLine.h"
#include "vfxTypes.h"

VfxNodeDelayLine::VfxNodeDelayLine()
	: VfxNodeBase()
	, outputValue()
	, dtRemaining(0.f)
	, delayLine(nullptr)
{
	delayLine = new DelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Delay1, kVfxPlugType_Float);
	addInput(kInput_Delay2, kVfxPlugType_Float);
	addInput(kInput_Delay3, kVfxPlugType_Float);
	addInput(kInput_Delay4, kVfxPlugType_Float);
	addOutput(kOutput_Value1, kVfxPlugType_Float, &outputValue[0]);
	addOutput(kOutput_Value2, kVfxPlugType_Float, &outputValue[1]);
	addOutput(kOutput_Value3, kVfxPlugType_Float, &outputValue[2]);
	addOutput(kOutput_Value4, kVfxPlugType_Float, &outputValue[3]);
}

VfxNodeDelayLine::~VfxNodeDelayLine()
{
	delete delayLine;
	delayLine = nullptr;
}

void VfxNodeDelayLine::tick(const float dt)
{
	const float delay1 = getInputFloat(kInput_Delay1, 0.f);
	const float delay2 = getInputFloat(kInput_Delay2, 0.f);
	const float delay3 = getInputFloat(kInput_Delay3, 0.f);
	const float delay4 = getInputFloat(kInput_Delay4, 0.f);

	const float maxDelay = std::max(std::max(delay1, delay2), std::max(delay3, delay4));
	
	{
		// set delay line length
		
		const int numSamples = maxDelay * kSampleRate;
		
		if (numSamples != delayLine->getLength())
		{
			delayLine->setLength(numSamples);
		}
	}
	
	if (delayLine->getLength() > 0)
	{
		const float value = getInputFloat(kInput_Value, 0.f);
		
		const float dtTotal = dtRemaining + dt;
		
		const int numSamples = std::floor(dtTotal * kSampleRate);
		
		dtRemaining = dtTotal - numSamples / float(kSampleRate);
		
		for (int i = 0; i < numSamples; ++i)
			delayLine->push(value);
		
		const int offset1 = std::min(delayLine->getLength() - 1, int((maxDelay - delay1) * kSampleRate));
		const int offset2 = std::min(delayLine->getLength() - 1, int((maxDelay - delay2) * kSampleRate));
		const int offset3 = std::min(delayLine->getLength() - 1, int((maxDelay - delay3) * kSampleRate));
		const int offset4 = std::min(delayLine->getLength() - 1, int((maxDelay - delay4) * kSampleRate));
		
		outputValue[0] = delayLine->read(offset1);
		outputValue[1] = delayLine->read(offset2);
		outputValue[2] = delayLine->read(offset3);
		outputValue[3] = delayLine->read(offset4);
	}
	else
	{
		outputValue[0] = 0.f;
		outputValue[1] = 0.f;
		outputValue[2] = 0.f;
		outputValue[3] = 0.f;
	}
}
