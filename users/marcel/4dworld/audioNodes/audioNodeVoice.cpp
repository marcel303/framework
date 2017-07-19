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

#include "audioNodeVoice.h"

AUDIO_NODE_TYPE(voice, AudioNodeVoice)
{
	typeName = "voice";
	
	in("audio", "audioValue");
}

void AudioNodeVoice::AudioSourceVoice::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(numSamples == AUDIO_UPDATE_SIZE);
	
	const AudioFloat * audio = voiceNode->getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
	
	audio->expand();
	
	memcpy(samples, audio->samples, numSamples * sizeof(float));
}

//

AudioNodeVoice::AudioNodeVoice()
	: AudioNodeBase()
	, source()
	, voice(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Audio, kAudioPlugType_FloatVec);
	
	//
	
	source.voiceNode = this;
	g_voiceMgr->allocVoice(voice, &source);
}

AudioNodeVoice::~AudioNodeVoice()
{
	g_voiceMgr->freeVoice(voice);
}

//

AUDIO_NODE_TYPE(voice_4d, AudioNodeVoice4D)
{
	typeName = "voice.4d";
	
	in("audio", "audioValue");
	in("global", "bool", "1");
	in("pos.x", "audioValue");
	in("pos.y", "audioValue");
	in("pos.z", "audioValue");
	in("rot.x", "audioValue");
	in("rot.y", "audioValue");
	in("rot.z", "audioValue");
	in("dim.x", "audioValue", "1");
	in("dim.y", "audioValue", "1");
	in("dim.z", "audioValue", "1");
	in("dopp", "bool", "1");
	in("dopp.scale", "audioValue", "1");
	in("dopp.smooth", "audioValue", "0.2");
	in("dint", "bool", "1");
	in("dint.tresh", "treshold", "100");
	in("dint.curve", "curve", "-0.4");
	in("ddamp", "bool", "1");
	in("ddamp.tresh", "audioValue", "100");
	in("ddamp.curve", "audioValue", "-0.4");
	in("ddiff", "bool");
	in("ddiff.tresh", "audioValue", "50");
	in("ddiff.curve", "audioValue", "0.2");
}

void AudioNodeVoice4D::AudioSourceVoice::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(numSamples == AUDIO_UPDATE_SIZE);
	
	const AudioFloat * audio = voiceNode->getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
	
	audio->expand();
	
	memcpy(samples, audio->samples, numSamples * sizeof(float));
}

//

AudioNodeVoice4D::AudioNodeVoice4D()
	: AudioNodeBase()
	, source()
	, voice(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Audio, kAudioPlugType_FloatVec);
	addInput(kInput_Global, kAudioPlugType_Bool);
	addInput(kInput_PosX, kAudioPlugType_FloatVec);
	addInput(kInput_PosY, kAudioPlugType_FloatVec);
	addInput(kInput_PosZ, kAudioPlugType_FloatVec);
	addInput(kInput_RotX, kAudioPlugType_FloatVec);
	addInput(kInput_RotY, kAudioPlugType_FloatVec);
	addInput(kInput_RotZ, kAudioPlugType_FloatVec);
	addInput(kInput_DimX, kAudioPlugType_FloatVec);
	addInput(kInput_DimY, kAudioPlugType_FloatVec);
	addInput(kInput_DimZ, kAudioPlugType_FloatVec);
	addInput(kInput_Doppler, kAudioPlugType_Bool);
	addInput(kInput_DopplerScale, kAudioPlugType_FloatVec);
	addInput(kInput_DopplerSmooth, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceIntensity, kAudioPlugType_Bool);
	addInput(kInput_DistanceIntensityTreshold, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceIntensityCurve, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceDampening, kAudioPlugType_Bool);
	addInput(kInput_DistanceDampeningTreshold, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceDampeningCurve, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceDiffusion, kAudioPlugType_Bool);
	addInput(kInput_DistanceDiffusionTreshold, kAudioPlugType_FloatVec);
	addInput(kInput_DistanceDiffusionCurve, kAudioPlugType_FloatVec);
	
	//
	
	source.voiceNode = this;
	g_voiceMgr->allocVoice(voice, &source);
}

AudioNodeVoice4D::~AudioNodeVoice4D()
{
	g_voiceMgr->freeVoice(voice);
}

void AudioNodeVoice4D::tick(const float dt)
{
	voice->globalEnable = getInputBool(kInput_Global, true);
	
	// position
	voice->pos[0] = getInputAudioFloat(kInput_PosX, &AudioFloat::Zero)->getMean();
	voice->pos[1] = getInputAudioFloat(kInput_PosY, &AudioFloat::Zero)->getMean();
	voice->pos[2] = getInputAudioFloat(kInput_PosZ, &AudioFloat::Zero)->getMean();
	
	// rotation
	voice->rot[0] = getInputAudioFloat(kInput_RotX, &AudioFloat::Zero)->getMean();
	voice->rot[1] = getInputAudioFloat(kInput_RotY, &AudioFloat::Zero)->getMean();
	voice->rot[2] = getInputAudioFloat(kInput_RotZ, &AudioFloat::Zero)->getMean();
	
	// dimensions
	voice->size[0] = getInputAudioFloat(kInput_DimX, &AudioFloat::One)->getMean();
	voice->size[1] = getInputAudioFloat(kInput_DimY, &AudioFloat::One)->getMean();
	voice->size[2] = getInputAudioFloat(kInput_DimZ, &AudioFloat::One)->getMean();
	
	// doppler
	{
		const AudioFloat scale(1.f);
		const AudioFloat smooth(.2f);
		
		voice->dopplerEnable = getInputBool(kInput_Doppler, true);
		voice->dopplerScale = getInputAudioFloat(kInput_DopplerScale, &scale)->getMean();
		voice->dopplerSmooth = getInputAudioFloat(kInput_DopplerSmooth, &smooth)->getMean();
	}
	
	// distance intensity
	{
		const AudioFloat treshold(100.f);
		const AudioFloat curve(-.4f);
		
		voice->distanceIntensity.enable = getInputBool(kInput_DistanceIntensity, true);
		voice->distanceIntensity.threshold = getInputAudioFloat(kInput_DistanceIntensityTreshold, &treshold)->getMean();
		voice->distanceIntensity.curve = getInputAudioFloat(kInput_DistanceIntensityCurve, &curve)->getMean();
	}
	
	// distance dampening
	{
		const AudioFloat treshold(100.f);
		const AudioFloat curve(-.4f);
		
		voice->distanceDampening.enable = getInputBool(kInput_DistanceDampening, true);
		voice->distanceDampening.threshold = getInputAudioFloat(kInput_DistanceDampeningTreshold, &treshold)->getMean();
		voice->distanceDampening.curve = getInputAudioFloat(kInput_DistanceDampeningCurve, &curve)->getMean();
	}
	
	// distance diffusion
	{
		const AudioFloat treshold(50.f);
		const AudioFloat curve(.2f);
		
		voice->distanceDiffusion.enable = getInputBool(kInput_DistanceDiffusion, false);
		voice->distanceDiffusion.threshold = getInputAudioFloat(kInput_DistanceDiffusionTreshold, &treshold)->getMean();
		voice->distanceDiffusion.curve = getInputAudioFloat(kInput_DistanceDiffusionCurve, &curve)->getMean();
	}
}
