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

#include "audioGraph.h"
#include "audioNodeBase.h"
#include "framework.h"
#include "graph.h"
#include "soundmix.h"
#include "StringEx.h"
#include "Timer.h"
#include <string.h>
#include <xmmintrin.h>

//

AudioFloat AudioFloat::Zero(0.0);
AudioFloat AudioFloat::One(1.0);
AudioFloat AudioFloat::Half(0.5);

float AudioFloat::getMean() const
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

void AudioFloat::expand() const
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

void AudioFloat::setZero()
{
	setScalar(0.f);
}

void AudioFloat::setOne()
{
	setScalar(1.f);
}

void AudioFloat::set(const AudioFloat & other)
{
	if (other.isScalar)
	{
		setScalar(other.getScalar());
	}
	else
	{
		setVector();
		
		memcpy(samples, other.samples, AUDIO_UPDATE_SIZE * sizeof(float));
	}
}

void AudioFloat::setMul(const AudioFloat & other, const float gain)
{
	if (other.isScalar)
	{
		setScalar(other.getScalar());
	}
	else
	{
		setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] = other.samples[i] * gain;
	}
}

void AudioFloat::setMul(const AudioFloat & other, const AudioFloat & gain)
{
	if (other.isScalar && gain.isScalar)
	{
		setScalar(other.getScalar() * gain.getScalar());
	}
	else if (gain.isScalar)
	{
		Assert(other.isScalar == false);
		
		setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] = other.samples[i] * gain.getScalar();
	}
	else
	{
		Assert(other.isScalar == false && gain.isScalar == false);
		
		setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] = other.samples[i] * gain.samples[i];
	}
}

void AudioFloat::add(const AudioFloat & other)
{
	if (isScalar && other.isScalar)
	{
		setScalar(getScalar() + other.getScalar());
	}
	else
	{
		other.expand();
		
		expand();
		
		//
		
		setVector();
		
		audioBufferAdd(samples, other.samples, AUDIO_UPDATE_SIZE);
	}
}

void AudioFloat::addMul(const AudioFloat & other, const float gain)
{
	if (isScalar && other.isScalar)
	{
		setScalar(getScalar() + other.getScalar() * gain);
	}
	else
	{
		other.expand();
		
		expand();
		
		//
		
		setVector();
		
		audioBufferAdd(samples, other.samples, AUDIO_UPDATE_SIZE, gain);
	}
}

void AudioFloat::addMul(const AudioFloat & other, const AudioFloat & gain)
{
	if (isScalar && other.isScalar && gain.isScalar)
	{
		setScalar(getScalar() + other.getScalar() * gain.getScalar());
	}
	else
	{
		other.expand();
		
		expand();
		
		//
		
		setVector();
		
		if (gain.isScalar)
		{
			audioBufferAdd(samples, other.samples, AUDIO_UPDATE_SIZE, gain.getScalar());
		}
		else
		{
			// todo : write SSE code path for this case
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				samples[i] += other.samples[i] * gain.samples[i];
		}
	}
}

void AudioFloat::mul(const AudioFloat & other)
{
	if (isScalar && other.isScalar)
	{
		// both are scalar
		
		setScalar(getScalar() * other.getScalar());
	}
	else if (isScalar)
	{
		// we are scalar, other is vector
		
		Assert(isScalar == true);
		Assert(other.isScalar == false);
		
		const float value = getScalar();
		
		if (value == 0.f)
		{
			// multiplying everything by zero just results in all zeroes
			
			setZero();
		}
		else if (value == 1.f)
		{
			// multiplying everything by one is the same as a straight copy
			
			set(other);
		}
		else
		{
			expand();
			
			setVector();
			
			//
			
			audioBufferMul(samples, AUDIO_UPDATE_SIZE, other.samples);
		}
	}
	else if (other.isScalar)
	{
		// we are vector, other is scalar
		
		Assert(isScalar == false);
		Assert(other.isScalar == true);
		
		const float otherValue = other.getScalar();
		
		if (otherValue == 0.f)
		{
			// multiplying everything by zero just results in all zeroes
			
			setZero();
		}
		else if (otherValue == 1.f)
		{
			// we don't have to do anything. this is quite a common case !
		}
		else
		{
			audioBufferMul(samples, AUDIO_UPDATE_SIZE, otherValue);
		}
	}
	else
	{
		// both are vector
		
		Assert(isScalar == false);
		Assert(other.isScalar == false);
		
		//
		
		audioBufferMul(samples, AUDIO_UPDATE_SIZE, other.samples);
	}
}

