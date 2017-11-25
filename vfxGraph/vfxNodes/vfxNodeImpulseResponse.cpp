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

#include "delayLine.h"
#include "vfxNodeImpulseResponse.h"

VFX_NODE_TYPE(VfxNodeImpulseResponse)
{
	typeName = "impulse.response";
	
	in("value", "float");
	in("frequency", "float");
	out("response", "float");
}

VfxNodeImpulseResponse::VfxNodeImpulseResponse()
	: VfxNodeBase()
	, impulseResponse(0.f)
	, dtRemaining(0.f)
	, delayLine(nullptr)
{
	delayLine = new DelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Frequency, kVfxPlugType_Float);
	addOutput(kOutput_ImpulseResponse, kVfxPlugType_Float, &impulseResponse);
}

VfxNodeImpulseResponse::~VfxNodeImpulseResponse()
{
	delete delayLine;
	delayLine = nullptr;
}

void VfxNodeImpulseResponse::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImpulseResponse);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	const float frequency = getInputFloat(kInput_Frequency, 0.f);
	
	if (frequency <= 0.f)
	{
		delayLine->setLength(0);
	}
	else
	{
		// set delay line length
		
		//const float delay = 1.f / frequency * 4.f;
		const float delay = 1.f / frequency * 2.f;
		
		const int numSamples = delay * kSampleRate;
		
		if (numSamples != delayLine->getLength())
		{
			delayLine->setLength(numSamples);
		}
	}
	
	if (delayLine->getLength() > 0)
	{
		const float dtTotal = dtRemaining + dt;
		
		const int numSamples = std::floor(dtTotal * kSampleRate);
		
		dtRemaining = dtTotal - numSamples / float(kSampleRate);
		
		for (int i = 0; i < numSamples; ++i)
			delayLine->push(value);
	}
	
	//
	
	if (delayLine->getLength() > 0)
	{
		const double twoPi = 2.0 * M_PI;
		
		double measurementPhase = 0.0;
		double measurementStep = twoPi * frequency / kSampleRate;
		
		double sumX = 0.0;
		double sumY = 0.0;
		
		for (int i = 0; i < delayLine->getLength(); ++i)
		{
			const double value = delayLine->read(i);
			
			const double x = std::cos(measurementPhase);
			const double y = std::sin(measurementPhase);
			
			sumX += value * x;
			sumY += value * y;
			
			measurementPhase = std::fmod(measurementPhase + measurementStep, twoPi);
		}
		
		const double avgX = sumX / delayLine->getLength();
		const double avgY = sumY / delayLine->getLength();
		
		impulseResponse = float(std::hypot(avgX, avgY) * 2.0);
	}
}
