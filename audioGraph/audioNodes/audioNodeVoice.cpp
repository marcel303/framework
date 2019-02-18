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
#include "audioVoiceManager.h"
#include <string.h>

AUDIO_ENUM_TYPE(voiceSpeaker)
{
	elem("left+right");
	elem("left");
	elem("right");
	elem("channel");
}

AUDIO_NODE_TYPE(AudioNodeVoice)
{
	typeName = "voice";
	
	in("audio", "audioValue");
	in("gain", "audioValue", "1");
	inEnum("speaker", "voiceSpeaker");
	in("rampTime", "audioValue", "0.2");
	in("fadeTime", "audioValue", "0.2");
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
		setCurrentAudioGraphTraversalId(voiceNode->lastTickTraversalId);
		{
			const AudioFloat * audio = voiceNode->getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
			
			audio->expand();
			
			memcpy(samples, audio->samples, numSamples * sizeof(float));
		}
		clearCurrentAudioGraphTraversalId();
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
	addInput(kInput_RampTime, kAudioPlugType_FloatVec);
	addInput(kInput_FadeTime, kAudioPlugType_FloatVec);
	addInput(kInput_ChannelIndex, kAudioPlugType_Int);
	
	//
	
	source.voiceNode = this;
	
	//
	
	Assert(g_currentAudioGraph != nullptr);
	audioGraph = g_currentAudioGraph;
}

void AudioNodeVoice::shut()
{
	if (voice != nullptr)
	{
		g_currentAudioGraph->freeVoice(voice);
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
			g_currentAudioGraph->freeVoice(voice);
		}
		
		return;
	}
	else if (voice == nullptr || (speaker == kSpeaker_Channel && voice->channelIndex != channelIndex))
	{
		if (voice)
			g_currentAudioGraph->freeVoice(voice);
		
		AudioFloat defaultRampTime(.2f);
		const float rampTime = getInputAudioFloat(kInput_RampTime, &defaultRampTime)->getMean();
		
		if (speaker == kSpeaker_Channel)
		{
			if (channelIndex >= 0)
			{
				g_currentAudioGraph->allocVoice(
					voice, &source, "voice",
					rampTime > 0.f, 0.f, rampTime,
					channelIndex);
			}
		}
		else
		{
			g_currentAudioGraph->allocVoice(
				voice, &source, "voice",
				rampTime > .0f, 0.f, rampTime,
				-1);
		}
	}
	
	if (voice == nullptr)
	{
		return;
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
		AudioFloat defaultFadeTime(.2f);
		const float fadeTime = getInputAudioFloat(kInput_FadeTime, &defaultFadeTime)->getMean();
		
		voice->rampInfo.rampDown(0.f, fadeTime);
	}
}
