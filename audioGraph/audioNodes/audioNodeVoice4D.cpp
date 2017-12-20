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

#include "audioGraph.h"
#include "audioNodeVoice4D.h"
#include "Calc.h"
#include "Mat4x4.h"
#include "StringBuilder.h"

static const int kReturnBase = 41;
static const int kMaxReturns = 4;

AUDIO_ENUM_TYPE(spatialDelayMode)
{
	enumName = "4d.spatialDelayMode";
	
	elem("random");
	elem("grid");
}

AUDIO_ENUM_TYPE(subboost)
{
	enumName = "4d.subBoost";
	
	elem("off");
	elem("1");
	elem("2");
	elem("3");
}

AUDIO_NODE_TYPE(voice_4d, AudioNodeVoice4D)
{
	typeName = "voice.4d";
	
	in("audio", "audioValue");
	in("gain", "audioValue", "1");
	in("global", "bool", "1");
	in("rampTime", "audioValue", "1");
	//in("color", "string", "ff0000");
	//in("name", "string");
	in("pos.x", "audioValue");
	in("pos.y", "audioValue");
	in("pos.z", "audioValue");
	in("rot.x", "audioValue");
	in("rot.y", "audioValue");
	in("rot.z", "audioValue");
	//in("dim", "audioValue", "1");
	in("dim.x", "audioValue", "1");
	in("dim.y", "audioValue", "1");
	in("dim.z", "audioValue", "1");
	in("originRot.y", "audioValue");
	in("articulation", "audioValue");
	in("dopp", "bool", "1");
	in("dopp.scale", "audioValue", "1");
	in("dopp.smooth", "audioValue", "0.2");
	in("dint", "bool", "1");
	in("dint.tresh", "audioValue", "100");
	in("dint.curve", "audioValue", "-0.4");
	in("ddamp", "bool", "1");
	in("ddamp.tresh", "audioValue", "100");
	in("ddamp.curve", "audioValue", "-0.4");
	in("ddiff", "bool");
	in("ddiff.tresh", "audioValue", "50");
	in("ddiff.curve", "audioValue", "0.2");
	in("spatd", "bool");
	inEnum("spatd.mode", "4d.spatialDelayMode", "grid");
	in("spatd.feedback", "audioValue", "0.5");
	in("spatd.wetness", "audioValue", "0.5");
	in("spatd.scale", "audioValue", "1");
	in("spatd.noise.depth", "audioValue", "0");
	in("spatd.noise.freq", "audioValue", "1");
	inEnum("sub.boost", "4d.subBoost");
	in("rampUp!", "trigger");
	in("rampDown!", "trigger");
	out("rampedUp!", "trigger");
	out("rampedDown!", "trigger");
}

void AudioNodeVoice4D::AudioSourceVoiceNode::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(numSamples == AUDIO_UPDATE_SIZE);
	
	if (voiceNode->isPassthrough)
	{
		memset(samples, 0, numSamples * sizeof(float));
	}
	else
	{
		Assert(g_currentAudioGraph == nullptr);
		g_currentAudioGraph = voiceNode->audioGraph;
		{
			const AudioFloat * audio = voiceNode->getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
			
			audio->expand();
			
			memcpy(samples, audio->samples, numSamples * sizeof(float));
		}
		g_currentAudioGraph = nullptr;
	}
}