void AudioFloat::mulMul(const AudioFloat & other, const float gain)
{
	other.expand();
	
	expand();
	
	//
	
	setVector();
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		samples[i] *= other.samples[i] * gain;
}

void AudioFloat::mulMul(const AudioFloat & other, const AudioFloat & gain)
{
	other.expand();
	
	expand();
	
	//
	
	setVector();
	
	if (gain.isScalar)
	{
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] *= other.samples[i] * gain.getScalar();
	}
	else
	{
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] *= other.samples[i] * gain.samples[i];
	}
}

//

void AudioFloatArray::update()
{
	const int numElems = elems.size();
	
	if (numElems <= 1)
	{
		if (sum != nullptr)
		{
			delete sum;
			sum = nullptr;
		}
		
		//
		
		return;
	}
	
	//
	
	if (sum == nullptr)
	{
		sum = new AudioFloat();
	}
	
	bool allScalar = true;
	
	for (auto & elem : elems)
		allScalar &= elem.audioFloat->isScalar;
	
	
	if (allScalar)
	{
		float s = 0.f;
		
		for (auto & elem : elems)
		{
			s += elem.audioFloat->getScalar();
		}
		
		sum->setScalar(s);
	}
	else
	{
		sum->setVector();
		
		sum->set(*elems[0].audioFloat);
		
		for (int i = 1; i < numElems; ++i)
		{
			auto * a = elems[i].audioFloat;
			
			sum->add(*a);
		}
	}
}

AudioFloat * AudioFloatArray::get()
{
	// fixme : g_currentAudioGraph should always be valid here? right now we just validate without checking traversal id when g_currentAudioGraph is nullptr. potetially doing this work twice (or more). on the other hand, if get is called after the update, it may set the traversal id to that of the next frame, without working on the array that will be updated in the future. this would be even worse..
	
	if (g_currentAudioGraph == nullptr || lastUpdateTick != g_currentAudioGraph->nextTickTraversalId)
	{
		if (g_currentAudioGraph != nullptr)
		{
			lastUpdateTick = g_currentAudioGraph->nextTickTraversalId;
		}
		
		update();
	}
	
	const int numElems = elems.size();
	
	if (numElems == 0)
		return immediateValue;
	else if (numElems == 1)
		return elems[0].audioFloat;
	else
		return sum;
}

//

void AudioPlug::connectTo(AudioPlug & dst)
{
	if (dst.type != type)
	{
		logError("node connection failed. type mismatch");
	}
	else
	{
	#if MULTIPLE_AUDIO_INPUT
		if (dst.type == kAudioPlugType_FloatVec)
		{
			AudioFloatArray::Elem elem;
			elem.audioFloat = (AudioFloat*)dst.mem;
			floatArray.elems.push_back(elem);
		}
		else
	#endif
		{
			mem = dst.mem;
		}
	}
}

void AudioPlug::connectTo(void * dstMem, const AudioPlugType dstType, const bool isImmediate)
{
	if (dstType != type)
	{
		logError("node connection failed. type mismatch");
	}
	else
	{
	#if MULTIPLE_AUDIO_INPUT
		if (dstType == kAudioPlugType_FloatVec)
		{
			if (isImmediate)
			{
				floatArray.immediateValue = (AudioFloat*)dstMem;
			}
			else
			{
				AudioFloatArray::Elem elem;
				elem.audioFloat = (AudioFloat*)dstMem;
				floatArray.elems.push_back(elem);
			}
		}
		else
	#endif
		{
			mem = dstMem;
		}
	}
}

