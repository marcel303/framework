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

struct Graph_EnumDefinition;
struct Graph_LinkTypeDefinition;
struct Graph_TypeDefinition;
struct Graph_TypeDefinitionLibrary;
struct Graph_ValueTypeDefinition;

//

struct GraphEdit_ResourceEditorBase;

//

struct Graph_ValueTypeDefinition
{
	std::string typeName;
	bool multipleInputs;
	
	// ui
	
	std::string editor;
	std::string editorMin;
	std::string editorMax;
	std::string visualizer;
	bool typeValidation;
	
	Graph_ValueTypeDefinition()
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

struct Graph_EnumDefinition
{
	struct Elem
	{
		std::string valueText; // todo : why is valueText an std::string, and not just an int? because of the FSFX-v2 node that puts the PS filename in here?
		std::string name;
	};
	
	std::string enumName;
	std::vector<Elem> enumElems;
	
	Graph_EnumDefinition()
		: enumName()
		, enumElems()
	{
	}
	
	bool loadXml(const tinyxml2::XMLElement * xmlType);
};

struct Graph_TypeDefinition
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
		
		// backward compatibility
		
		std::vector<std::string> renames;
		
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
		
		bool canConnectTo(const Graph_TypeDefinitionLibrary * typeDefinitionLibrary, const OutputSocket & socket) const;
	};
	
	struct OutputSocket
	{
		std::string typeName;
		std::string name;
		bool isEditable;
		bool isDynamic;
		
		// backward compatibility
		
		std::vector<std::string> renames;
		
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
		
		bool canConnectTo(const Graph_TypeDefinitionLibrary * typeDefinitionLibrary, const InputSocket & socket) const;
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
	std::string description;
	
	std::string mainResourceType; // the type and name of the main node resource
	std::string mainResourceName; // GraphEdit will edit this resource by default
	
	Graph_TypeDefinition()
		: typeName()
		, inputSockets()
		, outputSockets()
		, resourceEditor()
		, displayName()
		, description()
		, mainResourceType()
		, mainResourceName()
	{
	}
	
	void createUi();
	
	bool loadXml(const tinyxml2::XMLElement * xmlNode);
};

struct Graph_LinkTypeDefinition
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

struct Graph_TypeDefinitionLibrary
{
	std::map<std::string, Graph_ValueTypeDefinition> valueTypeDefinitions;
	std::map<std::string, Graph_EnumDefinition> enumDefinitions;
	std::map<std::string, Graph_TypeDefinition> typeDefinitions;
	std::map<std::pair<std::string, std::string>, Graph_LinkTypeDefinition> linkTypeDefinitions;
	
	Graph_TypeDefinitionLibrary()
		: valueTypeDefinitions()
		, enumDefinitions()
		, typeDefinitions()
		, linkTypeDefinitions()
	{
	}
	
	const Graph_ValueTypeDefinition * tryGetValueTypeDefinition(const std::string & typeName) const;
	const Graph_EnumDefinition * tryGetEnumDefinition(const std::string & typeName) const;
	const Graph_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const;
	const Graph_LinkTypeDefinition * tryGetLinkTypeDefinition(const std::string & srcTypeName, const std::string & dstTypeName) const;
	
	bool loadXml(const tinyxml2::XMLElement * xmlLibrary);
};