AudioNodeVoice4D::AudioNodeVoice4D()
	: AudioNodeBase()
	, source()
	, voice(nullptr)
	, audioGraph(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Audio, kAudioPlugType_FloatVec);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_RampTime, kAudioPlugType_FloatVec);
	addInput(kInput_Global, kAudioPlugType_Bool);
	//addInput(kInput_Color, kAudioPlugType_String);
	//addInput(kInput_Name, kAudioPlugType_String);
	addInput(kInput_PosX, kAudioPlugType_FloatVec);
	addInput(kInput_PosY, kAudioPlugType_FloatVec);
	addInput(kInput_PosZ, kAudioPlugType_FloatVec);
	addInput(kInput_RotX, kAudioPlugType_FloatVec);
	addInput(kInput_RotY, kAudioPlugType_FloatVec);
	addInput(kInput_RotZ, kAudioPlugType_FloatVec);
	//addInput(kInput_Dim, kAudioPlugType_FloatVec);
	addInput(kInput_DimX, kAudioPlugType_FloatVec);
	addInput(kInput_DimY, kAudioPlugType_FloatVec);
	addInput(kInput_DimZ, kAudioPlugType_FloatVec);
	addInput(kInput_OriginRotY, kAudioPlugType_FloatVec);
	addInput(kInput_Articulation, kAudioPlugType_FloatVec);
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
	addInput(kInput_SpatialDelay, kAudioPlugType_Bool);
	addInput(kInput_SpatialDelayMode, kAudioPlugType_Int);
	addInput(kInput_SpatialDelayFeedback, kAudioPlugType_FloatVec);
	addInput(kInput_SpatialDelayWetness, kAudioPlugType_FloatVec);
	//addInput(kInput_SpatialDelaySmooth, kAudioPlugType_FloatVec);
	addInput(kInput_SpatialDelayScale, kAudioPlugType_FloatVec);
	addInput(kInput_SpatialDelayNoiseDepth, kAudioPlugType_FloatVec);
	addInput(kInput_SpatialDelayNoiseFreq, kAudioPlugType_FloatVec);
	addInput(kInput_SubBoost, kAudioPlugType_Int);
	addInput(kInput_RampUp, kAudioPlugType_Trigger);
	addInput(kInput_RampDown, kAudioPlugType_Trigger);
	addOutput(kOutput_RampedUp, kAudioPlugType_Trigger, nullptr);
	addOutput(kOutput_RampedDown, kAudioPlugType_Trigger, nullptr);
	
	source.voiceNode = this;
	
	Assert(g_currentAudioGraph != nullptr);
	audioGraph = g_currentAudioGraph;
}

AudioNodeVoice4D::~AudioNodeVoice4D()
{
	if (voice != nullptr)
	{
		g_voiceMgr->freeVoice(voice);
	}
}

