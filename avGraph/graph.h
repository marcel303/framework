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

#include "Mat4x4.h"
#include <list> // todo : remove std::list dependency
#include <map>
#include <stdint.h>
#include <string>

// forward declarations

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

//

struct GraphEdit_TypeDefinitionLibrary;

//

typedef unsigned int GraphNodeId;
typedef unsigned int GraphLinkId;

extern GraphNodeId kGraphNodeIdInvalid;
extern GraphLinkId kGraphLinkIdInvalid;

struct GraphNode
{
	struct Resource
	{
		std::string type;
		std::string name;
		std::string data;
	};
	
	GraphNodeId id;
	std::string typeName;
	bool isPassthrough;
	
	std::map<std::string, Resource> resources;
	
	std::map<std::string, std::string> inputValues;
	
	// editor
	
	std::string editorValue;
	
	GraphNode();
	
	void setResource(const char * type, const char * name, const char * data);
	void clearResource(const char * type, const char * name);
	const char * getResource(const char * type, const char * name, const char * defaultValue) const;
};

struct GraphLinkRoutePoint
{
	GraphLinkId linkId;
	float x;
	float y;
	
	GraphLinkRoutePoint()
		: linkId(kGraphLinkIdInvalid)
		, x(0.f)
		, y(0.f)
	{
	}
};

struct GraphLink
{
	GraphLinkId id;
	bool isEnabled;
	bool isDynamic;
	
	GraphNodeId srcNodeId;
	std::string srcNodeSocketName;
	int srcNodeSocketIndex;
	
	GraphNodeId dstNodeId;
	std::string dstNodeSocketName;
	int dstNodeSocketIndex;
	
	std::map<std::string, std::string> params;
	
	// editor
	
	std::list<GraphLinkRoutePoint> editorRoutePoints;
	
	float editorIsActiveAnimTime; // real-time connection node activation animation
	float editorIsActiveAnimTimeRcp;
	
	GraphLink();
	
	void setIsEnabled(const bool isEnabled);
	
	float floatParam(const char * name, const float defaultValue) const;
};

struct GraphEditConnection
{
	virtual ~GraphEditConnection()
	{
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
	{
	}
	
	virtual void nodeRemove(const GraphNodeId nodeId)
	{
	}
	
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
	}
	
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
	}
};

struct Graph
{
	std::map<GraphNodeId, GraphNode> nodes;
	std::map<GraphLinkId, GraphLink> links;
	
	GraphNodeId nextNodeId;
	GraphLinkId nextLinkId;
	
	GraphEditConnection * graphEditConnection;
	
	Graph();
	~Graph();
	
	GraphNodeId allocNodeId();
	GraphLinkId allocLinkId();
	
	void addNode(const GraphNode & node);
	void removeNode(const GraphNodeId nodeId);
	
	void addLink(const GraphLink & link, const bool clearInputDuplicates);
	void removeLink(const GraphLinkId linkId);
	
	GraphNode * tryGetNode(const GraphNodeId nodeId);
	GraphLink * tryGetLink(const GraphLinkId linkId);
	
	bool loadXml(const tinyxml2::XMLElement * xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	bool saveXml(tinyxml2::XMLPrinter & xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary) const;
	
	bool load(const char * filename, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
};
