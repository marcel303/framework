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

struct GraphNode;

//

struct AudioResourceBase
{
	virtual ~AudioResourceBase()
	{
	}
	
	virtual void save(tinyxml2::XMLPrinter * printer) = 0;
	virtual void load(tinyxml2::XMLElement * elem) = 0;
};

//

AudioResourceBase * createAudioNodeResourceImpl(const GraphNode & node, const char * type, const char * name);

template <typename T> bool createAudioNodeResource(const GraphNode & node, const char * type, const char * name, T *& resource)
{
	AudioResourceBase * resourceBase = createAudioNodeResourceImpl(node, type, name);
	
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

	AudioResourceTypeRegistration();
};

#define AUDIO_RESOURCE_TYPE(type, name) \
	struct type ## __audio_resourceRegistration : AudioResourceTypeRegistration \
	{ \
		type ## __audio_resourceRegistration() \
		{ \
			create = []() -> AudioResourceBase* { return new type(); }; \
			typeName = name; \
		} \
	}; \
	extern type ## __audio_resourceRegistration type ## __audio_resourceRegistrationInstance; \
	type ## __audio_resourceRegistration type ## __audio_resourceRegistrationInstance
