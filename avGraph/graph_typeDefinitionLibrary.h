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

#include <map>
#include <string>
#include <vector>

// forward declarations

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

struct GraphEdit_EnumDefinition;
struct GraphEdit_LinkTypeDefinition;
struct GraphEdit_ResourceEditorBase;
struct GraphEdit_TypeDefinition;
struct GraphEdit_TypeDefinitionLibrary;
struct GraphEdit_ValueTypeDefinition;

//

struct GraphEdit_ValueTypeDefinition
{
	std::string typeName;
	bool multipleInputs;
	
	// ui
	
	std::string editor;
	std::string editorMin;
	std::string editorMax;
	std::string visualizer;
	bool typeValidation;
	
	GraphEdit_ValueTypeDefinition()
		: typeName()
		, multipleInputs(false)
		, editor()
		, editorMin("0")
		, editorMax("1")
		, visualizer()
		, typeValidation(true)
	{
	}
	
	bool loadXml(const tinyxml2::XMLElement * xmlType);
};

struct GraphEdit_EnumDefinition
{
	struct Elem
	{
		std::string valueText;
		std::string name;
	};
	
	std::string enumName;
	std::vector<Elem> enumElems;
	
	GraphEdit_EnumDefinition()
		: enumName()
		, enumElems()
	{
	}
	
	bool loadXml(const tinyxml2::XMLElement * xmlType);
};

struct GraphEdit_TypeDefinition
{
	struct InputSocket;
	struct OutputSocket;
	
	struct InputSocket
	{
		std::string typeName;
		std::string enumName;
		std::string name;
		std::string defaultValue;
		bool hasDefaultValue;
		bool isDynamic;
		
		// ui
		
		int index;
		
		std::string displayName;
		
		InputSocket()
			: typeName()
			, enumName()
			, name()
			, defaultValue()
			, hasDefaultValue(false)
			, isDynamic(false)
			, index(-1)
			, displayName()
		{
		}
		
		bool canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const OutputSocket & socket) const;
	};
	
	struct OutputSocket
	{
		std::string typeName;
		std::string name;
		bool isEditable;
		bool isDynamic;
		
		// ui
		
		int index;
		
		std::string displayName;
		
		OutputSocket()
			: typeName()
			, name()
			, isEditable(false)
			, isDynamic(false)
			, index(-1)
			, displayName()
		{
		}
		
		bool canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const InputSocket & socket) const;
	};
	
	struct ResourceEditor
	{
		GraphEdit_ResourceEditorBase * (*create)(void * data);
		void * createData;
		
		ResourceEditor()
			: create(nullptr)
			, createData(nullptr)
		{
		}
	};
	
	std::string typeName;
	
	std::vector<InputSocket> inputSockets;
	std::vector<OutputSocket> outputSockets;
	
	ResourceEditor resourceEditor;
	
	// ui
	
	std::string displayName;
	
	std::string resourceTypeName;
	
	GraphEdit_TypeDefinition()
		: typeName()
		, inputSockets()
		, outputSockets()
		, resourceEditor()
		, displayName()
		, resourceTypeName()
	{
	}
	
	void createUi();
	
	bool loadXml(const tinyxml2::XMLElement * xmlNode);
};

struct GraphEdit_LinkTypeDefinition
{
	struct Param
	{
		std::string typeName;
		std::string name;
		std::string defaultValue;
		
		Param()
			: typeName()
			, name()
			, defaultValue()
		{
		}
	};
	
	std::string srcTypeName;
	std::string dstTypeName;
	std::vector<Param> params;
};

struct GraphEdit_TypeDefinitionLibrary
{
	std::map<std::string, GraphEdit_ValueTypeDefinition> valueTypeDefinitions;
	std::map<std::string, GraphEdit_EnumDefinition> enumDefinitions;
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	std::map<std::pair<std::string, std::string>, GraphEdit_LinkTypeDefinition> linkTypeDefinitions;
	
	GraphEdit_TypeDefinitionLibrary()
		: valueTypeDefinitions()
		, enumDefinitions()
		, typeDefinitions()
		, linkTypeDefinitions()
	{
	}
	
	const GraphEdit_ValueTypeDefinition * tryGetValueTypeDefinition(const std::string & typeName) const;
	const GraphEdit_EnumDefinition * tryGetEnumDefinition(const std::string & typeName) const;
	const GraphEdit_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const;
	const GraphEdit_LinkTypeDefinition * tryGetLinkTypeDefinition(const std::string & srcTypeName, const std::string & dstTypeName) const;
	
	bool loadXml(const tinyxml2::XMLElement * xmlLibrary);
};
