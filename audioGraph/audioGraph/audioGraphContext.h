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

#pragma once

#include "audioThreading.h"
#include "audioTypes.h"
#include <map>
#include <typeindex>
#include <vector>

struct AudioGraphManager;
struct AudioVoiceManager;

struct AudioGraphContext
{
	struct ObjectRegistration
	{
		std::type_index type = std::type_index(typeid(void));
		void * object;
	};

	struct Memf
	{
		float value1 = 0.f;
		float value2 = 0.f;
		float value3 = 0.f;
		float value4 = 0.f;
	};
	
// todo : add a method for adding 'objects'. query objects by typeid
// todo : do the same for vfxgraph! useful for registering FsfxLibrary, ...
/*

	context.addObject(new AudioGraphMemfMgr());
	context.addObject(new AudioGraphMemsMgr());
	context.addObject(new AudioGraphControlValueMgr());
	context.addObject(new JsusFxAudioGraphLibrary());
 
	auto * memfMgr = context.findObject<AudioGraphMemfMgr>();
 
	if (memfMgr != nullptr)
	{
		memfMgr->set("variable", 1.f);
	}
*/
	
	AudioMutexBase * audioMutex;
	
	AudioVoiceManager * voiceMgr;
	
	AudioThreadId mainThreadId;

	std::vector<ObjectRegistration> objects;
	
	std::vector<AudioControlValue> controlValues;
	
	std::map<std::string, Memf> memf;

	double time;
	
	AudioGraphContext();
	
	// called from the app thread
	void init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr);
	void shut();
	
	// called from the audio thread
	void tickAudio(const float dt);
	
	// called from any thread
	void registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY);
	void unregisterControlValue(const char * name);
	void exportControlValues();
	
	// called from any thread
	void setMemf(const char * name, const float value1, const float value2 = 0.f, const float value3 = 0.f, const float value4 = 0.f);
	Memf getMemf(const char * name);

	// called from the app thread
	template <typename T> void addObject(T * object)
	{
		return addObject(std::type_index(typeid(T)), object);
	}

	void addObject(const std::type_index & type, void * object);
	
	// called from the audio thread
	template <typename T> T * findObject()
	{
		return (T*)findObject(std::type_index(typeid(T)));
	}

	void * findObject(const std::type_index & type);
};
