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

// note : AudioNodeSourceMix is deprecated

struct AudioNodeSourceMix : AudioNodeBase
{
	enum Input
	{
		kInput_Source1,
		kInput_Gain1,
		kInput_Source2,
		kInput_Gain2,
		kInput_Source3,
		kInput_Gain3,
		kInput_Source4,
		kInput_Gain4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	AudioNodeSourceMix()
		: AudioNodeBase()
		, audioOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source1, kAudioPlugType_FloatVec);
		addInput(kInput_Gain1, kAudioPlugType_FloatVec);
		addInput(kInput_Source2, kAudioPlugType_FloatVec);
		addInput(kInput_Gain2, kAudioPlugType_FloatVec);
		addInput(kInput_Source3, kAudioPlugType_FloatVec);
		addInput(kInput_Gain3, kAudioPlugType_FloatVec);
		addInput(kInput_Source4, kAudioPlugType_FloatVec);
		addInput(kInput_Gain4, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
		
		isDeprecated = true;
	}
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			audioOutput.setScalar(0.f);
			
			return;
		}
		
		const AudioFloat * source1 = getInputAudioFloat(kInput_Source1, nullptr);
		const AudioFloat * source2 = getInputAudioFloat(kInput_Source2, nullptr);
		const AudioFloat * source3 = getInputAudioFloat(kInput_Source3, nullptr);
		const AudioFloat * source4 = getInputAudioFloat(kInput_Source4, nullptr);
		
		const AudioFloat * gain1 = getInputAudioFloat(kInput_Gain1, &AudioFloat::One);
		const AudioFloat * gain2 = getInputAudioFloat(kInput_Gain2, &AudioFloat::One);
		const AudioFloat * gain3 = getInputAudioFloat(kInput_Gain3, &AudioFloat::One);
		const AudioFloat * gain4 = getInputAudioFloat(kInput_Gain4, &AudioFloat::One);
		
		const bool normalizeGain = false;
		
		struct Input
		{
			const AudioFloat * source;
			const AudioFloat * gain;
			
			void set(const AudioFloat * _source, const AudioFloat * _gain)
			{
				source = _source;
				gain = _gain;
			}
		};
		
		Input inputs[4];
		int numInputs = 0;
		
		if (source1 != nullptr)
			inputs[numInputs++].set(source1, gain1);
		if (source2 != nullptr)
			inputs[numInputs++].set(source2, gain2);
		if (source3 != nullptr)
			inputs[numInputs++].set(source3, gain3);
		if (source4 != nullptr)
			inputs[numInputs++].set(source4, gain4);
		
		if (numInputs == 0)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				audioOutput.samples[i] = 0.f;
			return;
		}
		
		bool isFirst = true;
		
		for (int i = 0; i < numInputs; ++i)
		{
			auto & input = inputs[i];
			
			input.source->expand();
			
			if (isFirst)
			{
				isFirst = false;
				
				if (input.gain->isScalar && input.gain->getScalar() == 1.f)
					audioOutput.set(*input.source);
				else
					audioOutput.setMul(*input.source, *input.gain);
			}
			else
			{
				audioOutput.addMul(*input.source, *input.gain);
			}
		}
		
		if (normalizeGain)
		{
			float totalGain = 0.f;
			
			for (auto & input : inputs)
			{
				// todo : do this on a per-sample basis !
				
				totalGain += input.gain->getMean();
			}
			
			if (totalGain > 0.f)
			{
				float gainScale = 1.f;
				
				gainScale = 1.f / totalGain;
				
				audioOutput.mul(gainScale);
			}
		}
	}
};

AUDIO_NODE_TYPE(audioSourceMix, AudioNodeSourceMix)
{
	typeName = "audio.mix";
	
	in("source1", "audioValue");
	in("gain1", "audioValue", "1");
	in("source2", "audioValue");
	in("gain2", "audioValue", "1");
	in("source3", "audioValue");
	in("gain3", "audioValue", "1");
	in("source4", "audioValue");
	in("gain4", "audioValue", "1");
	out("audio", "audioValue");
}
