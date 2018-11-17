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

#include "Debugging.h"
#include "graph.h"
#include "Log.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "audioResource.h"
#include "audioTypes.h"
#include <SDL2/SDL_mutex.h>

using namespace tinyxml2;

static AudioResourceTypeRegistration * g_audioResourceTypeRegistrationList = nullptr;

struct AudioResourcePath
{
	GraphNodeId nodeId;
	std::string type;
	std::string name;
	
	AudioResourcePath()
		: nodeId(kGraphNodeIdInvalid)
		, type()
		, name()
	{
	}
	
	bool operator<(const AudioResourcePath & other) const
	{
		if (nodeId != other.nodeId)
			return nodeId < other.nodeId;
		if (type != other.type)
			return type < other.type;
		if (name != other.name)
			return name < other.name;
		
		return false;
	}
	
	std::string toString() const
	{
		return String::FormatC("%s:%s/%d/%s", type.c_str(), "nodes", nodeId, name.c_str());
	}
};

struct AudioResourceElem
{
	AudioResourceBase * resource;
	int refCount;
	
	AudioResourceElem()
		: resource(nullptr)
		, refCount(0)
	{
	}
};

//

AudioResourceBase::~AudioResourceBase()
{
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
}

void AudioResourceBase::lock()
{
	if (mutex == nullptr)
	{
		for (;;)
		{
			int expected = 0;
			
			if (mutexCreationLock.compare_exchange_strong(expected, 1))
				break;
		}
		
		mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);
		
		mutexCreationLock.exchange(0);
	}
	
	Verify(SDL_LockMutex(mutex) == 0);
}

void AudioResourceBase::unlock()
{
	Verify(SDL_UnlockMutex(mutex) == 0);
}

//

static std::map<AudioResourcePath, AudioResourceElem> resourcesByPath;
static std::map<AudioResourceBase*, AudioResourcePath> pathsByResource;

AudioResourceBase * createAudioNodeResourceImpl(const GraphNode & node, const char * type, const char * name)
{
	AudioResourceBase * resource = nullptr;
	
	AudioResourcePath path;
	path.nodeId = node.id;
	path.type = type;
	path.name = name;
	
	auto i = resourcesByPath.find(path);
	
	if (i != resourcesByPath.end())
	{
		LOG_DBG("incremented refCount for resource %s", path.toString().c_str());
		
		auto & e = i->second;
		
		e.refCount++;
		
		resource = e.resource;
	}
	else
	{
		const char * resourceData = node.getResource(type, name, nullptr);
		
		XMLDocument d;
		bool hasXml = false;
		
		if (resourceData != nullptr)
		{
			hasXml = d.Parse(resourceData) == XML_SUCCESS;
		}
		
		//
		
		for (AudioResourceTypeRegistration * registration = g_audioResourceTypeRegistrationList; registration != nullptr; registration = registration->next)
		{
			if (registration->typeName == type)
			{
				resource = registration->create();
			}
		}
		
		//
		
		Assert(resource != nullptr);
		if (resource == nullptr)
		{
			LOG_ERR("failed to create resource %s", path.toString().c_str());
		}
		else
		{
			LOG_DBG("created resource %s", path.toString().c_str());
			
			if (hasXml)
			{
				resource->load(d.RootElement());
			}
			
			AudioResourceElem e;
			e.resource = resource;
			e.refCount = 1;
			
			resourcesByPath[path] = e;
			pathsByResource[resource] = path;
		}
	}
	
	return resource;
}

bool freeAudioNodeResourceImpl(AudioResourceBase * resource)
{
	bool result = false;
	
	auto i = pathsByResource.find(resource);
	
	Assert(i != pathsByResource.end());
	if (i == pathsByResource.end())
	{
		LOG_ERR("failed to find resource %p", resource);
	}
	else
	{
		auto & path = i->second;
		
		auto j = resourcesByPath.find(path);
		
		Assert(j != resourcesByPath.end());
		if (j == resourcesByPath.end())
		{
			LOG_ERR("failed to find resource elem for resource %s", path.toString().c_str());
		}
		else
		{
			auto & e = j->second;
			
			e.refCount--;
			
			if (e.refCount == 0)
			{
				LOG_DBG("refCount reached zero for resource %s. resource will be freed", path.toString().c_str());
				
				resourcesByPath.erase(j);
				pathsByResource.erase(i);
				
				result = true;
			}
			else
			{
				LOG_DBG("decremented refCount for resource %s", path.toString().c_str());
			}
		}
	}
	
	return result;
}

AudioResourceTypeRegistration::AudioResourceTypeRegistration()
	: next(nullptr)
	, create(nullptr)
	, typeName()
{
	next = g_audioResourceTypeRegistrationList;
	g_audioResourceTypeRegistrationList = this;
}
