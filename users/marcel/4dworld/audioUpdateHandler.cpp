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
#include "audioGraphManager.h"
#include "audioProfiling.h"
#include "audioUpdateHandler.h"
#include "osc4d.h"
#include "soundmix.h"
#include "Timer.h"
#include <SDL2/SDL.h>
#include <vector>

Osc4DStream * g_oscStream = nullptr;

float * g_audioInputChannels = nullptr;
int g_numAudioInputChannels = 0;

AudioUpdateHandler::AudioUpdateHandler()
	: updateTasks()
	, voiceMgr(nullptr)
	, audioGraphMgr(nullptr)
	, commandQueue()
	, oscStream(nullptr)
	, time(0.0)
	, mutex(nullptr)
	, cpuTime(0)
	, nextCpuTimeUpdate()
{
}

AudioUpdateHandler::~AudioUpdateHandler()
{
	shut();
}

void AudioUpdateHandler::init(SDL_mutex * _mutex, const char * ipAddress, const int udpPort)
{
	Assert(_mutex != nullptr);
	
	//
	
	shut();
	
	//
	
	Assert(mutex == nullptr);
	mutex = _mutex;
	
	Assert(oscStream == nullptr);
	oscStream = new Osc4DStream();
	oscStream->init(ipAddress, udpPort);
	
	Assert(g_oscStream == nullptr);
	g_oscStream = oscStream;
}

void AudioUpdateHandler::shut()
{
	Assert(g_oscStream == oscStream);
	g_oscStream = nullptr;
	
	if (oscStream != nullptr)
	{
		oscStream->shut();
		delete oscStream;
		oscStream = nullptr;
	}
}

void AudioUpdateHandler::setOscEndpoint(const char * ipAddress, const int udpPort)
{
	SDL_LockMutex(mutex);
	{
		oscStream->setEndpoint(ipAddress, udpPort);
	}
	SDL_UnlockMutex(mutex);
}

void AudioUpdateHandler::portAudioCallback(
	const void * inputBuffer,
	const int numInputChannels,
	void * outputBuffer,
	const int framesPerBuffer)
{
	audioCpuTimingBlock(audioUpdateHandler);
	
	cpuTimeTotal -= g_TimerRT.TimeUS_get();
	
	//
	
	bool forceSyncOsc = false;
	
	//
	
	Command command;
	
	while (commandQueue.pop(command))
	{
		switch (command.type)
		{
		case Command::kType_None:
			break;
		case Command::kType_ForceOscSync:
			forceSyncOsc = true;
			break;
		}
	}
	
	//
	
	const float dt = framesPerBuffer / float(SAMPLE_RATE);
	
	g_currentAudioTime = time;
	
	g_audioInputChannels = (float*)inputBuffer;
	g_numAudioInputChannels = numInputChannels;
	
	for (auto updateTask : updateTasks)
	{
		updateTask->audioUpdate(dt);
	}
	
	if (audioGraphMgr != nullptr)
	{
		audioGraphMgr->tick(dt);
		
		audioGraphMgr->updateAudioValues();
	}
	
	g_audioInputChannels = nullptr;
	g_numAudioInputChannels = 0;
	
	if (voiceMgr != nullptr)
	{
		voiceMgr->portAudioCallback(inputBuffer, numInputChannels, outputBuffer, framesPerBuffer);
		
		//
		
		if (oscStream->isReady())
		{
			// todo : remove limiter hack
			static int limiter = 0;
			limiter++;
			
			//if ((limiter % 4) == 0)
			{
				oscStream->beginBundle();
				{
					voiceMgr->generateOsc(*oscStream, forceSyncOsc);
				}
				oscStream->endBundle();
			}
		}
	}
	
	g_currentAudioTime = 0.0;
	
	time += dt;
	
	//
	
	const int64_t currentTimeUs = g_TimerRT.TimeUS_get();
	
	cpuTimeTotal += currentTimeUs;
	
	if (currentTimeUs >= nextCpuTimeUpdate)
	{
		SDL_LockMutex(mutex);
		{
			cpuTime = cpuTimeTotal;
			cpuTimeTotal = 0;
		}
		SDL_UnlockMutex(mutex);
		
		nextCpuTimeUpdate = currentTimeUs + 1000000;
	}
}
