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

#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioGraphManager.h"
#include "osc4d.h"
#include "soundmix.h"
#include <SDL2/SDL.h>
#include <vector>

Osc4DStream * g_oscStream = nullptr;

AudioUpdateHandler::AudioUpdateHandler()
	: updateTasks()
	, voiceMgr(nullptr)
	, audioGraphMgr(nullptr)
	, commandQueue()
	, oscStream(nullptr)
	, mutex(nullptr)
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
	void * outputBuffer,
	int framesPerBuffer)
{
	Command command;
	
	while (commandQueue.pop(command))
	{
		switch (command.type)
		{
		case Command::kType_None:
			break;
		case Command::kType_ForceOscSync:
			break;
		}
	}
	
	//
	
	const float dt = framesPerBuffer / float(SAMPLE_RATE);
	
	for (auto updateTask : updateTasks)
	{
		updateTask->audioUpdate(dt);
	}
	
	if (audioGraphMgr != nullptr)
	{
		SDL_LockMutex(mutex);
		{
			audioGraphMgr->tick(dt);
			
			audioGraphMgr->updateAudioValues();
		}
		SDL_UnlockMutex(mutex);
	}
	
	if (voiceMgr != nullptr)
	{
		voiceMgr->portAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);
		
		//
		
		// todo : remove limiter hack
		static int limiter = 0;
		limiter++;
		
		if ((limiter % 4) == 0)
		{
			oscStream->beginBundle();
			{
				voiceMgr->generateOsc(*oscStream, false);
			}
			oscStream->endBundle();
		}
	}
}
