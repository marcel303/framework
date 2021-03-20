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

#include "audioNodeBase.h"
#include "graph_typeDefinitionLibrary.h"
#include "Log.h"
#include "soundmix.h"
#include "StringEx.h"
#include "Timer.h"
#include <string.h>

#if AUDIO_USE_SSE
	#include <xmmintrin.h>
#endif

#if AUDIO_USE_GCC_VECTOR
	typedef float vec4f __attribute__ ((vector_size(16))) __attribute__ ((aligned(16)));
#endif

//

AUDIO_THREAD_LOCAL int g_currentAudioGraphTraversalId = -1;

int getCurrentAudioGraphTraversalId()
{
	return g_currentAudioGraphTraversalId;
}

void setCurrentAudioGraphTraversalId(int id)
{
	Assert(g_currentAudioGraphTraversalId == -1 || id == -1);
	g_currentAudioGraphTraversalId = id;
}

void clearCurrentAudioGraphTraversalId()
{
	Assert(g_currentAudioGraphTraversalId != -1);
	g_currentAudioGraphTraversalId = -1;
}

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
		const float sum = audioBufferSum(samples, AUDIO_UPDATE_SIZE);
		
		const float mean = sum / AUDIO_UPDATE_SIZE;
		
		return mean;
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
			
		#if AUDIO_USE_SSE
			const __m128 scalar_4 = _mm_set1_ps(samples[0]);
			__m128 * samples_4 = (__m128*)self->samples;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE / 4; ++i)
				samples_4[i] = scalar_4;
		#elif AUDIO_USE_GCC_VECTOR
			const vec4f scalar_4 = { samples[0], samples[0], samples[0], samples[0] };
			vec4f * samples_4 = (vec4f*)self->samples;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE / 4; ++i)
				samples_4[i] = scalar_4;
		#else
			for (int i = 1; i < AUDIO_UPDATE_SIZE; ++i)
				self->samples[i] = samples[0];
		#endif
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
	else if (other.isScalar)
	{
		Assert(gain.isScalar == false);
		
		setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			samples[i] = other.getScalar() * gain.samples[i];
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
			audioBufferAdd(samples, other.samples, AUDIO_UPDATE_SIZE, gain.samples);
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

#if AUDIO_USE_SSE

void * AudioFloat::operator new(size_t size)
{
	return _mm_malloc(size, 16);
}

void AudioFloat::operator delete(void * mem)
{
	_mm_free(mem);
}

#elif AUDIO_USE_GCC_VECTOR

void * AudioFloat::operator new(size_t size)
{
	void * result = nullptr;
	posix_memalign(&result, 16, size);
	return result;
}

void AudioFloat::operator delete(void * mem)
{
	free(mem);
}

#endif

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
	bool anyScalar = false;
	
	float scalarSum = 0.f;
	
	for (auto & elem : elems)
	{
		if (elem.audioFloat->isScalar)
		{
			scalarSum += elem.audioFloat->getScalar();
			anyScalar = true;
		}
		else
		{
			allScalar = false;
		}
	}
	
	if (allScalar)
	{
		sum->setScalar(scalarSum);
	}
	else
	{
		if (anyScalar)
		{
			sum->setScalar(scalarSum);
			sum->expand();
			sum->setVector();
			
			for (int i = 0; i < numElems; ++i)
			{
				auto * a = elems[i].audioFloat;
				
				if (a->isScalar == false)
				{
					audioBufferAdd(sum->samples, a->samples, AUDIO_UPDATE_SIZE);
				}
			}
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
}

AudioFloat * AudioFloatArray::get()
{
	Assert(g_currentAudioGraphTraversalId != -1);
	if (lastUpdateTick != g_currentAudioGraphTraversalId)
	{
		lastUpdateTick = g_currentAudioGraphTraversalId;
		
		update();
	}
	
	const int numElems = elems.size();
	Assert(numElems != 0);
	
	if (numElems == 1)
		return elems[0].audioFloat;
	else
		return sum;
}

//

void AudioPlug::connectTo(AudioPlug & dst)
{
	if (dst.type != type)
	{
		LOG_ERR("node connection failed. type mismatch");
	}
	else if (dst.type == kAudioPlugType_FloatVec)
	{
		AudioFloatArray::Elem elem;
		elem.audioFloat = (AudioFloat*)dst.mem;
		floatArray.elems.push_back(elem);
		floatArray.lastUpdateTick = -1;
	}
	else
	{
		mem = dst.mem;
	}
}

void AudioPlug::connectToImmediate(void * dstMem, const AudioPlugType dstType)
{
	if (dstType != type)
	{
		LOG_ERR("node connection failed. type mismatch");
	}
	else
	{
		immediateMem = dstMem;
		
		if (mem == nullptr)
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
	, editorIsTriggered(false)
	, isPassthrough(false)
	, isDeprecated(false)
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	, tickTimeAvg(0)
#endif
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
	
	for (int i = 0; i < inputs.size(); ++i)
	{
		if (inputs[i].isTriggered)
		{
			inputs[i].isTriggered = false;
			
			handleTrigger(i);
		}
	}
	
	//
	
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	const uint64_t t1 = g_TimerRT.TimeUS_get();
#endif

	tick(dt);
	
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	const uint64_t t2 = g_TimerRT.TimeUS_get();
#endif
	
	//
	
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1) * 1 * (SAMPLE_RATE / AUDIO_UPDATE_SIZE)) / 100;
#endif
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
			// iterate the list of outgoing connections, call queueTrigger on nodes with correct outputSocketIndex
			
			for (auto & triggerTarget : triggerTargets)
			{
				if (triggerTarget.dstSocketIndex == outputSocketIndex)
				{
					triggerTarget.srcNode->editorIsTriggered = true;
					
					triggerTarget.srcNode->queueTrigger(triggerTarget.srcSocketIndex);
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

//

AudioNodeTypeRegistration * g_audioNodeTypeRegistrationList = nullptr;

AudioNodeTypeRegistration::AudioNodeTypeRegistration()
	: next(nullptr)
	, create(nullptr)
	, createData(nullptr)
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

void AudioNodeTypeRegistration::inEnum(const char * name, const char * enumName, const int defaultValue, const char * displayName)
{
	Input i;
	i.name = name;
	i.displayName = displayName;
	i.typeName = "int";
	i.enumName = enumName;
	i.defaultValue = String::FormatC("%d", defaultValue);
	
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
