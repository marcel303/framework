#include "vfxNodeImpulseResponse.h"
#include "vfxTypes.h"

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
