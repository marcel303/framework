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
#include "graph.h"
#include "graph_typeDefinitionLibrary.h"
#include "Parse.h"
#include "Log.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <algorithm>

using namespace tinyxml2;

#define ENABLE_FILE_FIXUPS 1

//

GraphNodeId kGraphNodeIdInvalid = 0;
GraphLinkId kGraphLinkIdInvalid = 0;

//

GraphNode::GraphNode()
	: id(kGraphNodeIdInvalid)
	, typeName()
	, isPassthrough(false)
	, resources()
	, inputValues()
	, editorValue()
{
}

void GraphNode::setResource(const char * type, const char * name, const char * data)
{
	auto & resource = resources[name];
	
	resource = GraphNodeResource();
	resource.type = type;
	resource.name = name;
	resource.data = data;
}

void GraphNode::clearResource(const char * type, const char * name)
{
	auto resourceItr = resources.find(name);
	
	if (resourceItr == resources.end())
		return;
	
	auto & resource = resourceItr->second;
	
	if (resource.type != type)
		return;
	
	resources.erase(resourceItr);
}

const char * GraphNode::getResource(const char * type, const char * name, const char * defaultValue) const
{
	auto resourceItr = resources.find(name);
	
	if (resourceItr == resources.end())
		return defaultValue;
	
	auto & resource = resourceItr->second;
	
	if (resource.type != type)
		return defaultValue;
	
	return resource.data.c_str();
}

//

GraphLink::GraphLink()
	: id(kGraphLinkIdInvalid)
	, isEnabled(true)
	, isDynamic(false)
	, srcNodeId(kGraphNodeIdInvalid)
	, srcNodeSocketName()
	, srcNodeSocketIndex(-1)
	, dstNodeId(kGraphNodeIdInvalid)
	, dstNodeSocketName()
	, dstNodeSocketIndex(-1)
	, params()
	, editorRoutePoints()
	, editorIsActiveAnimTime(0.f)
	, editorIsActiveAnimTimeRcp(0.f)
{
}

void GraphLink::setIsEnabled(const bool _isEnabled)
{
	isEnabled = _isEnabled;
}

float GraphLink::floatParam(const char * name, const float defaultValue) const
{
	auto i = params.find(name);
	
	if (i == params.end())
		return defaultValue;
	else
		return Parse::Float(i->second);
}

//

Graph::Graph()
	: nodes()
	, links()
	, nextNodeId(1)
	, nextLinkId(1)
	, graphEditConnection(nullptr)
{
}

Graph::~Graph()
{
}

GraphNodeId Graph::allocNodeId()
{
	GraphNodeId result = nextNodeId;
	
	nextNodeId++;
	
	Assert(nodes.find(result) == nodes.end());
	
	return result;
}

GraphNodeId Graph::allocLinkId()
{
	GraphLinkId result = nextLinkId;
	
	nextLinkId++;
	
	Assert(links.find(result) == links.end());
	
	return result;
}

void Graph::addNode(const GraphNode & node)
{
	Assert(node.id != kGraphNodeIdInvalid);
	Assert(nodes.find(node.id) == nodes.end());
	
	auto nodeItr = nodes.insert(std::pair<GraphNodeId, GraphNode>(node.id, node));
	
	if (graphEditConnection != nullptr)
	{
		graphEditConnection->nodeAdd(nodeItr.first->second);
	}
}

