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

#include "audioGraphContext.h"
#include "Debugging.h"
#include "Log.h"
#include <algorithm>
#include <math.h>
#include <SDL2/SDL_mutex.h>

AudioGraphContext::AudioGraphContext()
	: audioMutex(nullptr)
	, voiceMgr(nullptr)
	, audioGraphMgr(nullptr)
	, mainThreadId()
	, controlValues()
	, memf()
	, time(0.0)
{
}

void AudioGraphContext::init(SDL_mutex * mutex, AudioVoiceManager * _voiceMgr, AudioGraphManager * _audioGraphMgr)
{
	audioMutex = mutex;
	
	voiceMgr = _voiceMgr;
	
	audioGraphMgr = _audioGraphMgr;
	
	mainThreadId.setThreadId();
}

void AudioGraphContext::shut()
{
	mainThreadId.clearThreadId();

	audioGraphMgr = nullptr;
	
	audioMutex = nullptr;
}

void AudioGraphContext::tickAudio(const float dt)
{
	// update control values

	for (auto & controlValue : controlValues)
	{
		const float retain = powf(controlValue.smoothness, dt);
		
		controlValue.currentX = controlValue.currentX * retain + controlValue.desiredX * (1.f - retain);
		controlValue.currentY = controlValue.currentY * retain + controlValue.desiredY * (1.f - retain);
	}

	exportControlValues();

	// update time
	
	time += dt;
}

void AudioGraphContext::registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY)
{
	//rteMutex.lock(); // todo
	
	audioMutex.lock();
	{
		bool exists = false;
		
		for (auto & controlValue : controlValues)
		{
			if (controlValue.name == name)
			{
				controlValue.refCount++;
				exists = true;
				break;
			}
		}
		
		if (exists == false)
		{
			controlValues.resize(controlValues.size() + 1);
			
			auto & controlValue = controlValues.back();
			
			controlValue.type = type;
			controlValue.name = name;
			controlValue.refCount = 1;
			controlValue.min = min;
			controlValue.max = max;
			controlValue.smoothness = smoothness;
			controlValue.defaultX = defaultX;
			controlValue.defaultY = defaultY;
			controlValue.desiredX = defaultX;
			controlValue.desiredY = defaultY;
			controlValue.currentX = defaultX;
			controlValue.currentY = defaultY;
			
			controlValue.finalize();
			
			std::sort(controlValues.begin(), controlValues.end(), [](const AudioControlValue & a, const AudioControlValue & b) { return a.name < b.name; });
			
			// immediately export it. this is necessary during real-time editing only. for regular processing, it would export the control values before processing the audio graph anyway
			
			setMemf(controlValue.name.c_str(), controlValue.currentX, controlValue.currentY);
		}
	}
	audioMutex.unlock();
	
	//rteMutex.unlock();
}

void AudioGraphContext::unregisterControlValue(const char * name)
{
	audioMutex.lock();
	{
		bool exists = false;
		
		for (auto controlValueItr = controlValues.begin(); controlValueItr != controlValues.end(); ++controlValueItr)
		{
			auto & controlValue = *controlValueItr;
			
			if (controlValue.name == name)
			{
				controlValue.refCount--;
				
				if (controlValue.refCount == 0)
				{
					//LOG_DBG("erasing control value %s", name);
					
					controlValues.erase(controlValueItr);
				}
				
				exists = true;
				break;
			}
		}
		
		Assert(exists);
		if (exists == false)
		{
			LOG_WRN("failed to unregister control value %s", name);
		}
	}
	audioMutex.unlock();
}

void AudioGraphContext::exportControlValues()
{
	audioMutex.lock();
	{
		for (auto & controlValue : controlValues)
		{
			setMemf(controlValue.name.c_str(), controlValue.currentX, controlValue.currentY);
		}
	}
	audioMutex.unlock();
}

void AudioGraphContext::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	audioMutex.lock();
	{
		auto & mem = memf[name];
		
		mem.value1 = value1;
		mem.value2 = value2;
		mem.value3 = value3;
		mem.value4 = value4;
	}
	audioMutex.unlock();
}

AudioGraphContext::Memf AudioGraphContext::getMemf(const char * name)
{
	Memf result;
	
	audioMutex.lock();
	{
		auto memfItr = memf.find(name);
		
		if (memfItr != memf.end())
		{
			result = memfItr->second;
		}
	}
	audioMutex.unlock();
	
	return result;
}
