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

#include "graph_typeDefinitionLibrary.h"
#include "vfxNodeBase.h" // VfxEnumTypeRegistration, VfxNodeTypeRegistration
#include "vfxTypeDB.h"

void createVfxValueTypeDefinitions(Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	auto addSimple = [&](const char * typeName, const char * editor, const char * visualizer = nullptr) -> Graph_ValueTypeDefinition &
	{
		auto & td = typeDefinitionLibrary.valueTypeDefinitions[typeName];
		
		td.typeName = typeName;
		td.editor = editor;
		if (visualizer)
			td.visualizer = visualizer;
		
		return td;
	};
	
	addSimple("bool", "checkbox");
	addSimple("int", "textbox_int", "valueplotter");
	addSimple("float", "textbox_float", "valueplotter").multipleInputs = true;
	addSimple("string", "textbox");
	addSimple("color", "colorpicker");
	addSimple("trigger", "button").multipleInputs = true;
	addSimple("image", "none", "opengl-texture");
	addSimple("image_cpu", "none");
	addSimple("channel", "textbox", "channels");
	addSimple("draw", "none");
	
	{
		std::pair<std::string, std::string> key("float", "float");
		
		Graph_LinkTypeDefinition & floatLink = typeDefinitionLibrary.linkTypeDefinitions[key];
		
		floatLink.srcTypeName = "float";
		floatLink.dstTypeName = "float";
		
		floatLink.params.resize(4);
		
		floatLink.params[0].typeName = "float";
		floatLink.params[0].name = "in.min";
		floatLink.params[0].defaultValue = "0";
		
		floatLink.params[1].typeName = "float";
		floatLink.params[1].name = "in.max";
		floatLink.params[1].defaultValue = "1";
		
		floatLink.params[2].typeName = "float";
		floatLink.params[2].name = "out.min";
		floatLink.params[2].defaultValue = "0";
		
		floatLink.params[3].typeName = "float";
		floatLink.params[3].name = "out.max";
		floatLink.params[3].defaultValue = "1";
	}
}

void createVfxEnumTypeDefinitions(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const VfxEnumTypeRegistration * registrationList)
{
	for (const VfxEnumTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions[registration->enumName];
		
		enumDefinition.enumName = registration->enumName;
		
		for (auto & src : registration->elems)
		{
			Graph_EnumDefinition::Elem dst;
			
			dst.name = src.name;
			dst.valueText = src.valueText;
			
			enumDefinition.enumElems.push_back(dst);
		}
		
		// create additional elems if the enum has dynamic elements
		// todo : remove this code. replace it with explicit methods for nodes/systems that define nodes and enum dynamically
		
		if (registration->getElems != nullptr)
		{
			auto elems = registration->getElems();
			
			for (auto & src : elems)
			{
				Graph_EnumDefinition::Elem dst;
				
				dst.name = src.name;
				dst.valueText = src.valueText;
				
				enumDefinition.enumElems.push_back(dst);
			}
		}
	}
}

//

void createVfxNodeTypeDefinitions(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const VfxNodeTypeRegistration * registrationList)
{
	for (const VfxNodeTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
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
		
		for (int i = 0; i < registration->inputs.size(); ++i)
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
		
		for (int i = 0; i < registration->outputs.size(); ++i)
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
		
		typeDefinition.createUi();
		
		typeDefinitionLibrary.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

//

void createVfxTypeDefinitionLibrary(
	Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const VfxEnumTypeRegistration * enumRegistrationList,
	const VfxNodeTypeRegistration * nodeRegistrationList)
{
	createVfxValueTypeDefinitions(typeDefinitionLibrary);
	createVfxEnumTypeDefinitions(typeDefinitionLibrary, enumRegistrationList);
	createVfxNodeTypeDefinitions(typeDefinitionLibrary, nodeRegistrationList);
}

//

void createVfxTypeDefinitionLibrary(Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	createVfxTypeDefinitionLibrary(
		typeDefinitionLibrary,
		g_vfxEnumTypeRegistrationList,
		g_vfxNodeTypeRegistrationList);
}

Graph_TypeDefinitionLibrary * createVfxTypeDefinitionLibrary()
{
	Graph_TypeDefinitionLibrary * result = new Graph_TypeDefinitionLibrary();

	createVfxTypeDefinitionLibrary(
		*result,
		g_vfxEnumTypeRegistrationList,
		g_vfxNodeTypeRegistrationList);
	
	return result;
}
