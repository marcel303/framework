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

struct AudioFloat
{
	static AudioFloat Zero;
	static AudioFloat One;
	
	bool isScalar;
	bool isExpanded;
	
	ALIGN16 float samples[AUDIO_UPDATE_SIZE];
	
	AudioFloat()
		: isScalar(false)
		, isExpanded(true)
	{
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
	
	float getValue(const int index) const
	{
		return isScalar ? samples[0] : samples[index];
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
	void set(const AudioFloat & other);
	void setMul(const AudioFloat & other, const float gain);
	void setMul(const AudioFloat & other, const AudioFloat & gain);
	void add(const AudioFloat & other);
	void addMul(const AudioFloat & other, const float gain);
	void addMul(const AudioFloat & other, const AudioFloat & gain);
	void mulMul(const AudioFloat & other, const float gain);
	void mulMul(const AudioFloat & other, const AudioFloat & gain);
};

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
	
	const AudioFloat & getAudioFloat() const
	{
		Assert(type == kAudioPlugType_FloatVec);
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

struct AudioOutputChannel
{
	float * samples;
	int stride;
	
	AudioOutputChannel()
		: samples(nullptr)
		, stride(0)
	{
	}
};

struct AudioNodeDisplay : AudioNodeBase
{
	enum Input
	{
		kInput_AudioL,
		kInput_AudioR,
		kInput_Gain,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	AudioOutputChannel * outputChannelL;
	AudioOutputChannel * outputChannelR;
	
	AudioNodeDisplay()
		: AudioNodeBase()
		, outputChannelL(nullptr)
		, outputChannelR(nullptr)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_AudioL, kAudioPlugType_FloatVec);
		addInput(kInput_AudioR, kAudioPlugType_FloatVec);
		addInput(kInput_Gain, kAudioPlugType_Float);
	}
	
	virtual void draw() override
	{
		const AudioFloat * audioL = getInputAudioFloat(kInput_AudioL, nullptr);
		const AudioFloat * audioR = getInputAudioFloat(kInput_AudioR, nullptr);
		const float gain = getInputFloat(kInput_Gain, 1.f);
	
		if (outputChannelL != nullptr)
		{
			if (audioL == nullptr)
			{
				float * channelPtr = outputChannelL->samples;
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelL->stride)
					*channelPtr = 0.f;
			}
			else
			{
				audioL->expand();
				
				float * channelPtr = outputChannelL->samples;
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelL->stride)
					*channelPtr = audioL->samples[i] * gain;
			}
		}
		
		if (outputChannelR != nullptr)
		{
			if (audioR == nullptr)
			{
				float * channelPtr = outputChannelR->samples;
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelR->stride)
					*channelPtr = 0.f;
			}
			else
			{
				audioR->expand();
				
				float * channelPtr = outputChannelR->samples;
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += outputChannelR->stride)
					*channelPtr = audioR->samples[i] * gain;
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
		kOutput_Audio,
		kOutput_Length,
		kOutput_COUNT
	};
	
	AudioSourcePcm audioSource;
	AudioFloat audioOutput;
	float lengthOutput;
	
	AudioNodeSourcePcm()
		: AudioNodeBase()
		, audioSource()
		, audioOutput()
		, lengthOutput(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_PcmData, kAudioPlugType_PcmData);
		addInput(kInput_RangeBegin, kAudioPlugType_FloatVec);
		addInput(kInput_RangeLength, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
		addOutput(kOutput_Length, kAudioPlugType_Float, &lengthOutput);
		
		audioSource.play();
	}
	
	virtual void tick(const float dt) override
	{
		const PcmData * pcmData = getInputPcmData(kInput_PcmData);
		const AudioFloat * rangeBegin = getInputAudioFloat(kInput_RangeBegin, nullptr);
		const AudioFloat * rangeLength = getInputAudioFloat(kInput_RangeLength, nullptr);
		
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
		audioSource.generate(audioOutput.samples, AUDIO_UPDATE_SIZE);
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
	
	virtual void draw() override
	{
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
					audioOutput.set(*input.source);
				else
					audioOutput.setMul(*input.source, gain);
			}
			else
			{
				// todo : do this on a per-sample basis !
				
				const float gain = input.gain->getValue(0) * gainScale;
				
				audioOutput.addMul(*input.source, gain);
			}
		}
	}
};