void Graph::removeNode(const GraphNodeId nodeId)
{
	Assert(nodeId != kGraphNodeIdInvalid);
	Assert(nodes.find(nodeId) != nodes.end());
	
	for (auto linkItr = links.begin(); linkItr != links.end(); )
	{
		auto & link = linkItr->second;
		
		if (link.srcNodeId == nodeId)
		{
			if (graphEditConnection != nullptr)
			{
				graphEditConnection->linkRemove(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
			}
			
			linkItr = links.erase(linkItr);
		}
		else if (link.dstNodeId == nodeId)
		{
			if (graphEditConnection != nullptr)
			{
				graphEditConnection->linkRemove(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
			}
			
			linkItr = links.erase(linkItr);
		}
		else
			++linkItr;
	}
	
	if (graphEditConnection != nullptr)
	{
		graphEditConnection->nodeRemove(nodeId);
	}

	nodes.erase(nodeId);
}

void Graph::addLink(const GraphLink & link, const bool clearInputDuplicates)
{
	if (clearInputDuplicates)
	{
		for (auto i = links.begin(); i != links.end(); )
		{
			auto & otherLink = i->second;
			
			if (link.srcNodeId == otherLink.srcNodeId && link.srcNodeSocketIndex == otherLink.srcNodeSocketIndex)
			{
				if (graphEditConnection)
				{
					graphEditConnection->linkRemove(otherLink.id, otherLink.srcNodeId, otherLink.srcNodeSocketIndex, otherLink.dstNodeId, otherLink.dstNodeSocketIndex);
				}
				
				i = links.erase(i);
			}
			else
			{
				++i;
			}
		}
	}
	
	links[link.id] = link;
	
	if (graphEditConnection)
	{
		graphEditConnection->linkAdd(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
	}
}

void Graph::removeLink(const GraphLinkId linkId)
{
	Assert(linkId != kGraphLinkIdInvalid);
	Assert(links.find(linkId) != links.end());
	
	if (graphEditConnection)
	{
		auto & link = links.find(linkId)->second;
		
		graphEditConnection->linkRemove(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
	}
	
	links.erase(linkId);
}

GraphNode * Graph::tryGetNode(const GraphNodeId nodeId)
{
	auto nodeItr = nodes.find(nodeId);
	
	if (nodeItr == nodes.end())
		return nullptr;
	else
		return &nodeItr->second;
}

GraphLink * Graph::tryGetLink(const GraphLinkId linkId)
{
	auto linkItr = links.find(linkId);
	
	if (linkItr == links.end())
		return nullptr;
	else
		return &linkItr->second;
}

const GraphNode * Graph::tryGetNode(const GraphNodeId nodeId) const
{
	return const_cast<Graph*>(this)->tryGetNode(nodeId);
}

const GraphLink * Graph::tryGetLink(const GraphLinkId linkId) const
{
	return const_cast<Graph*>(this)->tryGetLink(linkId);
}

bool Graph::loadXml(const XMLElement * xmlGraph, const Graph_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	Assert(nodes.empty());
	Assert(links.empty());
	
	nextNodeId = intAttrib(xmlGraph, "nextNodeId", nextNodeId);
	nextLinkId = intAttrib(xmlGraph, "nextLinkId", nextLinkId);
	
	for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
	{
	#if ENABLE_FILE_FIXUPS
		const int nodeType = intAttrib(xmlNode, "nodeType", 0);
		if (nodeType != 0)
			continue;
	#endif
	
		GraphNode node;
		node.id = intAttrib(xmlNode, "id", node.id);
		node.typeName = stringAttrib(xmlNode, "typeName", node.typeName.c_str());
		node.isPassthrough = boolAttrib(xmlNode, "passthrough", node.isPassthrough);
		node.editorValue = stringAttrib(xmlNode, "editorValue", node.editorValue.c_str());
		
		for (const XMLElement * xmlInput = xmlNode->FirstChildElement("input"); xmlInput != nullptr; xmlInput = xmlInput->NextSiblingElement("input"))
		{
			const std::string socket = stringAttrib(xmlInput, "socket", "");
			const std::string value = stringAttrib(xmlInput, "value", "");
			
			node.inputValues[socket] = value;
		}
		
		for (const XMLElement * xmlResource = xmlNode->FirstChildElement("resource"); xmlResource != nullptr; xmlResource = xmlResource->NextSiblingElement("resource"))
		{
			GraphNodeResource resource;
			
			resource.type = stringAttrib(xmlResource, "type", "");
			resource.name = stringAttrib(xmlResource, "name", "");
			resource.data = xmlResource->GetText();
			
			node.resources[resource.name] = resource;
		}
		
		addNode(node);
		
		nextNodeId = std::max(nextNodeId, node.id + 1);
	}
	
	for (const XMLElement * xmlLink = xmlGraph->FirstChildElement("link"); xmlLink != nullptr; xmlLink = xmlLink->NextSiblingElement("link"))
	{
		GraphLink link;
		link.id = intAttrib(xmlLink, "id", link.id);
		link.srcNodeId = intAttrib(xmlLink, "srcNodeId", link.srcNodeId);
		link.srcNodeSocketName = stringAttrib(xmlLink, "srcNodeSocketName", link.srcNodeSocketName.c_str());
		link.dstNodeId = intAttrib(xmlLink, "dstNodeId", link.dstNodeId);
		link.dstNodeSocketName = stringAttrib(xmlLink, "dstNodeSocketName", link.dstNodeSocketName.c_str());
		
		link.isDynamic = boolAttrib(xmlLink, "dynamic", link.isDynamic);
		link.isEnabled = boolAttrib(xmlLink, "enabled", link.isEnabled);
		
		for (const XMLElement * xmlParam = xmlLink->FirstChildElement("param"); xmlParam != nullptr; xmlParam = xmlParam->NextSiblingElement("param"))
		{
			const std::string name = stringAttrib(xmlParam, "name", "");
			const std::string value = stringAttrib(xmlParam, "value", "");
			
			Assert(name.empty() == false);
			if (name.empty() == false)
			{
				link.params[name] = value;
			}
		}
		
		const XMLElement * xmlEditor = xmlLink->FirstChildElement("editor");
		
		if (xmlEditor != nullptr)
		{
			for (const XMLElement * xmlRoutePoint = xmlEditor->FirstChildElement("routePoint"); xmlRoutePoint != nullptr; xmlRoutePoint = xmlRoutePoint->NextSiblingElement("routePoint"))
			{
				GraphLinkRoutePoint routePoint;
				routePoint.linkId = link.id;
				routePoint.x = floatAttrib(xmlRoutePoint, "x", 0.f);
				routePoint.y = floatAttrib(xmlRoutePoint, "y", 0.f);
				
				link.editorRoutePoints.push_back(routePoint);
			}
		}
		
		addLink(link, false);
		
		nextLinkId = std::max(nextLinkId, link.id + 1);
	}
	
	// determine socket indices based on src and dst socket names. These indices are used for rendering and also when saving to XML for easier instantiation in the run-time.
	
	for (auto & linkItr : links)
	{
		auto & link = linkItr.second;
		
		auto srcNode = tryGetNode(link.srcNodeId);
		
		if (srcNode)
		{
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
			
			if (typeDefinition)
			{
				// try to find the src socket index
				
				for (auto & inputSocket : typeDefinition->inputSockets)
				{
					if (inputSocket.name == link.srcNodeSocketName)
					{
						//LOG_DBG("srcNodeSocketIndex: %d -> %d", link.srcNodeSocketIndex, inputSocket.index);
						link.srcNodeSocketIndex = inputSocket.index;
						break;
					}
				}
				
				// if not found, check for renames
				
				if (link.srcNodeSocketIndex == -1)
				{
					for (auto & inputSocket : typeDefinition->inputSockets)
					{
						bool found = false;
						
						for (auto & rename : inputSocket.renames)
						{
							if (rename == link.srcNodeSocketName)
							{
								LOG_DBG("srcNodeSocketIndex: %d (%s) -> %d (%s)",
									link.srcNodeSocketIndex,
									link.srcNodeSocketName.c_str(),
									inputSocket.index,
									inputSocket.name.c_str());
								link.srcNodeSocketName = inputSocket.name;
								link.srcNodeSocketIndex = inputSocket.index;
								found = true;
								break;
							}
						}
						
						if (found)
							break;
					}
				}
			}
		}
		
		if (link.srcNodeSocketIndex == -1 && !link.isDynamic)
		{
			LOG_ERR("failed to find srcSocketIndex. linkId=%d, srcNodeId=%d, srcSocketName=%s", link.id, link.srcNodeId, link.srcNodeSocketName.c_str());
		}
		
		auto dstNode = tryGetNode(link.dstNodeId);
		
		if (dstNode)
		{
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
			
			if (typeDefinition)
			{
				for (auto & outputSocket : typeDefinition->outputSockets)
				{
					if (outputSocket.name == link.dstNodeSocketName)
					{
						//LOG_DBG("dstNodeSocketIndex: %d -> %d", link.dstNodeSocketIndex, outputSocket.index);
						link.dstNodeSocketIndex = outputSocket.index;
						break;
					}
				}
				
				// if not found, check for renames
				
				if (link.dstNodeSocketIndex == -1)
				{
					for (auto & outputSocket : typeDefinition->outputSockets)
					{
						bool found = false;
						
						for (auto & rename : outputSocket.renames)
						{
							if (rename == link.dstNodeSocketName)
							{
								LOG_DBG("dstNodeSocketIndex: %d (%s) -> %d (%s)",
									link.dstNodeSocketIndex,
									link.dstNodeSocketName.c_str(),
									outputSocket.index,
									outputSocket.name.c_str());
								link.dstNodeSocketName = outputSocket.name;
								link.dstNodeSocketIndex = outputSocket.index;
								found = true;
								break;
							}
						}
						
						if (found)
							break;
					}
				}
			}
		}
		
		if (link.dstNodeSocketIndex == -1 && !link.isDynamic)
		{
			LOG_ERR("failed to find dstSocketIndex. linkId=%d, dstNodeId=%d, dstSocketName=%s", link.id, link.dstNodeId, link.dstNodeSocketName.c_str());
		}
	}

	return true;
}

bool Graph::saveXml(XMLPrinter & xmlGraph, const Graph_TypeDefinitionLibrary * typeDefinitionLibrary) const
{
	bool result = true;
	
	xmlGraph.PushAttribute("nextNodeId", nextNodeId);
	xmlGraph.PushAttribute("nextLinkId", nextLinkId);
	
	for (auto & nodeItr : nodes)
	{
		auto & node = nodeItr.second;
		
		xmlGraph.OpenElement("node");
		{
			xmlGraph.PushAttribute("id", node.id);
			xmlGraph.PushAttribute("typeName", node.typeName.c_str());
			
			if (node.isPassthrough)
				xmlGraph.PushAttribute("passthrough", node.isPassthrough);
			
			if (!node.editorValue.empty())
				xmlGraph.PushAttribute("editorValue", node.editorValue.c_str());
			
			for (auto & resourceItr : node.resources)
			{
				auto & resource = resourceItr.second;
				
				xmlGraph.OpenElement("resource");
				{
					xmlGraph.PushAttribute("type", resource.type.c_str());
					xmlGraph.PushAttribute("name", resource.name.c_str());
					
					xmlGraph.PushText(resource.data.c_str(), true);
				}
				xmlGraph.CloseElement();
			}
			
			for (auto & input : node.inputValues)
			{
				xmlGraph.OpenElement("input");
				{
					xmlGraph.PushAttribute("socket", input.first.c_str());
					xmlGraph.PushAttribute("value", input.second.c_str());
				}
				xmlGraph.CloseElement();
			}
		}
		xmlGraph.CloseElement();
	}
	
	for (auto & linkItr : links)
	{
		auto & link = linkItr.second;
		
		xmlGraph.OpenElement("link");
		{
			xmlGraph.PushAttribute("id", link.id);
			xmlGraph.PushAttribute("srcNodeId", link.srcNodeId);
			xmlGraph.PushAttribute("srcNodeSocketName", link.srcNodeSocketName.c_str());
			xmlGraph.PushAttribute("dstNodeId", link.dstNodeId);
			xmlGraph.PushAttribute("dstNodeSocketName", link.dstNodeSocketName.c_str());
			
			if (link.isDynamic)
				xmlGraph.PushAttribute("dynamic", link.isDynamic);
			if (!link.isEnabled)
				xmlGraph.PushAttribute("enabled", link.isEnabled);
			
			for (auto & paramItr : link.params)
			{
				auto & name = paramItr.first;
				auto & value = paramItr.second;
				
				xmlGraph.OpenElement("param");
				{
					xmlGraph.PushAttribute("name", name.c_str());
					xmlGraph.PushAttribute("value", value.c_str());
				}
				xmlGraph.CloseElement();
			}
			
			if (link.editorRoutePoints.empty() == false)
			{
				xmlGraph.OpenElement("editor");
				{
					if (link.editorRoutePoints.empty() == false)
					{
						for (auto & routePoint : link.editorRoutePoints)
						{
							xmlGraph.OpenElement("routePoint");
							{
								xmlGraph.PushAttribute("x", routePoint.x);
								xmlGraph.PushAttribute("y", routePoint.y);
							}
							xmlGraph.CloseElement();
						}
					}
				}
				xmlGraph.CloseElement();
			}
		}
		xmlGraph.CloseElement();
	}
	
	return result;
}

bool Graph::load(const char * filename, const Graph_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	XMLDocument document;
	
	if (document.LoadFile(filename) != XML_SUCCESS)
	{
		LOG_ERR("failed to load %s", filename);
		return false;
	}
	
	XMLElement * xmlGraph = document.FirstChildElement("graph");
	
	if (xmlGraph == nullptr)
	{
		LOG_ERR("missing 'graph' XML element");
		return false;
	}
	
	return loadXml(xmlGraph, typeDefinitionLibrary);
}
