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

#include <GL/glew.h> // GL_TEXTURE_WIDTH, GL_TEXTURE_HEIGHT
#include "Calc.h"
#include "Debugging.h"
#include "graph.h"
#include "graphEdit_nodeResourceEditorWindow.h"
#include "graphEdit_nodeTypeSelect.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <algorithm>
#include <cmath>

using namespace tinyxml2;

#define ENABLE_FILE_FIXUPS 1

//

GraphNodeId kGraphNodeIdInvalid = 0;
GraphLinkId kGraphLinkIdInvalid = 0;

//

static const int kGridSize = 16;

//

static bool areCompatibleSocketLinkTypeNames(const std::string & srcTypeName, const bool srcTypeValidation, const std::string & dstTypeName)
{
	if (srcTypeValidation == false)
		return true;
	if (srcTypeName == dstTypeName)
		return true;
	
	return false;
}

static bool areNodeSocketsVisible(const GraphEdit::NodeData & nodeData)
{
	return nodeData.foldAnimProgress == 1.f;
}

static const std::string & getDisplayName(const GraphNode & node, const GraphEdit::NodeData & nodeData)
{
	return !nodeData.displayName.empty() ? nodeData.displayName : node.typeName;
}

const std::vector<GraphEdit_TypeDefinition::InputSocket> & getInputSockets(const GraphEdit_TypeDefinition & typeDefinition, const GraphEdit::NodeData & nodeData)
{
	return
		nodeData.dynamicSockets.hasDynamicSockets
		? nodeData.dynamicSockets.inputSockets
		: typeDefinition.inputSockets;
}

const std::vector<GraphEdit_TypeDefinition::OutputSocket> & getOutputSockets(const GraphEdit_TypeDefinition & typeDefinition, const GraphEdit::NodeData & nodeData)
{
	return
		nodeData.dynamicSockets.hasDynamicSockets
		? nodeData.dynamicSockets.outputSockets
		: typeDefinition.outputSockets;
}

//

static const float kNodeSx = 100.f;
static const float kNodePadding = 5.f;
static const float kNodeLabelSy = 15.f;
static const float kNodeSocketPaddingSy = 20.f;
static const float kNodeSocketRadius = 6.f;

static void getNodeRect(const int numInputs, const int numOutputs, const bool isFolded, float & sx, float & sy)
{
	if (isFolded)
	{
		sx = kNodeSx;
		sy = kNodePadding + kNodeLabelSy + kNodePadding;
	}
	else
	{
		const int numSockets = std::max(numInputs, numOutputs);
		
		sx = kNodeSx;
		sy = kNodePadding + kNodeLabelSy + kNodeSocketPaddingSy * numSockets + kNodePadding;
	}
}

static void getNodeInputSocketCircle(const int index, float & x, float & y, float & radius)
{
	x = 0.f;
	y = kNodePadding + kNodeLabelSy + kNodeSocketPaddingSy * (index + .5f);
	
	radius = kNodeSocketRadius;
}

static void getNodeOutputSocketCircle(const int index, float & x, float & y, float & radius)
{
	x = kNodeSx;
	y = kNodePadding + kNodeLabelSy + kNodeSocketPaddingSy * (index + .5f);
	
	radius = kNodeSocketRadius;
}

static bool testRectOverlap(
	const int _ax1, const int _ay1, const int _ax2, const int _ay2,
	const int _bx1, const int _by1, const int _bx2, const int _by2)
{
	const int ax1 = std::min(_ax1, _ax2);
	const int ay1 = std::min(_ay1, _ay2);
	const int ax2 = std::max(_ax1, _ax2);
	const int ay2 = std::max(_ay1, _ay2);
	
	const int bx1 = std::min(_bx1, _bx2);
	const int by1 = std::min(_by1, _by2);
	const int bx2 = std::max(_bx1, _bx2);
	const int by2 = std::max(_by1, _by2);
	
	if (ax2 < bx1 || ay2 < by1 || ax1 > bx2 || ay1 > by2)
		return false;
	else
		return true;
}

static bool testLineOverlap(
	const int lx1, const int ly1,
	const int lx2, const int ly2,
	const int cx, const int cy, const int cr)
{
	{
		const int dx = lx1 - cx;
		const int dy = ly1 - cy;
		const int dsSq = dx * dx + dy * dy;
		if (dsSq <= cr * cr)
			return true;
	}
	
	{
		const int dx = lx2 - cx;
		const int dy = ly2 - cy;
		const int dsSq = dx * dx + dy * dy;
		if (dsSq <= cr * cr)
			return true;
	}
	
	{
		const double ldx = lx2 - lx1;
		const double ldy = ly2 - ly1;
		const double lds = std::hypot(ldx, ldy);
		const double nx = -ldy / lds;
		const double ny = +ldx / lds;
		const double nd = nx * lx1 + ny * ly1;
		
		const double dMin = ldx * lx1 + ldy * ly1;
		const double dMax = ldx * lx2 + ldy * ly2;
		Assert(dMin <= dMax);
		
		const double dd = ldx * cx + ldy * cy;
		
		if (dd < dMin || dd > dMax)
			return false;
		
		const double dThreshold = cr;
		const double d = std::abs(cx * nx + cy * ny - nd);
		
		//printf("d = %f / %f\n", float(d), float(dThreshold));
		
		if (d <= dThreshold)
			return true;
	}
	
	return false;
}

static bool testCircleOverlap(
	const int x1, const int y1,
	const int x2, const int y2,
	const int r)
{
	const int dx = x2 - x1;
	const int dy = y2 - y1;
	const int dsSq = dx * dx + dy * dy;
	
	return dsSq <= r * r;
}

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
	
	resource = Resource();
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
	
	nodes.insert(std::pair<GraphNodeId, GraphNode>(node.id, node));
	
	if (graphEditConnection != nullptr)
	{
		graphEditConnection->nodeAdd(node.id, node.typeName);
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

bool Graph::loadXml(const XMLElement * xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
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
			GraphNode::Resource resource;
			
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
				for (auto & inputSocket : typeDefinition->inputSockets)
				{
					if (inputSocket.name == link.srcNodeSocketName)
					{
						//printf("srcNodeSocketIndex: %d -> %d\n", link.srcNodeSocketIndex, inputSocket.index);
						link.srcNodeSocketIndex = inputSocket.index;
						break;
					}
				}
			}
		}
		
		if (link.srcNodeSocketIndex == -1 && !link.isDynamic)
		{
			printf("failed to find srcSocketIndex. linkId=%d, srcNodeId=%d, srcSocketName=%s\n", link.id, link.srcNodeId, link.srcNodeSocketName.c_str());
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
						//printf("dstNodeSocketIndex: %d -> %d\n", link.dstNodeSocketIndex, outputSocket.index);
						link.dstNodeSocketIndex = outputSocket.index;
						break;
					}
				}
			}
		}
		
		if (link.dstNodeSocketIndex == -1 && !link.isDynamic)
		{
			printf("failed to find dstSocketIndex. linkId=%d, dstNodeId=%d, dstSocketName=%s\n", link.id, link.dstNodeId, link.dstNodeSocketName.c_str());
		}
	}

	return true;
}

bool Graph::saveXml(XMLPrinter & xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary) const
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

bool Graph::load(const char * filename, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	XMLDocument document;
	
	if (document.LoadFile(filename) != XML_SUCCESS)
		return false;
	
	XMLElement * xmlGraph = document.FirstChildElement("graph");
	
	if (xmlGraph == nullptr)
		return false;
	
	return loadXml(xmlGraph, typeDefinitionLibrary);
}
