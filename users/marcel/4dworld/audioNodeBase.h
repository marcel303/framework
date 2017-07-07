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

struct GraphEdit_TypeDefinitionLibrary;
struct GraphNode;

struct AudioBuffer;
struct AudioValue;

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
	
	void setZero();
	void set(const AudioBuffer & other);
	void setMul(const AudioBuffer & other, const float gain);
	void setMul(const AudioBuffer & other, const AudioValue & gain);
	void add(const AudioBuffer & other);
	void addMul(const AudioBuffer & other, const float gain);
	void addMul(const AudioBuffer & other, const AudioValue & gain);
	void mulMul(const AudioBuffer & other, const float gain);
	void mulMul(const AudioBuffer & other, const AudioValue & gain);
};

struct AudioValue
{
	static AudioValue Zero;
	static AudioValue One;
	
	bool isScalar;
	
	float samples[AUDIO_UPDATE_SIZE];
	
	AudioValue()
		: isScalar(false)
	{
	}
	
	AudioValue(const double value)
		: isScalar(true)
	{
		samples[0] = value;
	}
	
	void setScalar(const float value)
	{
		isScalar = true;
		samples[0] = value;
	}
	
	void setVector()
	{
		isScalar = false;
	}
	
	float getValue(const int index) const
	{
		return isScalar ? samples[0] : samples[index];
	}
};

enum AudioPlugType
{
	kAudioPlugType_None,
	kAudioPlugType_Bool,
	kAudioPlugType_Int,
	kAudioPlugType_Float,
	kAudioPlugType_String,
	kAudioPlugType_AudioBuffer,
	kAudioPlugType_AudioValue,
	kAudioPlugType_PcmData,
	kAudioPlugType_Trigger
};

struct AudioPlug
{
	AudioPlugType type;
	void * mem;
	
	int referencedByRealTimeConnectionTick;
	
	AudioPlug()
		: type(kAudioPlugType_None)
		, mem(nullptr)
		, referencedByRealTimeConnectionTick(-1)
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
	
	const AudioBuffer & getAudioBuffer() const
	{
		Assert(type == kAudioPlugType_AudioBuffer);
		return *((AudioBuffer*)mem);
	}
	
	const AudioValue & getAudioValue() const
	{
		Assert(type == kAudioPlugType_AudioValue);
		return *((AudioValue*)mem);
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
	
	AudioValue & getRwAudioValue()
	{
		Assert(type == kAudioPlugType_AudioValue);
		return *((AudioValue*)mem);
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
	
	const AudioBuffer * getInputAudioBuffer(const int index) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getAudioBuffer();
	}
	
	const AudioValue * getInputAudioValue(const int index, const AudioValue * defaultValue) const
	{
		const AudioPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return &plug->getAudioValue();
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
	
	AudioBuffer * outputBuffer;
	
	AudioNodeDisplay()
		: AudioNodeBase()
		, outputBuffer(nullptr)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_AudioBuffer, kAudioPlugType_AudioBuffer);
	}
	
	virtual void draw() override
	{
		if (outputBuffer != nullptr)
		{
			const AudioBuffer * audioBuffer = getInputAudioBuffer(kInput_AudioBuffer);
			
			if (audioBuffer == nullptr)
			{
				outputBuffer->setZero();
			}
			else
			{
				outputBuffer->set(*audioBuffer);
			}
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
	
	std::string currentFilename;
	
	AudioNodePcmData()
		: AudioNodeBase()
		, pcmData()
		, currentFilename()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Filename, kAudioPlugType_String);
		addOutput(kOutput_PcmData, kAudioPlugType_PcmData, &pcmData);
	}
	
	virtual void tick(const float dt) override;
};

struct AudioNodeSourcePcm : AudioNodeBase
{
	enum Input
	{
		kInput_PcmData,
		kInput_RangeBegin,
		kInput_RangeLength,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioBuffer,
		kOutput_Length,
		kOutput_COUNT
	};
	
	AudioSourcePcm audioSource;
	AudioBuffer audioBufferOutput;
	float lengthOutput;
	
	AudioNodeSourcePcm()
		: AudioNodeBase()
		, audioSource()
		, audioBufferOutput()
		, lengthOutput(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_PcmData, kAudioPlugType_PcmData);
		addInput(kInput_RangeBegin, kAudioPlugType_AudioValue);
		addInput(kInput_RangeLength, kAudioPlugType_AudioValue);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
		addOutput(kOutput_Length, kAudioPlugType_Float, &lengthOutput);
		
