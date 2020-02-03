/*
	Copyright (C) 2020 Marcel Smit
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
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioProfiling.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "Debugging.h"
#include "Timer.h"

AudioUpdateHandler::AudioUpdateHandler()
	: updateTasks()
	, mutex(nullptr)
	, voiceMgr(nullptr)
	, audioGraphMgr(nullptr)
	, msecsPerTick(0)
	, msecsPerSecond(0)
	, msecsTotal(0)
	, nextCpuTimeUpdate(0)
{
}

AudioUpdateHandler::~AudioUpdateHandler()
{
	shut();
}

void AudioUpdateHandler::init(AudioMutexBase * in_mutex, AudioVoiceManager * in_voiceMgr, AudioGraphManager * in_audioGraphMgr)
{
	Assert(in_mutex != nullptr);
	
	//
	
	shut();
	
	//
	
	Assert(mutex == nullptr);
	Assert(voiceMgr == nullptr);
	Assert(audioGraphMgr == nullptr);
	
	mutex = in_mutex;
	voiceMgr = in_voiceMgr;
	audioGraphMgr = in_audioGraphMgr;
	
	//
	
	audioGraphMgr->getContext()->addObject(&audioIO);
}

void AudioUpdateHandler::shut()
{
}

void AudioUpdateHandler::portAudioCallback(
	const void * inputBuffer,
	const int numInputChannels,
	void * outputBuffer,
	const int numOutputChannels,
	const int framesPerBuffer)
{
	audioCpuTimingBlock(audioUpdateHandler);
	
	msecsTotal -= g_TimerRT.TimeUS_get();
	
	//
	
	const float dt = framesPerBuffer / float(SAMPLE_RATE);
	
	audioIO.numInputChannels = numInputChannels;
	audioIO.numOutputChannels = numOutputChannels;
	audioIO.inputBuffer = (float*)inputBuffer;
	
	//
	
	for (auto updateTask : updateTasks)
	{
		updateTask->preAudioUpdate(dt);
	}
	
	//
	
	if (audioGraphMgr != nullptr)
	{
		audioGraphMgr->tickAudio(dt);
	}
	
	//
	
	if (voiceMgr != nullptr)
	{
		voiceMgr->generateAudio((float*)outputBuffer, framesPerBuffer, numOutputChannels);
	}
	
	//
	
	for (auto updateTask : updateTasks)
	{
		updateTask->postAudioUpdate(dt);
	}
	
	//
	
	if (audioGraphMgr != nullptr)
	{
		audioGraphMgr->tickVisualizers();
	}
	
	//
	
	audioIO.numInputChannels = 0;
	audioIO.numOutputChannels = 0;
	audioIO.inputBuffer = nullptr;
	
	//
	
	const int64_t currentTimeUs = g_TimerRT.TimeUS_get();
	
	msecsTotal += currentTimeUs;
	
	if (currentTimeUs >= nextCpuTimeUpdate)
	{
		msecsPerSecond = msecsTotal;
		msecsTotal = 0;
		
		nextCpuTimeUpdate = currentTimeUs + 1000000;
	}
}