void AudioNodeVoice4D::tick(const float dt)
{
	audioCpuTimingBlock(AudioNodeVoice4D);
	
	if (isPassthrough)
	{
		if (voice != nullptr)
		{
			g_voiceMgr->freeVoice(voice);
		}
		
		return;
	}
	else if (voice == nullptr)
	{
		const float rampTime = getInputAudioFloat(kInput_RampTime, &AudioFloat::One)->getMean();
		
		g_voiceMgr->allocVoice(voice, &source, "voice.4d", true, .3f, rampTime, -1);
		voice->isSpatial = true;
	}
	
	voice->spat.globalEnable = getInputBool(kInput_Global, true);
	
	const float originRotY = getInputAudioFloat(kInput_OriginRotY, &AudioFloat::Zero)->getMean();
	
	Mat4x4 originMatrix;
	originMatrix.MakeRotationY(Calc::DegToRad(originRotY));
	
	StringBuilder<64> name;
	name.Append("channel(");
	name.Append(voice->channelIndex + 1);
	name.Append(")");
	
	//voice->spat.color = getInputString(kInput_Color, "ff0000");
	if (voice->spat.name != name.ToString())
		voice->spat.name = name.ToString();
	voice->spat.gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One)->getMean();
	
	// position
	voice->spat.pos[0] = getInputAudioFloat(kInput_PosX, &AudioFloat::Zero)->getMean();
	voice->spat.pos[1] = getInputAudioFloat(kInput_PosY, &AudioFloat::Zero)->getMean();
	voice->spat.pos[2] = getInputAudioFloat(kInput_PosZ, &AudioFloat::Zero)->getMean();
	voice->spat.pos = originMatrix.Mul4(voice->spat.pos);
	
	// rotation
	voice->spat.rot[0] = getInputAudioFloat(kInput_RotX, &AudioFloat::Zero)->getMean();
	voice->spat.rot[1] = getInputAudioFloat(kInput_RotY, &AudioFloat::Zero)->getMean();
	voice->spat.rot[2] = getInputAudioFloat(kInput_RotZ, &AudioFloat::Zero)->getMean();
	voice->spat.rot = originMatrix.Mul3(voice->spat.rot);
	
	// dimensions
	//const float size = getInputAudioFloat(kInput_Dim, &AudioFloat::One)->getMean();
	const float size = 1.f;
	voice->spat.size[0] = size * getInputAudioFloat(kInput_DimX, &AudioFloat::One)->getMean();
	voice->spat.size[1] = size * getInputAudioFloat(kInput_DimY, &AudioFloat::One)->getMean();
	voice->spat.size[2] = size * getInputAudioFloat(kInput_DimZ, &AudioFloat::One)->getMean();
	
	voice->spat.articulation = getInputAudioFloat(kInput_Articulation, &AudioFloat::Zero)->getMean();
	
	// doppler
	{
		const AudioFloat scale(1.f);
		const AudioFloat smooth(.2f);
		
		voice->spat.doppler.enable = getInputBool(kInput_Doppler, true);
		voice->spat.doppler.scale = getInputAudioFloat(kInput_DopplerScale, &scale)->getMean();
		voice->spat.doppler.smooth = getInputAudioFloat(kInput_DopplerSmooth, &smooth)->getMean();
	}
	
	// distance intensity
	{
		const AudioFloat treshold(100.f);
		const AudioFloat curve(-.4f);
		
		voice->spat.distanceIntensity.enable = getInputBool(kInput_DistanceIntensity, true);
		voice->spat.distanceIntensity.threshold = getInputAudioFloat(kInput_DistanceIntensityTreshold, &treshold)->getMean();
		voice->spat.distanceIntensity.curve = getInputAudioFloat(kInput_DistanceIntensityCurve, &curve)->getMean();
	}
	
	// distance dampening
	{
		const AudioFloat treshold(100.f);
		const AudioFloat curve(-.4f);
		
		voice->spat.distanceDampening.enable = getInputBool(kInput_DistanceDampening, true);
		voice->spat.distanceDampening.threshold = getInputAudioFloat(kInput_DistanceDampeningTreshold, &treshold)->getMean();
		voice->spat.distanceDampening.curve = getInputAudioFloat(kInput_DistanceDampeningCurve, &curve)->getMean();
	}
	
	// distance diffusion
	{
		const AudioFloat treshold(50.f);
		const AudioFloat curve(.2f);
		
		voice->spat.distanceDiffusion.enable = getInputBool(kInput_DistanceDiffusion, false);
		voice->spat.distanceDiffusion.threshold = getInputAudioFloat(kInput_DistanceDiffusionTreshold, &treshold)->getMean();
		voice->spat.distanceDiffusion.curve = getInputAudioFloat(kInput_DistanceDiffusionCurve, &curve)->getMean();
	}
	
	// spatial delay
	
	{
		voice->spat.spatialDelay.enable = getInputBool(kInput_SpatialDelay, false);
		voice->spat.spatialDelay.mode = (Osc4D::SpatialDelayMode)getInputInt(kInput_SpatialDelayMode, Osc4D::kSpatialDelay_Grid);
		voice->spat.spatialDelay.feedback = getInputAudioFloat(kInput_SpatialDelayFeedback, &AudioFloat::Half)->getMean();
		voice->spat.spatialDelay.wetness = getInputAudioFloat(kInput_SpatialDelayWetness, &AudioFloat::Half)->getMean();
		//voice->spat.spatialDelay.smooth = getInputAudioFloat(kInput_SpatialDelaySmooth, &AudioFloat::Zero)->getMean();
		voice->spat.spatialDelay.smooth = 0.f;
		voice->spat.spatialDelay.scale = getInputAudioFloat(kInput_SpatialDelayScale, &AudioFloat::One)->getMean();
		voice->spat.spatialDelay.noiseDepth = getInputAudioFloat(kInput_SpatialDelayNoiseDepth, &AudioFloat::Zero)->getMean();
		voice->spat.spatialDelay.noiseFrequency = getInputAudioFloat(kInput_SpatialDelayNoiseFreq, &AudioFloat::One)->getMean();
	}
	
	// sub boost
	
	voice->spat.subBoost = (Osc4D::SubBoost)getInputInt(kInput_SubBoost, 0);
	
	//
	
	if (g_currentAudioGraph->isFLagSet("voice.4d.rampUp"))
	{
		voice->ramp = true;
	}
	else if (g_currentAudioGraph->isFLagSet("voice.4d.rampDown"))
	{
		voice->ramp = false;
	}
	
	if (voice->hasRamped)
	{
		if (voice->ramp)
		{
			trigger(kOutput_RampedUp);
			
			g_currentAudioGraph->setFlag("voice.4d.rampedUp");
		}
		else
		{
			trigger(kOutput_RampedDown);
			
			g_currentAudioGraph->setFlag("voice.4d.rampedDown");
		}
	}
}