		audioSource.play();
	}
	
	virtual void tick(const float dt) override
	{
		const PcmData * pcmData = getInputPcmData(kInput_PcmData);
		const AudioValue * rangeBegin = getInputAudioValue(kInput_RangeBegin, nullptr);
		const AudioValue * rangeLength = getInputAudioValue(kInput_RangeLength, nullptr);
		
		if (pcmData != audioSource.pcmData)
		{
			audioSource.init(pcmData, 0);
		}
		
		if (rangeBegin == nullptr || rangeLength == nullptr)
		{
			audioSource.clearRange();
		}
		else
		{
			const int _rangeBegin = rangeBegin->getValue(0) * SAMPLE_RATE;
			const int _rangeLength = rangeLength->getValue(0) * SAMPLE_RATE;
			
			audioSource.setRange(_rangeBegin, _rangeLength);
		}
		
		if (pcmData == nullptr)
		{
			lengthOutput = 0.f;
		}
		else
		{
			lengthOutput = pcmData->numSamples / float(SAMPLE_RATE);
		}
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
	
	AudioBuffer audioBufferOutput;
	
	AudioNodeSourceMix()
		: AudioNodeBase()
		, audioBufferOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source1, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain1, kAudioPlugType_AudioValue);
		addInput(kInput_Source2, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain2, kAudioPlugType_AudioValue);
		addInput(kInput_Source3, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain3, kAudioPlugType_AudioValue);
		addInput(kInput_Source4, kAudioPlugType_AudioBuffer);
		addInput(kInput_Gain4, kAudioPlugType_AudioValue);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
	}
	
	virtual void draw() override
	{
		const AudioBuffer * source1 = getInputAudioBuffer(kInput_Source1);
		const AudioBuffer * source2 = getInputAudioBuffer(kInput_Source2);
		const AudioBuffer * source3 = getInputAudioBuffer(kInput_Source3);
		const AudioBuffer * source4 = getInputAudioBuffer(kInput_Source4);
		
		const AudioValue * gain1 = getInputAudioValue(kInput_Gain1, &AudioValue::One);
		const AudioValue * gain2 = getInputAudioValue(kInput_Gain2, &AudioValue::One);
		const AudioValue * gain3 = getInputAudioValue(kInput_Gain3, &AudioValue::One);
		const AudioValue * gain4 = getInputAudioValue(kInput_Gain4, &AudioValue::One);
		
		const bool normalizeGain = false;
		
		struct Input
		{
			const AudioBuffer * source;
			const AudioValue * gain;
			
			void set(const AudioBuffer * _source, const AudioValue * _gain)
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
				audioBufferOutput.samples[i] = 0.f;
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
				
				totalGain += input.gain->getValue(0);
			}
			
			if (totalGain > 0.f)
			{
				gainScale = 1.f / totalGain;
			}
		}
		
		for (int i = 0; i < numInputs; ++i)
		{
			auto & input = inputs[i];
			
			// todo : do this on a per-sample basis !
			
			const float gain = input.gain->getValue(0) * gainScale;
			
			if (isFirst)
			{
				isFirst = false;
				
				if (gain == 1.f)
					audioBufferOutput.set(*input.source);
				else
					audioBufferOutput.setMul(*input.source, gain);
			}
			else
			{
				// todo : do this on a per-sample basis !
				
				const float gain = input.gain->getValue(0) * gainScale;
				
				audioBufferOutput.addMul(*input.source, gain);
			}
		}
	}
};