struct AudioNodeSourceSine : AudioNodeBase
{
	enum Mode
	{
		kMode_SNorm,
		kMode_UNorm
	};
	
	enum Input
	{
		kInput_Mode,
		kInput_Frequency,
		kInput_Phase,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	float phase;
	
	AudioNodeSourceSine()
		: AudioNodeBase()
		, audioOutput()
		, phase(0.0)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Mode, kAudioPlugType_Int);
		addInput(kInput_Frequency, kAudioPlugType_Float);
		addInput(kInput_Phase, kAudioPlugType_Float);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	}
	
	virtual void tick(const float dt) override
	{
	}
	
	virtual void draw() override
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		const float phaseOffset = getInputFloat(kInput_Phase, 0.f);
		const float phaseStep = frequency / double(SAMPLE_RATE);
		const float twoPi = 2.f * M_PI;
		
		if (mode == kMode_SNorm)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				audioOutput.samples[i] = std::sinf((phase + phaseOffset) * twoPi);
				
				phase += phaseStep;
				phase = std::fmodf(phase, 1.f);
			}
		}
		else if (mode == kMode_UNorm)
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				audioOutput.samples[i] = .5f + .5f * std::sinf((phase + phaseOffset) * twoPi);
				
				phase += phaseStep;
				phase = std::fmodf(phase, 1.f);
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
	
	virtual void draw() override
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
	
	AudioFloat resultOutput;
	
	AudioNodeMapRange()
		: AudioNodeBase()
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kAudioPlugType_FloatVec);
		addInput(kInput_InMin, kAudioPlugType_FloatVec);
		addInput(kInput_InMax, kAudioPlugType_FloatVec);
		addInput(kInput_OutMin, kAudioPlugType_FloatVec);
		addInput(kInput_OutMax, kAudioPlugType_FloatVec);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
	}
	
	virtual void draw()
	{
		const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
		const AudioFloat * inMin = getInputAudioFloat(kInput_InMin, &AudioFloat::Zero);
		const AudioFloat * inMax = getInputAudioFloat(kInput_InMax, &AudioFloat::One);
		const AudioFloat * outMin = getInputAudioFloat(kInput_OutMin, &AudioFloat::Zero);
		const AudioFloat * outMax = getInputAudioFloat(kInput_OutMax, &AudioFloat::One);
		
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
	
	AudioFloat resultOutput;
	
	AudioNodeTime()
		: AudioNodeBase()
		, time(0.0)
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_FineGrained, kAudioPlugType_Bool);
		addInput(kInput_Scale, kAudioPlugType_Float);
		addInput(kInput_Offset, kAudioPlugType_Float);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
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
	
	AudioFloat resultOutput;
	
	AudioNodePhase()
		: AudioNodeBase()
		, phase(0.0)
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_FineGrained, kAudioPlugType_Bool);
		addInput(kInput_Frequency, kAudioPlugType_FloatVec);
		addInput(kInput_PhaseOffset, kAudioPlugType_FloatVec);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
	}
	
	virtual void draw()
	{
		const bool fineGrained = getInputBool(kInput_FineGrained, true);
		const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::Zero);
		const AudioFloat * phaseOffset = getInputAudioFloat(kInput_PhaseOffset, &AudioFloat::Zero);
		
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
	
	AudioFloat resultOutput;
	
	AudioNodeMathSine()
		: AudioNodeBase()
		, resultOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kAudioPlugType_FloatVec);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
	}
	
	virtual void draw()
	{
		const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
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

// perform the computation needed to get the gain values for two audio sources, given various parameters
struct AudioNodePan2
{
};

struct AudioNodeBinauralize
{
};