void AudioNodeVoice4D::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_RampUp)
	{
		voice->ramp = true;
	}
	else if (inputSocketIndex == kInput_RampDown)
	{
		voice->ramp = false;
	}
}

//

AUDIO_NODE_TYPE(return_4d, AudioNodeVoice4DReturn)
{
	typeName = "return.4d";
	
	in("audio", "audioValue");
	in("index", "int", "-1");
	in("rampTime", "audioValue");
	
	in("left", "bool");
	in("left.distance", "audioValue", "100");
	in("left.scatter", "audioValue", "0");
	
	in("right", "bool");
	in("right.distance", "audioValue", "100");
	in("right.scatter", "audioValue", "0");
	
	in("top", "bool");
	in("top.distance", "audioValue", "100");
	in("top.scatter", "audioValue", "0");
	
	in("bottom", "bool");
	in("bottom.distance", "audioValue", "100");
	in("bottom.scatter", "audioValue", "0");
	
	in("front", "bool");
	in("front.distance", "audioValue", "100");
	in("front.scatter", "audioValue", "0");
	
	in("back", "bool");
	in("back.distance", "audioValue", "100");
	in("back.scatter", "audioValue", "0");
}

void AudioNodeVoice4DReturn::AudioSourceReturnNode::generate(ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(numSamples == AUDIO_UPDATE_SIZE);
	
	if (returnNode->isPassthrough)
	{
		memset(samples, 0, numSamples * sizeof(float));
	}
	else
	{
		Assert(g_currentAudioGraph == nullptr);
		g_currentAudioGraph = returnNode->audioGraph;
		{
			const AudioFloat * audio = returnNode->getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
			
			audio->expand();
			
			memcpy(samples, audio->samples, numSamples * sizeof(float));
		}
		g_currentAudioGraph = nullptr;
	}
}

AudioNodeVoice4DReturn::AudioNodeVoice4DReturn()
	: AudioNodeBase()
	, source()
	, voice(nullptr)
	, audioGraph(nullptr)
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	
	addInput(kInput_Audio, kAudioPlugType_FloatVec);
	addInput(kInput_Index, kAudioPlugType_Int);
	addInput(kInput_RampTime, kAudioPlugType_FloatVec);
	
	addInput(kInput_LeftEnabled, kAudioPlugType_Bool);
	addInput(kInput_LeftDistance, kAudioPlugType_FloatVec);
	addInput(kInput_LeftScatter, kAudioPlugType_FloatVec);
	
	addInput(kInput_RightEnabled, kAudioPlugType_Bool);
	addInput(kInput_RightDistance, kAudioPlugType_FloatVec);
	addInput(kInput_RightScatter, kAudioPlugType_FloatVec);
	
	addInput(kInput_TopEnabled, kAudioPlugType_Bool);
	addInput(kInput_TopDistance, kAudioPlugType_FloatVec);
	addInput(kInput_TopScatter, kAudioPlugType_FloatVec);
	
	addInput(kInput_BottomEnabled, kAudioPlugType_Bool);
	addInput(kInput_BottomDistance, kAudioPlugType_FloatVec);
	addInput(kInput_BottomScatter, kAudioPlugType_FloatVec);
	
	addInput(kInput_FrontEnabled, kAudioPlugType_Bool);
	addInput(kInput_FrontDistance, kAudioPlugType_FloatVec);
	addInput(kInput_FrontScatter, kAudioPlugType_FloatVec);
	
	addInput(kInput_BackEnabled, kAudioPlugType_Bool);
	addInput(kInput_BackDistance, kAudioPlugType_FloatVec);
	addInput(kInput_BackScatter, kAudioPlugType_FloatVec);
	
	Assert(g_currentAudioGraph != nullptr);
	audioGraph = g_currentAudioGraph;
}

AudioNodeVoice4DReturn::~AudioNodeVoice4DReturn()
{
	if (voice != nullptr)
	{
		g_voiceMgr->freeVoice(voice);
	}
}

