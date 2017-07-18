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

struct AudioNodeVoice : AudioNodeBase
{
	struct AudioSourceVoice : AudioSource
	{
		AudioNodeVoice * voiceNode;
		
		virtual void generate(ALIGN16 float * __restrict samples, const int numSamples);
	};

	enum Input
	{
		kInput_Audio,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioSourceVoice source;
	AudioVoice * voice;
	
	AudioNodeVoice();
	~AudioNodeVoice() override;
};

//

struct AudioNodeVoice4D : AudioNodeBase
{
	struct AudioSourceVoice : AudioSource
	{
		AudioNodeVoice4D * voiceNode;
		
		virtual void generate(ALIGN16 float * __restrict samples, const int numSamples);
	};

	enum Input
	{
		kInput_Audio,
		kInput_Global,
		kInput_PosX,
		kInput_PosY,
		kInput_PosZ,
		kInput_RotX,
		kInput_RotY,
		kInput_RotZ,
		kInput_DimX,
		kInput_DimY,
		kInput_DimZ,
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
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioSourceVoice source;
	AudioVoice * voice;
	
	AudioNodeVoice4D();
	~AudioNodeVoice4D() override;

	virtual void tick(const float dt) override;
};
