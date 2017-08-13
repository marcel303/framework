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

#pragma once

#include "audioNodeBase.h"
#include "soundmix.h"

struct AudioNodeVoice4D : AudioNodeBase
{
	struct AudioSourceVoiceNode : AudioSource
	{
		AudioNodeVoice4D * voiceNode;
		
		virtual void generate(ALIGN16 float * __restrict samples, const int numSamples);
	};

	enum Input
	{
		kInput_Audio,
		kInput_Gain,
		kInput_Global,
		kInput_RampTime,
		//kInput_Color,
		//kInput_Name,
		kInput_PosX,
		kInput_PosY,
		kInput_PosZ,
		kInput_RotX,
		kInput_RotY,
		kInput_RotZ,
		//kInput_Dim,
		kInput_DimX,
		kInput_DimY,
		kInput_DimZ,
		kInput_OriginRotY,
		kInput_Articulation,
		kInput_Doppler,
		kInput_DopplerScale,
		kInput_DopplerSmooth,
		kInput_DistanceIntensity,
		kInput_DistanceIntensityTreshold,
		kInput_DistanceIntensityCurve,
		kInput_DistanceDampening,
		kInput_DistanceDampeningTreshold,
		kInput_DistanceDampeningCurve,
		kInput_DistanceDiffusion,
		kInput_DistanceDiffusionTreshold,
		kInput_DistanceDiffusionCurve,
		kInput_SpatialDelay,
		kInput_SpatialDelayMode,
		kInput_SpatialDelayFeedback,
		kInput_SpatialDelayWetness,
		//kInput_SpatialDelaySmooth,
		kInput_SpatialDelayScale,
		kInput_SpatialDelayNoiseDepth,
		kInput_SpatialDelayNoiseFreq,
		kInput_SubBoost,
		kInput_RampUp,
		kInput_RampDown,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_RampedUp,
		kOutput_RampedDown,
		kOutput_COUNT
	};
	
	AudioSourceVoiceNode source;
	AudioVoice * voice;
	
	AudioTriggerData dummyTriggerData;
	
	AudioNodeVoice4D();
	~AudioNodeVoice4D() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int inputSocketIndex, const AudioTriggerData & data) override;
};

//

struct AudioNodeVoice4DReturn : AudioNodeBase
{
	struct AudioSourceReturnNode : AudioSource
	{
		AudioNodeVoice4DReturn * returnNode;
		
		virtual void generate(ALIGN16 float * __restrict samples, const int numSamples);
	};

	enum Input
	{
		kInput_Audio,
		kInput_Index,
		kInput_RampTime,
		
		kInput_LeftEnabled,
		kInput_LeftDistance,
		kInput_LeftScatter,
		
		kInput_RightEnabled,
		kInput_RightDistance,
		kInput_RightScatter,
		
		kInput_TopEnabled,
		kInput_TopDistance,
		kInput_TopScatter,
		
		kInput_BottomEnabled,
		kInput_BottomDistance,
		kInput_BottomScatter,
		
		kInput_FrontEnabled,
		kInput_FrontDistance,
		kInput_FrontScatter,
		
		kInput_BackEnabled,
		kInput_BackDistance,
		kInput_BackScatter,
		
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};

	AudioSourceReturnNode source;
	AudioVoice * voice;

	AudioFloat audioOutput;
	
	AudioNodeVoice4DReturn();
	virtual ~AudioNodeVoice4DReturn() override;
	
	virtual void tick(const float dt) override;
};

//

struct AudioNodeVoice4DGlobals : AudioNodeBase
{
	enum Input
	{
		kInput_SyncOsc,
		kInput_Gain,
		kInput_PosX,
		kInput_PosY,
		kInput_PosZ,
		kInput_Dim,
		kInput_DimX,
		kInput_DimY,
		kInput_DimZ,
		kInput_RotX,
		kInput_RotY,
		kInput_RotZ,
		kInput_Plode,
		kInput_PlodeX,
		kInput_PlodeY,
		kInput_PlodeZ,
		kInput_OriginX,
		kInput_OriginY,
		kInput_OriginZ,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioNodeVoice4DGlobals()
		: AudioNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_SyncOsc, kAudioPlugType_Trigger);
		addInput(kInput_Gain, kAudioPlugType_FloatVec);
		addInput(kInput_PosX, kAudioPlugType_FloatVec);
		addInput(kInput_PosY, kAudioPlugType_FloatVec);
		addInput(kInput_PosZ, kAudioPlugType_FloatVec);
		addInput(kInput_Dim, kAudioPlugType_FloatVec);
		addInput(kInput_DimX, kAudioPlugType_FloatVec);
		addInput(kInput_DimY, kAudioPlugType_FloatVec);
		addInput(kInput_DimZ, kAudioPlugType_FloatVec);
		addInput(kInput_RotX, kAudioPlugType_FloatVec);
		addInput(kInput_RotY, kAudioPlugType_FloatVec);
		addInput(kInput_RotZ, kAudioPlugType_FloatVec);
		addInput(kInput_Plode, kAudioPlugType_FloatVec);
		addInput(kInput_PlodeX, kAudioPlugType_FloatVec);
		addInput(kInput_PlodeY, kAudioPlugType_FloatVec);
		addInput(kInput_PlodeZ, kAudioPlugType_FloatVec);
		addInput(kInput_OriginX, kAudioPlugType_FloatVec);
		addInput(kInput_OriginY, kAudioPlugType_FloatVec);
		addInput(kInput_OriginZ, kAudioPlugType_FloatVec);
	}
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int inputSocketIndex, const AudioTriggerData & data) override;
};
