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

#include "audioNodeWavefield1D.h"
#include "Noise.h"
#include "wavefield.h"

#include "framework.h" // random. todo : remove

AUDIO_NODE_TYPE(wavefield_1d, AudioNodeWavefield1D)
{
	typeName = "wavefield.1d";
	
	in("size", "int", "16");
	in("gain", "audioValue", "1");
	in("pos.dampen", "audioValue");
	in("vel.dampen", "audioValue");
	in("tension", "audioValue", "1");
	in("wrap", "bool", "0");
	in("sample.pos", "audioValue", "0.5");
	in("trigger!", "trigger");
	in("trigger.pos", "audioValue", "0.5");
	in("trigger.amount", "audioValue", "0.5");
	in("trigger.size", "1");
	in("randomize!", "trigger");
	out("audio", "audioValue");
}

AudioNodeWavefield1D::AudioNodeWavefield1D()
	: AudioNodeBase()
	, wavefield(nullptr)
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Size, kAudioPlugType_Int);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_PositionDampening, kAudioPlugType_FloatVec);
	addInput(kInput_VelocityDampening, kAudioPlugType_FloatVec);
	addInput(kInput_Tension, kAudioPlugType_FloatVec);
	addInput(kInput_Wrap, kAudioPlugType_Bool);
	addInput(kInput_SampleLocation, kAudioPlugType_FloatVec);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addInput(kInput_TriggerLocation, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerAmount, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerSize, kAudioPlugType_FloatVec);
	addInput(kInput_Randomize, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);

	wavefield = new Wavefield1D();
}

AudioNodeWavefield1D::~AudioNodeWavefield1D()
{
	delete wavefield;
	wavefield = nullptr;
}

void AudioNodeWavefield1D::randomize()
{
	const double xRatio = random(0.0, 1.0 / 10.0);
	const double randomFactor = random(0.0, 1.0);
	//const double cosFactor = random(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = random(0.0, 1.0);
	
	for (int x = 0; x < wavefield->numElems; ++x)
	{
		wavefield->p[x] = 0.0;
		wavefield->v[x] = 0.0;
		wavefield->d[x] = 0.0;
		
		wavefield->f[x] = 1.0;
		wavefield->f[x] *= lerp<double>(1.0, random(0.0, 1.0), randomFactor);
		wavefield->f[x] *= lerp<double>(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
		
		//wavefield->f[x] = 1.0 - std::pow(m_wavefield.f[x], 2.0);
		//wavefield->f[x] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
		
		wavefield->f[x] *= lerp<double>(1.0, scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
	}
}

void AudioNodeWavefield1D::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeWavefield1D);
	
	const AudioFloat defaultTension(1.f);
	
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const AudioFloat * positionDampening = getInputAudioFloat(kInput_PositionDampening, &AudioFloat::Zero);
	const AudioFloat * velocityDampening = getInputAudioFloat(kInput_VelocityDampening, &AudioFloat::Zero);
	const AudioFloat * tension = getInputAudioFloat(kInput_Tension, &defaultTension);
	const bool wrap = getInputBool(kInput_Wrap, false);
	const AudioFloat * sampleLocation = getInputAudioFloat(kInput_SampleLocation, &AudioFloat::Half);
	const int size = getInputInt(kInput_Size, 16);
	
	//

	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
		return;
	}
	
	//
	
	if (size != wavefield->numElems)
	{
		wavefield->init(size);
		
		randomize();
	}
	
	//
	
	audioOutput.setVector();
	
	positionDampening->expand();
	velocityDampening->expand();
	tension->expand();
	sampleLocation->expand();
	
	//
	
	const double dt = 1.0 / double(SAMPLE_RATE);
	
	const double maxTension = 2000000000.0;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const double c = clamp<double>(tension->samples[i] * 1000000.0, -maxTension, +maxTension);
		
		wavefield->tick(dt, c, 1.0 - velocityDampening->samples[i], 1.0 - positionDampening->samples[i], wrap == false);
		
		audioOutput.samples[i] = wavefield->sample(sampleLocation->samples[i] * wavefield->numElems);
	}
	
	audioOutput.mul(*gain);
}

void AudioNodeWavefield1D::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		const float triggerPosition = getInputAudioFloat(kInput_TriggerLocation, &AudioFloat::Half)->getMean();
		const float triggerAmount = getInputAudioFloat(kInput_TriggerAmount, &AudioFloat::Half)->getMean();
		const float triggerSize = getInputAudioFloat(kInput_TriggerSize, &AudioFloat::One)->getMean();
		
		if (wavefield->numElems > 0)
		{
			const int elemIndex = int(std::round(triggerPosition * wavefield->numElems)) % wavefield->numElems;
			
			if (triggerSize == 1.f)
				wavefield->d[elemIndex] += triggerAmount;
			else
				wavefield->d[elemIndex] += triggerAmount; // todo : implement gaussian impact
		}
	}
	else if (inputSocketIndex == kInput_Randomize)
	{
		randomize();
	}
}
