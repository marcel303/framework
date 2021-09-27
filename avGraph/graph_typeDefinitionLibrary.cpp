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

#include "Debugging.h"
#include "graph_typeDefinitionLibrary.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

using namespace tinyxml2;

//

static bool areCompatibleSocketLinkTypeNames(const std::string & srcTypeName, const bool srcTypeValidation, const std::string & dstTypeName)
{
	if (srcTypeValidation == false)
		return true;
	if (srcTypeName == dstTypeName)
		return true;
	
	const char * end1 = strchr(srcTypeName.c_str(), '.');
	const char * end2 = strchr(dstTypeName.c_str(), '.');
	
	if (end1 != nullptr || end2 != nullptr)
	{
		const size_t length1 = end1 == nullptr ? srcTypeName.size() : (end1 - srcTypeName.c_str());
		const size_t length2 = end2 == nullptr ? dstTypeName.size() : (end2 - dstTypeName.c_str());
		
		if (length1 == length2)
		{
			const size_t length = length1;
			
			if (memcmp(srcTypeName.c_str(), dstTypeName.c_str(), length) == 0)
				return true;
		}
	}
	
	return false;
}

//

bool Graph_ValueTypeDefinition::loadXml(const XMLElement * xmlType)
{
	bool result = true;
	
	typeName = stringAttrib(xmlType, "typeName", "");
	result &= !typeName.empty();
	
	multipleInputs = boolAttrib(xmlType, "multipleInputs", false);
	
	editor = stringAttrib(xmlType, "editor", "textbox");
	editorMin = stringAttrib(xmlType, "editorMin", "0");
	editorMax = stringAttrib(xmlType, "editorMax", "1");
	visualizer = stringAttrib(xmlType, "visualizer", "");
	typeValidation = boolAttrib(xmlType, "typeValidation", true);
	
	if (result == false)
	{
		*this = Graph_ValueTypeDefinition();
	}
	
	return result;
}

bool Graph_EnumDefinition::loadXml(const XMLElement * xmlEnum)
{
	bool result = true;
	
	enumName = stringAttrib(xmlEnum, "name", "");
	result &= !enumName.empty();
	
	int value = 0;
	
	for (const XMLElement * xmlElem = xmlEnum->FirstChildElement("elem"); xmlElem != nullptr; xmlElem = xmlElem->NextSiblingElement("elem"))
	{
		value = intAttrib(xmlElem, "value", value);
		
		Elem elem;
		
		elem.valueText = String::FormatC("%d", value);
		elem.name = stringAttrib(xmlElem, "name", "");
		result &= !elem.name.empty();
		
		enumElems.push_back(elem);
		
		value++;
	}
	
	if (result == false)
	{
		*this = Graph_EnumDefinition();
	}
	
	return result;
}

bool Graph_TypeDefinition::InputSocket::canConnectTo(const Graph_TypeDefinitionLibrary * typeDefintionLibrary, const Graph_TypeDefinition::OutputSocket & socket) const
{
	auto valueTypeDefinition = typeDefintionLibrary->tryGetValueTypeDefinition(typeName);
	
	if (!areCompatibleSocketLinkTypeNames(typeName, valueTypeDefinition ? valueTypeDefinition->typeValidation : true, socket.typeName))
		return false;
	
	return true;
}

bool Graph_TypeDefinition::OutputSocket::canConnectTo(const Graph_TypeDefinitionLibrary * typeDefintionLibrary, const Graph_TypeDefinition::InputSocket & socket) const
{
	auto valueTypeDefinition = typeDefintionLibrary->tryGetValueTypeDefinition(socket.typeName);
	
	if (!areCompatibleSocketLinkTypeNames(socket.typeName, valueTypeDefinition ? valueTypeDefinition->typeValidation : true, typeName))
		return false;
	
	return true;
}

void Graph_TypeDefinition::createUi()
{
	// setup input sockets
	
	{
		int index = 0;
		
		for (auto & inputSocket : inputSockets)
		{
			inputSocket.index = index++;
		}
	}
	
	// setup output sockets
	
	{
		int index = 0;
		
		for (auto & outputSocket : outputSockets)
		{
			outputSocket.index = index++;
		}
	}
}

