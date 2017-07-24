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

#include "audioNodePhysicalSpring.h"

AUDIO_NODE_TYPE(physical_spring, AudioNodePhysicalSpring)
{
	typeName = "physical.spring";
	
	in("strength", "audioValue", "1.0");
	in("dampen", "audioValue", "0.5");
	in("force", "audioValue");
	in("impulse", "audioValue");
	in("impulse!", "trigger");
	out("value", "audioValue");
	out("speed", "audioValue");
}

AudioNodePhysicalSpring::AudioNodePhysicalSpring()
	: AudioNodeBase()
	, value(0.0)
	, speed(0.0)
	, outputValue(0.f)
	, outputSpeed(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Strength, kAudioPlugType_FloatVec);
	addInput(kInput_Dampen, kAudioPlugType_FloatVec);
	addInput(kInput_Force, kAudioPlugType_FloatVec);
	addInput(kInput_Impulse, kAudioPlugType_FloatVec);
	addInput(kInput_ImpulseTrigger, kAudioPlugType_Trigger);
	addOutput(kOutput_Value, kAudioPlugType_FloatVec, &outputValue);
	addOutput(kOutput_Speed, kAudioPlugType_FloatVec, &outputSpeed);
}

void AudioNodePhysicalSpring::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodePhysicalSpring);
	
	if (isPassthrough)
	{
		outputValue.setScalar(0.f);
		
		return;
	}

	const AudioFloat * strength = getInputAudioFloat(kInput_Strength, &AudioFloat::One);
	const AudioFloat * dampen = getInputAudioFloat(kInput_Dampen, &AudioFloat::Half);
	const AudioFloat * externalForce = getInputAudioFloat(kInput_Force, &AudioFloat::Zero);

	strength->expand();
	externalForce->expand();

	const double dt = 1.0 / AUDIO_UPDATE_SIZE;

	outputValue.setVector();
	outputSpeed.setVector();
	
	double dampenThisTick;
	
	if (dampen->isScalar)
	{
		dampenThisTick = std::pow(1.0 - dampen->getScalar(), dt);
	}
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		if (dampen->isScalar == false)
		{
			dampenThisTick = std::pow(1.0 - dampen->samples[i], dt);
		}

		const double delta = value;
		const double force = externalForce->samples[i] - strength->samples[i] * delta;

		speed *= dampenThisTick;
		
		speed += force * dt;
		value += speed * dt;

		outputValue.samples[i] = value;
		outputSpeed.samples[i] = speed;
	}
}

void AudioNodePhysicalSpring::handleTrigger(const int inputSocketIndex, const AudioTriggerData & data)
{
	if (inputSocketIndex == kInput_ImpulseTrigger)
	{
		const AudioFloat * impulse = getInputAudioFloat(kInput_Impulse, &AudioFloat::Zero);
		
		speed += impulse->getMean();
	}
}