void AudioNodeVoice4DReturn::tick(const float dt)
{
	const int index = getInputInt(kInput_Index, 0);
	
	if (index >= 0 && index < kMaxReturns)
	{
		const int absoluteIndex = kReturnBase + index;
		
		if (voice == nullptr || absoluteIndex != voice->channelIndex)
		{
			if (voice != nullptr)
			{
				g_voiceMgr->freeVoice(voice);
			}
			
			//
			
			const float rampTime = getInputAudioFloat(kInput_RampTime, &AudioFloat::One)->getMean();
			
			source.returnNode = this;
			if (g_voiceMgr->allocVoice(voice, &source, "return.4d", true, .2f, rampTime, absoluteIndex))
			{
				voice->isReturn = true;
				voice->speaker = AudioVoice::kSpeaker_LeftAndRight;
			}
		}
	}
	
	//
	
	if (voice != nullptr)
	{
		voice->returnInfo.returnIndex = getInputInt(kInput_Index, -1);
		
		const int inputOffset[Osc4D::kReturnSide_COUNT] =
		{
			kInput_LeftEnabled,
			kInput_RightEnabled,
			kInput_TopEnabled,
			kInput_BottomEnabled,
			kInput_FrontEnabled,
			kInput_BackEnabled
		};
		
		const AudioFloat distanceDefault(100.f);
		
		for (int i = 0; i < Osc4D::kReturnSide_COUNT; ++i)
		{
			voice->returnInfo.sides[i].enabled = getInputBool(inputOffset[i] + 0, false);
			voice->returnInfo.sides[i].distance = getInputAudioFloat(inputOffset[i] + 1, &distanceDefault)->getMean();
			voice->returnInfo.sides[i].scatter = getInputAudioFloat(inputOffset[i] + 2, &AudioFloat::Zero)->getMean();
		}
	}
}

//

#include "osc4d.h"

AUDIO_NODE_TYPE(globals_4d, AudioNodeVoice4DGlobals)
{
	typeName = "globals.4d";
	
	in("gain", "audioValue", "1");
	in("pos.x", "audioValue");
	in("pos.y", "audioValue");
	in("pos.z", "audioValue");
	in("dim", "audioValue", "1");
	in("dim.x", "audioValue", "1");
	in("dim.y", "audioValue", "1");
	in("dim.z", "audioValue", "1");
	in("rot.x", "audioValue");
	in("rot.y", "audioValue");
	in("rot.z", "audioValue");
	in("plode", "audioValue", "1");
	in("plode.x", "audioValue", "1");
	in("plode.y", "audioValue", "1");
	in("plode.z", "audioValue", "1");
	in("origin.x", "audioValue");
	in("origin.y", "audioValue");
	in("origin.z", "audioValue");
}

void AudioNodeVoice4DGlobals::tick(const float dt)
{
	if (isPassthrough)
	{
		return;
	}
	
	const float size = getInputAudioFloat(kInput_Dim, &AudioFloat::One)->getMean();
	const float plode = getInputAudioFloat(kInput_Plode, &AudioFloat::One)->getMean();
	
	g_voiceMgr->spat.globalGain = getInputAudioFloat(kInput_Gain, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalPos[0] = getInputAudioFloat(kInput_PosX, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalPos[1] = getInputAudioFloat(kInput_PosY, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalPos[2] = getInputAudioFloat(kInput_PosZ, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalSize[0] = size * getInputAudioFloat(kInput_DimX, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalSize[1] = size * getInputAudioFloat(kInput_DimY, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalSize[2] = size * getInputAudioFloat(kInput_DimZ, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalRot[0] = getInputAudioFloat(kInput_RotX, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalRot[1] = getInputAudioFloat(kInput_RotY, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalRot[2] = getInputAudioFloat(kInput_RotZ, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalPlode[0] = plode * getInputAudioFloat(kInput_PlodeX, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalPlode[1] = plode * getInputAudioFloat(kInput_PlodeY, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalPlode[2] = plode * getInputAudioFloat(kInput_PlodeZ, &AudioFloat::One)->getMean();
	g_voiceMgr->spat.globalOrigin[0] = getInputAudioFloat(kInput_OriginX, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalOrigin[1] = getInputAudioFloat(kInput_OriginY, &AudioFloat::Zero)->getMean();
	g_voiceMgr->spat.globalOrigin[2] = getInputAudioFloat(kInput_OriginZ, &AudioFloat::Zero)->getMean();
}
