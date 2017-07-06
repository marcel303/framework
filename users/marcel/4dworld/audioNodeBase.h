#pragma once

#include "audioProfiling.h"
#include "Debugging.h"
#include "soundmix.h"
#include <cmath>
#include <string>
#include <vector>

struct GraphEdit_TypeDefinitionLibrary;
struct GraphNode;

enum AudioTriggerDataType
{
	kAudioTriggerDataType_None,
	kAudioTriggerDataType_Float
};

struct AudioTriggerData
{
	AudioTriggerDataType type;
	
	union
	{
		float floatValue;
		uint8_t mem[8];
	};
	
	AudioTriggerData();
	
	void setFloat(const float value)
	{
		type = kAudioTriggerDataType_Float;
		floatValue = value;
	}
	
	bool asBool() const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return false;
		case kAudioTriggerDataType_Float:
			return floatValue != 0.f;
		}

		return false;
	}
	
	int asInt() const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return 0;
		case kAudioTriggerDataType_Float:
			return std::round(floatValue);
		}

		return 0;
	}
	
	float asFloat() const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return 0.f;
		case kAudioTriggerDataType_Float:
			return floatValue;
		}

		return 0.f;
	}
};

struct AudioBuffer
{
	ALIGN16 float samples[AUDIO_UPDATE_SIZE];
};

enum AudioPlugType
{
	kAudioPlugType_None,
	kAudioPlugType_Float,
	kAudioPlugType_String,
	kAudioPlugType_AudioBuffer,
	kAudioPlugType_PcmData,
	kAudioPlugType_Trigger
};

struct AudioPlug
{
	AudioPlugType type;
	void * mem;
	
	AudioPlug()
		: type(kAudioPlugType_None)
		, mem(nullptr)
	{
	}
	
	void connectTo(AudioPlug & dst);
	void connectTo(void * dstMem, const AudioPlugType dstType);
	
	void disconnect()
	{
		mem = nullptr;
	}
	
	bool isConnected() const
	{
		return mem != nullptr;
	}
	
	float getFloat() const
	{
		Assert(type == kAudioPlugType_Float);
		return *((float*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(type == kAudioPlugType_String);
		return *((std::string*)mem);
	}
	
	const AudioBuffer & getAudioBuffer() const
	{
		Assert(type == kAudioPlugType_AudioBuffer);
		return *((AudioBuffer*)mem);
	}
	
	const PcmData & getPcmData() const
	{
		Assert(type == kAudioPlugType_PcmData);
		return *((PcmData*)mem);
	}
	
	AudioTriggerData & getTriggerData() const
	{
		Assert(type == kAudioPlugType_Trigger);
		return *((AudioTriggerData*)mem);
	}
	
	//
	
	float & getRwFloat()
	{
		Assert(type == kAudioPlugType_Float);
		return *((float*)mem);
	}
	
	std::string & getRwString()
	{
		Assert(type == kAudioPlugType_String);
		return *((std::string*)mem);
	}
};

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
	int lastDrawTraversalId;
	bool editorIsTriggered; // only here for real-time connection with graph editor
	
	bool isPassthrough;
	
	int tickTimeAvg;
	int drawTimeAvg;
	
	AudioNodeBase();
	
	virtual ~AudioNodeBase()
	{
	}
	
	void traverseTick(const int traversalId, const float dt);
	void traverseDraw(const int traversalId);
	
	void trigger(const int outputSocketIndex);
	
	void resizeSockets(const int numInputs, const int numOutputs)
	{
		inputs.resize(numInputs);
		outputs.resize(numOutputs);
	}
	
	void addInput(const int index, AudioPlugType type)
	{
		Assert(index >= 0 && index < inputs.size());
		if (index >= 0 && index < inputs.size())
		{
			inputs[index].type = type;
		}
	}
	
	void addOutput(const int index, AudioPlugType type, void * mem)
	{
		Assert(index >= 0 && index < outputs.size());
		if (index >= 0 && index < outputs.size())
		{
			outputs[index].type = type;
			outputs[index].mem = mem;
		}
	}
	
	AudioPlug * tryGetInput(const int index)
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	AudioPlug * tryGetOutput(const int index)
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	const AudioPlug * tryGetInput(const int index) const
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	const AudioPlug * tryGetOutput(const int index) const
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
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
	
	const AudioBuffer * getInputAudioBuffer(const int index) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getAudioBuffer();
	}
	
	const PcmData * getInputPcmData(const int index) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getPcmData();
	}
	
	const AudioTriggerData * getInputTriggerData(const int index) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getTriggerData();
	}
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void tick(const float dt) { }
	virtual void handleTrigger(const int inputSocketIndex, const AudioTriggerData & data) { }
	virtual void draw() { }
	
	//virtual void getDescription(AudioNodeDescription & d) { }
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

