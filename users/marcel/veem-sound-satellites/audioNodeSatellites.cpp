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
#include "audioNodeSatellites.h"
#include <string.h>

#define voiceMgr g_currentAudioGraph->globals->voiceMgr

AUDIO_NODE_TYPE(satellites, AudioNodeSatellites)
{
	typeName = "satellites";
	
	in("channel1", "int", "-1");
	in("channel2", "int", "-1");
	in("channel3", "int", "-1");
	in("audio1", "audioValue");
	in("audio2", "audioValue");
	in("audio3", "audioValue");
	in("gain1", "audioValue", "1");
	in("gain2", "audioValue", "1");
	in("gain3", "audioValue", "1");
}

void AudioNodeSatellites::AudioSourceSatellitesNode::generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(numSamples == AUDIO_UPDATE_SIZE);
	
	if (satellitesNode->isPassthrough)
	{
		memset(samples, 0, numSamples * sizeof(float));
	}
	else
	{
		Assert(g_currentAudioGraph == nullptr);
		g_currentAudioGraph = satellitesNode->audioGraph;
		{
			const AudioFloat * audio = satellitesNode->getInputAudioFloat(kInput_Audio1 + index, &AudioFloat::Zero);
			
			audio->expand();
			
			memcpy(samples, audio->samples, numSamples * sizeof(float));
		}
		g_currentAudioGraph = nullptr;
	}
}

//

AudioNodeSatellites::AudioNodeSatellites()
	: AudioNodeBase()
	, sources()
	, voices()
	, audioGraph(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channel1, kAudioPlugType_Int);
	addInput(kInput_Channel2, kAudioPlugType_Int);
	addInput(kInput_Channel3, kAudioPlugType_Int);
	addInput(kInput_Audio1, kAudioPlugType_FloatVec);
	addInput(kInput_Audio2, kAudioPlugType_FloatVec);
	addInput(kInput_Audio3, kAudioPlugType_FloatVec);
	addInput(kInput_Gain1, kAudioPlugType_FloatVec);
	addInput(kInput_Gain2, kAudioPlugType_FloatVec);
	addInput(kInput_Gain3, kAudioPlugType_FloatVec);
	
	//
	
	for (int i = 0; i < kNumChannels; ++i)
	{
		sources[i].satellitesNode = this;
		sources[i].index = i;
	}
	
	//
	
	Assert(g_currentAudioGraph != nullptr);
	audioGraph = g_currentAudioGraph;
}

AudioNodeSatellites::~AudioNodeSatellites()
{
	for (int i = 0; i < kNumChannels; ++i)
	{
		auto & voice = voices[i];
		
		if (voice != nullptr)
		{
			voiceMgr->freeVoice(voice);
		}
	}
}

void AudioNodeSatellites::tick(const float dt)
{
	if (isPassthrough)
	{
		for (int i = 0; i < kNumChannels; ++i)
		{
			auto & voice = voices[i];
			
			if (voice != nullptr)
			{
				voiceMgr->freeVoice(voice);
			}
		}
		
		return;
	}

	const int channels[kNumChannels] =
	{
		getInputInt(kInput_Channel1, -1),
		getInputInt(kInput_Channel2, -1),
		getInputInt(kInput_Channel3, -1)
	};

	for (int i = 0; i < kNumChannels; ++i)
	{
		auto & voice = voices[i];
		auto & source = sources[i];
		
		const int channel = channels[i];
		
		if (channel < 0)
		{
			if (voice != nullptr)
			{
				voiceMgr->freeVoice(voice);
			}
		}
		else if (voice == nullptr || voice->channelIndex != channel)
		{
			if (voice != nullptr)
				voiceMgr->freeVoice(voice);
			
			voiceMgr->allocVoice(voice, &source, "voice", true, 0.f, 1.f, channel);
		}
		
		if (voice != nullptr)
		{
			voice->gain = getInputAudioFloat(kInput_Gain1 + source.index, &AudioFloat::One)->getMean();
		}
	}
}