//

void AudioNodeDescription::add(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	lines.push_back(text);
}

void AudioNodeDescription::newline()
{
	add("");
}

//

AudioNodeBase::TriggerTarget::TriggerTarget()
	: srcNode(nullptr)
	, srcSocketIndex(-1)
	, dstSocketIndex(-1)
{
}

AudioNodeBase::AudioNodeBase()
	: inputs()
	, outputs()
	, predeps()
	, triggerTargets()
	, lastTickTraversalId(-1)
	, lastDrawTraversalId(-1)
	, editorIsTriggered(false)
	, isPassthrough(false)
	, isDeprecated(false)
	, tickTimeAvg(0)
{
}

void AudioNodeBase::traverseTick(const int traversalId, const float dt)
{
	Assert(lastTickTraversalId != traversalId);
	lastTickTraversalId = traversalId;
	
	//
	
	for (auto predep : predeps)
	{
		if (predep->lastTickTraversalId != traversalId)
			predep->traverseTick(traversalId, dt);
	}
	
	//
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	tick(dt);
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	//
	
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1) * 1 * (SAMPLE_RATE / AUDIO_UPDATE_SIZE)) / 100;
}

void AudioNodeBase::trigger(const int outputSocketIndex)
{
	editorIsTriggered = true;
	
	Assert(outputSocketIndex >= 0 && outputSocketIndex < (int)outputs.size());
	if (outputSocketIndex >= 0 && outputSocketIndex < (int)outputs.size())
	{
		auto & outputSocket = outputs[outputSocketIndex];
		Assert(outputSocket.type == kAudioPlugType_Trigger);
		if (outputSocket.type == kAudioPlugType_Trigger)
		{
			// iterate the list of outgoing connections, call handleTrigger on nodes with correct outputSocketIndex
			
			for (auto & triggerTarget : triggerTargets)
			{
				if (triggerTarget.dstSocketIndex == outputSocketIndex)
				{
					triggerTarget.srcNode->editorIsTriggered = true;
					
					triggerTarget.srcNode->handleTrigger(triggerTarget.srcSocketIndex);
				}
			}
		}
	}
}

//

AudioEnumTypeRegistration * g_audioEnumTypeRegistrationList = nullptr;

AudioEnumTypeRegistration::AudioEnumTypeRegistration()
	: enumName()
	, nextValue(0)
	, elems()
{
	next = g_audioEnumTypeRegistrationList;
	g_audioEnumTypeRegistrationList = this;
}
	
void AudioEnumTypeRegistration::elem(const char * name, const int value)
{
	Elem e;
	e.name = name;
	e.value = value == -1 ? nextValue : value;
	
	elems.push_back(e);
	
	nextValue = e.value + 1;
}

// todo : move elsewhere ?

#include "graph.h"

void createAudioValueTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "bool";
		typeDefinition.editor = "checkbox";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "int";
		typeDefinition.editor = "textbox_int";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "float";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "string";
		typeDefinition.editor = "textbox";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "audioValue";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "channels";
	#if MULTIPLE_AUDIO_INPUT
		typeDefinition.multipleInputs = true;
	#endif
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "trigger";
		typeDefinition.editor = "button";
		typeDefinition.multipleInputs = true;
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

void createAudioEnumTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, AudioEnumTypeRegistration * registrationList)
{
	for (AudioEnumTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions[registration->enumName];
		
		enumDefinition.enumName = registration->enumName;
		
		for (auto & src : registration->elems)
		{
			GraphEdit_EnumDefinition::Elem dst;
			
			dst.name = src.name;
			dst.value = src.value;
			
			enumDefinition.enumElems.push_back(dst);
		}
	}
}

//

AudioNodeTypeRegistration * g_audioNodeTypeRegistrationList = nullptr;

