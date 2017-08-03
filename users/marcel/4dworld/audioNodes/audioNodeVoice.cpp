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

AUDIO_ENUM_TYPE(voiceSpeaker)
{
	elem("left+right");
	elem("left");
	elem("right");
}

AUDIO_NODE_TYPE(voice, AudioNodeVoice)
{
	typeName = "voice";
	
	in("audio", "audioValue");
	inEnum("speaker", "voiceSpeaker");
}

void AudioNodeVoice::AudioSourceVoiceNode::generate(ALIGN16 float * __restrict samples, const int numSamples)
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
	addInput(kInput_Speaker, kAudioPlugType_Int);
	
	//
	
	source.voiceNode = this;
	g_voiceMgr->allocVoice(voice, &source, "voice", true, 0.f);
}

AudioNodeVoice::~AudioNodeVoice()
{
	g_voiceMgr->freeVoice(voice);
}

void AudioNodeVoice::tick(const float dt)
{
	const Speaker speaker = (Speaker)getInputInt(kInput_Speaker, 0);
	
	if (speaker == kSpeaker_LeftAndRight)
		voice->speaker = AudioVoice::kSpeaker_LeftAndRight;
	else if (speaker == kSpeaker_Left)
		voice->speaker = AudioVoice::kSpeaker_Left;
	else if (speaker == kSpeaker_Right)
		voice->speaker = AudioVoice::kSpeaker_Right;
}
