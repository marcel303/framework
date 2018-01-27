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

#include "audioNodeWavefield2D.h"
#include "Noise.h"
#include "wavefield.h"
#include <cmath>

#include "framework.h" // todo : remove

AUDIO_NODE_TYPE(wavefield_2d, AudioNodeWavefield2D)
{
	typeName = "wavefield.2d";
	
	in("size", "int", "16");
	in("gain", "audioValue", "1");
	in("pos.dampen", "audioValue");
	in("vel.dampen", "audioValue");
	in("tension", "audioValue", "1");
	in("wrap", "bool", "0");
	in("sample.pos.x", "audioValue", "0.5");
	in("sample.pos.y", "audioValue", "0.5");
	in("trigger!", "trigger");
	in("trigger.pos.x", "audioValue", "0.5");
	in("trigger.pos.y", "audioValue", "0.5");
	in("trigger.amount", "audioValue", "0.5");
	in("trigger.size", "audioValue", "1");
	in("randomize!", "trigger");
	out("audio", "audioValue");
}

#if 0

template <typename T>
inline T lerp(T a, T b, T t)
{
	return a * (1.0 - t) + b * t;
}

template <typename T>
inline T clamp(T value, T min, T max)
{
	return value < min ? min : value > max ? max : value;
}

#endif

AudioNodeWavefield2D::AudioNodeWavefield2D()
	: AudioNodeBase()
	, wavefield(nullptr)
	, rng()
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Size, kAudioPlugType_Int);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_PositionDampening, kAudioPlugType_FloatVec);
	addInput(kInput_VelocityDampening, kAudioPlugType_FloatVec);
	addInput(kInput_Tension, kAudioPlugType_FloatVec);
	addInput(kInput_Wrap, kAudioPlugType_Bool);
	addInput(kInput_SampleLocationX, kAudioPlugType_FloatVec);
	addInput(kInput_SampleLocationY, kAudioPlugType_FloatVec);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addInput(kInput_TriggerLocationX, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerLocationY, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerAmount, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerSize, kAudioPlugType_FloatVec);
	addInput(kInput_Randomize, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);

	wavefield = new Wavefield2D();
}

AudioNodeWavefield2D::~AudioNodeWavefield2D()
{
	delete wavefield;
	wavefield = nullptr;
}

void AudioNodeWavefield2D::randomize()
{
	const double xRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double yRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double randomFactor = rng.nextd(0.0, 1.0);
	//const double cosFactor = rng.nextd(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = rng.nextd(0.0, 1.0);
	
	for (int x = 0; x < wavefield->numElems; ++x)
	{
		for (int y = 0; y < wavefield->numElems; ++y)
		{
			wavefield->p[x][y] = 0.0;
			wavefield->v[x][y] = 0.0;
			wavefield->d[x][y] = 0.0;
			
			wavefield->f[x][y] = 1.0;
			
			wavefield->f[x][y] *= lerp<double>(1.0, rng.nextd(0.f, 1.f), randomFactor);
			wavefield->f[x][y] *= lerp<double>(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
			//wavefield->f[x][y] = 1.0 - std::pow(wavefield->f[x][y], 2.0);
			
			//wavefield->f[x][y] = 1.0 - std::pow(rng.nextd(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			wavefield->f[x][y] *= lerp<double>(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
		}
	}
}

void AudioNodeWavefield2D::init(const GraphNode & node)
{
	if (isPassthrough)
	{
		return;
	}
	
	const int size = getInputInt(kInput_Size, 16);
	
	if (size != wavefield->numElems)
	{
		wavefield->init(size);
		
		randomize();
	}
}

void AudioNodeWavefield2D::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeWavefield2D);
	
	const AudioFloat defaultTension(1.f);
	
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const AudioFloat * positionDampening = getInputAudioFloat(kInput_PositionDampening, &AudioFloat::Zero);
	const AudioFloat * velocityDampening = getInputAudioFloat(kInput_VelocityDampening, &AudioFloat::Zero);
	const AudioFloat * tension = getInputAudioFloat(kInput_Tension, &defaultTension);
	const bool wrap = getInputBool(kInput_Wrap, false);
	const AudioFloat * sampleLocationX = getInputAudioFloat(kInput_SampleLocationX, &AudioFloat::Half);
	const AudioFloat * sampleLocationY = getInputAudioFloat(kInput_SampleLocationY, &AudioFloat::Half);
	const int size = getInputInt(kInput_Size, 16);
	
	//

	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
		return;
	}
	
	//
	
	if (wavefield->roundNumElems(size) != wavefield->numElems)
	{
		wavefield->init(size);
		
		randomize();
	}
	
	//
	
	audioOutput.setVector();
	
	positionDampening->expand();
	velocityDampening->expand();
	tension->expand();
	sampleLocationX->expand();
	sampleLocationY->expand();
	
	//
	
	const double dt = 1.0 / double(SAMPLE_RATE);
	
	const double maxTension = 2000000000.0;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const double c = clamp<double>(tension->samples[i] * 1000000.0, -maxTension, +maxTension);
		
		wavefield->tick(dt, c, 1.0 - velocityDampening->samples[i], 1.0 - positionDampening->samples[i], wrap == false);
		
		audioOutput.samples[i] = wavefield->sample(
			sampleLocationX->samples[i] * wavefield->numElems,
			sampleLocationY->samples[i] * wavefield->numElems);
	}
	
	audioOutput.mul(*gain);
}

void AudioNodeWavefield2D::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		const float triggerPositionX = getInputAudioFloat(kInput_TriggerLocationX, &AudioFloat::Half)->getMean();
		const float triggerPositionY = getInputAudioFloat(kInput_TriggerLocationY, &AudioFloat::Half)->getMean();
		const float triggerAmount = getInputAudioFloat(kInput_TriggerAmount, &AudioFloat::Half)->getMean();
		const float triggerSize = getInputAudioFloat(kInput_TriggerSize, &AudioFloat::One)->getMean();
		
		if (wavefield->numElems > 0)
		{
			const int elemX = int(std::round(std::abs(triggerPositionX) * wavefield->numElems)) % wavefield->numElems;
			const int elemY = int(std::round(std::abs(triggerPositionY) * wavefield->numElems)) % wavefield->numElems;
			
			if (triggerSize == 1.f)
				wavefield->d[elemX][elemY] += triggerAmount;
			else
				wavefield->doGaussianImpact(elemX, elemX, triggerSize, triggerAmount);
		}
	}
	else if (inputSocketIndex == kInput_Randomize)
	{
		randomize();
	}
}

