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

#pragma once

#include "commandQueue.h"
#include "osc4d.h"
#include "paobject.h"
#include <vector>

//

struct AudioGraphManager;
struct AudioVoiceManager;
struct Osc4DStream;

struct SDL_mutex;

//

extern Osc4DStream * g_oscStream;

//

struct AudioUpdateTask
{
	virtual ~AudioUpdateTask()
	{
	}
	
	virtual void audioUpdate(const float dt) = 0;
};

//

struct AudioUpdateHandler : PortAudioHandler
{
	struct Command
	{
		enum Type
		{
			kType_None,
			kType_ForceOscSync
		};
		
		Type type;
		
		Command()
			: type(kType_None)
		{
		}
	};
	
	std::vector<AudioUpdateTask*> updateTasks;
	
	AudioVoiceManager * voiceMgr;
	
	AudioGraphManager * audioGraphMgr;
	
	CommandQueue<Command> commandQueue;
	
	Osc4DStream * oscStream;
	
	double time;
	
	SDL_mutex * mutex;
	
	int64_t cpuTime;
	int64_t cpuTimeTotal;
	int64_t nextCpuTimeUpdate;
	
	AudioUpdateHandler();
	~AudioUpdateHandler();
	
	void init(SDL_mutex * mutex, const char * ipAddress, const int udpPort);
	void shut();
	
	void setOscEndpoint(const char * ipAddress, const int udpPort);
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override;
};
