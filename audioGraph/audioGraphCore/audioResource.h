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

#include <atomic>
#include <string>

//

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

//

struct AudioMutex;
struct GraphNode;

//

struct AudioResourceBase
{
	std::atomic<int> version;

	AudioMutex * mutex;
	
	AudioResourceBase();
	virtual ~AudioResourceBase();
	
	void lock();
	void unlock();
	
	virtual void save(tinyxml2::XMLPrinter * printer) = 0;
	virtual void load(tinyxml2::XMLElement * elem) = 0;
};

//

#include <typeindex>

AudioResourceBase * createAudioNodeResourceImpl(const GraphNode & node, const char * type, const std::type_index & typeIndex, const char * name);

template <typename T> bool createAudioNodeResource(const GraphNode & node, const char * type, const char * name, T *& resource)
{
	const std::type_index typeIndex = std::type_index(typeid(T));
	
	AudioResourceBase * resourceBase = createAudioNodeResourceImpl(node, type, typeIndex, name);
	
	if (resourceBase == nullptr)
	{
		return false;
	}
	else
	{
		resource = static_cast<T*>(resourceBase);
		
		return true;
	}
}

bool freeAudioNodeResourceImpl(AudioResourceBase * resource);

template <typename T> void freeAudioNodeResource(T *& resource)
{
	if (resource != nullptr)
	{
		if (freeAudioNodeResourceImpl(resource))
		{
			delete resource;
		}
		
		resource = nullptr;
	}
}

//

struct AudioResourceTypeRegistration
{
	AudioResourceTypeRegistration * next;
	
	AudioResourceBase* (*create)();
	
	std::string typeName;
	std::type_index typeIndex;

	AudioResourceTypeRegistration(const std::type_index & typeIndex);
};

#define AUDIO_RESOURCE_TYPE(type, name) \
	struct type ## __audio_resourceRegistration : AudioResourceTypeRegistration \
	{ \
		type ## __audio_resourceRegistration() \
			: AudioResourceTypeRegistration(std::type_index(typeid(type))) \
		{ \
			create = []() -> AudioResourceBase* { return new type(); }; \
			typeName = name; \
		} \
	}; \
	extern type ## __audio_resourceRegistration type ## __audio_resourceRegistrationInstance; \
	type ## __audio_resourceRegistration type ## __audio_resourceRegistrationInstance
