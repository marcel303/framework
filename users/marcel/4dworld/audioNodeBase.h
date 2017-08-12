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
#include "soundmix.h"
#include <cmath>
#include <string>
#include <vector>

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
			float sum = 0.f;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				sum += samples[i];
			}
			
			sum /= AUDIO_UPDATE_SIZE;
			
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

//

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
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	AudioNodeSourceMix()
		: AudioNodeBase()
		, audioOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source1, kAudioPlugType_FloatVec);
		addInput(kInput_Gain1, kAudioPlugType_FloatVec);
		addInput(kInput_Source2, kAudioPlugType_FloatVec);
		addInput(kInput_Gain2, kAudioPlugType_FloatVec);
		addInput(kInput_Source3, kAudioPlugType_FloatVec);
		addInput(kInput_Gain3, kAudioPlugType_FloatVec);
		addInput(kInput_Source4, kAudioPlugType_FloatVec);
		addInput(kInput_Gain4, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	}
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			audioOutput.setScalar(0.f);
			
			return;
		}
		
		const AudioFloat * source1 = getInputAudioFloat(kInput_Source1, nullptr);
		const AudioFloat * source2 = getInputAudioFloat(kInput_Source2, nullptr);
		const AudioFloat * source3 = getInputAudioFloat(kInput_Source3, nullptr);
		const AudioFloat * source4 = getInputAudioFloat(kInput_Source4, nullptr);
		
		const AudioFloat * gain1 = getInputAudioFloat(kInput_Gain1, &AudioFloat::One);
		const AudioFloat * gain2 = getInputAudioFloat(kInput_Gain2, &AudioFloat::One);
		const AudioFloat * gain3 = getInputAudioFloat(kInput_Gain3, &AudioFloat::One);
		const AudioFloat * gain4 = getInputAudioFloat(kInput_Gain4, &AudioFloat::One);
		
		const bool normalizeGain = false;
		
		struct Input
		{
			const AudioFloat * source;
			const AudioFloat * gain;
			
			void set(const AudioFloat * _source, const AudioFloat * _gain)
			{
				source = _source;
				gain = _gain;
			}
		};
		
		Input inputs[4];
		int numInputs = 0;
		
		if (source1 != nullptr)
			inputs[numInputs++].set(source1, gain1);
		if (source2 != nullptr)
			inputs[numInputs++].set(source2, gain2);
		if (source3 != nullptr)
			inputs[numInputs++].set(source3, gain3);
		if (source4 != nullptr)
			inputs[numInputs++].set(source4, gain4);
		
		if (numInputs == 0)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				audioOutput.samples[i] = 0.f;
			return;
		}
		
		bool isFirst = true;
		
		float gainScale = 1.f;
		
		if (normalizeGain)
		{
			float totalGain = 0.f;
			
			for (auto & input : inputs)
			{
				// todo : do this on a per-sample basis !
				
				totalGain += input.gain->getScalar();
			}
			
			if (totalGain > 0.f)
			{
				gainScale = 1.f / totalGain;
			}
		}
		
		for (int i = 0; i < numInputs; ++i)
		{
			auto & input = inputs[i];
			
			input.source->expand();
			
			// todo : do this on a per-sample basis !
			
			const float gain = input.gain->getScalar() * gainScale;
			
			if (isFirst)
			{
				isFirst = false;
				
				if (gain == 1.f)
					audioOutput.set(*input.source);
				else
					audioOutput.setMul(*input.source, gain);
			}
			else
			{
				// todo : do this on a per-sample basis !
				
				const float gain = input.gain->getScalar() * gainScale;
				
				audioOutput.addMul(*input.source, gain);
			}
		}
	}
};

struct AudioNodeMix : AudioNodeBase
{
	enum Mode
	{
		kMode_Add,
		kMode_Mul,
	};
	
	enum Input
	{
		kInput_Mode,
		kInput_AudioA,
		kInput_GainA,
		kInput_AudioB,
		kInput_GainB,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	AudioNodeMix()
		: AudioNodeBase()
		, audioOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Mode, kAudioPlugType_Int);
		addInput(kInput_AudioA, kAudioPlugType_FloatVec);
		addInput(kInput_GainA, kAudioPlugType_FloatVec);
		addInput(kInput_AudioB, kAudioPlugType_FloatVec);
		addInput(kInput_GainB, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		const AudioFloat * audioA = getInputAudioFloat(kInput_AudioA, nullptr);
		const AudioFloat * gainA = getInputAudioFloat(kInput_GainA, &AudioFloat::One);
		const AudioFloat * audioB = getInputAudioFloat(kInput_AudioB, nullptr);
		const AudioFloat * gainB = getInputAudioFloat(kInput_GainB, &AudioFloat::One);
		
		if (audioA == nullptr || audioB == nullptr)
		{
			if (audioA != nullptr)
			{
				audioOutput.setMul(*audioA, *gainA);
			}
			else if (audioB != nullptr)
			{
				audioOutput.setMul(*audioB, *gainB);
			}
			else
			{
				audioOutput.setZero();
			}
		}
		else
		{
			if (mode == kMode_Add)
			{
				audioOutput.setMul(*audioA, *gainA);
				audioOutput.addMul(*audioB, *gainB);
			}
			else if (mode == kMode_Mul)
			{
				audioOutput.setMul(*audioA, *gainA);
				audioOutput.mulMul(*audioB, *gainB);
			}
			else
			{
				Assert(false);
			}
		}
	}
};

struct AudioNodeMathSine : AudioNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	AudioFloat resultOutput;
	
	AudioNodeMathSine()
		: AudioNodeBase()
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kAudioPlugType_FloatVec);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
		const float twoPi = 2.f * M_PI;
		
		if (value->isScalar)
		{
			resultOutput.setScalar(std::sinf(value->getScalar() * twoPi));
		}
		else
		{
			resultOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				resultOutput.samples[i] = std::sinf(value->samples[i] * twoPi);
			}
		}
	}
};
