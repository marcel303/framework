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

#include "audioProfiling.h"
#include "Debugging.h"
#include "soundmix.h" // todo : remove
#include <string>
#include <vector>

#include <xmmintrin.h>

#define MULTIPLE_AUDIO_INPUT 1

struct GraphEdit_TypeDefinitionLibrary;
struct GraphNode;

struct AudioFloat;

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
	
	bool asBool(const bool defaultValue = false) const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return defaultValue;
		case kAudioTriggerDataType_Float:
			return floatValue != 0.f;
		}

		return defaultValue;
	}
	
	int asInt(const int defaultValue = 0) const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return defaultValue;
		case kAudioTriggerDataType_Float:
			return std::round(floatValue);
		}

		return defaultValue;
	}
	
	float asFloat(const float defaultValue = 0.f) const
	{
		switch (type)
		{
		case kAudioTriggerDataType_None:
			return defaultValue;
		case kAudioTriggerDataType_Float:
			return floatValue;
		}

		return defaultValue;
	}
};

struct AudioFloat
{
	static AudioFloat Zero;
	static AudioFloat One;
	static AudioFloat Half;
	
	bool isScalar;
	bool isExpanded;
	
	ALIGN16 float samples[AUDIO_UPDATE_SIZE];
	
	AudioFloat()
		: isScalar(true)
		, isExpanded(false)
	{
		samples[0] = 0.f;
	}
	
	AudioFloat(const float value)
		: isScalar(true)
		, isExpanded(false)
	{
		samples[0] = value;
	}
	
	void setScalar(const float value)
	{
		isScalar = true;
		isExpanded = false;
		
		samples[0] = value;
	}
	
	void setVector()
	{
		isScalar = false;
		isExpanded = true;
	}
	
	float getScalar() const
	{
		return samples[0];
	}
	
	float getMean() const
	{
		if (isScalar)
			return getScalar();
		else
		{
			// todo : add audioBuffer*** function to calculate the mean value of a buffer
			
		#if 0
			const __m128 * __restrict samples4 = (__m128*)samples;
			
			__m128 sum4 = _mm_setzero_ps();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE / 4; ++i)
			{
				sum4 += samples4[i];
			}
			
			__m128 x = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 y = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 z = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 w = _mm_shuffle_ps(sum4, sum4, _MM_SHUFFLE(3, 3, 3, 3));
			
			__m128 sum1 = _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, w));
			
			const float sum = _mm_cvtss_f32(_mm_mul_ps(sum1, _mm_set1_ps(1.f / AUDIO_UPDATE_SIZE)));
		#else
			float sum = 0.f;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				sum += samples[i];
			}
			
			sum /= AUDIO_UPDATE_SIZE;
		#endif
			
			return sum;
		}
	}
	
	void expand() const
	{
		AudioFloat * self = const_cast<AudioFloat*>(this);
		
		if (isScalar)
		{
			if (isExpanded == false)
			{
				self->isExpanded = true;
				
				for (int i = 1; i < AUDIO_UPDATE_SIZE; ++i)
					self->samples[i] = samples[0];
			}
		}
		else
		{
			Assert(isExpanded);
		}
	}
	
	void setZero();
	void setOne();
	void set(const AudioFloat & other);
	void setMul(const AudioFloat & other, const float gain);
	void setMul(const AudioFloat & other, const AudioFloat & gain);
	void add(const AudioFloat & other);
	void addMul(const AudioFloat & other, const float gain);
	void addMul(const AudioFloat & other, const AudioFloat & gain);
	void mul(const AudioFloat & other);
	void mulMul(const AudioFloat & other, const float gain);
	void mulMul(const AudioFloat & other, const AudioFloat & gain);
};

#if MULTIPLE_AUDIO_INPUT

struct AudioFloatArray
{
	struct Elem
	{
		AudioFloat* audioFloat;
		
		Elem()
			: audioFloat(nullptr)
		{
		}
	};
	
	AudioFloat * sum;
	std::vector<Elem> elems;
	AudioFloat * immediateValue;
	
	int lastUpdateTick;
	
	AudioFloatArray()
		: sum(nullptr)
		, elems()
		, immediateValue(nullptr)
		, lastUpdateTick(-1)
	{
	}
	
	~AudioFloatArray()
	{
		delete sum;
		sum = nullptr;
	}
	
	void update();
	
	AudioFloat * get();
};

#endif

enum AudioPlugType
{
	kAudioPlugType_None,
	kAudioPlugType_Bool,
	kAudioPlugType_Int,
	kAudioPlugType_Float,
	kAudioPlugType_String,
	kAudioPlugType_FloatVec,
	kAudioPlugType_PcmData,
	kAudioPlugType_Trigger
};

struct AudioPlug
{
	AudioPlugType type;
	void * mem;
	
#if MULTIPLE_AUDIO_INPUT
	mutable AudioFloatArray floatArray;
#endif

	AudioPlug()
		: type(kAudioPlugType_None)
		, mem(nullptr)
	#if MULTIPLE_AUDIO_INPUT
		, floatArray()
	#endif
	{
	}
	
	void connectTo(AudioPlug & dst);
	void connectTo(void * dstMem, const AudioPlugType dstType, const bool isImmediate);
	
	void disconnect()
	{
		mem = nullptr;
		
	#if MULTIPLE_AUDIO_INPUT
		floatArray.immediateValue = nullptr;
	#endif
	}
	
	bool isConnected() const
	{
		if (mem != nullptr)
			return true;
		
	#if MULTIPLE_AUDIO_INPUT
		if (floatArray.elems.empty() == false)
			return true;
		if (floatArray.immediateValue != nullptr)
			return true;
	#endif
	
		return false;
	}
	
	int getBool() const
	{
		Assert(type == kAudioPlugType_Bool);
		return *((bool*)mem);
	}
	
	int getInt() const
	{
		Assert(type == kAudioPlugType_Int);
		return *((int*)mem);
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
	
	const AudioFloat & getAudioFloat() const
	{
		Assert(type == kAudioPlugType_FloatVec);
	#if MULTIPLE_AUDIO_INPUT
		if (mem == nullptr)
			return *floatArray.get();
	#endif
		return *((AudioFloat*)mem);
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
	
	bool & getRwBool()
	{
		Assert(type == kAudioPlugType_Bool);
		return *((bool*)mem);
	}
	
	int & getRwInt()
	{
		Assert(type == kAudioPlugType_Int);
		return *((int*)mem);
	}
	
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
	
	AudioFloat & getRwAudioFloat()
	{
		Assert(type == kAudioPlugType_FloatVec);
	#if MULTIPLE_AUDIO_INPUT
		if (mem == nullptr)
			return *floatArray.get();
	#endif
		return *((AudioFloat*)mem);
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
	bool isDeprecated;
	
	int tickTimeAvg;
	
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
	
	int getInputBool(const int index, const bool defaultValue) const
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
	
	virtual void getDescription(AudioNodeDescription & d) { }
};

//

void createAudioValueTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary);

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
