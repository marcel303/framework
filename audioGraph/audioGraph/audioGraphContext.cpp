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

#include "audioGraphContext.h"
#include "Debugging.h"
#include "Log.h"
#include <algorithm> // std::sort
#include <math.h>

AudioGraphContext::AudioGraphContext()
	: mutex_mem(nullptr)
	, mutex_reg(nullptr)
	, voiceMgr(nullptr)
	, mainThreadId()
	, controlValues()
	, memf()
	, time(0.0)
{
}

void AudioGraphContext::init(
	AudioMutexBase * in_mutex_mem,
	AudioMutexBase * in_mutex_reg,
	AudioVoiceManager * in_voiceMgr)
{
	mutex_mem = in_mutex_mem;
	mutex_reg = in_mutex_reg;
	
	voiceMgr = in_voiceMgr;
	
	mainThreadId.setThreadId();
}

void AudioGraphContext::shut()
{
	mainThreadId.clearThreadId();
	
	mutex_mem = nullptr;
	mutex_reg = nullptr;
}

void AudioGraphContext::tickAudio(const float dt)
{
	mutex_mem->lock();
	{
		// update control values
		
		updateControlValues(dt);
		
		// export control values
		
		exportControlValues();
		
		// synchronize memory from main -> audio
		
		for (auto & memf_itr : memf)
		{
			auto & memf = memf_itr.second;
			
			memf.syncMainToAudio();
		}
	}
	mutex_mem->unlock();

	// update time
	
	time += dt;
}

void AudioGraphContext::registerMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	mutex_reg->lock();
	mutex_mem->lock();
	{
		auto mem_itr = memf.find(name);
		
		if (mem_itr == memf.end())
		{
			auto & mem = memf[name];
			
			mem.value1_mainThread = value1;
			mem.value2_mainThread = value2;
			mem.value3_mainThread = value3;
			mem.value4_mainThread = value4;
			
			mem.syncMainToAudio();
		}
	}
	mutex_mem->unlock();
	mutex_reg->unlock();
}

void AudioGraphContext::registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY)
{
	// thread: main, audio
	
	mutex_reg->lock();
	mutex_mem->lock();
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
			
			{
				// note : it won't be safe to reference controlValue after the sort,
				//        so we make sure to let it leave scope before the sort
				
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
			}
			
			std::sort(controlValues.begin(), controlValues.end(), [](const AudioControlValue & a, const AudioControlValue & b) { return a.name < b.name; });
			
			// register the memf. if we don't, setMemf would fail!
			
			registerMemf(name, defaultX, defaultY);
		}
	}
	mutex_mem->unlock();
	mutex_reg->unlock();
}

void AudioGraphContext::unregisterControlValue(const char * name)
{
	// thread: main, audio
	
	mutex_reg->lock();
	mutex_mem->lock();
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
	mutex_mem->unlock();
	mutex_reg->unlock();
}

void AudioGraphContext::lockControlValues()
{
	mutex_mem->lock();
}

void AudioGraphContext::unlockControlValues()
{
	mutex_mem->unlock();
}

void AudioGraphContext::updateControlValues(const float dt)
{
	for (auto & controlValue : controlValues)
	{
		const float retain = powf(controlValue.smoothness, dt);
		
		controlValue.currentX = controlValue.currentX * retain + controlValue.desiredX * (1.f - retain);
		controlValue.currentY = controlValue.currentY * retain + controlValue.desiredY * (1.f - retain);
	}
}

void AudioGraphContext::exportControlValues()
{
	for (auto & controlValue : controlValues)
	{
		setMemf(
			controlValue.name.c_str(),
			controlValue.currentX,
			controlValue.currentY);
	}
}

void AudioGraphContext::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	mutex_mem->lock();
	{
		auto mem_itr = memf.find(name);
		
		if (mem_itr != memf.end())
		{
			auto & mem = mem_itr->second;
		
			mem.value1_mainThread = value1;
			mem.value2_mainThread = value2;
			mem.value3_mainThread = value3;
			mem.value4_mainThread = value4;
		}
	}
	mutex_mem->unlock();
}

AudioGraphContext::Memf AudioGraphContext::getMemf(const char * name, const bool isMainThread)
{
	Assert(mainThreadId.checkThreadId() == isMainThread);
	
	Memf result;
	
	mutex_mem->lock();
	{
		auto memfItr = memf.find(name);
		
		if (memfItr != memf.end())
		{
			result = memfItr->second;
		}
	}
	mutex_mem->unlock();
	
	return result;
}

void AudioGraphContext::addObject(const std::type_index & type, void * object, const char * name)
{
	LOG_DBG("AudioGraphContext: Adding object '%s'", name);
	
	ObjectRegistration r;
	r.type = type;
	r.object = object;

	objects.push_back(r);
}

void * AudioGraphContext::findObject(const std::type_index & type)
{
	for (auto & r : objects)
		if (r.type == type)
			return r.object;

	return nullptr;
}

