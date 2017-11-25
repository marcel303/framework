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
#include "audioNodePcm.h"

AUDIO_NODE_TYPE(audioSourcePcm, AudioNodeSourcePcm)
{
	typeName = "audio.pcm";
	
	in("pcm", "pcmData");
	in("filename", "string");
	in("gain", "audioValue", "1");
	in("autoPlay", "bool", "1");
	in("loop", "bool", "1");
	in("loopCount", "int");
	in("play!", "trigger");
	in("pause!", "trigger");
	in("resume!", "trigger");
	in("rangeBegin", "audioValue");
	in("rangeLength", "audioValue", "-1");
	out("audio", "audioValue");
	out("duration", "float");
	out("done!", "trigger");
	out("loop!", "trigger");
}

AudioNodeSourcePcm::AudioNodeSourcePcm()
	: AudioNodeBase()
	, wasDone(false)
	, audioSource()
	, audioOutput()
	, lengthOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_PcmData, kAudioPlugType_PcmData);
	addInput(kInput_Filename, kAudioPlugType_String);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_AutoPlay, kAudioPlugType_Bool);
	addInput(kInput_Loop, kAudioPlugType_Bool);
	addInput(kInput_LoopCount, kAudioPlugType_Int);
	addInput(kInput_Play, kAudioPlugType_Trigger);
	addInput(kInput_Pause, kAudioPlugType_Trigger);
	addInput(kInput_Resume, kAudioPlugType_Trigger);
	addInput(kInput_RangeBegin, kAudioPlugType_FloatVec);
	addInput(kInput_RangeLength, kAudioPlugType_FloatVec);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	addOutput(kOutput_Length, kAudioPlugType_Float, &lengthOutput);
	addOutput(kOutput_Done, kAudioPlugType_Trigger, nullptr);
	addOutput(kOutput_Loop, kAudioPlugType_Trigger, nullptr);
}

void AudioNodeSourcePcm::init(const GraphNode & node)
{
	if (getInputBool(kInput_AutoPlay, true))
	{
		audioSource.isPlaying = true;
	}
}

void AudioNodeSourcePcm::tick(const float dt)
{
	const PcmData * pcmData = getInputPcmData(kInput_PcmData);
	const char * filename = getInputString(kInput_Filename, nullptr);
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const bool loop = getInputBool(kInput_Loop, true);
	const int loopCount = getInputInt(kInput_LoopCount, 0);
	const AudioFloat * rangeBegin = getInputAudioFloat(kInput_RangeBegin, nullptr);
	const AudioFloat * rangeLength = getInputAudioFloat(kInput_RangeLength, nullptr);
	
	// update PCM data and length output. note : same pcmData doesn't necessarily mean the PCM data hasn't changed. it could have been re-allocated at the same address. just always update the length here instead of trying to be clever and detect changes
	
	if (filename != nullptr)
	{
		pcmData = getPcmData(filename);
	}
	
	audioSource.pcmData = pcmData;
	
	if (pcmData == nullptr)
	{
		lengthOutput = 0.f;
	}
	else
	{
		lengthOutput = pcmData->numSamples / float(SAMPLE_RATE);
	}
	
	//
	
	audioSource.loop = loop;
	audioSource.maxLoopCount = loopCount;
	
	if ((rangeBegin == nullptr && rangeLength == nullptr) || pcmData == nullptr)
	{
		audioSource.clearRange();
	}
	else
	{
		// todo : do this on a per-sample basis !
		
		const int _rangeBegin = rangeBegin ? rangeBegin->getScalar() * pcmData->numSamples : 0;
		const int _rangeLength = rangeLength ? rangeLength->getScalar() * pcmData->numSamples : pcmData->numSamples - _rangeBegin;
		
		audioSource.setRange(_rangeBegin, _rangeLength);
	}
	
	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
	}
	else if (audioSource.isPlaying)
	{
		audioOutput.setVector();
		
		audioSource.generate(audioOutput.samples, AUDIO_UPDATE_SIZE);
		
		audioOutput.mul(*gain);
		
		if (audioSource.hasLooped)
		{
			trigger(kOutput_Loop);
		}
		
		if (audioSource.isDone && wasDone == false)
		{
			trigger(kOutput_Done);
		}
		
		wasDone = audioSource.isDone;
	}
	else
	{
		audioOutput.setScalar(0.f);
	}
}

void AudioNodeSourcePcm::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Play)
	{
		audioSource.isPlaying = true;
		audioSource.samplePosition = audioSource.rangeBegin;
		audioSource.loopCount = 0;
		
		wasDone = false;
	}
	else if (inputSocketIndex == kInput_Pause)
	{
		audioSource.isPlaying = false;
	}
	else if (inputSocketIndex == kInput_Resume)
	{
		audioSource.isPlaying = true;
	}
}
