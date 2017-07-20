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
#include "wavefield.h"

#include "framework.h" // random. todo : remove

AUDIO_NODE_TYPE(wavefield_1d, AudioNodeWavefield1D)
{
	typeName = "wavefield.1d";
	
	in("size", "int", "16");
	in("pos.dampen", "audioValue");
	in("vel.dampen", "audioValue");
	in("tension", "audioValue", "1");
	in("wrap", "bool", "0");
	in("trigger", "trigger");
	out("audio", "audioValue");
}

AudioNodeWavefield1D::AudioNodeWavefield1D()
	: AudioNodeBase()
	, wavefield(nullptr)
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Size, kAudioPlugType_Int);
	addInput(kInput_PositionDampening, kAudioPlugType_FloatVec);
	addInput(kInput_VelocityDampening, kAudioPlugType_FloatVec);
	addInput(kInput_Tension, kAudioPlugType_FloatVec);
	addInput(kInput_Wrap, kAudioPlugType_Bool);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);

	wavefield = new Wavefield1D();
}

AudioNodeWavefield1D::~AudioNodeWavefield1D()
{
	delete wavefield;
	wavefield = nullptr;
}

void AudioNodeWavefield1D::draw()
{
	const AudioFloat defaultTension(1.f);
	
	const AudioFloat * positionDampening = getInputAudioFloat(kInput_PositionDampening, &AudioFloat::Zero);
	const AudioFloat * velocityDampening = getInputAudioFloat(kInput_VelocityDampening, &AudioFloat::Zero);
	const AudioFloat * tension = getInputAudioFloat(kInput_Tension, &defaultTension);
	const bool wrap = getInputBool(kInput_Wrap, false);
	const int size = getInputInt(kInput_Size, 16);
	
	//
	
	if (size != wavefield->numElems)
	{
		wavefield->init(size);
	}
	
	//
	
	audioOutput.setVector();
	
	positionDampening->expand();
	velocityDampening->expand();
	tension->expand();
	
	//
	
	const double dt = 1.0 / double(SAMPLE_RATE);
	
	const double maxTension = 2000000000.0;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const double c = clamp<double>(tension->samples[i] * 1000000.0, -maxTension, +maxTension);
		
		wavefield->tick(dt, c, 1.0 - positionDampening->samples[i], 1.0 - velocityDampening->samples[i], wrap == false);
		
		audioOutput.samples[i] = wavefield->sample(0.5 * wavefield->numElems);
	}
}

void AudioNodeWavefield1D::handleTrigger(const int inputSocketIndex, const AudioTriggerData & data)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		if (wavefield->numElems > 0)
		{
			wavefield->d[rand() % wavefield->numElems] += random(-1.f, +1.f);
		}
	}
}

