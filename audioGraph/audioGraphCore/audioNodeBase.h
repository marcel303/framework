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

#include "audioPlug.h"
#include "audioProfiling.h"
#include "audioTypes.h"
#include "Debugging.h"
#include <string>
#include <vector>

struct Graph_TypeDefinitionLibrary;
struct GraphEdit_ResourceEditorBase;
struct GraphNode;

struct AudioFloat;
struct PcmData;

//

int getCurrentAudioGraphTraversalId();
void setCurrentAudioGraphTraversalId(int id);
void clearCurrentAudioGraphTraversalId();

//

struct AudioNodeDescription
{
	std::vector<std::string> lines;
	
	void add(const char * format, ...);
	
	void newline();
};

struct AudioNodeBase
{
	struct TriggerTarget
	{
		AudioNodeBase * srcNode;
		int srcSocketIndex;
		int dstSocketIndex;
		
		TriggerTarget();
	};
	
	std::vector<AudioPlug> inputs;
	std::vector<AudioPlug> outputs;
	
	std::vector<AudioNodeBase*> predeps;
	std::vector<TriggerTarget> triggerTargets;
	
	int lastTickTraversalId;
	bool editorIsTriggered; // only here for real-time connection with graph editor
	
	bool isPassthrough;
	bool isDeprecated;
	
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	int tickTimeAvg;
#endif
	
	AudioNodeBase();
	
	virtual ~AudioNodeBase()
	{
	}
	
	void traverseTick(const int traversalId, const float dt);
	
	void trigger(const int outputSocketIndex);
	
	void resizeSockets(const int numInputs, const int numOutputs)
	{
		inputs.resize(numInputs);
		outputs.resize(numOutputs);
	}
	
	void addInput(const int index, AudioPlugType type)
	{
		Assert(index >= 0 && index < (int)inputs.size());
		if (index >= 0 && index < (int)inputs.size())
		{
			inputs[index].type = type;
		}
	}
	
	void addOutput(const int index, AudioPlugType type, void * mem)
	{
		Assert(index >= 0 && index < (int)outputs.size());
		if (index >= 0 && index < (int)outputs.size())
		{
			outputs[index].type = type;
			outputs[index].mem = mem;
		}
	}
	
	AudioPlug * tryGetInput(const int index)
	{
		Assert(index >= 0 && index <= (int)inputs.size());
		if (index < 0 || index >= (int)inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	AudioPlug * tryGetOutput(const int index)
	{
		Assert(index >= 0 && index <= (int)outputs.size());
		if (index < 0 || index >= (int)outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	const AudioPlug * tryGetInput(const int index) const
	{
		Assert(index >= 0 && index <= (int)inputs.size());
		if (index < 0 || index >= (int)inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	const AudioPlug * tryGetOutput(const int index) const
	{
		Assert(index >= 0 && index <= (int)outputs.size());
		if (index < 0 || index >= (int)outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	bool getInputBool(const int index, const bool defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getBool();
	}
	
	int getInputInt(const int index, const int defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getInt();
	}
	
	float getInputFloat(const int index, const float defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getFloat();
	}
	
	const char * getInputString(const int index, const char * defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getString().c_str();
	}
	
	const AudioFloat * getInputAudioFloat(const int index, const AudioFloat * defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return &plug->getAudioFloat();
	}
	
	const PcmData * getInputPcmData(const int index) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getPcmData();
	}
	
	void queueTrigger(const int index)
	{
		AudioPlug * plug = tryGetInput(index);
		
		if (plug != nullptr)
			plug->isTriggered = true;
	}
	
	ALIGNED_AUDIO_NEW_AND_DELETE();
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void shut() { }
	virtual void tick(const float dt) { }
	virtual void handleTrigger(const int inputSocketIndex) { }
	
	virtual void getDescription(AudioNodeDescription & d) { }
	virtual bool getFilterResponse(float * magnitude, const int numSteps) const { return false; }
};

//

struct AudioEnumTypeRegistration
{
	struct Elem
	{
		std::string name;
		int value;
	};
	
	AudioEnumTypeRegistration * next;
	
	std::string enumName;
	int nextValue;
	
	std::vector<Elem> elems;
	
	AudioEnumTypeRegistration();
	
	void elem(const char * name, const int value = -1);
};

#define AUDIO_ENUM_TYPE(name) \
	struct name ## __audio_registration : AudioEnumTypeRegistration \
	{ \
		name ## __audio_registration() \
		{ \
			enumName = # name; \
			init(); \
		} \
		void init(); \
	}; \
	extern name ## __audio_registration name ## __audio_registrationInstance; \
	name ## __audio_registration name ## __audio_registrationInstance; \
	void name ## __audio_registration :: init()

//

struct AudioNodeTypeRegistration
{
	struct Input
	{
		std::string typeName;
		std::string name;
		std::string displayName;
		std::string enumName;
		std::string defaultValue;
	};
	
	struct Output
	{
		std::string typeName;
		std::string name;
		std::string displayName;
		bool isEditable;
		
		Output()
			: typeName()
			, name()
			, displayName()
			, isEditable(false)
		{
		}
	};
	
	AudioNodeTypeRegistration * next;
	
	AudioNodeBase* (*create)(void * data);
	void * createData;
	
	GraphEdit_ResourceEditorBase * (*createResourceEditor)(void * data);
	
	std::string typeName;
	std::string displayName;
	
	std::string resourceTypeName;
	
	std::vector<Input> inputs;
	std::vector<Output> outputs;
	
	AudioNodeTypeRegistration();
	
	void in(const char * name, const char * typeName, const char * defaultValue = "", const char * displayName = "");
	void inEnum(const char * name, const char * enumName, const char * defaultValue = "", const char * displayName = "");
	void out(const char * name, const char * typeName, const char * displayName = "");
	void outEditable(const char * name);
};

#define AUDIO_NODE_TYPE(type) \
	struct type ## __registration : AudioNodeTypeRegistration \
	{ \
		type ## __registration() \
		{ \
			create = [](void * data) -> AudioNodeBase* { return new type(); }; \
			init(); \
		} \
		void init(); \
	}; \
	extern type ## __registration type ## __registrationInstance; \
	type ## __registration type ## __registrationInstance; \
	void type ## __registration :: init()

extern AudioEnumTypeRegistration * g_audioEnumTypeRegistrationList;
extern AudioNodeTypeRegistration * g_audioNodeTypeRegistrationList;