bool Graph_TypeDefinition::loadXml(const XMLElement * xmlType)
{
	bool result = true;
	
	typeName = stringAttrib(xmlType, "typeName", "");
	result &= !typeName.empty();
	
	displayName = stringAttrib(xmlType, "displayName", "");
	
	mainResourceType = stringAttrib(xmlType, "mainResourceType", "");
	mainResourceName = stringAttrib(xmlType, "mainResourceName", "");
	
	for (auto xmlInput = xmlType->FirstChildElement("input"); xmlInput != nullptr; xmlInput = xmlInput->NextSiblingElement("input"))
	{
		InputSocket socket;
		socket.typeName = stringAttrib(xmlInput, "typeName", socket.typeName.c_str());
		socket.enumName = stringAttrib(xmlInput, "enumName", socket.enumName.c_str());
		socket.name = stringAttrib(xmlInput, "name", socket.name.c_str());
		
		auto defaultValue = stringAttrib(xmlInput, "default", nullptr);
		if (defaultValue != nullptr)
		{
			socket.defaultValue = defaultValue;
			socket.hasDefaultValue = true;
		}
		
		inputSockets.push_back(socket);
		
		result &= !socket.typeName.empty();
		result &= !socket.name.empty();
	}
	
	for (auto xmlOutput = xmlType->FirstChildElement("output"); xmlOutput != nullptr; xmlOutput = xmlOutput->NextSiblingElement("output"))
	{
		OutputSocket socket;
		socket.typeName = stringAttrib(xmlOutput, "typeName", socket.typeName.c_str());
		socket.name = stringAttrib(xmlOutput, "name", socket.name.c_str());
		socket.isEditable = boolAttrib(xmlOutput, "editable", socket.isEditable);
		
		outputSockets.push_back(socket);
		
		result &= !socket.typeName.empty();
		result &= !socket.name.empty();
	}
	
	if (result == false)
	{
		*this = Graph_TypeDefinition();
	}
	
	return result;
}

//

const Graph_ValueTypeDefinition * Graph_TypeDefinitionLibrary::tryGetValueTypeDefinition(const std::string & typeName) const
{
	auto i = valueTypeDefinitions.find(typeName);
	
	if (i != valueTypeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const Graph_EnumDefinition * Graph_TypeDefinitionLibrary::tryGetEnumDefinition(const std::string & typeName) const
{
	auto i = enumDefinitions.find(typeName);
	
	if (i != enumDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const Graph_TypeDefinition * Graph_TypeDefinitionLibrary::tryGetTypeDefinition(const std::string & typeName) const
{
	auto i = typeDefinitions.find(typeName);
	
	if (i != typeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const Graph_LinkTypeDefinition * Graph_TypeDefinitionLibrary::tryGetLinkTypeDefinition(const std::string & srcTypeName, const std::string & dstTypeName) const
{
	auto key = std::make_pair(srcTypeName, dstTypeName);
	
	auto i = linkTypeDefinitions.find(key);
	
	if (i != linkTypeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

bool Graph_TypeDefinitionLibrary::loadXml(const XMLElement * xmlLibrary)
{
	bool result = true;
	
	for (auto xmlType = xmlLibrary->FirstChildElement("valueType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("valueType"))
	{
		Graph_ValueTypeDefinition typeDefinition;
		
		result &= typeDefinition.loadXml(xmlType);
		
		// check typeName doesn't exist yet
		
		auto itr = valueTypeDefinitions.find(typeDefinition.typeName);
		Assert(itr == valueTypeDefinitions.end());
		result &= itr == valueTypeDefinitions.end();
		
		valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	for (auto xmlEnum = xmlLibrary->FirstChildElement("enum"); xmlEnum != nullptr; xmlEnum = xmlEnum->NextSiblingElement("enum"))
	{
		Graph_EnumDefinition enumDefinition;
		
		result &= enumDefinition.loadXml(xmlEnum);
		
		// check typeName doesn't exist yet
		
		auto itr = enumDefinitions.find(enumDefinition.enumName);
		Assert(itr == enumDefinitions.end());
		result &= itr == enumDefinitions.end();
		
		enumDefinitions[enumDefinition.enumName] = enumDefinition;
	}
	
	for (auto xmlType = xmlLibrary->FirstChildElement("linkType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("linkType"))
	{
		Graph_LinkTypeDefinition typeDefinition;
		
		typeDefinition.srcTypeName = stringAttrib(xmlType, "srcTypeName", "");
		typeDefinition.dstTypeName = stringAttrib(xmlType, "dstTypeName", "");
		
		result &= typeDefinition.srcTypeName.empty() == false;
		result &= typeDefinition.dstTypeName.empty() == false;
		
		for (auto xmlParam = xmlType->FirstChildElement("param"); xmlParam != nullptr; xmlParam = xmlParam->NextSiblingElement("param"))
		{
			Graph_LinkTypeDefinition::Param param;
			
			param.typeName = stringAttrib(xmlParam, "typeName", "");
			param.name = stringAttrib(xmlParam, "name", "");
			param.defaultValue = stringAttrib(xmlParam, "default", "");
			
			result &= param.typeName.empty() == false;
			result &= param.name.empty() == false;
			
			typeDefinition.params.push_back(param);
		}
		
		auto key = std::make_pair(typeDefinition.srcTypeName, typeDefinition.dstTypeName);
		
		linkTypeDefinitions[key] = typeDefinition;
	}
	
	for (auto xmlType = xmlLibrary->FirstChildElement("type"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("type"))
	{
		Graph_TypeDefinition typeDefinition;
		
		result &= typeDefinition.loadXml(xmlType);
		typeDefinition.createUi();
		
		// check typeName doesn't exist yet
		
		auto itr = typeDefinitions.find(typeDefinition.typeName);
		Assert(itr == typeDefinitions.end());
		result &= itr == typeDefinitions.end();
		
		typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	return result;
}
