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
#include "audioNodeVoice.h"
#include <string.h>

#define voiceMgr g_currentAudioGraph->globals->voiceMgr

AUDIO_ENUM_TYPE(voiceSpeaker)
{
	elem("left+right");
	elem("left");
	elem("right");
	elem("channel");
}

AUDIO_NODE_TYPE(voice, AudioNodeVoice)
{
	typeName = "voice";
	
	in("audio", "audioValue");
	in("gain", "audioValue", "1");
	inEnum("speaker", "voiceSpeaker");
	in("channel", "int");
}

void AudioNodeVoice::AudioSourceVoiceNode::generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples)
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

//

AudioNodeVoice::AudioNodeVoice()
	: AudioNodeBase()
	, source()
	, voice(nullptr)
	, audioGraph(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Audio, kAudioPlugType_FloatVec);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_Speaker, kAudioPlugType_Int);
	addInput(kInput_ChannelIndex, kAudioPlugType_Int);
	
	//
	
	source.voiceNode = this;
	
	//
	
	Assert(g_currentAudioGraph != nullptr);
	audioGraph = g_currentAudioGraph;
}

AudioNodeVoice::~AudioNodeVoice()
{
	if (voice != nullptr)
	{
		voiceMgr->freeVoice(voice);
	}
}

void AudioNodeVoice::tick(const float dt)
{
	const Speaker speaker = (Speaker)getInputInt(kInput_Speaker, 0);
	const int channelIndex = getInputInt(kInput_ChannelIndex, 0);
	
	if (isPassthrough)
	{
		if (voice != nullptr)
		{
			voiceMgr->freeVoice(voice);
		}
		
		return;
	}
	else if (voice == nullptr || (speaker == kSpeaker_Channel && voice->channelIndex != channelIndex))
	{
		const int voiceChannelIndex =
			speaker == kSpeaker_Channel
			? channelIndex
			: -1;
		
		if (voice)
			voiceMgr->freeVoice(voice);
		
		voiceMgr->allocVoice(voice, &source, "voice", true, 0.f, 1.f, voiceChannelIndex);
	}
	
	voice->gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One)->getMean();
	
	if (speaker == kSpeaker_LeftAndRight)
		voice->speaker = AudioVoice::kSpeaker_LeftAndRight;
	else if (speaker == kSpeaker_Left)
		voice->speaker = AudioVoice::kSpeaker_Left;
	else if (speaker == kSpeaker_Right)
		voice->speaker = AudioVoice::kSpeaker_Right;
	else if (speaker == kSpeaker_Channel)
		voice->speaker = AudioVoice::kSpeaker_Channel;
	else
	{
		Assert(false);
		voice->speaker = AudioVoice::kSpeaker_None;
	}
	
	if (g_currentAudioGraph->rampDown)
	{
		voice->rampInfo.rampDown(0.f, AUDIO_UPDATE_SIZE / float(SAMPLE_RATE));
	}
}
