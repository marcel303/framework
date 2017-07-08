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

#include "audioNodeDisplay.h"

AUDIO_NODE_TYPE(display, AudioNodeDisplay)
{
	typeName = "display";
	
	in("audioL", "audioValue");
	in("audioR", "audioValue");
	in("gain", "float", "1");
}

AudioNodeDisplay::AudioNodeDisplay()
	: AudioNodeBase()
	, outputChannelL(nullptr)
	, outputChannelR(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_AudioL, kAudioPlugType_FloatVec);
	addInput(kInput_AudioR, kAudioPlugType_FloatVec);
	addInput(kInput_Gain, kAudioPlugType_Float);
}

void AudioNodeDisplay::draw()
{
	const AudioFloat * audioL = getInputAudioFloat(kInput_AudioL, nullptr);
	const AudioFloat * audioR = getInputAudioFloat(kInput_AudioR, nullptr);
	const float gain = getInputFloat(kInput_Gain, 1.f);

	if (outputChannelL != nullptr)
	{
		if (audioL == nullptr)
		{
			float * channelPtr = outputChannelL->samples;
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelL->stride)
				*channelPtr = 0.f;
		}
		else
		{
			audioL->expand();
			
			float * channelPtr = outputChannelL->samples;
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelL->stride)
				*channelPtr = audioL->samples[i] * gain;
		}
	}
	
	if (outputChannelR != nullptr)
	{
		if (audioR == nullptr)
		{
			float * channelPtr = outputChannelR->samples;
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelR->stride)
				*channelPtr = 0.f;
		}
		else
		{
			audioR->expand();
			
			float * channelPtr = outputChannelR->samples;
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelR->stride)
				*channelPtr = audioR->samples[i] * gain;
		}
	}
}
