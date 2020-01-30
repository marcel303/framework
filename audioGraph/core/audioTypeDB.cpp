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

#include "audioNodeBase.h" // AudioEnumTypeRegistration, AudioNodeTypeRegistration
#include "audioTypeDB.h"
#include "graph_typeDefinitionLibrary.h"
#include "StringEx.h" // FormatC %d

void createAudioValueTypeDefinitions(Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "bool";
		typeDefinition.editor = "checkbox";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "int";
		typeDefinition.editor = "textbox_int";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "float";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "string";
		typeDefinition.editor = "textbox";
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "audioValue";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "channels";
		typeDefinition.multipleInputs = true;
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		Graph_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "trigger";
		typeDefinition.editor = "button";
		typeDefinition.multipleInputs = true;
		typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

void createAudioEnumTypeDefinitions(Graph_TypeDefinitionLibrary & typeDefinitionLibrary, const AudioEnumTypeRegistration * registrationList)
{
	for (const AudioEnumTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions[registration->enumName];
		
		enumDefinition.enumName = registration->enumName;
		
		for (auto & src : registration->elems)
		{
			Graph_EnumDefinition::Elem dst;
			
			dst.name = src.name;
			dst.valueText = String::FormatC("%d", src.value);
			
			enumDefinition.enumElems.push_back(dst);
		}
	}
}

void createAudioNodeTypeDefinitions(Graph_TypeDefinitionLibrary & typeDefinitionLibrary, const AudioNodeTypeRegistration * registrationList)
{
	for (const AudioNodeTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		Graph_TypeDefinition typeDefinition;
		
		typeDefinition.typeName = registration->typeName;
		
		if (registration->displayName.empty())
		{
			auto pos = registration->typeName.rfind('.');
			
			if (pos == std::string::npos)
				typeDefinition.displayName = registration->typeName;
			else
				typeDefinition.displayName = registration->typeName.substr(pos + 1);
		}
		else
		{
			typeDefinition.displayName = registration->displayName;
		}
		
		typeDefinition.resourceTypeName = registration->resourceTypeName;
		
		for (int i = 0; i < (int)registration->inputs.size(); ++i)
		{
			auto & src = registration->inputs[i];
			
			Graph_TypeDefinition::InputSocket inputSocket;
			inputSocket.typeName = src.typeName;
			inputSocket.name = src.name;
			inputSocket.index = i;
			inputSocket.enumName = src.enumName;
			inputSocket.defaultValue = src.defaultValue;
			inputSocket.hasDefaultValue = true;
			inputSocket.displayName = src.displayName;
			
			typeDefinition.inputSockets.push_back(inputSocket);
		}
		
		for (int i = 0; i < (int)registration->outputs.size(); ++i)
		{
			auto & src = registration->outputs[i];
			
			Graph_TypeDefinition::OutputSocket outputSocket;
			outputSocket.typeName = src.typeName;
			outputSocket.name = src.name;
			outputSocket.isEditable = src.isEditable;
			outputSocket.index = i;
			outputSocket.displayName = src.displayName;
			
			typeDefinition.outputSockets.push_back(outputSocket);
		}
		
		typeDefinition.resourceEditor.create = registration->createResourceEditor;
		typeDefinition.resourceEditor.createData = registration->createData;
		
		typeDefinition.createUi();
		
		typeDefinitionLibrary.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

//

void createAudioTypeDefinitionLibrary(Graph_TypeDefinitionLibrary & typeDefinitionLibrary, const AudioEnumTypeRegistration * enumRegistrationList, const AudioNodeTypeRegistration * nodeRegistrationList)
{
	createAudioValueTypeDefinitions(typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(typeDefinitionLibrary, enumRegistrationList);
	createAudioNodeTypeDefinitions(typeDefinitionLibrary, nodeRegistrationList);
}

//

void createAudioTypeDefinitionLibrary(Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	createAudioTypeDefinitionLibrary(typeDefinitionLibrary, g_audioEnumTypeRegistrationList, g_audioNodeTypeRegistrationList);
}