struct AudioNodeSourceSine : AudioNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Phase,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioBuffer,
		kOutput_COUNT
	};
	
	AudioBuffer audioBufferOutput;
	
	float phase;
	
	AudioNodeSourceSine()
		: AudioNodeBase()
		, audioBufferOutput()
		, phase(0.0)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Frequency, kAudioPlugType_Float);
		addInput(kInput_Phase, kAudioPlugType_Float);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
	}
	
	virtual void tick(const float dt) override
	{
	}
	
	virtual void draw() override
	{
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		const float phaseOffset = getInputFloat(kInput_Phase, 0.f);
		const float phaseStep = frequency / double(SAMPLE_RATE);
		const float twoPi = 2.f * M_PI;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			audioBufferOutput.samples[i] = std::sinf((phase + phaseOffset) * twoPi);
			
			phase += phaseStep;
			phase = std::fmodf(phase, 1.f);
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
		kInput_AudioBufferA,
		kInput_GainA,
		kInput_AudioBufferB,
		kInput_GainB,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_AudioBuffer,
		kOutput_COUNT
	};
	
	AudioBuffer audioBufferOutput;
	
	AudioNodeMix()
		: AudioNodeBase()
		, audioBufferOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Mode, kAudioPlugType_Int);
		addInput(kInput_AudioBufferA, kAudioPlugType_AudioBuffer);
		addInput(kInput_GainA, kAudioPlugType_AudioValue);
		addInput(kInput_AudioBufferB, kAudioPlugType_AudioBuffer);
		addInput(kInput_GainB, kAudioPlugType_AudioValue);
		addOutput(kOutput_AudioBuffer, kAudioPlugType_AudioBuffer, &audioBufferOutput);
	}
	
	virtual void draw() override
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		const AudioBuffer * audioBufferA = getInputAudioBuffer(kInput_AudioBufferA);
		const AudioValue * gainA = getInputAudioValue(kInput_GainA, &AudioValue::One);
		const AudioBuffer * audioBufferB = getInputAudioBuffer(kInput_AudioBufferB);
		const AudioValue * gainB = getInputAudioValue(kInput_GainB, &AudioValue::One);
		
		if (audioBufferA == nullptr || audioBufferB == nullptr)
		{
			if (audioBufferA != nullptr)
			{
				audioBufferOutput.setMul(*audioBufferA, *gainA);
			}
			else if (audioBufferB != nullptr)
			{
				audioBufferOutput.setMul(*audioBufferB, *gainB);
			}
			else
			{
				audioBufferOutput.setZero();
			}
		}
		else
		{
			if (mode == kMode_Add)
			{
				audioBufferOutput.setMul(*audioBufferA, *gainA);
				audioBufferOutput.addMul(*audioBufferB, *gainB);
			}
			else if (mode == kMode_Mul)
			{
				audioBufferOutput.setMul(*audioBufferA, *gainA);
				audioBufferOutput.mulMul(*audioBufferB, *gainB);
			}
			else
			{
				Assert(false);
			}
		}
	}
};

struct AudioNodeMapRange : AudioNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_InMin,
		kInput_InMax,
		kInput_OutMin,
		kInput_OutMax,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	AudioValue resultOutput;
	
	AudioNodeMapRange()
		: AudioNodeBase()
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kAudioPlugType_AudioValue);
		addInput(kInput_InMin, kAudioPlugType_AudioValue);
		addInput(kInput_InMax, kAudioPlugType_AudioValue);
		addInput(kInput_OutMin, kAudioPlugType_AudioValue);
		addInput(kInput_OutMax, kAudioPlugType_AudioValue);
		addOutput(kOutput_Result, kAudioPlugType_AudioValue, &resultOutput);
	}
	
	virtual void draw()
	{
		const AudioValue * value = getInputAudioValue(kInput_Value, &AudioValue::Zero);
		const AudioValue * inMin = getInputAudioValue(kInput_InMin, &AudioValue::Zero);
		const AudioValue * inMax = getInputAudioValue(kInput_InMax, &AudioValue::One);
		const AudioValue * outMin = getInputAudioValue(kInput_OutMin, &AudioValue::Zero);
		const AudioValue * outMax = getInputAudioValue(kInput_OutMax, &AudioValue::One);
		
		const bool scalarRange =
			inMin->isScalar &&
			inMax->isScalar &&
			outMin->isScalar &&
			outMax->isScalar;
		
		if (scalarRange)
		{
			const float _inMin = inMin->getValue(0);
			const float _inMax = inMax->getValue(0);
			const float _outMin = outMin->getValue(0);
			const float _outMax = outMax->getValue(0);
			const float scale = 1.f / (_inMax - _inMin);
			
			if (value->isScalar)
			{
				const float t2 = (value->getValue(0) - _inMin) * scale;
				const float t1 = 1.f - t2;
				const float result = t1 * _outMin + t2 * _outMax;
				
				resultOutput.setScalar(result);
			}
			else
			{
				resultOutput.setVector();
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					const float t2 = (value->samples[i] - _inMin) * scale;
					const float t1 = 1.f - t2;
					const float result = t1 * _outMin + t2 * _outMax;
					
					resultOutput.samples[i] = result;
				}
			}
		}
		else
		{
			resultOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				const float _inMin = inMin->getValue(i);
				const float _inMax = inMax->getValue(i);
				const float _outMin = outMin->getValue(i);
				const float _outMax = outMax->getValue(i);
				const float scale = 1.f / (_inMax - _inMin);
				const float _value = value->getValue(i);
				
				const float t2 = (_value - _inMin) * scale;
				const float t1 = 1.f - t2;
				const float result = t1 * _outMin + t2 * _outMax;
				
				resultOutput.samples[i] = result;
			}
		}
	}
};

