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

#include "audioNodeBase.h"

#include <cmath>

// note : AudioNodeMix is deprecated

struct AudioNodeMix : AudioNodeBase
{
	enum Mode
	{
		kMode_Add,
		kMode_Mul,
	};
	
	enum Input
	{
		kInput_Mode,
		kInput_AudioA,
		kInput_GainA,
		kInput_AudioB,
		kInput_GainB,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	AudioNodeMix()
		: AudioNodeBase()
		, audioOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Mode, kAudioPlugType_Int);
		addInput(kInput_AudioA, kAudioPlugType_FloatVec);
		addInput(kInput_GainA, kAudioPlugType_FloatVec);
		addInput(kInput_AudioB, kAudioPlugType_FloatVec);
		addInput(kInput_GainB, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
		
		isDeprecated = true;
	}
	
	virtual void tick(const float dt) override
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		const AudioFloat * audioA = getInputAudioFloat(kInput_AudioA, nullptr);
		const AudioFloat * gainA = getInputAudioFloat(kInput_GainA, &AudioFloat::One);
		const AudioFloat * audioB = getInputAudioFloat(kInput_AudioB, nullptr);
		const AudioFloat * gainB = getInputAudioFloat(kInput_GainB, &AudioFloat::One);
		
		if (audioA == nullptr || audioB == nullptr)
		{
			if (audioA != nullptr)
			{
				audioOutput.setMul(*audioA, *gainA);
			}
			else if (audioB != nullptr)
			{
				audioOutput.setMul(*audioB, *gainB);
			}
			else
			{
				audioOutput.setZero();
			}
		}
		else
		{
			if (mode == kMode_Add)
			{
				audioOutput.setMul(*audioA, *gainA);
				audioOutput.addMul(*audioB, *gainB);
			}
			else if (mode == kMode_Mul)
			{
				audioOutput.setMul(*audioA, *gainA);
				audioOutput.mulMul(*audioB, *gainB);
			}
			else
			{
				Assert(false);
			}
		}
	}
};

AUDIO_ENUM_TYPE(audioMixMode)
{
	elem("add");
	elem("mul");
}

AUDIO_NODE_TYPE(AudioNodeMix)
{
	typeName = "mix";
	
	inEnum("mode", "audioMixMode");
	in("sourceA", "audioValue");
	in("gainA", "audioValue", "1");
	in("sourceB", "audioValue");
	in("gainB", "audioValue", "1");
	out("audio", "audioValue");
}
