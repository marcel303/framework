#include "audioNodeBase.h"

//

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

#include "audioNodeBase.h"
#include "framework.h"
#include "graph.h"
#include "StringEx.h"
#include "Timer.h"
#include <string.h>

//

AudioTriggerData::AudioTriggerData()
	: type(kAudioTriggerDataType_None)
{
	memset(mem, 0, sizeof(mem));
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
		mem = dst.mem;
	}
}

void AudioPlug::connectTo(void * dstMem, const AudioPlugType dstType)
{
	if (dstType != type)
	{
		logError("node connection failed. type mismatch");
	}
	else
	{
		mem = dstMem;
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
	, tickTimeAvg(0)
	, drawTimeAvg(0)
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
	
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1)) / 100;
}

void AudioNodeBase::traverseDraw(const int traversalId)
{
	Assert(lastDrawTraversalId != traversalId);
	lastDrawTraversalId = traversalId;
	
	//
	
	for (auto predep : predeps)
	{
		if (predep->lastDrawTraversalId != traversalId)
			predep->traverseDraw(traversalId);
	}
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	draw();
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	//
	
	drawTimeAvg = (drawTimeAvg * 95 + (t2 - t1) * 5) / 100;
}

void AudioNodeBase::trigger(const int outputSocketIndex)
{
	editorIsTriggered = true;
	
	Assert(outputSocketIndex >= 0 && outputSocketIndex < outputs.size());
	if (outputSocketIndex >= 0 && outputSocketIndex < outputs.size())
	{
		auto & outputSocket = outputs[outputSocketIndex];
		Assert(outputSocket.type == kAudioPlugType_Trigger);
		if (outputSocket.type == kAudioPlugType_Trigger)
		{
			// iterate the list of outgoing connections, call handleTrigger on nodes with correct outputSocketIndex
			
			const AudioTriggerData & triggerData = outputSocket.getTriggerData();
			
			for (auto & triggerTarget : triggerTargets)
			{
				if (triggerTarget.dstSocketIndex == outputSocketIndex)
				{
					triggerTarget.srcNode->editorIsTriggered = true;
					
					triggerTarget.srcNode->handleTrigger(triggerTarget.srcSocketIndex, triggerData);
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
		
		for (int i = 0; i < registration->inputs.size(); ++i)
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
		
		for (int i = 0; i < registration->outputs.size(); ++i)
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

AUDIO_NODE_TYPE(display, AudioNodeDisplay)
{
	typeName = "display";
	
	in("audio", "audioBuffer");
}

AUDIO_NODE_TYPE(pcmData, AudioNodePcmData)
{
	typeName = "pcm.fromFile";
	
	in("filename", "string");
	out("pcm", "pcmData");
}

AUDIO_NODE_TYPE(audioSourcePcm, AudioNodeSourcePcm)
{
	typeName = "audio.pcm";
	
	in("pcm", "pcmData");
	out("audio", "audioBuffer");
}

AUDIO_NODE_TYPE(audioSourceMix, AudioNodeSourceMix)
{
	typeName = "audio.mix";
	
	in("source1", "audioBuffer");
	in("gain1", "float", "1");
	in("source2", "audioBuffer");
	in("gain2", "float", "1");
	in("source3", "audioBuffer");
	in("gain3", "float", "1");
	in("source4", "audioBuffer");
	in("gain4", "float", "1");
	out("audio", "audioBuffer");
}

//

void AudioNodePcmData::tick(const float dt)
{
	const char * filename = getInputString(kInput_Filename, "");
	
	if (filename != currentFilename)
	{
		currentFilename = filename;
		
		pcmData.load(filename, 0);
	}
}