struct AudioNodeTime : AudioNodeBase
{
	enum Input
	{
		kInput_FineGrained,
		kInput_Scale,
		kInput_Offset,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	double time;
	
	AudioValue resultOutput;
	
	AudioNodeTime()
		: AudioNodeBase()
		, time(0.0)
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_FineGrained, kAudioPlugType_Bool);
		addInput(kInput_Scale, kAudioPlugType_Float);
		addInput(kInput_Offset, kAudioPlugType_Float);
		addOutput(kOutput_Result, kAudioPlugType_AudioValue, &resultOutput);
	}
	
	virtual void draw()
	{
		const bool fineGrained = getInputBool(kInput_FineGrained, true);
		const float scale = getInputFloat(kInput_Scale, 1.f);
		const float offset = getInputFloat(kInput_Offset, 0.f);
		
		if (fineGrained)
		{
			resultOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				resultOutput.samples[i] = time * scale + offset;
				
				time += 1.0 / SAMPLE_RATE;
			}
		}
		else
		{
			resultOutput.setScalar(time * scale + offset);
			
			time += AUDIO_UPDATE_SIZE / double(SAMPLE_RATE);
		}
	}
};

struct AudioNodePhase : AudioNodeBase
{
	enum Input
	{
		kInput_FineGrained,
		kInput_Frequency,
		kInput_PhaseOffset,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	float phase;
	
	AudioValue resultOutput;
	
	AudioNodePhase()
		: AudioNodeBase()
		, phase(0.0)
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_FineGrained, kAudioPlugType_Bool);
		addInput(kInput_Frequency, kAudioPlugType_AudioValue);
		addInput(kInput_PhaseOffset, kAudioPlugType_AudioValue);
		addOutput(kOutput_Result, kAudioPlugType_AudioValue, &resultOutput);
	}
	
	virtual void draw()
	{
		const bool fineGrained = getInputBool(kInput_FineGrained, true);
		const AudioValue * frequency = getInputAudioValue(kInput_Frequency, &AudioValue::Zero);
		const AudioValue * phaseOffset = getInputAudioValue(kInput_PhaseOffset, &AudioValue::Zero);
		
		const bool scalarInputs = frequency->isScalar && phaseOffset->isScalar;
		
		if (fineGrained)
		{
			resultOutput.setVector();
			
			if (scalarInputs)
			{
				const float _frequency = frequency->getValue(0);
				const float _phaseOffset = phaseOffset->getValue(0);
				const float phaseStep = _frequency / SAMPLE_RATE;
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					resultOutput.samples[i] = phase + _phaseOffset;
					
					phase += phaseStep;
				}
			}
			else
			{
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					const float _frequency = frequency->getValue(i);
					const float _phaseOffset = phaseOffset->getValue(i);
					
					resultOutput.samples[i] = phase + _phaseOffset;
					
					phase += _frequency / SAMPLE_RATE;
				}
			}
		}
		else
		{
			const float _frequency = frequency->getValue(0);
			const float _phaseOffset = phaseOffset->getValue(0);
			
			resultOutput.setScalar(phase + _phaseOffset);
			
			phase += _frequency / SAMPLE_RATE * AUDIO_UPDATE_SIZE;
		}
		
		phase = std::fmodf(phase, 1.f);
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
	
	AudioValue resultOutput;
	
	AudioNodeMathSine()
		: AudioNodeBase()
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kAudioPlugType_AudioValue);
		addOutput(kOutput_Result, kAudioPlugType_AudioValue, &resultOutput);
	}
	
	virtual void draw()
	{
		const AudioValue * value = getInputAudioValue(kInput_Value, &AudioValue::Zero);
		const float twoPi = 2.f * M_PI;
		
		if (value->isScalar)
		{
			resultOutput.setScalar(std::sinf(value->samples[0] * twoPi));
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