extern AudioEnumTypeRegistration * g_audioEnumTypeRegistrationList;

void createAudioEnumTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, AudioEnumTypeRegistration * registrationList);

#define AUDIO_ENUM_TYPE(name) \
	struct name ## __registration : AudioEnumTypeRegistration \
	{ \
		name ## __registration() \
		{ \
			enumName = # name; \
			init(); \
		} \
		void init(); \
	}; \
	extern name ## __registration name ## __registrationInstance; \
	name ## __registration name ## __registrationInstance; \
	void name ## __registration :: init()

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
	
	AudioNodeBase* (*create)();
	
	std::string typeName;
	std::string displayName;
	
	std::string author;
	std::string copyright;
	std::string description;
	std::string helpText;
	
	std::vector<Input> inputs;
	std::vector<Output> outputs;
	
	AudioNodeTypeRegistration();
	
	void in(const char * name, const char * typeName, const char * defaultValue = "", const char * displayName = "");
	void inEnum(const char * name, const char * enumName, const char * defaultValue = "", const char * displayName = "");
	void out(const char * name, const char * typeName, const char * displayName = "");
	void outEditable(const char * name);
};

extern AudioNodeTypeRegistration * g_audioNodeTypeRegistrationList;

#define AUDIO_NODE_TYPE(name, type) \
	struct name ## __audio_registration : AudioNodeTypeRegistration \
	{ \
		name ## __audio_registration() \
		{ \
			create = []() -> AudioNodeBase* { return new type(); }; \
			init(); \
		} \
		void init(); \
	}; \
	extern name ## __audio_registration name ## __audio_registrationInstance; \
	name ## __audio_registration name ## __audio_registrationInstance; \
	void name ## __audio_registration :: init()

void createAudioNodeTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, AudioNodeTypeRegistration * registrationList);

//

struct AudioNodeDisplay : AudioNodeBase
{
	enum Input
	{
		kInput_AudioBuffer,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioBuffer audioBufferOutput;
	
	AudioNodeDisplay()
		: AudioNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_AudioBuffer, kAudioPlugType_AudioBuffer);
	}
	
	const AudioBuffer & getAudioBuffer() const
	{
		return audioBufferOutput;
	}
	
	virtual void draw() override
	{
		const AudioBuffer * audioBuffer = getInputAudioBuffer(kInput_AudioBuffer);
		
		if (audioBuffer == nullptr)
		{
			memset(&audioBufferOutput, 0, sizeof(audioBufferOutput));
		}
		else
		{
			audioBufferOutput = *audioBuffer;
		}
	}
};

struct AudioNodePcmData : AudioNodeBase
{
	enum Input
	{
		kInput_Filename,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_PcmData,
		kOutput_COUNT
	};
	
	PcmData pcmData;
	
	AudioNodePcmData()
		: AudioNodeBase()
		, pcmData()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Filename, kAudioPlugType_String);
		addOutput(kOutput_PcmData, kAudioPlugType_PcmData, &pcmData);
	}
};

struct AudioNodeSourcePcm : AudioNodeBase
{
	enum Input
	{
		kInput_PcmData,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioBuffer,
		kOutput_COUNT
	};
	
	AudioSourcePcm audioSource;
	AudioBuffer audioBufferOutput;
	
	AudioNodeSourcePcm()
		: AudioNodeBase()
		, audioSource()
		, audioBufferOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_PcmData, kAudioPlugType_PcmData);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
	}
	
	virtual void draw() override
	{
		audioSource.generate(audioBufferOutput.samples, AUDIO_UPDATE_SIZE);
	}
};

struct AudioNodeSourceMix : AudioNodeBase
{
	enum Input
	{
		kInput_Source1,
		kInput_Gain1,
		kInput_Source2,
		kInput_Gain2,
		kInput_Source3,
		kInput_Gain3,
		kInput_Source4,
		kInput_Gain4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioBuffer,
		kOutput_COUNT
	};
	
	AudioSourceMix audioSource;
	AudioBuffer audioBufferOutput;
	
	AudioNodeSourceMix()
		: AudioNodeBase()
		, audioSource()
		, audioBufferOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Gain1, kAudioPlugType_Float);
		addInput(kInput_Source1, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain2, kAudioPlugType_Float);
		addInput(kInput_Source2, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain3, kAudioPlugType_Float);
		addInput(kInput_Source3, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain4, kAudioPlugType_Float);
		addInput(kInput_Source4, kAudioPlugType_AudioBuffer);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
	}
};