AudioNodeTypeRegistration::AudioNodeTypeRegistration()
	: next(nullptr)
	, create(nullptr)
	, typeName()
	, inputs()
	, outputs()
{
	next = g_audioNodeTypeRegistrationList;
	g_audioNodeTypeRegistrationList = this;
}

void AudioNodeTypeRegistration::in(const char * name, const char * typeName, const char * defaultValue, const char * displayName)
{
	Input i;
	i.name = name;
	i.displayName = displayName;
	i.typeName = typeName;
	i.defaultValue = defaultValue;
	
	inputs.push_back(i);
}

void AudioNodeTypeRegistration::inEnum(const char * name, const char * enumName, const char * defaultValue, const char * displayName)
{
	Input i;
	i.name = name;
	i.displayName = displayName;
	i.typeName = "int";
	i.enumName = enumName;
	i.defaultValue = defaultValue;
	
	inputs.push_back(i);
}

void AudioNodeTypeRegistration::out(const char * name, const char * typeName, const char * displayName)
{
	Output o;
	o.name = name;
	o.typeName = typeName;
	o.displayName = displayName;
	
	outputs.push_back(o);
}

void AudioNodeTypeRegistration::outEditable(const char * name)
{
	for (auto & o : outputs)
		if (o.name == name)
			o.isEditable = true;
}

//

// todo : move elsewhere ?

#include "graph.h"

void createAudioNodeTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, AudioNodeTypeRegistration * registrationList)
{
	for (AudioNodeTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		GraphEdit_TypeDefinition typeDefinition;
		
		typeDefinition.typeName = registration->typeName;
		typeDefinition.displayName = registration->displayName;
		
		for (int i = 0; i < (int)registration->inputs.size(); ++i)
		{
			auto & src = registration->inputs[i];
			
			GraphEdit_TypeDefinition::InputSocket inputSocket;
			inputSocket.typeName = src.typeName;
			inputSocket.name = src.name;
			inputSocket.index = i;
			inputSocket.enumName = src.enumName;
			inputSocket.defaultValue = src.defaultValue;
			
			typeDefinition.inputSockets.push_back(inputSocket);
		}
		
		for (int i = 0; i < (int)registration->outputs.size(); ++i)
		{
			auto & src = registration->outputs[i];
			
			GraphEdit_TypeDefinition::OutputSocket outputSocket;
			outputSocket.typeName = src.typeName;
			outputSocket.name = src.name;
			outputSocket.isEditable = src.isEditable;
			outputSocket.index = i;
			
			typeDefinition.outputSockets.push_back(outputSocket);
		}
		
		typeDefinition.createUi();
		
		typeDefinitionLibrary.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

//

#include <cmath>

// note : AudioNodeSourceMix is deprecated

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
		
		isDeprecated = true;
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

AUDIO_NODE_TYPE(audioSourceMix, AudioNodeSourceMix)
{
	typeName = "audio.mix";
	
	in("source1", "audioValue");
	in("gain1", "audioValue", "1");
	in("source2", "audioValue");
	in("gain2", "audioValue", "1");
	in("source3", "audioValue");
	in("gain3", "audioValue", "1");
	in("source4", "audioValue");
	in("gain4", "audioValue", "1");
	out("audio", "audioValue");
}

// note : AudioNodeMix is deprecated

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
		
		isDeprecated = true;
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

AUDIO_ENUM_TYPE(audioMixMode)
{
	elem("add");
	elem("mul");
}

AUDIO_NODE_TYPE(audioMix, AudioNodeMix)
{
	typeName = "mix";
	
	inEnum("mode", "audioMixMode");
	in("sourceA", "audioValue");
	in("gainA", "audioValue", "1");
	in("sourceB", "audioValue");
	in("gainB", "audioValue", "1");
	out("audio", "audioValue");
}

// note : AudioNodeMathSine is deprecated

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

AUDIO_NODE_TYPE(sine, AudioNodeMathSine)
{
	typeName = "math.sine";
	
	in("value", "audioValue");
	out("result", "audioValue");
}
