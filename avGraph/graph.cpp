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

//

#include "framework.h"
#include "particle.h"
#include "ui.h"

int GRAPHEDIT_SX = 0;
int GRAPHEDIT_SY = 0;

static bool selectionMod()
{
	return keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
}

static bool commandMod()
{
#ifdef WIN32
	return keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL);
#else
	return keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);
#endif
}

static ParticleColor toParticleColor(const Color & color)
{
	ParticleColor particleColor;
	particleColor.rgba[0] = color.r;
	particleColor.rgba[1] = color.g;
	particleColor.rgba[2] = color.b;
	particleColor.rgba[3] = color.a;
	return particleColor;
}

static Color toColor(const ParticleColor & particleColor)
{
	Color color;
	color.r = particleColor.rgba[0];
	color.g = particleColor.rgba[1];
	color.b = particleColor.rgba[2];
	color.a = particleColor.rgba[3];
	return color;
}

bool GraphEdit_ValueTypeDefinition::loadXml(const XMLElement * xmlType)
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
		*this = GraphEdit_ValueTypeDefinition();
	}
	
	return result;
}

bool GraphEdit_EnumDefinition::loadXml(const XMLElement * xmlEnum)
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
		*this = GraphEdit_EnumDefinition();
	}
	
	return result;
}

bool GraphEdit_TypeDefinition::InputSocket::canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const GraphEdit_TypeDefinition::OutputSocket & socket) const
{
	auto valueTypeDefinition = typeDefintionLibrary->tryGetValueTypeDefinition(typeName);
	
	if (!areCompatibleSocketLinkTypeNames(typeName, valueTypeDefinition ? valueTypeDefinition->typeValidation : true, socket.typeName))
		return false;
	
	return true;
}

bool GraphEdit_TypeDefinition::OutputSocket::canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const GraphEdit_TypeDefinition::InputSocket & socket) const
{
	auto valueTypeDefinition = typeDefintionLibrary->tryGetValueTypeDefinition(socket.typeName);
	
	if (!areCompatibleSocketLinkTypeNames(socket.typeName, valueTypeDefinition ? valueTypeDefinition->typeValidation : true, typeName))
		return false;
	
	return true;
}

void GraphEdit_TypeDefinition::createUi()
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

bool GraphEdit_TypeDefinition::loadXml(const XMLElement * xmlType)
{
	bool result = true;
	
	typeName = stringAttrib(xmlType, "typeName", "");
	result &= !typeName.empty();
	
	displayName = stringAttrib(xmlType, "displayName", "");
	
	resourceTypeName = stringAttrib(xmlType, "resourceTypeName", "");
	
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
		*this = GraphEdit_TypeDefinition();
	}
	
	return result;
}

//

const GraphEdit_ValueTypeDefinition * GraphEdit_TypeDefinitionLibrary::tryGetValueTypeDefinition(const std::string & typeName) const
{
	auto i = valueTypeDefinitions.find(typeName);
	
	if (i != valueTypeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const GraphEdit_EnumDefinition * GraphEdit_TypeDefinitionLibrary::tryGetEnumDefinition(const std::string & typeName) const
{
	auto i = enumDefinitions.find(typeName);
	
	if (i != enumDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const GraphEdit_TypeDefinition * GraphEdit_TypeDefinitionLibrary::tryGetTypeDefinition(const std::string & typeName) const
{
	auto i = typeDefinitions.find(typeName);
	
	if (i != typeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

const GraphEdit_LinkTypeDefinition * GraphEdit_TypeDefinitionLibrary::tryGetLinkTypeDefinition(const std::string & srcTypeName, const std::string & dstTypeName) const
{
	auto key = std::make_pair(srcTypeName, dstTypeName);
	
	auto i = linkTypeDefinitions.find(key);
	
	if (i != linkTypeDefinitions.end())
		return &i->second;
	else
		return nullptr;
}

bool GraphEdit_TypeDefinitionLibrary::loadXml(const XMLElement * xmlLibrary)
{
	bool result = true;
	
	for (auto xmlType = xmlLibrary->FirstChildElement("valueType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("valueType"))
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		
		result &= typeDefinition.loadXml(xmlType);
		
		// check typeName doesn't exist yet
		
		auto itr = valueTypeDefinitions.find(typeDefinition.typeName);
		Assert(itr == valueTypeDefinitions.end());
		result &= itr == valueTypeDefinitions.end();
		
		valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	for (auto xmlEnum = xmlLibrary->FirstChildElement("enum"); xmlEnum != nullptr; xmlEnum = xmlEnum->NextSiblingElement("enum"))
	{
		GraphEdit_EnumDefinition enumDefinition;
		
		result &= enumDefinition.loadXml(xmlEnum);
		
		// check typeName doesn't exist yet
		
		auto itr = enumDefinitions.find(enumDefinition.enumName);
		Assert(itr == enumDefinitions.end());
		result &= itr == enumDefinitions.end();
		
		enumDefinitions[enumDefinition.enumName] = enumDefinition;
	}
	
	for (auto xmlType = xmlLibrary->FirstChildElement("linkType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("linkType"))
	{
		GraphEdit_LinkTypeDefinition typeDefinition;
		
		typeDefinition.srcTypeName = stringAttrib(xmlType, "srcTypeName", "");
		typeDefinition.dstTypeName = stringAttrib(xmlType, "dstTypeName", "");
		
		result &= typeDefinition.srcTypeName.empty() == false;
		result &= typeDefinition.dstTypeName.empty() == false;
		
		for (auto xmlParam = xmlType->FirstChildElement("param"); xmlParam != nullptr; xmlParam = xmlParam->NextSiblingElement("param"))
		{
			GraphEdit_LinkTypeDefinition::Param param;
			
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
		GraphEdit_TypeDefinition typeDefinition;
		
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

//

void GraphEdit_Visualizer::init(const GraphNodeId _nodeId, const std::string & _srcSocketName, const int _srcSocketIndex, const std::string & _dstSocketName, const int _dstSocketIndex)
{
	Assert(nodeId == kGraphNodeIdInvalid);
	Assert(srcSocketName.empty());
	Assert(srcSocketIndex == -1);
	Assert(dstSocketName.empty());
	Assert(dstSocketIndex == -1);
	Assert(value.empty());
	Assert(hasValue == false);
	Assert(texture == 0);
	
	nodeId = _nodeId;
	srcSocketName = _srcSocketName;
	srcSocketIndex = _srcSocketIndex;
	dstSocketName = _dstSocketName;
	dstSocketIndex = _dstSocketIndex;
	history.resize(100);
}

void GraphEdit_Visualizer::init()
{
	Assert(nodeId != kGraphNodeIdInvalid);
	Assert(!srcSocketName.empty() || !dstSocketName.empty());
	Assert(value.empty());
	Assert(hasValue == false);
	Assert(texture == 0);
	
	history.resize(100);
}

void GraphEdit_Visualizer::tick(const GraphEdit & graphEdit, const float dt)
{
	auto srcSocket = graphEdit.tryGetInputSocket(nodeId, srcSocketIndex);
	auto dstSocket = graphEdit.tryGetOutputSocket(nodeId, dstSocketIndex);
	
	texture = 0;
	
	channelData.clear();
	
	if (srcSocket != nullptr)
	{
		auto valueTypeDefinition = graphEdit.typeDefinitionLibrary->tryGetValueTypeDefinition(srcSocket->typeName);
		
		if (valueTypeDefinition == nullptr)
		{
			hasValue = false;
		}
		else
		{
			if (valueTypeDefinition->visualizer == "channels")
			{
				hasValue = graphEdit.realTimeConnection->getSrcSocketChannelData(nodeId, srcSocketIndex, srcSocket->name, channelData);
			}
			else
			{
				hasValue = graphEdit.realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocket->name, value);
			}
			
			if (hasValue == false)
			{
				if (srcSocket->hasDefaultValue)
				{
					value = srcSocket->defaultValue;
					
					hasValue = true;
				}
			}
		}
		
		if (hasValue)
		{
			//logDebug("real time srcSocket value: %s", value.c_str());
			
			if (valueTypeDefinition->visualizer == "valueplotter")
			{
				const float valueAsFloat = Parse::Float(value);
				
				history.add(valueAsFloat);
			}
			
			if (valueTypeDefinition->visualizer == "opengl-texture")
			{
				texture = Parse::Int32(value);
			}
		}
		else
		{
			value.clear();
			
			texture = 0;
			
			channelData.clear();
		}
	}
	
	if (dstSocket != nullptr)
	{
		auto valueTypeDefinition = graphEdit.typeDefinitionLibrary->tryGetValueTypeDefinition(dstSocket->typeName);
		
		if (valueTypeDefinition == nullptr)
		{
			hasValue = false;
		}
		else
		{
			if (valueTypeDefinition->visualizer == "channels")
			{
				hasValue = graphEdit.realTimeConnection->getDstSocketChannelData(nodeId, dstSocketIndex, dstSocket->name, channelData);
			}
			else
			{
				hasValue = graphEdit.realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocket->name, value);
			}
		}
		
		if (hasValue)
		{
			//logDebug("real time srcSocket value: %s", value.c_str());
			
			if (valueTypeDefinition->visualizer == "valueplotter")
			{
				const float valueAsFloat = Parse::Float(value);
				
				history.add(valueAsFloat);
			}
			
			if (valueTypeDefinition->visualizer == "opengl-texture")
			{
				texture = Parse::Int32(value);
			}
		}
		else
		{
			value.clear();
			
			texture = 0;
			
			channelData.clear();
		}
	}
	
	if (hasValue == false && nodeId != kGraphNodeIdInvalid)
	{
		//logDebug("reset realTimeSocketCapture");
		//*this = GraphEdit_Visualizer();
	}
	
	//
	
	const bool hasChannels = channelData.hasChannels();
	
	if (hasChannels)
	{
		float min = 0.f;
		float max = 0.f;
		
		bool isFirst = true;
		
		if (hasChannelDataMinMax)
		{
			isFirst = false;
			
			const float retain = powf(.7f, dt);
			
			min = channelDataMin * retain;
			max = channelDataMax * retain;
		}
		
		for (auto & channel : channelData.channels)
		{
			if (channel.numValues > 0)
			{
				if (isFirst)
				{
					isFirst = false;
					
					min = channel.values[0];
					max = channel.values[0];
				}
				
				for (int i = 0; i < channel.numValues; ++i)
				{
					const float value = channel.values[i];
					
					if (value < min)
						min = value;
					else if (value > max)
						max = value;
				}
			}
		}
		
		if (isFirst == false)
		{
			hasChannelDataMinMax = true;
			
			channelDataMin = min;
			channelDataMax = max;
		}
	}
}

void GraphEdit_Visualizer::measure(
	const GraphEdit & graphEdit, const std::string & nodeName,
	const int graphSx, const int graphSy,
	const int maxTextureSx, const int maxTextureSy,
	const int channelsSx, const int channelsSy,
	int & sx, int & sy) const
{
	const int kFontSize = 12;
	const int kPadding = 8;
	const int kElemPadding = 4;
	
	setFont("calibri.ttf");
	
	std::string caption;
	
	if (srcSocketIndex != -1)
	{
		auto srcSocket = graphEdit.tryGetInputSocket(nodeId, srcSocketIndex);
		
		if (srcSocket != nullptr)
			caption = srcSocket->name;
	}
	
	if (dstSocketIndex != -1)
	{
		auto dstSocket = graphEdit.tryGetOutputSocket(nodeId, dstSocketIndex);
		
		if (dstSocket != nullptr)
			caption = dstSocket->name;
	}
	
	if (!nodeName.empty())
	{
		caption = String::FormatC("%s : %s", nodeName.c_str(), caption.c_str());
	}
	
	float captionSx;
	float captionSy;
	measureText(kFontSize, captionSx, captionSy, "%s", caption.c_str());
	
	//
	
	float valueSx;
	float valueSy;
	measureText(kFontSize, valueSx, valueSy, "%s", value.c_str());
	
	//
	
	sx = 0;
	sy = 0;
	
	sx = std::max(sx, 120);
	sx = std::max(sx, int(captionSx));
	sx = std::max(sx, int(valueSx));
	
	sy += kPadding;
	
	sy += kFontSize; // caption
	sy += kElemPadding;
	sy += kFontSize; // value
	
	//
	
	const bool hasGraph = history.historySize > 0;
	
	if (hasGraph)
	{
		sy += kElemPadding;
		
		sx = std::max(sx, graphSx);
		sy += graphSy;
	}
	
	//
	
	const bool hasTexture = texture != 0;
	
	int textureSx = 0;
	int textureSy = 0;
	
	if (hasTexture)
	{
		sy += kElemPadding;
		
		GLint baseTextureSx;
		GLint baseTextureSy;
		gxSetTexture(texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &baseTextureSx);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &baseTextureSy);
		gxSetTexture(0);
		
		if (baseTextureSy == 1)
			baseTextureSy = std::max(1, baseTextureSx/4);
		
		const float scaleX = baseTextureSx == 0 ? 1.f : (maxTextureSx / float(baseTextureSx));
		const float scaleY = baseTextureSy == 0 ? 1.f : (maxTextureSy / float(baseTextureSy));
		const float scale = std::min(scaleX, scaleY);
		textureSx = std::floor(baseTextureSx * scale);
		textureSy = std::floor(baseTextureSy * scale);
		
		sx = std::max(sx, textureSx);
		sy += textureSy;
	}
	
	//
	
	const bool hasChannels = channelData.hasChannels();
	
	if (hasChannels)
	{
		sy += kElemPadding;
		
		sx = std::max(sx, channelsSx);
		sy += channelsSy;
	}
	
	// add a border around everything
	
	sx += kPadding * 2;
	sy += kPadding;
}

void GraphEdit_Visualizer::draw(const GraphEdit & graphEdit, const std::string & nodeName, const bool isSelected, const int * _sx, const int * _sy) const
{
	const int kFontSize = 12;
	const int kPadding = 8;
	const int kElemPadding = 4;
	
	setFont("calibri.ttf");
	
	int visualSx = 0;
	int visualSy = 0;
	
	if (_sx && _sy)
	{
		int minSx;
		int minSy;
		
		measure(graphEdit, nodeName, 0, 0, 0, 0, 0, 0, minSx, minSy);
		
		//logDebug("minSize: %d, %d", minSx, minSy);
		
		visualSx = *_sx - kPadding * 2;
		visualSy = *_sy - minSy;
	}
	
	std::string caption;
	
	if (srcSocketIndex != -1)
	{
		auto srcSocket = graphEdit.tryGetInputSocket(nodeId, srcSocketIndex);
		
		if (srcSocket != nullptr)
		{
			caption = srcSocket->displayName.empty()
				? srcSocket->name
				: srcSocket->displayName;
		}
	}
	
	if (dstSocketIndex != -1)
	{
		auto dstSocket = graphEdit.tryGetOutputSocket(nodeId, dstSocketIndex);
		
		if (dstSocket != nullptr)
		{
			caption = dstSocket->displayName.empty()
				? dstSocket->name
				: dstSocket->displayName;
		}
	}
	
	if (!nodeName.empty())
	{
		caption = String::FormatC("%s : %s", nodeName.c_str(), caption.c_str());
	}
	
	float captionSx;
	float captionSy;
	measureText(kFontSize, captionSx, captionSy, "%s", caption.c_str());
	
	//
	
	float valueSx;
	float valueSy;
	measureText(kFontSize, valueSx, valueSy, "%s", value.c_str());
	
	//
	
	int sx = 0;
	int sy = 0;
	
	sx = std::max(sx, 120);
	sx = std::max(sx, int(captionSx));
	sx = std::max(sx, int(valueSx));
	
	sy += kPadding;
	
	sy += kFontSize; // caption
	sy += kElemPadding;
	sy += kFontSize; // value
	
	//
	
	const bool hasGraph = history.historySize > 0;
	
	float graphMin = 0.f;
	float graphMax = 0.f;
	
	if (hasGraph)
	{
		history.getRange(graphMin, graphMax);
	}
	
	//
	
	const bool hasTexture = texture != 0;
	
	//
	
	const bool hasChannels = channelData.hasChannels();
	
	//
	
	const bool hasVisualSy = _sx != nullptr && _sy != nullptr;
	
	int perVisualSy = 0;
	
	if (hasVisualSy)
	{
		int numVisuals = 0;
		
		if (hasGraph)
			numVisuals++;
		if (hasTexture)
			numVisuals++;
		if (hasChannels)
			numVisuals++;
		
		if (numVisuals > 0)
		{
			perVisualSy = visualSy / numVisuals;
		}
		else
		{
			sx = std::max(sx, visualSx);
			sy += visualSy;
		}
	}
	
	//
	
	int graphSx = hasVisualSy ? (*_sx - kPadding * 2) : kDefaultGraphSx;
	int graphSy = hasVisualSy ? perVisualSy : kDefaultGraphSy;
	
	if (hasGraph)
	{
		sy += kElemPadding;
		
		sx = std::max(sx, graphSx);
		sy += graphSy;
	}
	
	//
	
	int textureSx = hasVisualSy ? (*_sx - kPadding * 2) : kDefaultMaxTextureSx;
	int textureSy = hasVisualSy ? perVisualSy : kDefaultMaxTextureSy;
	
	int textureAreaSx = 0;
	int textureAreaSy = 0;
	
	if (hasTexture)
	{
		sy += kElemPadding;
		
		GLint baseTextureSx;
		GLint baseTextureSy;
		gxSetTexture(texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &baseTextureSx);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &baseTextureSy);
		gxSetTexture(0);
		
		if (baseTextureSy == 1)
			baseTextureSy = std::max(1, baseTextureSx/4);
			
		const float scaleX = baseTextureSx == 0 ? 1.f : (textureSx / float(baseTextureSx));
		const float scaleY = baseTextureSy == 0 ? 1.f : (textureSy / float(baseTextureSy));
		const float scale = std::min(scaleX, scaleY);
		textureSx = std::floor(baseTextureSx * scale);
		textureSy = std::floor(baseTextureSy * scale);
		
		if (hasVisualSy)
		{
			textureAreaSx = (*_sx - kPadding * 2);
			textureAreaSy = perVisualSy;
		}
		else
		{
			textureAreaSx = textureSx;
			textureAreaSy = textureSy;
		}
		
		sx = std::max(sx, textureAreaSx);
		sy += textureAreaSy;
	}
	
	//
	
	int channelsSx = hasVisualSy ? (*_sx - kPadding * 2) : kDefaultChannelsSx;
	int channelsSy = hasVisualSy ? perVisualSy : kDefaultChannelsSy;
	
	if (hasChannels)
	{
		sy += kElemPadding;
		
		sx = std::max(sx, channelsSx);
		sy += channelsSy;
	}
	
	//
	
	sx += kPadding * 2;
	sy += kPadding;
	
	//
	
	if (isSelected)
		setColor(63, 63, 127, 191);
	else
		setColor(7, 7, 15, 191);
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	{
		hqFillRoundedRect(0, 0, sx, sy, 12.f);
	}
	hqEnd();
	
	if (isSelected)
		setColor(255, 255, 255);
	else
		setColor(191, 191, 191);
	//drawRectLine(0, 0, sx, sy);
	
	int y = kPadding;
	
	setColor(255, 255, 255);
	drawText(sx/2, y, kFontSize, 0.f, +1.f, "%s", caption.c_str());
	y += kFontSize;
	
	y += kElemPadding;
	if (hasValue)
	{
		setColor(191, 191, 255);
		drawText(sx/2, y, kFontSize, 0.f, +1.f, "%s", value.c_str());
	}
	else
	{
		setColor(255, 191, 127);
		drawText(sx/2, y, kFontSize, 0.f, +1.f, "n/a");
	}
	y += kFontSize;
	
	//
	
	if (hasGraph)
	{
		y += kElemPadding;
		
		const int graphX = (sx - graphSx) / 2;
		const int xOffset = history.maxHistorySize - history.historySize;
		
		setColor(127, 127, 255);
		gxBegin(GL_QUADS);
		{
			for (int i = 0; i < history.historySize; ++i)
			{
				const float value = history.getGraphValue(i);
				
				const float plotX1 = graphX + (i + 0 + xOffset) * graphSx / float(history.maxHistorySize);
				const float plotX2 = graphX + (i + 1 + xOffset) * graphSx / float(history.maxHistorySize);
				const float plotY = (graphMin == graphMax) ? .5f : (value - graphMax) / (graphMin - graphMax);
				
				const float x1 = plotX1;
				const float y1 = y + graphSy;
				const float x2 = plotX2;
				const float y2 = y + plotY * graphSy;
				
				gxVertex2f(x1, y1);
				gxVertex2f(x2, y1);
				gxVertex2f(x2, y2);
				gxVertex2f(x1, y2);
			}
		}
		gxEnd();
		
		setColor(255, 255, 255);
		drawText(graphX + graphSx - 3, y           + 4, 10, -1.f, +1.f, "%0.03f", graphMax);
		drawText(graphX + graphSx - 3, y + graphSy - 3, 10, -1.f, -1.f, "%0.03f", graphMin);
		
		setColor(colorWhite);
		//drawRectLine(graphX, y, graphX + graphSx, y + graphSy);
		
		y += graphSy;
	}
	
	//
	
	if (hasTexture)
	{
		y += kElemPadding;
		
		const int textureX = (sx - textureSx) / 2;
		const int textureY = (textureAreaSy - textureSy) / 2;
		
		if (texture != 0)
		{
			setColor(colorWhite);
			pushColorPost(POST_SET_ALPHA_TO_ONE);
			gxSetTexture(texture);
			{
				drawRect(textureX, y + textureY, textureX + textureSx, y + textureY + textureSy);
			}
			gxSetTexture(0);
			popColorPost();
		}
		
		y += textureAreaSy;
	}
	
	//
	
	if (hasChannels)
	{
		const int kDataBorder = 2;
		
		y += kElemPadding;
		
		const int channelsDataSx = channelsSx - kDataBorder * 2;
		const int channelsDataSy = channelsSy - kDataBorder * 2;
		
		//const int channelsEdgeX = (sx - channelsSx) / 2;
		const int channelsDataX = (sx - channelsDataSx) / 2;
		
		const int dataY = y + kDataBorder;
		
		const float min = channelDataMin;
		const float max = channelDataMax;
		
		const float strokeSize = 1.f * std::sqrt(std::abs(graphEdit.dragAndZoom.zoom));
		
		for (int c = 0; c < channelData.channels.size(); ++c)
		{
			auto & channel = channelData.channels[c];
			
			if (channel.numValues == 0)
				continue;
		
			setColor(Color::fromHSL(c / float(channelData.channels.size()), .5f, .5f));
			
			if (channel.continuous)
			{
				hqBegin(HQ_LINES, true);
				{
					if (channel.numValues == 1)
					{
						// special 'flat line' visual for single value channels
						
						const float value = channel.values[0];
							
						const float plotX1 = channelsDataX;
						const float plotX2 = channelsDataX + channelsDataSx;
						const float plotY = dataY + (min == max ? .5f : 1.f - (value - min) / (max - min)) * channelsDataSy;
						
						hqLine(plotX1, plotY, strokeSize, plotX2, plotY, strokeSize);
					}
					else
					{
					#if 1
						// connected lines between each sample with a maximum of kMaxValues-1 line segments
						
						const int kMaxValues = 1024;
						
						const int numValues = std::min(channel.numValues, kMaxValues);
						
						int total = 0;
						
						float lastX = 0.f;
						float lastY = 0.f;
			
						for (int i = 0; i < numValues; ++i)
						{
							const int s1 = (i + 0) * channel.numValues / numValues;
							const int s2 = (i + 1) * channel.numValues / numValues;
							
							total += s2 - s1;
							
							float sum = 0.f;
							
							for (int s = s1; s < s2; ++s)
								sum += channel.values[s];
							
							const float value = sum / (s2 - s1);
							
							const float plotX = channelsDataX + i * channelsDataSx / (numValues - 1.f);
							const float plotY = dataY + (min == max ? .5f : 1.f - (value - min) / (max - min)) * channelsDataSy;
							
							if (i > 0)
							{
								hqLine(lastX, lastY, strokeSize, plotX, plotY, strokeSize);
							}
							
							lastX = plotX;
							lastY = plotY;
						}
						
						Assert(total == channel.numValues);
						
					#else
						// connected lines between each sample
						
						float lastX = 0.f;
						float lastY = 0.f;
						
						for (int i = 0; i < channel.numValues; ++i)
						{
							const float value = channel.values[i];
							
							const float plotX = channelsDataX + i * channelsDataSx / (channel.numValues - 1.f);
							const float plotY = dataY + (min == max ? .5f : 1.f - (value - min) / (max - min)) * channelsDataSy;
							
							if (i > 0)
							{
								hqLine(lastX, lastY, strokeSize, plotX, plotY, strokeSize);
							}
							
							lastX = plotX;
							lastY = plotY;
						}
					#endif
					}
				}
				hqEnd();
			}
			else
			{
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 0; i < channel.numValues; ++i)
					{
						const float value = channel.values[i];
						
						const float plotX = channelsDataX + (channel.numValues == 1 ? .5f : i / (channel.numValues - 1.f)) * channelsDataSx;
						const float plotY = dataY + (min == max ? .5f : 1.f - (value - min) / (max - min)) * channelsDataSy;
						
						hqFillCircle(plotX, plotY, 2.f);
					}
				}
				hqEnd();
			}
		}
		
		setColor(255, 255, 255);
		beginTextBatch();
		drawText(channelsDataX                  + 3, dataY                  + 4, 10, +1.f, +1.f, "%d x %d", channelData.channels.size(), channelData.channels[0].numValues);
		drawText(channelsDataX + channelsDataSx - 3, dataY                  + 4, 10, -1.f, +1.f, "%0.03f", max);
		drawText(channelsDataX + channelsDataSx - 3, dataY + channelsDataSy - 3, 10, -1.f, -1.f, "%0.03f", min);
		endTextBatch();
		
		y += channelsSy;
	}
	
	//
	
	y += kPadding;
	//fassert(y == sy); // fixme
}

//

GraphEdit::EditorVisualizer::EditorVisualizer()
	: GraphEdit_Visualizer()
	, id(kGraphNodeIdInvalid)
	, x(0)
	, y(0)
	, sx(250)
	, sy(250)
	, zKey(-1)
{
}

void GraphEdit::EditorVisualizer::tick(const GraphEdit & graphEdit, const float dt)
{
	GraphEdit_Visualizer::tick(graphEdit, dt);
	
	updateSize(graphEdit);
}

void GraphEdit::EditorVisualizer::updateSize(const GraphEdit & graphEdit)
{
	if (sx == 0 || sy == 0)
	{
		if (hasValue)
		{
			int sxi = 0;
			int syi = 0;
			
			measure(graphEdit, "measure",
				kDefaultGraphSx, kDefaultGraphSy,
				kDefaultMaxTextureSx, kDefaultMaxTextureSy,
				kDefaultChannelsSx, kDefaultChannelsSy,
				sxi, syi);
			
			sx = sxi;
			sy = syi;
		}
	}
}

void GraphEdit::NodeData::setIsFolded(const bool _isFolded)
{
	if (_isFolded == isFolded)
		return;
	
	isFolded = _isFolded;
	foldAnimTimeRcp = 1.f / (_isFolded ? .1f : .07f) / 2.f;
}

//

GraphEdit::GraphEdit(
	const int _displaySx,
	const int _displaySy,
	GraphEdit_TypeDefinitionLibrary * _typeDefinitionLibrary,
	GraphEdit_RealTimeConnection * _realTimeConnection)
	: graph(nullptr)
	, nextZKey(1)
	, nodeDatas()
	, typeDefinitionLibrary(nullptr)
	, realTimeConnection(nullptr)
	, selectedNodes()
	, selectedLinks()
	, selectedLinkRoutePoints()
	, selectedVisualizers()
	, state(kState_Idle)
	, flags(kFlag_All)
	, nodeSelect()
	, socketConnect()
	, nodeResize()
	, nodeInsert()
	, nodeDoubleClickTime(0.f)
	, handleNodeDoubleClicked()
	, touches()
	, mousePosition()
	, dragAndZoom()
	, realTimeSocketCapture()
	, documentInfo()
	, editorOptions()
	, propertyEditor(nullptr)
	, linkParamsEditorLinkId(kGraphLinkIdInvalid)
	, nodeTypeNameSelect(nullptr)
	, nodeInsertMenu(nullptr)
	, nodeResourceEditor()
	, nodeResourceEditorWindows()
	, displaySx(0)
	, displaySy(0)
	, uiState(nullptr)
	, cursorHand(nullptr)
	, animationIsDone(true)
	, idleTime(0.f)
	, hideTime(1.f)
{
	graph = new Graph();
	
	graph->graphEditConnection = this;
	
	typeDefinitionLibrary = _typeDefinitionLibrary;
	
	realTimeConnection = _realTimeConnection;
	
	propertyEditor = new GraphUi::PropEdit(_typeDefinitionLibrary, this);
	
	nodeTypeNameSelect = new GraphUi::NodeTypeNameSelect(this);
	
	nodeInsertMenu = new GraphEdit_NodeTypeSelect();
	nodeInsertMenu->x = (_displaySx - nodeInsertMenu->sx)/2;
	nodeInsertMenu->y = (_displaySy - nodeInsertMenu->sy)/2;
	
	displaySx = _displaySx;
	displaySy = _displaySy;
	
	uiState = new UiState();
	
	const int kPadding = 10;
	uiState->sx = 200;
	uiState->x = kPadding;
	uiState->y = kPadding;
	uiState->textBoxTextOffset = 64;
	
	cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	
	//
	
	propertyEditor->setGraph(graph);
}

GraphEdit::~GraphEdit()
{
	selectedNodes.clear();
	
	SDL_FreeCursor(cursorHand);
	cursorHand = nullptr;
	
	delete uiState;
	uiState = nullptr;
	
	delete nodeTypeNameSelect;
	nodeTypeNameSelect = nullptr;
	
	delete propertyEditor;
	propertyEditor = nullptr;
	
	delete nodeInsertMenu;
	nodeInsertMenu = nullptr;
	
	nodeDatas.clear();
	
	delete graph;
	graph = nullptr;
}

GraphNode * GraphEdit::tryGetNode(const GraphNodeId id) const
{
	auto i = graph->nodes.find(id);
	
	if (i != graph->nodes.end())
		return &i->second;
	else
		return nullptr;
}

GraphEdit::NodeData * GraphEdit::tryGetNodeData(const GraphNodeId nodeId) const
{
	auto i = nodeDatas.find(nodeId);
	
	if (i != nodeDatas.end())
		return const_cast<GraphEdit::NodeData*>(&i->second);
	else
		return nullptr;
}

GraphLink * GraphEdit::tryGetLink(const GraphLinkId id) const
{
	auto i = graph->links.find(id);
	
	if (i != graph->links.end())
		return &i->second;
	else
		return nullptr;
}

const GraphEdit_TypeDefinition::InputSocket * GraphEdit::tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const
{
	auto node = tryGetNode(nodeId);
	
	Assert(node);
	if (node == nullptr)
		return nullptr;
	
	const GraphEdit_TypeDefinition * typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
	
	if (typeDefinition == nullptr)
		return nullptr;
	
	auto nodeData = tryGetNodeData(nodeId);
	
	Assert(nodeData);
	if (nodeData == nullptr)
		return nullptr;
	
	auto & inputSockets = getInputSockets(*typeDefinition, *nodeData);
	
	if (socketIndex < 0 || socketIndex >= inputSockets.size())
		return nullptr;
	return &inputSockets[socketIndex];
}

const GraphEdit_TypeDefinition::OutputSocket * GraphEdit::tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const
{
	auto node = tryGetNode(nodeId);
	
	Assert(node);
	if (node == nullptr)
		return nullptr;
	
	auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
	
	Assert(typeDefinition);
	if (typeDefinition == nullptr)
		return nullptr;
	
	auto nodeData = tryGetNodeData(nodeId);
	
	Assert(nodeData);
	if (nodeData == nullptr)
		return nullptr;
	
	auto & outputSockets = getOutputSockets(*typeDefinition, *nodeData);
	
	if (socketIndex < 0 || socketIndex >= outputSockets.size())
		return nullptr;
	return &outputSockets[socketIndex];
}

bool GraphEdit::getLinkPath(const GraphLinkId linkId, LinkPath & path) const
{
	path = LinkPath();
	
	//
	
	auto linkItr = graph->links.find(linkId);
	
	Assert(linkItr != graph->links.end());
	if (linkItr == graph->links.end())
		return false;
	
	//
	
	auto & link = linkItr->second;
	
	//
	
	auto srcNode = tryGetNode(link.srcNodeId);
	auto dstNode = tryGetNode(link.dstNodeId);
	
	auto srcNodeData = tryGetNodeData(link.srcNodeId);
	auto dstNodeData = tryGetNodeData(link.dstNodeId);
	
	auto inputSocket = tryGetInputSocket(link.srcNodeId, link.srcNodeSocketIndex);
	auto outputSocket = tryGetOutputSocket(link.dstNodeId, link.dstNodeSocketIndex);
	
	if (srcNode == nullptr ||
		dstNode == nullptr ||
		srcNodeData == nullptr ||
		dstNodeData == nullptr ||
		inputSocket == nullptr ||
		outputSocket == nullptr)
	{
		return false;
	}
	else
	{
		auto srcTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
		auto dstTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
		
		float srcSx, srcSy;
		float dstSx, dstSy;
		
		if (srcTypeDefinition)
		{
			auto & inputSockets = getInputSockets(*srcTypeDefinition, *srcNodeData);
			auto & outputSockets = getOutputSockets(*dstTypeDefinition, *dstNodeData);
		
			getNodeRect(inputSockets.size(), outputSockets.size(), true, srcSx, srcSy);
		}
		else
			srcSx = srcSy = 0.f;
		
		if (dstTypeDefinition)
		{
			auto & inputSockets = getInputSockets(*dstTypeDefinition, *dstNodeData);
			auto & outputSockets = getOutputSockets(*dstTypeDefinition, *dstNodeData);
			
			getNodeRect(inputSockets.size(), outputSockets.size(), true, dstSx, dstSy);
		}
		else
			dstSx = dstSy = 0.f;
		
		const bool srcNodeSocketsAreVisible = areNodeSocketsVisible(*srcNodeData);
		const bool dstNodeSocketsAreVisible = areNodeSocketsVisible(*dstNodeData);
		
		float srcSocketX, srcSocketY, srcSocketRadius;
		getNodeInputSocketCircle(inputSocket->index, srcSocketX, srcSocketY, srcSocketRadius);
		
		float dstSocketX, dstSocketY, dstSocketRadius;
		getNodeOutputSocketCircle(outputSocket->index, dstSocketX, dstSocketY, dstSocketRadius);
		
		const float srcX = srcNodeData->x + srcSocketX;
		const float srcY = srcNodeData->y + (srcNodeSocketsAreVisible ? srcSocketY : srcSy/2.f);
		const float dstX = dstNodeData->x + dstSocketX;
		const float dstY = dstNodeData->y + (dstNodeSocketsAreVisible ? dstSocketY : dstSy/2.f);
		
		LinkPath::Point p;
		
		p.x = srcX;
		p.y = srcY;
		path.points.push_back(p);
		
		for (auto & routePoint : link.editorRoutePoints)
		{
			p.x = routePoint.x;
			p.y = routePoint.y;
			path.points.push_back(p);
		}
		
		p.x = dstX;
		p.y = dstY;
		path.points.push_back(p);
		
		return true;
	}
}

const GraphEdit_LinkTypeDefinition * GraphEdit::tryGetLinkTypeDefinition(const GraphLinkId linkId) const
{
	const GraphEdit_LinkTypeDefinition * result = nullptr;
	
	auto link = tryGetLink(linkId);
	Assert(link != nullptr);
	
	if (link != nullptr)
	{
		auto srcSocket = tryGetInputSocket(link->srcNodeId, link->srcNodeSocketIndex);
		auto dstSocket = tryGetOutputSocket(link->dstNodeId, link->dstNodeSocketIndex);
		
		Assert(srcSocket != nullptr);
		Assert(dstSocket != nullptr);
		
		if (srcSocket != nullptr && dstSocket != nullptr)
		{
			result = typeDefinitionLibrary->tryGetLinkTypeDefinition(
				srcSocket->typeName,
				dstSocket->typeName);
		}
	}
	
	return result;
}

GraphEdit::EditorVisualizer * GraphEdit::tryGetVisualizer(const GraphNodeId id) const
{
	auto i = visualizers.find(id);
	
	if (i != visualizers.end())
		return const_cast<EditorVisualizer*>(&i->second);
	else
		return nullptr;
}

bool GraphEdit::enabled(const int flag) const
{
	return (flags & flag) == flag;
}

struct SortedGraphElem
{
	GraphNode * node;
	GraphEdit::NodeData * nodeData;
	GraphEdit::EditorVisualizer * visualizer;
	
	int zKey;
};

bool GraphEdit::hitTest(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	// sort nodes and visualizers according to Z key
	
	const int numElems = graph->nodes.size() + visualizers.size();
	
	SortedGraphElem * sortedElems = (SortedGraphElem*)alloca(numElems * sizeof(SortedGraphElem));
	memset(sortedElems, 0, numElems * sizeof(SortedGraphElem));
	
	int elemIndex = 0;
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		auto nodeData = tryGetNodeData(node.id);
		
		sortedElems[elemIndex].node = &node;
		sortedElems[elemIndex].nodeData = nodeData;
		sortedElems[elemIndex].zKey = nodeData->zKey;
		
		elemIndex++;
	}
	
	for (auto & visualizerItr : visualizers)
	{
		auto & visualizer = visualizerItr.second;
		
		sortedElems[elemIndex].visualizer = const_cast<EditorVisualizer*>(&visualizer);
		sortedElems[elemIndex].zKey = visualizer.zKey;
		
		elemIndex++;
	}
	
	Assert(elemIndex == numElems);
	
	std::sort(sortedElems, sortedElems + numElems, [](const SortedGraphElem & e1, const SortedGraphElem & e2) { return e2.zKey < e1.zKey; });
	
	// traverse elements hit testing against the ones on top first
	
	for (int i = 0; i < numElems; ++i)
	{
		if (sortedElems[i].node != nullptr)
		{
			auto & node = *sortedElems[i].node;
			auto & nodeData = *sortedElems[i].nodeData;
			
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
			
			if (typeDefinition == nullptr)
			{
				typeDefinition = &emptyTypeDefinition;
			}
			
			if (typeDefinition != nullptr)
			{
				NodeHitTestResult hitTestResult;
				
				const bool socketsAreVisible = areNodeSocketsVisible(nodeData);
				
				if (hitTestNode(nodeData, *typeDefinition, x - nodeData.x, y - nodeData.y, socketsAreVisible, hitTestResult))
				{
					result.hasNode = true;
					result.node = &node;
					result.nodeHitTestResult = hitTestResult;
					return true;
				}
			}
		}
		else if (sortedElems[i].visualizer != nullptr)
		{
			auto & visualizer = *sortedElems[i].visualizer;
			
			if (testRectOverlap(
				x, y,
				x, y,
				visualizer.x,
				visualizer.y,
				visualizer.x + visualizer.sx,
				visualizer.y + visualizer.sy))
			{
				VisualizerHitTestResult hitTestResult;
				
				const int borderSize = 6;
				
				if (testRectOverlap(
					x, y,
					x, y,
					visualizer.x + borderSize * +0 + visualizer.sx * 0,
					visualizer.y + borderSize * +0 + visualizer.sy * 0,
					visualizer.x + borderSize * +1 + visualizer.sx * 0,
					visualizer.y + borderSize * +0 + visualizer.sy * 1))
				{
					hitTestResult.borderL = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					visualizer.x + borderSize * -1 + visualizer.sx * 1,
					visualizer.y + borderSize * +0 + visualizer.sy * 0,
					visualizer.x + borderSize * +0 + visualizer.sx * 1,
					visualizer.y + borderSize * +0 + visualizer.sy * 1))
				{
					hitTestResult.borderR = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					visualizer.x + borderSize * +0 + visualizer.sx * 0,
					visualizer.y + borderSize * +0 + visualizer.sy * 0,
					visualizer.x + borderSize * +0 + visualizer.sx * 1,
					visualizer.y + borderSize * +1 + visualizer.sy * 0))
				{
					hitTestResult.borderT = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					visualizer.x + borderSize * +0 + visualizer.sx * 0,
					visualizer.y + borderSize * -1 + visualizer.sy * 1,
					visualizer.x + borderSize * +0 + visualizer.sx * 1,
					visualizer.y + borderSize * +0 + visualizer.sy * 1))
				{
					hitTestResult.borderB = true;
				}
				
				if (hitTestResult.borderL || hitTestResult.borderR || hitTestResult.borderT || hitTestResult.borderB)
				{
					// border, not background
				}
				else
				{
					hitTestResult.background = true;
				}
				
				result.hasVisualizer = true;
				result.visualizer = &visualizer;
				result.visualizerHitTestResult = hitTestResult;
				
				return true;
			}
		}
		else
		{
			Assert(false);
		}
	}
	
	for (auto linkItr = graph->links.rbegin(); linkItr != graph->links.rend(); ++linkItr)
	{
		auto linkId = linkItr->first;
		auto & link = linkItr->second;
		
		for (auto & routePoint : link.editorRoutePoints)
		{
			if (testCircleOverlap(routePoint.x, routePoint.y, x, y, 5))
			{
				result.hasLinkRoutePoint = true;
				result.linkRoutePoint = &routePoint;
				return true;
			}
		}
		
		LinkPath path;
			
		if (getLinkPath(linkId, path) == false)
			continue;
		
		float x1 = path.points[0].x;
		float y1 = path.points[0].y;
		
		for (int i = 1; i < path.points.size(); ++i)
		{
			const float x2 = path.points[i].x;
			const float y2 = path.points[i].y;
			
			if (testLineOverlap(x1, y1, x2, y2, x, y, 5.f))
			{
				result.hasLink = true;
				result.link = &link;
				result.linkSegmentIndex = i - 1;
				return true;
			}
			
			x1 = x2;
			y1 = y2;
		}
	}
	
	return false;
}

bool GraphEdit::hitTestNode(const NodeData & nodeData, const GraphEdit_TypeDefinition & typeDefinition, const float x, const float y, const bool socketsAreVisible, NodeHitTestResult & result) const
{
	result = NodeHitTestResult();
	
	auto & inputSockets = getInputSockets(typeDefinition, nodeData);
	auto & outputSockets = getOutputSockets(typeDefinition, nodeData);
	
	if (socketsAreVisible)
	{
		for (auto & inputSocket : inputSockets)
		{
			float socketX, socketY, socketRadius;
			getNodeInputSocketCircle(inputSocket.index, socketX, socketY, socketRadius);
			
			const float dx = x - socketX;
			const float dy = y - socketY;
			const float ds = std::hypotf(dx, dy);
			
			if (ds <= socketRadius)
			{
				result.inputSocket = &inputSocket;
				return true;
			}
		}
		
		for (auto & outputSocket : outputSockets)
		{
			float socketX, socketY, socketRadius;
			getNodeOutputSocketCircle(outputSocket.index, socketX, socketY, socketRadius);
			
			const float dx = x - socketX;
			const float dy = y - socketY;
			const float ds = std::hypotf(dx, dy);
			
			if (ds <= socketRadius)
			{
				result.outputSocket = &outputSocket;
				return true;
			}
		}
	}
	
	float sx;
	float sy;
	getNodeRect(inputSockets.size(), outputSockets.size(), !socketsAreVisible, sx, sy);
	
	if (x >= 0.f && y >= 0.f && x < sx && y < sy)
	{
		result.background = true;
		return true;
	}
	
	return false;
}

bool GraphEdit::tick(const float dt, const bool _inputIsCaptured)
{
	cpuTimingBlock(GraphEdit_Tick);
	
	GRAPHEDIT_SX = displaySx;
	GRAPHEDIT_SY = displaySy;
	
#if defined(DEBUG) // todo : remove
#if !defined(WINDOWS) // fixme : compile error ?
	for (auto & nodeItr : graph->nodes)
		Assert(nodeDatas.count(nodeItr.first) != 0);
	for (auto & nodeDataItr : nodeDatas)
		Assert(graph->nodes.count(nodeDataItr.first) != 0);
#endif
#endif

	if (_inputIsCaptured)
	{
		cancelEditing();
	}
	
	{
		mousePosition.uiX = mouse.x;
		mousePosition.uiY = mouse.y;
		
		const Vec2 srcMousePosition(mouse.x, mouse.y);
		const Vec2 dstMousePosition = dragAndZoom.invTransform * srcMousePosition;
		mousePosition.dx = dstMousePosition[0] - mousePosition.x;
		mousePosition.dy = dstMousePosition[1] - mousePosition.y;
		mousePosition.x = dstMousePosition[0];
		mousePosition.y = dstMousePosition[1];
	}
	
	// update dynamic sockets
	
	for (auto & nodeDataItr : nodeDatas)
	{
		auto nodeId = nodeDataItr.first;
		auto & nodeData = nodeDataItr.second;
		auto & node = graph->nodes[nodeId];
		
		std::vector<GraphEdit_RealTimeConnection::DynamicInput> inputs;
		std::vector<GraphEdit_RealTimeConnection::DynamicOutput> outputs;
		
		if (realTimeConnection == nullptr || realTimeConnection->getNodeDynamicSockets(node.id, inputs, outputs) == false)
		{
			nodeData.dynamicSockets.reset();
		}
		else
		{
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
			
			nodeData.dynamicSockets.update(*typeDefinition, inputs, outputs);
		}
	}
	
	// update dynamic links
	
	for (auto & linkItr : graph->links)
	{
		auto & link = linkItr.second;
		
		if (!link.isDynamic)
			continue;
		
		resolveSocketIndices(
			link.srcNodeId, link.srcNodeSocketName, link.srcNodeSocketIndex,
			link.dstNodeId, link.dstNodeSocketName, link.dstNodeSocketIndex);
	}
	
	// update dynamic visualizers
	
	for (auto & visualizerItr : visualizers)
	{
		auto & visualizer = visualizerItr.second;
		
		resolveSocketIndices(
			visualizer.nodeId, visualizer.srcSocketName, visualizer.srcSocketIndex,
			visualizer.nodeId, visualizer.dstSocketName, visualizer.dstSocketIndex);
	}
	
	// update node resource editor windows
	
	for (auto resourceEditorItr = nodeResourceEditorWindows.begin(); resourceEditorItr != nodeResourceEditorWindows.end(); )
	{
		auto & resourceEditor = *resourceEditorItr;
		
		if (resourceEditor->tick(dt))
		{
			delete resourceEditor;
			resourceEditor = nullptr;
			
			resourceEditorItr = nodeResourceEditorWindows.erase(resourceEditorItr);
		}
		else
		{
			resourceEditorItr++;
		}
	}
	
	//
	
	if (realTimeConnection != nullptr)
	{
		bool isValid = false;
		
		HitTestResult result;
		
		if (hitTest(mousePosition.x, mousePosition.y, result))
		{
			if (result.hasNode && result.nodeHitTestResult.inputSocket != nullptr)
			{
				isValid = true;
				
				auto srcSocket = result.nodeHitTestResult.inputSocket;
				
				auto nodeId = result.node->id;
				auto & srcSocketName = srcSocket->name;
				auto srcSocketIndex = srcSocket->index;
				
				if (nodeId != realTimeSocketCapture.visualizer.nodeId ||
					srcSocketName != realTimeSocketCapture.visualizer.srcSocketName ||
					srcSocketIndex != realTimeSocketCapture.visualizer.srcSocketIndex)
				{
					//logDebug("reset realTimeSocketCapture");
					realTimeSocketCapture = RealTimeSocketCapture();
					
					realTimeSocketCapture.visualizer.init(nodeId, srcSocketName, srcSocketIndex, String::Empty, -1);
				}
			}
			
			if (result.hasNode && result.nodeHitTestResult.outputSocket != nullptr)
			{
				isValid = true;
				
				auto dstSocket = result.nodeHitTestResult.outputSocket;
				
				auto nodeId = result.node->id;
				auto & dstSocketName = dstSocket->name;
				auto dstSocketIndex = dstSocket->index;
				
				if (nodeId != realTimeSocketCapture.visualizer.nodeId ||
					dstSocketName != realTimeSocketCapture.visualizer.dstSocketName ||
					dstSocketIndex != realTimeSocketCapture.visualizer.dstSocketIndex)
				{
					//logDebug("reset realTimeSocketCapture");
					realTimeSocketCapture = RealTimeSocketCapture();
					
					realTimeSocketCapture.visualizer.init(nodeId, String::Empty, -1, dstSocketName, dstSocketIndex);
				}
			}
			
			if (result.hasLink)
			{
				isValid = true;
				
				auto nodeId = result.link->dstNodeId;
				auto & dstSocketName = result.link->dstNodeSocketName;
				auto dstSocketIndex = result.link->dstNodeSocketIndex;
				
				if (nodeId != realTimeSocketCapture.visualizer.nodeId ||
					dstSocketName != realTimeSocketCapture.visualizer.dstSocketName ||
					dstSocketIndex != realTimeSocketCapture.visualizer.dstSocketIndex)
				{
					//logDebug("reset realTimeSocketCapture");
					realTimeSocketCapture = RealTimeSocketCapture();
					
					realTimeSocketCapture.visualizer.init(nodeId, String::Empty, -1, dstSocketName, dstSocketIndex);
				}
			}
		}
		
		if (isValid == false && realTimeSocketCapture.visualizer.nodeId != kGraphNodeIdInvalid)
		{
			//logDebug("reset realTimeSocketCapture");
			realTimeSocketCapture = RealTimeSocketCapture();
		}
	}
	
	//
	
	bool inputIsCaptured = _inputIsCaptured;
	
	if (nodeResourceEditor.resourceEditor != nullptr)
	{
		inputIsCaptured |= nodeResourceEditor.resourceEditor->tick(dt, inputIsCaptured);
	}
	
	inputIsCaptured |= (state != kState_Idle);
	
	if (inputIsCaptured == false || state == kState_TouchDrag || state == kState_TouchZoom)
	{
		inputIsCaptured |= tickTouches();
	}
	else
	{
		touches = Touches();
	}
	
	makeActive(uiState, true, false);
	
	if (inputIsCaptured == false)
	{
		doMenu(dt);
		
		doLinkParams(dt);
		
		doEditorOptions(dt);
		
		nodeTypeNameSelect->doMenus(uiState, dt);
		
		propertyEditor->doMenus(uiState, dt);
		
		inputIsCaptured |= uiState->activeElem != nullptr;
	}
	
	//
	
	const bool isIdle = isInputIdle() && (state == kState_Idle || state == kState_HiddenIdle);
	
	if (isIdle)
		idleTime += dt;
	else
		idleTime = 0.f;
	
	//
	
	highlightedSockets = SocketSelection();
	highlightedLinks.clear();
	highlightedLinkRoutePoints.clear();
	
	mousePosition.hover = false;
	
	switch (state)
	{
	case kState_Idle:
		{
			if (inputIsCaptured)
				break;
			
			HitTestResult hitTestResult;
			
			if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
			{
				if (hitTestResult.hasNode)
				{
					if (hitTestResult.nodeHitTestResult.inputSocket)
					{
						highlightedSockets.srcNodeId = hitTestResult.node->id;
						highlightedSockets.srcNodeSocket = hitTestResult.nodeHitTestResult.inputSocket;
						
						mousePosition.hover = true;
					}
					
					if (hitTestResult.nodeHitTestResult.outputSocket)
					{
						highlightedSockets.dstNodeId = hitTestResult.node->id;
						highlightedSockets.dstNodeSocket = hitTestResult.nodeHitTestResult.outputSocket;
						
						mousePosition.hover = true;
					}
				}
				
				if (hitTestResult.hasLink)
				{
					highlightedLinks.insert(hitTestResult.link->id);
				}
				
				if (hitTestResult.hasLinkRoutePoint)
				{
					highlightedLinkRoutePoints.insert(hitTestResult.linkRoutePoint);
					
					mousePosition.hover = true;
				}
				
				if (hitTestResult.hasVisualizer)
				{
					if (hitTestResult.visualizerHitTestResult.borderL ||
						hitTestResult.visualizerHitTestResult.borderR ||
						hitTestResult.visualizerHitTestResult.borderT ||
						hitTestResult.visualizerHitTestResult.borderB)
					{
						mousePosition.hover = true;
					}
				}
			}
			
			if (mouse.wentDown(BUTTON_LEFT))
			{
				const bool appendSelection = selectionMod();
				
				HitTestResult hitTestResult;
				
				if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
				{
					// todo : clear node selection ?
					// todo : clear link selection ?
					// todo : make method to update selection and move logic for selecting/deselecting items there ?
					
					if (hitTestResult.hasNode)
					{
						if (enabled(kFlag_LinkAdd) && hitTestResult.nodeHitTestResult.inputSocket)
						{
							socketConnect.srcNodeId = hitTestResult.node->id;
							socketConnect.srcNodeSocket = hitTestResult.nodeHitTestResult.inputSocket;
							
							state = kState_InputSocketConnect;
							break;
						}
						
						if (enabled(kFlag_LinkAdd) && hitTestResult.nodeHitTestResult.outputSocket)
						{
							socketConnect.dstNodeId = hitTestResult.node->id;
							socketConnect.dstNodeSocket = hitTestResult.nodeHitTestResult.outputSocket;
							
							state = kState_OutputSocketConnect;
							break;
						}
						
						if (enabled(kFlag_Select))
						{
							if (appendSelection == false)
							{
								// todo : implement the following bahvior: click + hold/move = drag, click + release = select single and discard the rest of the selection
								
								if (selectedNodes.count(hitTestResult.node->id) == 0)
								{
									selectNode(hitTestResult.node->id, true);
									
									auto nodeData = tryGetNodeData(hitTestResult.node->id);
									nodeData->zKey = nextZKey++;
								}
							}
							else
							{
								if (selectedNodes.count(hitTestResult.node->id) == 0)
									selectNode(hitTestResult.node->id, false);
								else
									selectedNodes.erase(hitTestResult.node->id);
							}
						}
						
						if (hitTestResult.nodeHitTestResult.background)
						{
							if (appendSelection == false && nodeDoubleClickTime > 0.f)
							{
								if (enabled(kFlag_NodeResourceEdit))
								{
									nodeDoubleClickTime = 0.f;
									
									if (handleNodeDoubleClicked != nullptr)
									{
										handleNodeDoubleClicked(hitTestResult.node->id);
									}
									
								#if 1
									GraphEdit_NodeResourceEditorWindow * resourceEditorWindow = new GraphEdit_NodeResourceEditorWindow();
									
									if (resourceEditorWindow->init(this, hitTestResult.node->id))
									{
										nodeResourceEditorWindows.push_back(resourceEditorWindow);
									}
									else
									{
										delete resourceEditorWindow;
										resourceEditorWindow = nullptr;
										
										showNotification("Failed to create resource editor!");
									}
								#else
									if (nodeResourceEditBegin(hitTestResult.node->id))
									{
										state = kState_NodeResourceEdit;
										break;
									}
								#endif
								}
							}
							else
							{
								nodeDoubleClickTime = .4f;
								
								if (enabled(kFlag_NodeDrag))
								{
									state = kState_NodeDrag;
									break;
								}
							}
						}
					}
					
					if (enabled(kFlag_NodeAdd) && hitTestResult.hasLink && commandMod())
					{
						nodeInsert.x = mousePosition.x;
						nodeInsert.y = mousePosition.y;
						nodeInsert.linkId = hitTestResult.link->id;
					
						state = kState_NodeInsert;
						break;
					}
					
					if (enabled(kFlag_Select) && hitTestResult.hasLink)
					{
						if (appendSelection == false)
						{
							selectLink(hitTestResult.link->id, true);
						}
						else
						{
							if (selectedLinks.count(hitTestResult.link->id) == 0)
								selectLink(hitTestResult.link->id, false);
							else
								selectedLinks.erase(hitTestResult.link->id);
						}
					}
					
					if (enabled(kFlag_Select) && hitTestResult.hasLinkRoutePoint)
					{
						if (appendSelection == false)
						{
							selectLinkRoutePoint(hitTestResult.linkRoutePoint, true);
						}
						else
						{
							if (selectedLinkRoutePoints.count(hitTestResult.linkRoutePoint) == 0)
								selectLinkRoutePoint(hitTestResult.linkRoutePoint, false);
							else
								selectedLinkRoutePoints.erase(hitTestResult.linkRoutePoint);
						}
						
						if (enabled(kFlag_NodeDrag))
						{
							state = kState_NodeDrag;
							break;
						}
					}
					
					if (enabled(kFlag_Select) && hitTestResult.hasVisualizer)
					{
						if (appendSelection == false)
						{
							if (selectedVisualizers.count(hitTestResult.visualizer) == 0)
							{
								selectVisualizer(hitTestResult.visualizer, true);
								
								hitTestResult.visualizer->zKey = nextZKey++;
							}
						}
						else
						{
							if (selectedVisualizers.count(hitTestResult.visualizer) == 0)
								selectVisualizer(hitTestResult.visualizer, false);
							else
								selectedVisualizers.erase(hitTestResult.visualizer);
						}
						
						if (hitTestResult.visualizerHitTestResult.borderL ||
							hitTestResult.visualizerHitTestResult.borderR ||
							hitTestResult.visualizerHitTestResult.borderT ||
							hitTestResult.visualizerHitTestResult.borderB)
						{
							nodeResize.nodeId = hitTestResult.visualizer->id;
							nodeResize.dragL = hitTestResult.visualizerHitTestResult.borderL;
							nodeResize.dragR = hitTestResult.visualizerHitTestResult.borderR;
							nodeResize.dragT = hitTestResult.visualizerHitTestResult.borderT;
							nodeResize.dragB = hitTestResult.visualizerHitTestResult.borderB;
						 
							state = kState_NodeResize;
							break;
						}
						
						if (enabled(kFlag_NodeDrag))
						{
							state = kState_NodeDrag;
							break;
						}
					}
				}
				else if (enabled(kFlag_Select))
				{
					nodeSelect.beginX = mousePosition.x;
					nodeSelect.beginY = mousePosition.y;
					nodeSelect.endX = nodeSelect.beginX;
					nodeSelect.endY = nodeSelect.beginY;
					
					state = kState_NodeSelect;
					break;
				}
			}
			
			if (mouse.wentDown(BUTTON_RIGHT))
			{
				HitTestResult hitTestResult;
				
				if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
				{
					if (hitTestResult.hasNode)
					{
						if (enabled(kFlag_NodeAdd) && hitTestResult.nodeHitTestResult.inputSocket)
						{
							tryAddVisualizer(
								hitTestResult.node->id,
								hitTestResult.nodeHitTestResult.inputSocket->name,
								hitTestResult.nodeHitTestResult.inputSocket->index,
								String::Empty, -1,
								mousePosition.x, mousePosition.y, true, nullptr);
						}
						
						if (enabled(kFlag_NodeAdd) && hitTestResult.nodeHitTestResult.outputSocket)
						{
							tryAddVisualizer(
								hitTestResult.node->id,
								String::Empty, -1,
								hitTestResult.nodeHitTestResult.outputSocket->name,
								hitTestResult.nodeHitTestResult.outputSocket->index,
								mousePosition.x, mousePosition.y, true, nullptr);
						}
					}
					
					if (enabled(kFlag_LinkAdd) && hitTestResult.hasLink)
					{
						GraphLinkRoutePoint routePoint;
						routePoint.linkId = hitTestResult.link->id;
						routePoint.x = mousePosition.x;
						routePoint.y = mousePosition.y;
						
						auto at = hitTestResult.link->editorRoutePoints.begin();
						for (int i = 0; i < hitTestResult.linkSegmentIndex; ++i)
							++at;
						
						auto result = hitTestResult.link->editorRoutePoints.insert(at, routePoint);
						
						selectLinkRoutePoint(&(*result), true);
					}
				}
				else if (enabled(kFlag_NodeAdd))
				{
					nodeInsert.x = mousePosition.x;
					nodeInsert.y = mousePosition.y;
					
					state = kState_NodeInsert;
					break;
				}
			}
			
			if (enabled(kFlag_NodeAdd) && keyboard.wentDown(SDLK_i))
			{
				if (commandMod())
				{
					nodeInsert.x = mousePosition.x;
					nodeInsert.y = mousePosition.y;
					
					state = kState_NodeInsert;
					break;
				}
				else
				{
					std::string typeName = nodeTypeNameSelect->getNodeTypeName();
					
					tryAddNode(typeName, mousePosition.x, mousePosition.x, true, nullptr);
				}
			}
			
			if (keyboard.wentDown(SDLK_a))
			{
				if (commandMod())
				{
					if (enabled(kFlag_Select))
					{
						selectAll();
					}
				}
				else if (enabled(kFlag_Drag))
				{
					for (auto nodeId : selectedNodes)
					{
						auto nodeData = tryGetNodeData(nodeId);
						
						if (nodeData != nullptr)
						{
							snapToGrid(nodeData->x, nodeData->y);
						}
					}
					
					for (auto visualizer : selectedVisualizers)
					{
						snapToGrid(visualizer->x, visualizer->y);
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_o) || keyboard.wentDown(SDLK_0))
			{
				if (commandMod())
				{
					if (enabled(kFlag_Zoom))
					{
						dragAndZoom.desiredZoom = 1.f;
					}
					
					if (enabled(kFlag_Drag) && selectionMod())
					{
						dragAndZoom.desiredFocusX = 0.f;
						dragAndZoom.desiredFocusY = 0.f;
					}
				}
			}
			
		#if !defined(MACOS) // fixme : mouse wheel event seems to trigger when interacting with the touch pad on MacOS
			{
				const float magnitude = std::pow<float>(1.1f, mouse.scrollY);
			
				dragAndZoom.desiredZoom *= magnitude;
			}
		#endif
			
			if (enabled(kFlag_Zoom) && keyboard.wentDown(SDLK_MINUS) && commandMod())
			{
				dragAndZoom.desiredZoom /= 1.5f;
			}
			
			if (enabled(kFlag_Zoom) && keyboard.wentDown(SDLK_EQUALS) && commandMod())
			{
				dragAndZoom.desiredZoom *= 1.5f;
			}
			
			if (enabled(kFlag_NodeAdd) && keyboard.wentDown(SDLK_d))
			{
				// copy nodes
				
				std::set<GraphNodeId> newSelectedNodes;
				
				std::map<GraphNodeId, GraphNodeId> oldToNewNodeIds;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					auto nodeData = tryGetNodeData(nodeId);
					
					Assert(node != nullptr && nodeData != nullptr);
					if (node != nullptr && nodeData != nullptr)
					{
						GraphNode newNode = *node;
						newNode.id = graph->allocNodeId();
						newNode.isPassthrough = false;
						newNode.inputValues.clear();
						newNode.editorValue.clear();
						
						graph->addNode(newNode);
						
						auto newNodeData = tryGetNodeData(newNode.id);
						Assert(newNodeData != nullptr);
						if (newNodeData != nullptr)
						{
							*newNodeData = *nodeData;
							
							newNodeData->x = nodeData->x + kGridSize;
							newNodeData->y = nodeData->y + kGridSize;
							newNodeData->zKey = nextZKey++;
						}
						
						newSelectedNodes.insert(newNode.id);
						
						oldToNewNodeIds.insert({ node->id, newNode.id});
						
						if (commandMod())
						{
							auto newNodePtr = tryGetNode(newNode.id);
							Assert(newNodePtr != nullptr);
							
							// deep copy node including values
							
							auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(newNode.typeName);
							Assert(typeDefinition != nullptr);
							
							// let real-time editing know the socket values have changed
							for (auto & inputValueItr : node->inputValues)
							{
								auto & srcSocketName = inputValueItr.first;
								auto & srcSocketValue = inputValueItr.second;
								
								newNodePtr->inputValues[srcSocketName] = srcSocketValue;
								
								if (typeDefinition != nullptr)
								{
									auto & inputSockets = getInputSockets(*typeDefinition, *nodeData);
									
									for (auto & srcSocket : inputSockets)
									{
										if (srcSocket.name == srcSocketName)
										{
											if (realTimeConnection != nullptr)
											{
												realTimeConnection->setSrcSocketValue(newNode.id, srcSocket.index, srcSocket.name, srcSocketValue);
											}
										}
									}
								}
							}
							
							if (!node->editorValue.empty())
							{
								newNodePtr->editorValue = node->editorValue;
								
								if (realTimeConnection != nullptr)
								{
									auto & outputSockets = getOutputSockets(*typeDefinition, *nodeData);
									
									for (auto & outputSocket : outputSockets)
									{
										if (!outputSocket.isEditable)
											continue;
										
										realTimeConnection->setDstSocketValue(newNodePtr->id, outputSocket.index, outputSocket.name, newNodePtr->editorValue);
									}
								}
							}
						}
					}
				}
				
				// copy links between copied nodes
				
				for (auto & linkItr : graph->links)
				{
					auto & link = linkItr.second;
					
					auto newSrcNodeIdItr = oldToNewNodeIds.find(link.srcNodeId);
					auto newDstNodeIdItr = oldToNewNodeIds.find(link.dstNodeId);
					
					if (newSrcNodeIdItr != oldToNewNodeIds.end() &&
						newDstNodeIdItr != oldToNewNodeIds.end())
					{
						auto newSrcNodeId = newSrcNodeIdItr->second;
						auto newDstNodeId = newDstNodeIdItr->second;
						
						auto newLink = link;
						newLink.id = graph->allocLinkId();
						
						newLink.srcNodeId = newSrcNodeId;
						newLink.dstNodeId = newDstNodeId;
						
						graph->addLink(newLink, false);
					}
				}
				
				// copy visualizers
				
				std::set<EditorVisualizer*> newSelectedVisualizers;
				
				for (auto visualizer : selectedVisualizers)
				{
					EditorVisualizer * newVisualizer = nullptr;
					
					if (tryAddVisualizer(
						visualizer->nodeId,
						visualizer->srcSocketName,
						visualizer->srcSocketIndex,
						visualizer->dstSocketName,
						visualizer->dstSocketIndex,
						visualizer->x + kGridSize,
						visualizer->y + kGridSize,
						false,
						&newVisualizer))
					{
						newVisualizer->sx = visualizer->sx;
						newVisualizer->sy = visualizer->sy;
						
						newSelectedVisualizers.insert(newVisualizer);
					}
				}
				
				if (!newSelectedNodes.empty() || !newSelectedVisualizers.empty())
				{
					selectedNodes = newSelectedNodes;
					selectedLinks.clear();
					selectedLinkRoutePoints.clear();
					selectedVisualizers = newSelectedVisualizers;
				}
			}
			
			if (enabled(kFlag_ToggleIsPassthrough) && keyboard.wentDown(SDLK_p))
			{
				bool allPassthrough = true;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr)
						allPassthrough &= node->isPassthrough;
				}
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					Assert(node);
					if (node)
					{
						node->isPassthrough = allPassthrough ? false : true;
						
						if (realTimeConnection != nullptr)
						{
							realTimeConnection->setNodeIsPassthrough(node->id, node->isPassthrough);
						}
					}
				}
			}
			
			if (selectedNodes.empty() && selectedLinkRoutePoints.empty() && selectedVisualizers.empty())
			{
				tickKeyboardScroll();
			}
			else if (!enabled(kFlag_NodeDrag))
			{
				tickKeyboardScroll();
			}
			else
			{
				int moveX = 0;
				int moveY = 0;
				
				if (keyboard.wentDown(SDLK_LEFT, true))
					moveX -= 1;
				if (keyboard.wentDown(SDLK_RIGHT, true))
					moveX += 1;
				if (keyboard.wentDown(SDLK_UP, true))
					moveY -= 1;
				if (keyboard.wentDown(SDLK_DOWN, true))
					moveY += 1;
				
				moveX *= kGridSize;
				moveY *= kGridSize;
				
				if (moveX != 0 || moveY != 0)
				{
					for (auto nodeId : selectedNodes)
					{
						auto nodeData = tryGetNodeData(nodeId);
						
						Assert(nodeData);
						if (nodeData)
						{
							nodeData->x += moveX;
							nodeData->y += moveY;
						}
					}
					
					for (auto routePoint : selectedLinkRoutePoints)
					{
						routePoint->x += moveX;
						routePoint->y += moveY;
					}
					
					for (auto visualizer : selectedVisualizers)
					{
						visualizer->x += moveX;
						visualizer->y += moveY;
					}
				}
			}
			
			if (enabled(kFlag_ToggleIsFolded) && keyboard.wentDown(SDLK_SPACE))
			{
				bool anyUnfolded = false;
				
				for (auto nodeId : selectedNodes)
				{
					auto nodeData = tryGetNodeData(nodeId);
					
					Assert(nodeData != nullptr);
					if (nodeData != nullptr && !nodeData->isFolded)
					{
						anyUnfolded = true;
					}
				}
				
				for (auto nodeId : selectedNodes)
				{
					auto nodeData = tryGetNodeData(nodeId);
					
					Assert(nodeData != nullptr);
					if (nodeData != nullptr)
					{
						nodeData->setIsFolded(anyUnfolded ? true : false);
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_BACKSPACE) || keyboard.wentDown(SDLK_DELETE))
			{
				if (enabled(kFlag_NodeRemove))
				{
					auto nodesToRemove = selectedNodes;
					for (auto nodeId : nodesToRemove)
					{
						graph->removeNode(nodeId);
					}
					
					Assert(selectedNodes.empty());
					selectedNodes.clear();
				}
				
				if (enabled(kFlag_LinkRemove))
				{
					auto linksToRemove = selectedLinks;
					for (auto linkId : linksToRemove)
					{
						if (graph->tryGetLink(linkId) != nullptr)
							graph->removeLink(linkId);
					}
					
					Assert(selectedLinks.empty());
					selectedLinks.clear();
				}
				
				if (enabled(kFlag_LinkRemove))
				{
					auto routePointsToRemove = selectedLinkRoutePoints;
					for (auto routePoint : routePointsToRemove)
					{
						auto link = graph->tryGetLink(routePoint->linkId);
						
						for (auto routePointItr = link->editorRoutePoints.begin(); routePointItr != link->editorRoutePoints.end(); ++routePointItr)
						{
							auto routePoint2 = &(*routePointItr);
							
							if (routePoint2 == routePoint)
							{
								link->editorRoutePoints.erase(routePointItr);
								break;
							}
						}
					}
				}
				
				if (enabled(kFlag_NodeRemove))
				{
					for (auto visualizer : selectedVisualizers)
					{
						auto i = visualizers.find(visualizer->id);
						
						Assert(i != visualizers.end());
						if (i != visualizers.end())
							visualizers.erase(i);
					}
					
					selectedVisualizers.clear();
				}
			}
			
			//
			
			if (selectedNodes.empty() == false)
			{
				const GraphNodeId nodeId = *selectedNodes.begin();
					
				propertyEditor->setNode(nodeId);
			}
			
			//
			
			if (keyboard.wentDown(SDLK_TAB))
			{
				state = kState_Hidden;
				break;
			}
			
			if (editorOptions.autoHideUi && idleTime > 30.f)
			{
				hideTime = 1.f;
				
				state = kState_HiddenIdle;
				break;
			}
			
			// drag and zoom
			
			if (enabled(kFlag_Drag))
			{
				if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL) || mouse.isDown(BUTTON_RIGHT))
				{
					dragAndZoom.focusX -= mouse.dx / std::max(1.f, dragAndZoom.zoom);
					dragAndZoom.focusY -= mouse.dy / std::max(1.f, dragAndZoom.zoom);
					dragAndZoom.desiredFocusX = dragAndZoom.focusX;
					dragAndZoom.desiredFocusY = dragAndZoom.focusY;
				}
			}
			
			if (enabled(kFlag_Zoom) && (keyboard.isDown(SDLK_LALT) || keyboard.isDown(SDLK_RALT)))
			{
				dragAndZoom.desiredZoom += mouse.dy / 200.f * (std::abs(dragAndZoom.zoom) + .5f);
				dragAndZoom.zoom = dragAndZoom.desiredZoom;
			}
		}
		break;
		
	case kState_NodeSelect:
		{
			tickMouseScroll(dt);
			
			tickKeyboardScroll();
			
			nodeSelect.endX = mousePosition.x;
			nodeSelect.endY = mousePosition.y;
			
			// hit test nodes
			
			nodeSelect.nodeIds.clear();
			
			for (auto & nodeItr : graph->nodes)
			{
				auto & node = nodeItr.second;
				auto & nodeData = nodeDatas[node.id];
				
				auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
				
				Assert(typeDefinition != nullptr);
				if (typeDefinition != nullptr)
				{
					auto & inputSockets = getInputSockets(*typeDefinition, nodeData);
					auto & outputSockets = getOutputSockets(*typeDefinition, nodeData);
					
					float sx;
					float sy;
					getNodeRect(inputSockets.size(), outputSockets.size(), nodeData.isFolded, sx, sy);
					
					if (testRectOverlap(
						nodeSelect.beginX,
						nodeSelect.beginY,
						nodeSelect.endX,
						nodeSelect.endY,
						nodeData.x,
						nodeData.y,
						nodeData.x + sx,
						nodeData.y + sy))
					{
						nodeSelect.nodeIds.insert(node.id);
					}
				}
			}
			
			// hit test visualizers
			
			nodeSelect.visualizers.clear();
			
			for (auto & visualizerItr : visualizers)
			{
				auto & visualizer = visualizerItr.second;
				
				if (testRectOverlap(
					nodeSelect.beginX,
					nodeSelect.beginY,
					nodeSelect.endX,
					nodeSelect.endY,
					visualizer.x,
					visualizer.y,
					visualizer.x + visualizer.sx,
					visualizer.y + visualizer.sy))
				{
					nodeSelect.visualizers.insert(&visualizer);
				}
			}
			
			//
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				nodeSelectEnd();
				
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_NodeDrag:
		{
			tickMouseScroll(dt);
			
			tickKeyboardScroll();
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				nodeDragEnd();
				
				state = kState_Idle;
				break;
			}
			
			for (auto nodeId : selectedNodes)
			{
				auto nodeData = tryGetNodeData(nodeId);
				
				Assert(nodeData != nullptr);
				if (nodeData != nullptr)
				{
					nodeData->x += mousePosition.dx;
					nodeData->y += mousePosition.dy;
				}
			}
			
			for (auto & routePoint : selectedLinkRoutePoints)
			{
				routePoint->x += mousePosition.dx;
				routePoint->y += mousePosition.dy;
			}
			
			for (auto & visualizer : selectedVisualizers)
			{
				visualizer->x += mousePosition.dx;
				visualizer->y += mousePosition.dy;
			}
		}
		break;
		
	case kState_NodeResourceEdit:
		{
			if (mouse.wentDown(BUTTON_RIGHT))
			{
				nodeResourceEditEnd();
				
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_InputSocketConnect:
		{
			tickMouseScroll(dt);
			
			tickKeyboardScroll();
			
			socketConnect.dstNodeId = kGraphNodeIdInvalid;
			socketConnect.dstNodeSocket = nullptr;
			
			HitTestResult hitTestResult;
			
			if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
			{
				if (hitTestResult.hasNode &&
					hitTestResult.node->id != socketConnect.srcNodeId &&
					hitTestResult.nodeHitTestResult.outputSocket)
				{
					if (socketConnect.srcNodeSocket->canConnectTo(typeDefinitionLibrary, *hitTestResult.nodeHitTestResult.outputSocket))
					{
						socketConnect.dstNodeId = hitTestResult.node->id;
						socketConnect.dstNodeSocket = hitTestResult.nodeHitTestResult.outputSocket;
					}
				}
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				socketConnectEnd();
				
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_OutputSocketConnect:
		{
			tickMouseScroll(dt);
			
			tickKeyboardScroll();
			
			socketConnect.srcNodeId = kGraphNodeIdInvalid;
			socketConnect.srcNodeSocket = nullptr;
			
			HitTestResult hitTestResult;
			
			if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
			{
				if (hitTestResult.hasNode &&
					hitTestResult.node->id != socketConnect.dstNodeId &&
					hitTestResult.nodeHitTestResult.inputSocket)
				{
					if (socketConnect.dstNodeSocket->canConnectTo(typeDefinitionLibrary, *hitTestResult.nodeHitTestResult.inputSocket))
					{
						socketConnect.srcNodeId = hitTestResult.node->id;
						socketConnect.srcNodeSocket = hitTestResult.nodeHitTestResult.inputSocket;
					}
				}
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				socketConnectEnd();
				
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_NodeResize:
		{
			EditorVisualizer * visualizer = tryGetVisualizer(nodeResize.nodeId);
			
			Assert(visualizer != nullptr);
			if (visualizer != nullptr)
			{
				const float dragX = mousePosition.dx;
				const float dragY = mousePosition.dy;
				
				if (nodeResize.dragL)
				{
					visualizer->x += dragX;
					visualizer->sx -= dragX;
				}
				
				if (nodeResize.dragR)
				{
					visualizer->sx += dragX;
				}
				
				if (nodeResize.dragT)
				{
					visualizer->y += dragY;
					visualizer->sy -= dragY;
				}
				
				if (nodeResize.dragB)
				{
					visualizer->sy += dragY;
				}
				
				mousePosition.hover = true;
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				nodeResize = NodeResize();
				
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_NodeInsert:
		{
			if (nodeInsert.linkId != kGraphLinkIdInvalid)
			{
				auto link = tryGetLink(nodeInsert.linkId);
				
				if (link != nullptr)
				{
					auto srcNode = tryGetNode(link->srcNodeId);
					auto dstNode = tryGetNode(link->dstNodeId);
					
					if (srcNode != nullptr && dstNode != nullptr)
					{
						auto srcTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
						auto dstTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
						
						if (srcTypeDefinition != nullptr && dstTypeDefinition != nullptr)
						{
							if (srcTypeDefinition->inputSockets.size() >= 1 &&
								dstTypeDefinition->outputSockets.size() >= 1)
							{
								auto srcSocketTypeName = srcTypeDefinition->inputSockets[0].typeName;
								auto dstSocketTypeName = dstTypeDefinition->outputSockets[0].typeName;
								
								nodeInsertMenu->filter.type = GraphEdit_NodeTypeSelect::kFilterType_PrimarySocketTypes;
								nodeInsertMenu->filter.srcSocketTypeName = srcSocketTypeName;
								nodeInsertMenu->filter.dstSocketTypeName = dstSocketTypeName;
							}
						}
					}
				}
				
			}
			
			std::string typeName;
			
			if (nodeInsertMenu->tick(*this, *typeDefinitionLibrary, dt, typeName))
			{
				nodeInsertMenu->cancel();
				
				const GraphLinkId linkId = nodeInsert.linkId;
				const float x = nodeInsert.x;
				const float y = nodeInsert.y;
				
				//
				
				nodeInsert = NodeInsert();
				state = kState_Idle;
				
				//
				
				if (typeName.empty() == false)
				{
					GraphNodeId nodeId;
					
					if (tryAddNode(typeName, x, y, true, &nodeId))
					{
						// hook up the new node between the endpoints of the existing link
						
						if (linkId != kGraphLinkIdInvalid)
						{
							auto linkPtr = tryGetLink(linkId);
							Assert(linkPtr != nullptr);
							
							if (linkPtr != nullptr)
							{
								// since the existing link may be removed due to adding new links,
								// we need to make a copy of it here so we know the node ids and
								// socket indices to connect with
								
								auto link = *linkPtr;
								
								auto dstSocket = tryGetOutputSocket(nodeId, 0);
								auto srcSocket = tryGetInputSocket(nodeId, 0);
								
								if (true)
								{
									GraphLink link1;
									link1.id = graph->allocLinkId();
									link1.srcNodeId = link.srcNodeId;
									link1.srcNodeSocketIndex = link.srcNodeSocketIndex;
									link1.srcNodeSocketName = link.srcNodeSocketName;
									link1.dstNodeId = nodeId;
									link1.dstNodeSocketIndex = dstSocket->index;
									link1.dstNodeSocketName = dstSocket->name;
									graph->addLink(link1, true);
								}
								
								if (true)
								{
									GraphLink link2;
									link2.id = graph->allocLinkId();
									link2.srcNodeId = nodeId;
									link2.srcNodeSocketIndex = srcSocket->index;
									link2.srcNodeSocketName = srcSocket->name;
									link2.dstNodeId = link.dstNodeId;
									link2.dstNodeSocketIndex = link.dstNodeSocketIndex;
									link2.dstNodeSocketName = link.dstNodeSocketName;
									graph->addLink(link2, true);
								}
							}
						}
						
						nodeTypeNameSelect->addToHistory(typeName);
					}
				}
				break;
			}
		}
		break;
		
	case kState_TouchDrag:
		{
		}
		break;
		
	case kState_TouchZoom:
		{
		}
		break;
			
	case kState_Hidden:
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				state = kState_Idle;
				break;
			}
		}
		break;
		
	case kState_HiddenIdle:
		{
			if (isIdle == false)
			{
				state = kState_Idle;
				break;
			}
		}
		break;
	}
	
	// update UI
	
	nodeDoubleClickTime = std::max(0.f, nodeDoubleClickTime - dt);
	
	dragAndZoom.tick(dt);
	
	if (graph != nullptr)
	{
		for (auto & nodeItr : graph->nodes)
		{
			auto nodeId = nodeItr.first;
			auto & node = nodeItr.second;
			auto & nodeData = nodeDatas[nodeId];
			
			//
			
			const int activity =
				realTimeConnection == nullptr
				? 0
				: realTimeConnection->getNodeActivity(node.id);
			
			if (activity & GraphEdit_RealTimeConnection::kActivity_OneShot)
			{
				nodeData.isActiveAnimTime = .2f;
				nodeData.isActiveAnimTimeRcp = 1.f / nodeData.isActiveAnimTime;
			}
			
			nodeData.isActiveContinuous = (activity & GraphEdit_RealTimeConnection::kActivity_Continuous) != 0;
			
			//
			
			nodeData.isCloseToConnectionSite = false;
			
			if (state == kState_InputSocketConnect || state == kState_OutputSocketConnect)
			{
				// todo : calculate distance from node center. take node dimensions into account for maximum radius. keep node open when the mouse is over the node
				
				const float dx = nodeData.x - mousePosition.x;
				const float dy = nodeData.y - mousePosition.y;
				const float distance = std::hypot(dx, dy);
				
				if (distance <= 300.f)
				{
					nodeData.isCloseToConnectionSite = true;
				}
			}
		}
		
		tickNodeDatas(dt);
		
		for (auto & linkItr : graph->links)
		{
			auto & link = linkItr.second;
			
			link.editorIsActiveAnimTime = Calc::Max(0.f, link.editorIsActiveAnimTime - dt);
			
			if (link.isEnabled)
			{
				const bool isActive =
					realTimeConnection == nullptr
					? false
					: realTimeConnection->getLinkActivity(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex) != 0;
				
				if (isActive)
				{
					link.editorIsActiveAnimTime = .6f;
					link.editorIsActiveAnimTimeRcp = 1.f / link.editorIsActiveAnimTime;
				}
			}
		}
	}
	
	const bool doHideAnimation = state == kState_HiddenIdle;
	
	if (doHideAnimation)
		hideTime = std::max(0.f, hideTime - dt / .5f);
	else
		hideTime = std::min(1.f, hideTime + dt / .25f);
	
	if (!notifications.empty())
	{
		Notification & n = notifications.front();
		
		n.displayTime = std::max(0.f, n.displayTime - dt);
		
		const float t = n.displayTime * n.displayTimeRcp;
		
		if (t <= 0.f)
		{
			notifications.pop_front();
		}
	}
	
	if (enabled(kFlag_SetCursor))
	{
		SDL_SetCursor(mousePosition.hover ? cursorHand : SDL_GetDefaultCursor());
	}
	
	animationIsDone =
		(hideTime == 0.f || hideTime == 1.f) &&
		notifications.empty() &&
		dragAndZoom.animationIsDone();
	
	inputIsCaptured &= (state != kState_Hidden);
	
	return inputIsCaptured;
}

bool GraphEdit::tickTouches()
{
	if ((state != kState_Idle && state != kState_TouchDrag && state != kState_TouchZoom) || !framework.windowIsActive)
	{
		touches = Touches();
		
		return false;
	}
	
	const float sx = 1920/2;
	const float sy = 1080/2;
	
	for (auto & event : framework.events)
	{
		if (event.type == SDL_FINGERDOWN)
		{
			//logDebug("touch down: fingerId=%d", event.tfinger.fingerId);
			
			if (state == kState_Idle)
			{
				if (touches.finger1.id == 0)
				{
					//logDebug("touch down: fingerId=%d, x=%.2f, y=%.2f", event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
					
					touches.finger1.id = event.tfinger.fingerId;
					touches.finger1.position.Set(event.tfinger.x * sx, event.tfinger.y * sy);
					touches.finger1.initialPosition = touches.finger1.position;
				}
				else if (touches.finger2.id == 0)
				{
					touches.finger2.id = event.tfinger.fingerId;
					touches.finger2.position.Set(event.tfinger.x * sx, event.tfinger.y * sy);
					touches.finger2.initialPosition = touches.finger2.position;
					
					//
					
					touches.distance = touches.getDistance();
					touches.initialDistance = touches.getDistance();
				}
			}
			else if (state == kState_TouchDrag)
			{
				Assert(touches.finger1.id != 0);
				
				if (touches.finger2.id == 0)
				{
					touches.finger2.id = event.tfinger.fingerId;
					touches.finger2.position.Set(event.tfinger.x * sx, event.tfinger.y * sy);
					touches.finger2.initialPosition = touches.finger2.position;
					
					touches.distance = touches.getDistance();
					touches.initialDistance = touches.getDistance();
				}
			}
			else
			{
				// ignore touch event
			}
		}
		else if (event.type == SDL_FINGERUP)
		{
			//logDebug("touch up: fingerId=%d", event.tfinger.fingerId);
			
			if (state == kState_Idle)
			{
				if (event.tfinger.fingerId == touches.finger1.id)
				{
					touches.finger1 = touches.finger2;
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
				}
				
				if (event.tfinger.fingerId == touches.finger2.id)
				{
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
				}
			}
			else if (state == kState_TouchDrag)
			{
				Assert(touches.finger1.id != 0);
				
				if (event.tfinger.fingerId == touches.finger1.id)
				{
					touches.finger1 = touches.finger2;
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
				}
				
				if (event.tfinger.fingerId == touches.finger2.id)
				{
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
				}
				
				if (touches.finger1.id == 0 && touches.finger2.id == 0)
				{
					//logDebug("touch down: TouchDrag -> Idle");
					
					state = kState_Idle;
				}
			}
			else if (state == kState_TouchZoom)
			{
				if (event.tfinger.fingerId == touches.finger1.id)
				{
					Assert(touches.finger1.id != 0);
					Assert(touches.finger2.id != 0);
				
					touches.finger1 = touches.finger2;
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
					
					if (enabled(kFlag_Zoom) && std::abs(std::abs(dragAndZoom.desiredZoom) - 1.f) < .1f)
						dragAndZoom.desiredZoom = Calc::Sign(dragAndZoom.desiredZoom);
					
					//logDebug("touch down: TouchZoom -> TouchDrag");
					
					state = kState_TouchDrag;
				}
				else if (event.tfinger.fingerId == touches.finger2.id)
				{
					Assert(touches.finger1.id != 0);
					Assert(touches.finger2.id != 0);
				
					touches.finger2 = Touches::FingerInfo();
					
					touches.distance = 0.f;
					touches.initialDistance = 0.f;
					
					if (enabled(kFlag_Zoom) && std::abs(std::abs(dragAndZoom.desiredZoom) - 1.f) < .1f)
						dragAndZoom.desiredZoom = Calc::Sign(dragAndZoom.desiredZoom);
					
					//logDebug("touch down: TouchZoom -> TouchDrag");
					
					state = kState_TouchDrag;
				}
			}
		}
		else if (event.type == SDL_FINGERMOTION)
		{
			if (event.tfinger.fingerId != touches.finger1.id &&
				event.tfinger.fingerId != touches.finger2.id)
			{
				continue;
			}
			
			auto & finger = event.tfinger.fingerId == touches.finger1.id ? touches.finger1 : touches.finger2;
			
			const float dragSpeed = 1.f / 2.f / std::max(1.f, std::abs(dragAndZoom.zoom)) * Calc::Sign(dragAndZoom.zoom);
			
			const Vec2 position(event.tfinger.x * sx, event.tfinger.y * sy);
			const Vec2 delta = position - finger.position;
			const float lengthFromInitialPosition = (position - finger.initialPosition).CalcSize();
			
			finger.position = position;
			
			if (state == kState_Idle)
			{
				touches.distance = touches.getDistance();
				
				if (touches.finger1.id != 0 && touches.finger2.id != 0)
				{
					const float delta = std::abs(touches.getDistance() - touches.initialDistance);
					
					if (delta > 40.f)
					{
						state = kState_TouchZoom;
						
						continue;
					}
					else if (lengthFromInitialPosition > 40.f)
					{
						state = kState_TouchDrag;
						
						continue;
					}
				}
			}
			else if (state == kState_TouchDrag)
			{
				Assert(touches.finger1.id != 0);
				
				if (enabled(kFlag_Drag))
				{
					dragAndZoom.focusX -= delta[0] * dragSpeed;
					dragAndZoom.focusY -= delta[1] * dragSpeed;
					
					dragAndZoom.desiredFocusX = dragAndZoom.focusX;
					dragAndZoom.desiredFocusY = dragAndZoom.focusY;
				}
				
				if (touches.finger2.id != 0)
				{
					Assert(touches.finger1.id != 0);
					
					touches.distance = touches.getDistance();
					
					const float delta = std::abs(touches.getDistance() - touches.initialDistance);
					
					if (delta > 40.f)
					{
						state = kState_TouchZoom;
						
						//logDebug("touch down: TouchDrag -> TouchZoom");
					}
				}
			}
			else if (state == kState_TouchZoom)
			{
				Assert(touches.finger1.id != 0);
				Assert(touches.finger2.id != 0);
				
				{
					// update zoom
					
					{
						const float kZoomPixels = 1000.f / 3.f;
						
						// calculate movement
				
						const float distance = touches.getDistance();
						
						// update zoom
						
						const float deltaLength = distance - touches.distance;
						
						const float zoomDelta = deltaLength / kZoomPixels;

						// update current touch distance
						
						touches.distance = distance;
						
						// update zoom info
						
						if (enabled(kFlag_Zoom))
						{
							dragAndZoom.zoom += zoomDelta * (std::abs(dragAndZoom.zoom) + .5f);
							dragAndZoom.desiredZoom = dragAndZoom.zoom;
						}
					}
				
					// update movement
					
					if (enabled(kFlag_Drag))
					{
						dragAndZoom.focusX -= delta[0] * dragSpeed;
						dragAndZoom.focusY -= delta[1] * dragSpeed;
						
						dragAndZoom.desiredFocusX = dragAndZoom.focusX;
						dragAndZoom.desiredFocusY = dragAndZoom.focusY;
					}
				}
			}
		}
	}
	
	return
		state == kState_TouchDrag ||
		state == kState_TouchZoom;
}

void GraphEdit::tickVisualizers(const float dt)
{
	if (realTimeSocketCapture.visualizer.nodeId != kGraphNodeIdInvalid)
	{
		realTimeSocketCapture.visualizer.tick(*this, dt);
	}
	
	for (auto & visualizerItr : visualizers)
	{
		auto & visualizer = visualizerItr.second;
		
		visualizer.tick(*this, dt);
	}
}

void GraphEdit::tickNodeDatas(const float dt)
{
	for (auto & nodeDataItr : nodeDatas)
	{
		auto & nodeData = nodeDataItr.second;
		
		if (nodeData.isFolded && nodeData.isCloseToConnectionSite == false)
		{
			// todo : receive automatic unfold flag from graph edit, store it, and integrate with hit testing code so it works when automatic unfold is in effect
			nodeData.foldAnimProgress = Calc::Max(0.f, nodeData.foldAnimProgress - dt * nodeData.foldAnimTimeRcp);
		}
		else
		{
			nodeData.foldAnimProgress = Calc::Min(1.f, nodeData.foldAnimProgress + dt * nodeData.foldAnimTimeRcp);
		}
		
		nodeData.isActiveAnimTime = Calc::Max(0.f, nodeData.isActiveAnimTime - dt);
	}
}

void GraphEdit::tickMouseScroll(const float dt)
{
	if (enabled(kFlag_Drag) == false)
		return;
	
	const int kScrollBorder = 100;
	const float kScrollSpeed = 600.f;
	
	int tX = 0;
	int tY = 0;
	
	if (mouse.x < kScrollBorder)
		tX = mouse.x - kScrollBorder;
	if (mouse.y < kScrollBorder)
		tY = mouse.y - kScrollBorder;
	if (mouse.x > GRAPHEDIT_SX - 1 - kScrollBorder)
		tX = mouse.x - (GRAPHEDIT_SX - 1 - kScrollBorder);
	if (mouse.y > GRAPHEDIT_SY - 1 - kScrollBorder)
		tY = mouse.y - (GRAPHEDIT_SY - 1 - kScrollBorder);
	
	dragAndZoom.desiredFocusX += tX / float(kScrollBorder) * kScrollSpeed * dt;
	dragAndZoom.desiredFocusY += tY / float(kScrollBorder) * kScrollSpeed * dt;
}

void GraphEdit::tickKeyboardScroll()
{
	if (enabled(kFlag_Drag) == false)
		return;
	
	int moveX = 0;
	int moveY = 0;
	
	if (keyboard.wentDown(SDLK_LEFT, true))
		moveX -= 1;
	if (keyboard.wentDown(SDLK_RIGHT, true))
		moveX += 1;
	if (keyboard.wentDown(SDLK_UP, true))
		moveY -= 1;
	if (keyboard.wentDown(SDLK_DOWN, true))
		moveY += 1;
	
	dragAndZoom.desiredFocusX += moveX / dragAndZoom.zoom * kGridSize * 8;
	dragAndZoom.desiredFocusY += moveY / dragAndZoom.zoom * kGridSize * 10;
}

void GraphEdit::nodeSelectEnd()
{
	if (selectionMod())
	{
		selectedNodes.insert(nodeSelect.nodeIds.begin(), nodeSelect.nodeIds.end());
		selectedVisualizers.insert(nodeSelect.visualizers.begin(), nodeSelect.visualizers.end());
	}
	else
	{
		selectedNodes = nodeSelect.nodeIds;
		selectedVisualizers = nodeSelect.visualizers;
	}
	
	selectedLinks.clear();
	selectedLinkRoutePoints.clear();
	
	nodeSelect = NodeSelect();
}

void GraphEdit::nodeDragEnd()
{
	if (editorOptions.snapToGrid)
	{
		for (auto nodeId : selectedNodes)
		{
			auto nodeData = tryGetNodeData(nodeId);
			
			Assert(nodeData != nullptr);
			if (nodeData != nullptr)
			{
				snapToGrid(nodeData->x, nodeData->y);
			}
		}
		
		for (auto & routePoint : selectedLinkRoutePoints)
		{
			snapToGrid(routePoint->x, routePoint->y);
		}
		
		for (auto & visualizer : selectedVisualizers)
		{
			snapToGrid(visualizer->x, visualizer->y);
		}
	}
}

bool GraphEdit::nodeResourceEditBegin(const GraphNodeId nodeId)
{
	nodeResourceEditEnd();
	
	//
	
	auto node = tryGetNode(nodeId);
	
	Assert(node != nullptr);
	if (node != nullptr)
	{
		auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
		
		Assert(typeDefinition != nullptr);
		if (typeDefinition != nullptr)
		{
			auto & resourceTypeName = typeDefinition->resourceTypeName;
			
			if (resourceTypeName.empty() == false)
			{
				nodeResourceEditor.nodeId = nodeId;
				nodeResourceEditor.resourceTypeName = resourceTypeName;
				if (typeDefinition->resourceEditor.create != nullptr)
					nodeResourceEditor.resourceEditor = typeDefinition->resourceEditor.create();
				
				Assert(nodeResourceEditor.resourceEditor != nullptr);
				if (nodeResourceEditor.resourceEditor != nullptr)
				{
					nodeResourceEditor.resourceEditor->setResource(*node, resourceTypeName.c_str(), "editorData");
					
					// calculate the position based on size and move the editor over there
					
					const int sx = nodeResourceEditor.resourceEditor->initSx;
					const int sy = nodeResourceEditor.resourceEditor->initSy;
					
					const int x = (GRAPHEDIT_SX - sx) / 2;
					const int y = (GRAPHEDIT_SY - sy) / 2;
					
					nodeResourceEditor.resourceEditor->init(x, y, sx, sy);
				}
			}
		}
	}
	
	return nodeResourceEditor.resourceEditor != nullptr;
}

void GraphEdit::nodeResourceEditSave()
{
	if (nodeResourceEditor.resourceEditor != nullptr)
	{
		auto node = tryGetNode(nodeResourceEditor.nodeId);
		
		if (node != nullptr)
		{
			std::string resourceData;
			
			if (nodeResourceEditor.resourceEditor->serializeResource(resourceData))
			{
				node->setResource(nodeResourceEditor.resourceTypeName.c_str(), "editorData", resourceData.c_str());
			}
			else
			{
				node->clearResource(nodeResourceEditor.resourceTypeName.c_str(), "editorData");
			}
		}
	}
}

void GraphEdit::nodeResourceEditEnd()
{
	nodeResourceEditSave();
	
	//
	
	delete nodeResourceEditor.resourceEditor;
	nodeResourceEditor.resourceEditor = nullptr;
	
	nodeResourceEditor = NodeResourceEditor();
}

void GraphEdit::socketConnectEnd()
{
	if (socketConnect.srcNodeId != kGraphNodeIdInvalid && socketConnect.dstNodeId != kGraphNodeIdInvalid)
	{
		bool clearInputDuplicates = true;
		
		const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeDefinitionLibrary->tryGetValueTypeDefinition(socketConnect.srcNodeSocket->typeName);
		
		if (valueTypeDefinition != nullptr && valueTypeDefinition->multipleInputs)
		{
			clearInputDuplicates = false;
		}
		
		GraphLink link;
		link.id = graph->allocLinkId();
		link.isDynamic = socketConnect.srcNodeSocket->isDynamic || socketConnect.dstNodeSocket->isDynamic;
		link.srcNodeId = socketConnect.srcNodeId;
		link.srcNodeSocketName = socketConnect.srcNodeSocket->name;
		link.srcNodeSocketIndex = socketConnect.srcNodeSocket->index;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketName = socketConnect.dstNodeSocket->name;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		
		graph->addLink(link, clearInputDuplicates);
		
		selectLink(link.id, true);
	}
	else if (enabled(kFlag_NodeAdd) && socketConnect.srcNodeId != kGraphNodeIdInvalid)
	{
		if (commandMod())
		{
			tryAddVisualizer(
				socketConnect.srcNodeId,
				socketConnect.srcNodeSocket->name, socketConnect.srcNodeSocket->index,
				String::Empty, -1,
				mousePosition.x, mousePosition.y, true, nullptr);
		}
	}
	else if (enabled(kFlag_NodeAdd) && socketConnect.dstNodeId != kGraphNodeIdInvalid)
	{
		if (commandMod())
		{
			tryAddVisualizer(
				socketConnect.dstNodeId,
				String::Empty, -1,
				socketConnect.dstNodeSocket->name, socketConnect.dstNodeSocket->index,
				mousePosition.x, mousePosition.y, true, nullptr);
		}
	}
	
	socketConnect = SocketConnect();
}

void GraphEdit::doMenu(const float dt)
{
	if (!enabled(kFlag_SaveLoad))
		return;
	
	pushMenu("file");
	{
		const float size = 1.f / 3.f;
		
		if (doButton("load", size * 0, size, false))
		{
			const std::string filename = documentInfo.filename.c_str();
			
			load(filename.c_str());
			showNotification("Loaded '%s'", documentInfo.filename.c_str());
		}
		
		if (doButton("save", size * 1, size, false))
		{
			save(documentInfo.filename.c_str());
			showNotification("Saved '%s'", documentInfo.filename.c_str());
		}
		
	// todo
		if (doButton("save as", size * 2, size, true))
		{
			save(documentInfo.filename.c_str());
			showNotification("Saved as '%s'", documentInfo.filename.c_str());
		}
		
		if (doTextBox(documentInfo.filename, "filename", dt))
		{
			const std::string filename = documentInfo.filename.c_str();
			
			load(filename.c_str());
			showNotification("Loaded '%s'", documentInfo.filename.c_str());
		}

		if (realTimeConnection)
		{
			if (doButton("restart"))
			{
				realTimeConnection->loadBegin();
				realTimeConnection->loadEnd(*this);
			}
		}
		
		doBreak();
	}
	popMenu();
}

void GraphEdit::doEditorOptions(const float dt)
{
	if (!enabled(kFlag_EditorOptions))
		return;
	
	if (g_doActions && commandMod() && keyboard.wentDown(SDLK_g))
		editorOptions.showGrid = !editorOptions.showGrid;
	
	pushMenu("editorOptions");
	{
		if (doDrawer(editorOptions.menuIsVisible, "editor options"))
		{
			doTextBox(editorOptions.comment, "comment", dt);
			doCheckBox(editorOptions.realTimePreview, "real-time preview", false);
			doCheckBox(editorOptions.autoHideUi, "auto-hide UI", false);
			doCheckBox(editorOptions.showBackground, "show background", false);
			doCheckBox(editorOptions.showGrid, "show grid", false);
			doParticleColor(editorOptions.backgroundColor, "background color");
			doParticleColor(editorOptions.gridColor, "grid color");
			doCheckBox(editorOptions.snapToGrid, "snap to grid", false);
			doCheckBox(editorOptions.showOneShotActivity, "show one-shot activity", false);
			doCheckBox(editorOptions.showContinuousActivity, "show continuous activity", false);
			doCheckBox(editorOptions.showCpuHeat, "show CPU heat", false);
			doParticleColorCurve(editorOptions.cpuHeatColors, "0 .. 33ms");
			
			if (uiState->activeColor == &editorOptions.gridColor ||
				uiState->activeColor == &editorOptions.backgroundColor ||
				uiState->activeColor != nullptr)
			{
				doColorWheel(*uiState->activeColor, "colorwheel", dt);
			}
		}
		
		doBreak();
	}
	popMenu();
	
	if (uiState->activeElem == nullptr)
		uiState->activeColor = nullptr;
}

static bool doMenuItem(const GraphEdit & graphEdit, std::string & valueText, const std::string & name, const std::string & defaultValue, const std::string & editor, const float dt, const int index, ParticleColor * uiColors, const int maxUiColors, const GraphEdit_EnumDefinition * enumDefinition, bool & pressed);

void GraphEdit::doLinkParams(const float dt)
{
	if (!enabled(kFlag_LinkEdit))
		return;
	
	pushMenu("linkParams");
	{
		if (selectedLinks.size() != 1)
		{
			if (g_doActions)
			{
				linkParamsEditorLinkId = kGraphLinkIdInvalid;
			}
		}
		else
		{
			auto linkId = *selectedLinks.begin();
			
			if (g_doActions && linkId != linkParamsEditorLinkId)
			{
				linkParamsEditorLinkId = linkId;
				
				resetMenu();
			}
			
			auto link = tryGetLink(linkId);
			Assert(link != nullptr);
			
			auto linkTypeDefinition = tryGetLinkTypeDefinition(linkId);
			
			if (linkTypeDefinition != nullptr)
			{
				int menuItemIndex = 0;
				
				for (auto & param : linkTypeDefinition->params)
				{
					auto valueTextItr = link->params.find(param.name);
				
					const bool isPreExisting = valueTextItr != link->params.end();
					
					std::string oldValueText;
					
					if (isPreExisting)
						oldValueText = valueTextItr->second;
					else
						oldValueText = param.defaultValue;
					
					std::string newValueText = oldValueText;
					
					const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeDefinitionLibrary->tryGetValueTypeDefinition(param.typeName);
					
					//const GraphEdit_EnumDefinition * enumDefinition = typeLibrary->tryGetEnumDefinition(param.enumName);
					const GraphEdit_EnumDefinition * enumDefinition = nullptr;
					
					bool pressed = false;
					
					const bool hasValue = doMenuItem(*this, newValueText, param.name, param.defaultValue,
						enumDefinition != nullptr
						? "enum"
						: valueTypeDefinition != nullptr
						? valueTypeDefinition->editor
						: "textbox",
						dt, menuItemIndex, nullptr, 0, enumDefinition, pressed);
					
					menuItemIndex++;
					
					if (isPreExisting)
					{
						if (!hasValue)
						{
							link->params.erase(param.name);
							
							if (realTimeConnection)
							{
								realTimeConnection->clearLinkParameter(
									linkId,
									link->srcNodeId,
									link->srcNodeSocketIndex,
									link->dstNodeId,
									link->dstNodeSocketIndex,
									param.name);
							}
						}
						else
							valueTextItr->second = newValueText;
					}
					else
					{
						if (hasValue)
							link->params[param.name] = newValueText;
					}
					
					if (realTimeConnection)
					{
						if (newValueText != oldValueText && hasValue)
						{
							realTimeConnection->setLinkParameter(
								link->id,
								link->srcNodeId,
								link->srcNodeSocketIndex,
								link->dstNodeId,
								link->dstNodeSocketIndex,
								param.name,
								newValueText);
						}
					}
					
					++menuItemIndex;
				}
			}
		}
		
		doBreak();
	}
	popMenu();
}

bool GraphEdit::isInputIdle() const
{
	bool result = true;
	
	result &= keyboard.isIdle();
	result &= mouse.isIdle();

	for (auto & e : framework.events)
	{
		result &= e.type != SDL_FINGERDOWN;
		result &= e.type != SDL_FINGERUP;
		result &= e.type != SDL_FINGERMOTION;
	}
	
	return result;
}

bool GraphEdit::tryAddNode(const std::string & typeName, const float x, const float y, const bool select, GraphNodeId * nodeId)
{
	if (state != kState_Idle)
	{
		return false;
	}
	else if (typeName.empty())
	{
		return false;
	}
	else if (typeDefinitionLibrary->tryGetTypeDefinition(typeName) == nullptr)
	{
		return false;
	}
	else
	{
		GraphNode node;
		node.id = graph->allocNodeId();
		node.typeName = typeName;
		
		graph->addNode(node);
		
		auto nodeData = tryGetNodeData(node.id);
		Assert(nodeData != nullptr);
		if (nodeData != nullptr)
		{
			nodeData->x = x;
			nodeData->y = y;
			nodeData->zKey = nextZKey++;
		
			if (editorOptions.snapToGrid)
			{
				snapToGrid(nodeData->x, nodeData->y);
			}
		}
		
		if (select)
		{
			selectNode(node.id, true);
		}
		
		if (nodeId != nullptr)
		{
			*nodeId = node.id;
		}
		
		return true;
	}
}

bool GraphEdit::tryAddVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex, const float x, const float y, const bool select, EditorVisualizer ** out_visualizer)
{
	if (nodeId == kGraphNodeIdInvalid)
	{
		if (out_visualizer != nullptr)
			*out_visualizer = nullptr;
		
		return false;
	}
	else
	{
		auto visualizerId = graph->allocNodeId();
		EditorVisualizer & visualizer = visualizers[visualizerId];
		
		visualizer.id = visualizerId;
		visualizer.x = x;
		visualizer.y = y;
		visualizer.zKey = nextZKey++;
		visualizer.init(nodeId, srcSocketName, srcSocketIndex, dstSocketName, dstSocketIndex);
		visualizer.sx = 0;
		visualizer.sy = 0;
		
		if (select)
		{
			selectVisualizer(&visualizer, true);
		}
		
		if (out_visualizer != nullptr)
			*out_visualizer = &visualizer;
		
		return true;
	}
}

void GraphEdit::resolveSocketIndices(
	const GraphNodeId srcNodeId, const std::string & srcNodeSocketName, int & srcNodeSocketIndex,
	const GraphNodeId dstNodeId, const std::string & dstNodeSocketName, int & dstNodeSocketIndex)
{
	auto srcNode = tryGetNode(srcNodeId);
	Assert(srcNode != nullptr);
	
	if (srcNode != nullptr)
	{
		auto srcNodeTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
		
		if (srcNodeTypeDefinition != nullptr && (srcNodeSocketIndex < 0 || srcNodeSocketIndex >= srcNodeTypeDefinition->inputSockets.size()))
		{
			srcNodeSocketIndex = -1;
			
			auto srcNodeData = tryGetNodeData(srcNodeId);
			Assert(srcNodeData);
			
			if (srcNodeData)
			{
				int index = 0;
				
				for (auto & inputSocket : srcNodeData->dynamicSockets.inputSockets)
				{
					if (srcNodeSocketName == inputSocket.name)
						srcNodeSocketIndex = index;
					index++;
				}
			}
		}
	}

	//
	
	auto dstNode = tryGetNode(dstNodeId);
	Assert(dstNode != nullptr);

	if (dstNode != nullptr)
	{
		auto dstNodeTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
		
		if (dstNodeTypeDefinition != nullptr && (dstNodeSocketIndex < 0 || dstNodeSocketIndex >= dstNodeTypeDefinition->outputSockets.size()))
		{
			dstNodeSocketIndex = -1;
			
			auto dstNodeData = tryGetNodeData(dstNodeId);
			Assert(dstNodeData);
			
			if (dstNodeData)
			{
				int index = 0;
				
				for (auto & outputSocket : dstNodeData->dynamicSockets.outputSockets)
				{
					if (dstNodeSocketName == outputSocket.name)
						dstNodeSocketIndex = index;
					index++;
				}
			}
		}
	}
}

void GraphEdit::selectNode(const GraphNodeId nodeId, const bool clearSelection)
{
	Assert(selectedNodes.count(nodeId) == 0 || clearSelection);
	
	if (clearSelection)
	{
		selectedNodes.clear();
		selectedLinks.clear();
		selectedLinkRoutePoints.clear();
		selectedVisualizers.clear();
	}
	
	selectedNodes.insert(nodeId);
}

void GraphEdit::selectLink(const GraphLinkId linkId, const bool clearSelection)
{
	Assert(selectedLinks.count(linkId) == 0 || clearSelection);
	
	if (clearSelection)
	{
		selectedNodes.clear();
		selectedLinks.clear();
		selectedLinkRoutePoints.clear();
		selectedVisualizers.clear();
	}
	
	selectedLinks.insert(linkId);
}

void GraphEdit::selectLinkRoutePoint(GraphLinkRoutePoint * routePoint, const bool clearSelection)
{
	Assert(selectedLinkRoutePoints.count(routePoint) == 0 || clearSelection);
	
	if (clearSelection)
	{
		selectedNodes.clear();
		selectedLinks.clear();
		selectedLinkRoutePoints.clear();
		selectedVisualizers.clear();
	}
	
	selectedLinkRoutePoints.insert(routePoint);
}

void GraphEdit::selectVisualizer(EditorVisualizer * visualizer, const bool clearSelection)
{
	Assert(selectedVisualizers.count(visualizer) == 0 || clearSelection);
	
	if (clearSelection)
	{
		selectedNodes.clear();
		selectedLinks.clear();
		selectedLinkRoutePoints.clear();
		selectedVisualizers.clear();
	}
	
	selectedVisualizers.insert(visualizer);
}

void GraphEdit::selectNodeAll()
{
	selectedNodes.clear();
	
	for (auto & nodeItr : graph->nodes)
		selectedNodes.insert(nodeItr.first);
}

void GraphEdit::selectLinkAll()
{
	selectedLinks.clear();
	
	for (auto & linkItr : graph->links)
		selectedLinks.insert(linkItr.first);
}

void GraphEdit::selectLinkRoutePointAll()
{
	selectedLinkRoutePoints.clear();
	
	for (auto & linkItr : graph->links)
	{
		auto & link = linkItr.second;
		
		for (auto & routePoint : link.editorRoutePoints)
		{
			selectedLinkRoutePoints.insert(&routePoint);
		}
	}
}

void GraphEdit::selectVisualizerAll()
{
	selectedVisualizers.clear();
	
	for (auto & visualizerItr : visualizers)
		selectedVisualizers.insert(&visualizerItr.second);
}

void GraphEdit::selectAll()
{
	selectNodeAll();
	selectLinkAll();
	selectLinkRoutePointAll();
	selectVisualizerAll();
}

void GraphEdit::snapToGrid(float & x, float & y) const
{
	x = std::round(x / float(kGridSize)) * kGridSize;
	y = std::round(y / float(kGridSize)) * kGridSize;
}

void GraphEdit::beginEditing()
{
	if (state == kState_Hidden)
	{
		state = kState_Idle;
	}
}

void GraphEdit::cancelEditing()
{
	switch (state)
	{
	case kState_Idle:
		break;

	case kState_NodeSelect:
		{
			nodeSelectEnd();

			state = kState_Idle;
			break;
		}

	case kState_NodeDrag:
		{
			nodeDragEnd();

			state = kState_Idle;
			break;
		}
		
	case kState_NodeResourceEdit:
		{
			nodeResourceEditEnd();
			
			state = kState_Idle;
			break;
		}
		
	case kState_InputSocketConnect:
		{
			socketConnectEnd();
			
			state = kState_Idle;
			break;
		}
		
	case kState_OutputSocketConnect:
		{
			socketConnectEnd();
			
			state = kState_Idle;
			break;
		}
		
	case kState_NodeResize:
		{
			nodeResize = NodeResize();
			
			state = kState_Idle;
			break;
		}
		
	case kState_NodeInsert:
		{
			nodeInsertMenu->cancel();
			
			state = kState_Idle;
			break;
		}
		
	case kState_TouchDrag:
		{
			touches = Touches();

			state = kState_Idle;
			break;
		}
		
	case kState_TouchZoom:
		{
			touches = Touches();
			
			state = kState_Idle;
			break;
		}
		break;
			
	case kState_Hidden:
		//state = kState_Idle;
		break;
		
	case kState_HiddenIdle:
		//state = kState_Idle;
		break;
	}
}

void GraphEdit::showNotification(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	Notification n;
	n.text = text;
	n.displayTime = 2.f;
	n.displayTimeRcp = 1.f / n.displayTime;
	
	notifications.push_back(n);
	
	logDebug("notification: %s", text);
}

void GraphEdit::draw() const
{
	cpuTimingBlock(GraphEdit_Draw);
	
	if (state == kState_Hidden || (state == kState_HiddenIdle && hideTime == 0.f))
		return;
	
	GRAPHEDIT_SX = displaySx;
	GRAPHEDIT_SY = displaySy;
	
	gxPushMatrix();
	gxMultMatrixf(dragAndZoom.transform.m_v);
	gxTranslatef(.375f, .375f, 0.f); // avoid bad looking rectangles due to some diamond rule shenanigans in OpenGL by translating slightly
	
	pushFontMode(FONT_SDF);
	
	// draw background and grid
	
	{
		const Vec2 _p1 = dragAndZoom.invTransform * Vec2(0.f, 0.f);
		const Vec2 _p2 = dragAndZoom.invTransform * Vec2(GRAPHEDIT_SX, GRAPHEDIT_SY);
		
		const Vec2 p1 = _p1.Min(_p2);
		const Vec2 p2 = _p1.Max(_p2);
		
		if (editorOptions.showBackground)
		{
			setColorf(
				editorOptions.backgroundColor.rgba[0],
				editorOptions.backgroundColor.rgba[1],
				editorOptions.backgroundColor.rgba[2],
				editorOptions.backgroundColor.rgba[3]);
			drawRect(p1[0], p1[1], p2[0], p2[1]);
		}
		
		if (editorOptions.showGrid)
		{
			const int cx1 = std::floor(p1[0] / kGridSize);
			const int cy1 = std::floor(p1[1] / kGridSize);
			const int cx2 = std::ceil(p2[0] / kGridSize);
			const int cy2 = std::ceil(p2[1] / kGridSize);
			
			if ((cx2 - cx1) < GRAPHEDIT_SX / 2)
			{
				setColorf(
					editorOptions.gridColor.rgba[0],
					editorOptions.gridColor.rgba[1],
					editorOptions.gridColor.rgba[2],
					editorOptions.gridColor.rgba[3]);
				hqBegin(HQ_LINES, true);
				{
					for (int cx = cx1; cx <= cx2; ++cx)
					{
						hqLine(cx * kGridSize, cy1 * kGridSize, 1.f, cx * kGridSize, cy2 * kGridSize, 1.f);
					}
					
					for (int cy = cy1; cy <= cy2; ++cy)
					{
						hqLine(cx1 * kGridSize, cy * kGridSize, 1.f, cx2 * kGridSize, cy * kGridSize, 1.f);
					}
				}
				hqEnd();
			}
		}
	}

#if 0 // todo : remove. test bezier control points, drawing only circles
	{
		for (auto & linkItr : graph->links)
		{
			auto linkId = linkItr.first;
			auto & link = linkItr.second;
			
			LinkPath path;
			
			if (getLinkPath(linkId, path))
			{
				const bool isEnabled = link.isEnabled;
				const bool isSelected = selectedLinks.count(linkId) != 0;
				const bool isHighlighted = highlightedLinks.count(linkId) != 0;
				
				Color color;
				
				if (!isEnabled)
					color = Color(191, 191, 191);
				else if (isSelected)
					color = Color(127, 127, 255);
				else if (isHighlighted)
					color = Color(255, 255, 255);
				else
					color = Color(255, 255, 0);
				
				if (editorOptions.showOneShotActivity)
				{
					const float activeAnim = link.editorIsActiveAnimTime * link.editorIsActiveAnimTimeRcp;
					color = color.interp(Color(255, 63, 63), activeAnim);
				}
				
				setColor(color);
				
				float x1 = path.points[0].x;
				float y1 = path.points[0].y;
				
				Path2d path2d;
				
				path2d.moveTo(x1, y1);
				
				for (int i = 1; i < path.points.size(); ++i)
				{
					const float x2 = path.points[i].x;
					const float y2 = path.points[i].y;
					
					path2d.curveTo(x2, y2, -30.f, 0.f, +30.f, 0.f);
					
					x1 = x2;
					y1 = y2;
				}
				
				const int kMaxPoints = 100;

				float pxyStorage[kMaxPoints * 2];
				float hxyStorage[kMaxPoints * 2];

				float * pxy = pxyStorage;
				float * hxy = hxyStorage;

				int numPoints = 0;
				path2d.generatePoints(pxy, hxy, kMaxPoints, 1.f, numPoints);
				
			#if 1
				hqBegin(HQ_LINES);
				{
					for (int i = 0; i < numPoints - 1; ++i)
					{
						const int index1 = i + 0;
						const int index2 = i + 1;
						
						hqLine(pxy[index1 * 2 + 0], pxy[index1 * 2 + 1], 1.f, pxy[index2 * 2 + 0], pxy[index2 * 2 + 1], 1.f);
					}
				}
				hqEnd();
			#else
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 0; i < numPoints; ++i)
					{
						hqFillCircle(pxy[i * 2 + 0], pxy[i * 2 + 1], 1.f);
					}
				}
				hqEnd();
			#endif
			}
		}
	}
#elif 0 // todo : remove. test bezier control points
	{
		for (auto & linkItr : graph->links)
		{
			auto linkId = linkItr.first;
			
			LinkPath path;
			
			if (getLinkPath(linkId, path))
			{
				setColor(colorWhite);
				
				float x1 = path.points[0].x;
				float y1 = path.points[0].y;
				
				Path2d path2d;
				
				path2d.moveTo(x1, y1);
				
				for (int i = 1; i < path.points.size(); ++i)
				{
					const float x2 = path.points[i].x;
					const float y2 = path.points[i].y;
					
					path2d.curveTo(x2, y2, -30.f, 0.f, +30.f, 0.f);
					
					x1 = x2;
					y1 = y2;
				}
				
				hqDrawPath(path2d, 4.f);
			}
		}
	}
#else
	// traverse and draw links
	
	hqBegin(HQ_LINES);
	{
		for (auto & linkItr : graph->links)
		{
			auto linkId = linkItr.first;
			auto & link = linkItr.second;
			
			LinkPath path;
			
			if (getLinkPath(linkId, path))
			{
				const bool isEnabled = link.isEnabled;
				const bool isSelected = selectedLinks.count(linkId) != 0;
				const bool isHighlighted = highlightedLinks.count(linkId) != 0;
				
				Color color;
				
				if (!isEnabled)
					color = Color(191, 191, 191);
				else if (isSelected)
					color = Color(127, 127, 255);
				else if (isHighlighted)
					color = Color(255, 255, 255);
				else
					color = Color(255, 255, 0);
				
				if (editorOptions.showOneShotActivity)
				{
					const float activeAnim = link.editorIsActiveAnimTime * link.editorIsActiveAnimTimeRcp;
					color = color.interp(Color(255, 63, 63), activeAnim);
				}
				
				setColor(color);
				
				float x1 = path.points[0].x;
				float y1 = path.points[0].y;
				
				for (int i = 1; i < path.points.size(); ++i)
				{
					const float x2 = path.points[i].x;
					const float y2 = path.points[i].y;
					
					hqLine(
						x1, y1, 2.f,
						x2, y2, 2.f);
					
					x1 = x2;
					y1 = y2;
				}
			}
		}
	}
	hqEnd();
#endif

	hqBegin(HQ_FILLED_CIRCLES);
	{
		for (auto & linkItr : graph->links)
		{
			auto & link = linkItr.second;
			
			for (auto & routePoint : link.editorRoutePoints)
			{
				const bool isSelected = selectedLinkRoutePoints.count(&routePoint) != 0;
				const bool isHighlighted = highlightedLinkRoutePoints.count(&routePoint) != 0;
				
				if (isSelected)
					setColor(127, 127, 255);
				else if (isHighlighted)
					setColor(255, 255, 255);
				else
					setColor(255, 255, 0);
				
				hqFillCircle(routePoint.x, routePoint.y, 5.f);
			}
		}
	}
	hqEnd();
	
	// traverse and draw nodes
	
	const int numElems = graph->nodes.size() + visualizers.size();
	
	SortedGraphElem * sortedElems = (SortedGraphElem*)alloca(numElems * sizeof(SortedGraphElem));
	memset(sortedElems, 0, numElems * sizeof(SortedGraphElem));
	
	int elemIndex = 0;
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		auto nodeData = tryGetNodeData(node.id);
		
		sortedElems[elemIndex].node = &node;
		sortedElems[elemIndex].nodeData = nodeData;
		sortedElems[elemIndex].zKey = nodeData->zKey;
		
		elemIndex++;
	}
	
	for (auto & visualizerItr : visualizers)
	{
		auto & visualizer = visualizerItr.second;
		
		sortedElems[elemIndex].visualizer = const_cast<EditorVisualizer*>(&visualizer);
		sortedElems[elemIndex].zKey = visualizer.zKey;
		
		elemIndex++;
	}
	
	Assert(elemIndex == numElems);
	
	std::sort(sortedElems, sortedElems + numElems, [](const SortedGraphElem & e1, const SortedGraphElem & e2) { return e1.zKey < e2.zKey; });
	
	for (int i = 0; i < numElems; ++i)
	{
		if (sortedElems[i].node != nullptr)
		{
			auto & node = *sortedElems[i].node;
			auto & nodeData = *sortedElems[i].nodeData;
			
			const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
			
			if (typeDefinition == nullptr)
			{
				gxPushMatrix();
				{
					gxTranslatef(nodeData.x, nodeData.y, 0.f);
					
					drawNode(node, nodeData, emptyTypeDefinition, node.typeName.c_str());
				}
				gxPopMatrix();
			}
			else
			{
				gxPushMatrix();
				{
					gxTranslatef(nodeData.x, nodeData.y, 0.f);
					
					const char * displayName =
						!nodeData.displayName.empty()
						? nodeData.displayName.c_str()
						: typeDefinition->displayName.empty()
							? typeDefinition->typeName.c_str()
							: typeDefinition->displayName.c_str();
					
					drawNode(node, nodeData, *typeDefinition, displayName);
				}
				gxPopMatrix();
			}
		}
		else if (sortedElems[i].visualizer != nullptr)
		{
			auto & visualizer = *sortedElems[i].visualizer;
			
			gxPushMatrix();
			{
				gxTranslatef(visualizer.x, visualizer.y, 0.f);
				
				drawVisualizer(visualizer);
			}
			gxPopMatrix();
		}
		else
		{
			Assert(false);
		}
	}
	
	switch (state)
	{
	case kState_Idle:
		break;
		
	case kState_NodeSelect:
		{
			setColor(127, 127, 255, 127);
			drawRect(nodeSelect.beginX, nodeSelect.beginY, nodeSelect.endX, nodeSelect.endY);
			
			setColor(127, 127, 255, 255);
			drawRectLine(nodeSelect.beginX, nodeSelect.beginY, nodeSelect.endX, nodeSelect.endY);
		}
		break;
	
	case kState_NodeDrag:
		break;
		
	case kState_NodeResourceEdit:
		break;
	
	case kState_InputSocketConnect:
		{
			auto nodeData = tryGetNodeData(socketConnect.srcNodeId);
			
			Assert(nodeData != nullptr);
			if (nodeData != nullptr)
			{
				hqBegin(HQ_LINES);
				{
					float socketX, socketY, socketRadius;
					getNodeInputSocketCircle(socketConnect.srcNodeSocket->index, socketX, socketY, socketRadius);
					
					setColor(255, 255, 0);
					hqLine(
						nodeData->x + socketX,
						nodeData->y + socketY, 1.5f,
						mousePosition.x, mousePosition.y, 1.5f);
				}
				hqEnd();
			}
		}
		break;
	
	case kState_OutputSocketConnect:
		{
			auto nodeData = tryGetNodeData(socketConnect.dstNodeId);
			
			Assert(nodeData != nullptr);
			if (nodeData != nullptr)
			{
				hqBegin(HQ_LINES);
				{
					float socketX, socketY, socketRadius;
					getNodeOutputSocketCircle(socketConnect.dstNodeSocket->index, socketX, socketY, socketRadius);
					
					setColor(255, 255, 0);
					hqLine(
						nodeData->x + socketX,
						nodeData->y + socketY, 1.5f,
						mousePosition.x, mousePosition.y, 1.5f);
				}
				hqEnd();
			}
		}
		break;
		
	case kState_NodeResize:
		break;
		
	case kState_NodeInsert:
		break;
		
	case kState_TouchDrag:
		break;
		
	case kState_TouchZoom:
		break;
		
	case kState_Hidden:
		break;
		
	case kState_HiddenIdle:
		break;
	}

	gxPopMatrix();
	
	switch (state)
	{
	case kState_Idle:
		{
			if (realTimeSocketCapture.visualizer.nodeId != kGraphNodeIdInvalid)
			{
				gxPushMatrix();
				{
					gxTranslatef(mousePosition.uiX + 15, mousePosition.uiY, 0);
					
					realTimeSocketCapture.visualizer.draw(*this, String::Empty, false, nullptr, nullptr);
				}
				gxPopMatrix();
			}
		}
		break;
		
	case kState_NodeSelect:
		break;
		
	case kState_NodeDrag:
		break;
		
	case kState_NodeResourceEdit:
		break;
		
	case kState_InputSocketConnect:
		break;
		
	case kState_OutputSocketConnect:
		break;
		
	case kState_NodeResize:
		break;
		
	case kState_NodeInsert:
		break;
		
	case kState_TouchDrag:
		break;
		
	case kState_TouchZoom:
		break;
		
	case kState_Hidden:
		break;
	
	case kState_HiddenIdle:
		break;
	}
	
	makeActive(uiState, false, true);
	
	GraphEdit * self = const_cast<GraphEdit*>(this);
	
	self->doMenu(0.f);
	
	self->doLinkParams(0.f);
	
	self->doEditorOptions(0.f);
	
	nodeTypeNameSelect->doMenus(uiState, 0.f);
	
	propertyEditor->doMenus(uiState, 0.f);
	
	//
	
	if (state == kState_NodeResourceEdit)
	{
		setColor(0, 0, 0, 127);
		drawRect(0, 0, GRAPHEDIT_SX, GRAPHEDIT_SY);
	}
	
	if (nodeResourceEditor.resourceEditor != nullptr)
	{
		setColor(colorWhite);
		nodeResourceEditor.resourceEditor->draw();
	}
	
	//
	
	if (state == kState_NodeInsert)
	{
		nodeInsertMenu->draw(*this, *typeDefinitionLibrary);
	}
	
	//
	
	if (!notifications.empty())
	{
		gxPushMatrix();
		
		const int kWidth = 200;
		const int kHeight = 40;
		
		auto & firstNotification = notifications.front();
		
		const float t = firstNotification.displayTime * firstNotification.displayTimeRcp;
		const float tMoveUp = .05f;
		const float tMoveDown = .05f;
		
		float y = 1.f;
		
		if (t > (1.f - tMoveUp))
			y = 1.f - (t - (1.f - tMoveUp)) / tMoveUp;
		if (t < tMoveDown)
			y = t / tMoveDown;
		
		y *= 50;
		
		gxTranslatef(0, GRAPHEDIT_SY - y, 0);
		
		for (auto & n : notifications)
		{
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setColor(0, 0, 0, 191);
				hqFillRoundedRect(GRAPHEDIT_SX/2 - kWidth, 0, GRAPHEDIT_SX/2 + kWidth, kHeight, 7.f);
			}
			hqEnd();
			
			setColor(colorWhite);
			setFont("calibri.ttf");
			drawText(GRAPHEDIT_SX/2, kHeight/2, 18, 0.f, 0.f, "%s", n.text.c_str());
			
			gxTranslatef(0, -50, 0);
		}
		
		gxPopMatrix();
	}
	
	HitTestResult hitTestResult;
	
	if (state == kState_Idle && hitTest(mousePosition.x, mousePosition.y, hitTestResult))
	{
		if (hitTestResult.hasNode &&
			hitTestResult.nodeHitTestResult.background)
		{
			// draw node description
			
			if (realTimeConnection != nullptr)
			{
				std::vector<std::string> lines;
				
				if (realTimeConnection->getNodeDescription(hitTestResult.node->id, lines) && !lines.empty())
				{
					const int kFontSize = 14;
					const int kSpacing = 4;
					const int kBorderSize = 8;
					
					setFont("calibri.ttf");
					
					int sx = 0;
					int sy = 0;
					
					sy += kBorderSize;
					sy += kFontSize * lines.size() + kSpacing * (lines.size() - 1);
					sy += kBorderSize;
					
					for (auto & line : lines)
					{
						float textSx;
						float textSy;
						
						measureText(kFontSize, textSx, textSy, "%s", line.c_str());
						
						sx = std::max(sx, int(std::ceil(textSx)));
					}
					
					sx += kBorderSize * 2;
								
					gxPushMatrix();
					{
						gxTranslatef(mousePosition.uiX + 10, mousePosition.uiY + 10, 0);
						
						setColor(255, 255, 227);
						drawRect(0, 0, sx, sy);
						
						gxTranslatef(kBorderSize, kBorderSize, 0);
						
						setColor(colorBlack);
						beginTextBatch();
						{
							int y = 0;
							
							for (auto & line : lines)
							{
								drawText(0, y, kFontSize, +1, +1, "%s", line.c_str());
								y += kFontSize + kSpacing;
							}
						}
						endTextBatch();
					}
					gxPopMatrix();
				}
			}
		}
	}
	
	popFontMode();
}

void GraphEdit::drawNode(const GraphNode & node, const NodeData & nodeData, const GraphEdit_TypeDefinition & definition, const char * displayName) const
{
	cpuTimingBlock(drawNode);
	
	auto & inputSockets = getInputSockets(definition, nodeData);
	auto & outputSockets = getOutputSockets(definition, nodeData);
	
	const bool isSelected = selectedNodes.count(node.id) != 0;
	const bool socketsAreVisible = areNodeSocketsVisible(nodeData);
	
	float sx;
	float sy;
	
	if (nodeData.foldAnimProgress == 0.f)
		getNodeRect(inputSockets.size(), outputSockets.size(), true, sx, sy);
	else if (nodeData.foldAnimProgress == 1.f)
		getNodeRect(inputSockets.size(), outputSockets.size(), false, sx, sy);
	else
	{
		float sx1;
		float sy1;
		float sx2;
		float sy2;
		
		getNodeRect(inputSockets.size(), outputSockets.size(), true, sx1, sy1);
		getNodeRect(inputSockets.size(), outputSockets.size(), false, sx2, sy2);
		
		sx = lerp(sx1, sx2, nodeData.foldAnimProgress);
		sy = lerp(sy1, sy2, nodeData.foldAnimProgress);
	}
	
	Color color;
	
	if (isSelected)
		color = Color(63, 63, 127, 255);
	else if (definition.typeName.empty()) // error node
		color = Color(255, 31, 31, 255);
	else
		color = Color(63, 63, 63, 255);
	
	if (editorOptions.showOneShotActivity)
	{
		const float activeAnim = nodeData.isActiveAnimTime * nodeData.isActiveAnimTimeRcp;
		color = color.interp(Color(63, 63, 255), activeAnim);
	}
	
	if (editorOptions.showContinuousActivity)
	{
		if (nodeData.isActiveContinuous)
		{
			color = color.interp(Color(63, 63, 255), .5f + .5f * (std::cos(framework.time * 8.f) + 1.f) / 2.f);
		}
	}
	
	std::vector<std::string> issues;
	
	const bool hasIssues =
		realTimeConnection != nullptr
		? realTimeConnection->getNodeIssues(node.id, issues)
		: false;
	
	if (hasIssues)
	{
		const Color color1(255, 0, 0, 255);
		const Color color2(127, 0, 0, 255);
		Mat4x4 cmat = Mat4x4(true).Scale(1.f / GRAPHEDIT_SX, 1.f / GRAPHEDIT_SX, 1.f).RotateZ(45);
		
		hqSetGradient(GRADIENT_LINEAR, cmat, color1, color2, COLOR_ADD);
	}
	
	if (editorOptions.showCpuHeat && realTimeConnection != nullptr)
	{
		const int timeUs = realTimeConnection->getNodeCpuTimeUs(node.id);
		const float t = timeUs / float(realTimeConnection->getNodeCpuHeatMax());
		
		ParticleColor particleColor;
		editorOptions.cpuHeatColors.sample(t, true, particleColor);
		
		color = color.interp(Color(particleColor.rgba[0], particleColor.rgba[1], particleColor.rgba[2]), particleColor.rgba[3]);
	}
	
	const float border = 3.f;
	const float radius = 5.f;
	
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	{
		setColor(isSelected ? colorWhite : colorBlack);
		hqFillRoundedRect(0.f, 0, sx, sy, radius + border/2);
		
		setColor(color);
		hqFillRoundedRect(0.f + border/2, 0.f + border/2, sx - border/2, sy - border/2, radius);
	}
	hqEnd();
	
	if (hasIssues)
		hqClearGradient();
	
	if (isSelected)
		setColor(255, 255, 255, 255);
	else
		setColor(127, 127, 127, 255);
	//drawRectLine(0.f, 0.f, sx, sy);
	
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(sx/2, 12, 14, 0.f, 0.f, "%s", displayName);
	
	if (node.isPassthrough)
	{
		setColor(127, 127, 255);
		drawText(sx - 8, 12, 14, -1.f, 0.f, "P");
	}
	
	if (socketsAreVisible)
	{
		beginTextBatch();
		{
			for (auto & inputSocket : inputSockets)
			{
				float x, y, radius;
				getNodeInputSocketCircle(inputSocket.index, x, y, radius);
				
				const std::string name = inputSocket.displayName.empty()
					? inputSocket.name
					: inputSocket.displayName;
				
				setColor(255, 255, 255);
				drawText(x + radius + 2, y, 12, +1.f, 0.f, "%s", name.c_str());
			}
			
			for (auto & outputSocket : outputSockets)
			{
				float x, y, radius;
				getNodeOutputSocketCircle(outputSocket.index, x, y, radius);
				
				const std::string name = outputSocket.displayName.empty()
					? outputSocket.name
					: outputSocket.displayName;
				
				setColor(255, 255, 255);
				drawText(x - radius - 2, y, 12, -1.f, 0.f, "%s", name.c_str());
			}
		}
		endTextBatch();
		
		hqBegin(HQ_FILLED_CIRCLES);
		{
			for (auto & inputSocket : inputSockets)
			{
				if ((state == kState_InputSocketConnect) && (node.id == socketConnect.srcNodeId) && (&inputSocket == socketConnect.srcNodeSocket))
				{
					setColor(255, 255, 0);
				}
				else if ((node.id == highlightedSockets.srcNodeId) && (&inputSocket == highlightedSockets.srcNodeSocket))
				{
					setColor(255, 255, 255);
				}
				else if (state == kState_OutputSocketConnect && node.id != socketConnect.dstNodeId && inputSocket.canConnectTo(typeDefinitionLibrary, *socketConnect.dstNodeSocket))
				{
					setColor(255, 255, 255);
				}
				else
				{
					setColor(255, 0, 0);
				}
				
				float x, y, radius;
				getNodeInputSocketCircle(inputSocket.index, x, y, radius);
				
				hqFillCircle(x, y, radius);
				
				if (node.inputValues.count(inputSocket.name) != 0)
				{
					setColor(255, 255, 255);
					hqFillCircle(x, y, radius / 3.f);
				}
			}
			
			for (auto & outputSocket : outputSockets)
			{
				if ((state == kState_OutputSocketConnect) && (node.id == socketConnect.dstNodeId) && (&outputSocket == socketConnect.dstNodeSocket))
				{
					setColor(255, 255, 0);
				}
				else if ((node.id == highlightedSockets.dstNodeId) && (&outputSocket == highlightedSockets.dstNodeSocket))
				{
					setColor(255, 255, 255);
				}
				else if (state == kState_InputSocketConnect && node.id != socketConnect.srcNodeId && outputSocket.canConnectTo(typeDefinitionLibrary, *socketConnect.srcNodeSocket))
				{
					setColor(255, 255, 255);
				}
				else
				{
					setColor(0, 255, 0);
				}
				
				float x, y, radius;
				getNodeOutputSocketCircle(outputSocket.index, x, y, radius);
				
				hqFillCircle(x, y, radius);
			}
		}
		hqEnd();
	}
}

void GraphEdit::drawVisualizer(const EditorVisualizer & visualizer) const
{
	cpuTimingBlock(drawVisualizer);
	
	const bool isSelected = selectedVisualizers.count(const_cast<EditorVisualizer*>(&visualizer)) != 0;
	
	auto srcNode = tryGetNode(visualizer.nodeId);
	auto srcNodeData = tryGetNodeData(visualizer.nodeId);
	
	const std::string & nodeName = srcNode != nullptr && srcNodeData != nullptr ? getDisplayName(*srcNode, *srcNodeData) : String::Empty;
	
	const int visualizerSx = int(std::ceil(visualizer.sx));
	const int visualizerSy = int(std::ceil(visualizer.sy));
	visualizer.draw(*this, nodeName, isSelected, &visualizerSx, &visualizerSy);
}

bool GraphEdit::load(const char * filename)
{
	bool result = true;
	
	//
	
	if (realTimeConnection)
		realTimeConnection->loadBegin();
	
	//
	
	documentInfo = DocumentInfo();
	
	//
	
	propertyEditor->setGraph(nullptr);
	
	//
	
	nodeDatas.clear();
	
	visualizers.clear();
	
	nextZKey = 1;
	
	delete graph;
	graph = nullptr;
	
	graph = new Graph();
	graph->graphEditConnection = this;
	
	//
	
	XMLDocument document;
	
	result &= document.LoadFile(filename) == XML_SUCCESS;
	
	if (result == true)
	{
		const XMLElement * xmlGraph = document.FirstChildElement("graph");
		if (xmlGraph != nullptr)
		{
			result &= graph->loadXml(xmlGraph, typeDefinitionLibrary);
			
		#if ENABLE_FILE_FIXUPS
			// fixup zkey allocation
			
			nextZKey = intAttrib(xmlGraph, "nextZKey", nextZKey);
			
			// fixup node datas. todo : remove these fixups
			
			for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
			{
				const int nodeType = intAttrib(xmlNode, "nodeType", 0);
				if (nodeType != 0)
					continue;
				
				const GraphNodeId nodeId = intAttrib(xmlNode, "id", kGraphNodeIdInvalid);
				if (nodeId == kGraphNodeIdInvalid)
					continue;
				
				auto nodeDataPtr = tryGetNodeData(nodeId);
				Assert(nodeDataPtr != nullptr);
				if (nodeDataPtr == nullptr)
					continue;
				
				auto & nodeData = *nodeDataPtr;
				
				nodeData.x = floatAttrib(xmlNode, "editorX", nodeData.x);
				nodeData.y = floatAttrib(xmlNode, "editorY", nodeData.y);
				nodeData.zKey = intAttrib(xmlNode, "zKey", nodeData.zKey);
				nodeData.isFolded = boolAttrib(xmlNode, "folded", nodeData.isFolded);
				nodeData.foldAnimProgress = nodeData.isFolded ? 0.f : 1.f;
				
				nodeData.displayName = stringAttrib(xmlNode, "editorName", nodeData.displayName.c_str());
			}
			
			// fixup visualizers
			for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
			{
				const int nodeType = intAttrib(xmlNode, "nodeType", 0);
				if (nodeType != 1)
					continue;
				
				auto visualizerId = intAttrib(xmlNode, "id", kGraphNodeIdInvalid);
				Assert(visualizerId != kGraphNodeIdInvalid);
				if (visualizerId == kGraphNodeIdInvalid)
					continue;
				
				const XMLElement * xmlVisualizer = xmlNode->FirstChildElement("visualizer");
				Assert(xmlVisualizer != nullptr);
				if (xmlVisualizer == nullptr)
					continue;
				
				EditorVisualizer & visualizer = visualizers[visualizerId];
				
				visualizer.id = visualizerId;
				visualizer.x = floatAttrib(xmlNode, "editorX", visualizer.x);
				visualizer.y = floatAttrib(xmlNode, "editorY", visualizer.y);
				visualizer.zKey = intAttrib(xmlNode, "zKey", visualizer.zKey);
				
				visualizer.nodeId = visualizer.nodeId = intAttrib(xmlVisualizer, "nodeId", visualizer.nodeId);
				visualizer.srcSocketName = stringAttrib(xmlVisualizer, "srcSocketName", visualizer.srcSocketName.c_str());
				visualizer.dstSocketName = stringAttrib(xmlVisualizer, "dstSocketName", visualizer.dstSocketName.c_str());
				visualizer.sx = floatAttrib(xmlVisualizer, "sx", visualizer.sx);
				visualizer.sy = floatAttrib(xmlVisualizer, "sy", visualizer.sy);
			}
		#endif
		
			const XMLElement * xmlEditor = xmlGraph->FirstChildElement("editor");
			if (xmlEditor != nullptr)
			{
				result &= loadXml(xmlEditor);
			}
		}
	}
	
	if (result)
	{
		for (auto & visualizerItr : visualizers)
		{
			auto & visualizer = visualizerItr.second;
			
			auto linkedNode = tryGetNode(visualizer.nodeId);
			Assert(linkedNode != nullptr);
			
			if (linkedNode != nullptr)
			{
				auto & nodeData = *tryGetNodeData(visualizer.nodeId);
				
				auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(linkedNode->typeName);
				Assert(typeDefinition != nullptr);
				
				if (typeDefinition != nullptr)
				{
					if (!visualizer.srcSocketName.empty())
					{
						auto & inputSockets = getInputSockets(*typeDefinition, nodeData);
						
						for (auto & inputSocket : inputSockets)
						{
							if (inputSocket.name == visualizer.srcSocketName)
							{
								visualizer.srcSocketIndex = inputSocket.index;
								break;
							}
						}
					}
					
					if (!visualizer.dstSocketName.empty())
					{
						auto & outputSockets = getOutputSockets(*typeDefinition, nodeData);
						
						for (auto & outputSocket : outputSockets)
						{
							if (outputSocket.name == visualizer.dstSocketName)
							{
								visualizer.dstSocketIndex = outputSocket.index;
								break;
							}
						}
					}
				}
			}
			
			visualizer.init();
		}
	}
	
	//
	
	propertyEditor->setGraph(graph);
	
	//
	
	if (result)
	{
		documentInfo.filename = filename;
	}
	
	uiState->reset();
	
	//
	
	if (realTimeConnection)
		realTimeConnection->loadEnd(*this);
	
	// draw a notification for each node which is not represented in the type definition library
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefinition == nullptr)
		{
			showNotification("no node type for node %d:%s", node.id, node.typeName.c_str());
		}
	}
	
	//
	
	return result;
}

bool GraphEdit::save(const char * filename)
{
	bool result = true;
	
	FILE * file = fopen(filename, "wt");
	result &= file != nullptr;
	
	if (result)
	{
		if (realTimeConnection)
			realTimeConnection->saveBegin(*this);
		
		nodeResourceEditSave();
		
		XMLPrinter xmlGraph(file);
		
		xmlGraph.OpenElement("graph");
		{
			result &= graph->saveXml(xmlGraph, typeDefinitionLibrary);
			
			xmlGraph.OpenElement("editor");
			{
				result &= saveXml(xmlGraph);
			}
			xmlGraph.CloseElement();
		}
		xmlGraph.CloseElement();
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	//
	
	if (result)
	{
		documentInfo.filename = filename;
	}
	
	uiState->reset();
	
	return result;
}

bool GraphEdit::loadXml(const tinyxml2::XMLElement * editorElem)
{
	nextZKey = intAttrib(editorElem, "nextZKey", nextZKey);
	
	auto dragAndZoomElem = editorElem->FirstChildElement("dragAndZoom");
	
	if (dragAndZoomElem != nullptr)
	{
		dragAndZoom.desiredFocusX = floatAttrib(dragAndZoomElem, "x", 0.f);
		dragAndZoom.desiredFocusY = floatAttrib(dragAndZoomElem, "y", 0.f);
		dragAndZoom.desiredZoom = floatAttrib(dragAndZoomElem, "zoom", 1.f);
		
		/*
		dragAndZoom.focusX = dragAndZoom.desiredFocusX;
		dragAndZoom.focusY = dragAndZoom.desiredFocusY;
		dragAndZoom.zoom = dragAndZoom.desiredZoom;
		*/
	}
	
	auto nodeDatasElem = editorElem->FirstChildElement("nodeDatas");
	
	if (nodeDatasElem != nullptr)
	{
		for (auto nodeDataElem = nodeDatasElem->FirstChildElement("nodeData"); nodeDataElem != nullptr; nodeDataElem = nodeDataElem->NextSiblingElement("nodeData"))
		{
			const GraphNodeId nodeId = intAttrib(nodeDataElem, "nodeId", kGraphNodeIdInvalid);
			
			if (nodeId == kGraphNodeIdInvalid)
				continue;
			
			auto node = tryGetNode(nodeId);
			Assert(node != nullptr);
			if (node == nullptr)
				continue;
			
			NodeData & nodeData = nodeDatas[nodeId];
			
			nodeData.x = floatAttrib(nodeDataElem, "x", nodeData.x);
			nodeData.y = floatAttrib(nodeDataElem, "y", nodeData.y);
			nodeData.isFolded = boolAttrib(nodeDataElem, "folded", nodeData.isFolded);
			nodeData.foldAnimProgress = nodeData.isFolded ? 0.f : 1.f;
			
			nodeData.zKey = intAttrib(nodeDataElem, "zKey", nodeData.zKey);
			nodeData.displayName = stringAttrib(nodeDataElem, "displayName", nodeData.displayName.c_str());
			
			nextZKey = std::max(nextZKey, nodeData.zKey + 1);
		}
	}
	
	auto visualizersElem = editorElem->FirstChildElement("visualizers");
	
	if (visualizersElem != nullptr)
	{
		for (auto xmlVisualizer = visualizersElem->FirstChildElement("visualizer"); xmlVisualizer != nullptr; xmlVisualizer = xmlVisualizer->NextSiblingElement("visualizer"))
		{
			auto visualizerId = intAttrib(xmlVisualizer, "id", -1);
			
			if (visualizerId == -1)
				continue;
			
			auto & visualizer = visualizers[visualizerId];
			
			visualizer.id = visualizerId;
			visualizer.x = floatAttrib(xmlVisualizer, "x", visualizer.x);
			visualizer.y = floatAttrib(xmlVisualizer, "y", visualizer.y);
			visualizer.zKey = intAttrib(xmlVisualizer, "zKey", visualizer.zKey);
			
			visualizer.nodeId = intAttrib(xmlVisualizer, "nodeId", visualizer.nodeId);
			visualizer.srcSocketName = stringAttrib(xmlVisualizer, "srcSocketName", visualizer.srcSocketName.c_str());
			visualizer.dstSocketName = stringAttrib(xmlVisualizer, "dstSocketName", visualizer.dstSocketName.c_str());
			visualizer.sx = floatAttrib(xmlVisualizer, "sx", visualizer.sx);
			visualizer.sy = floatAttrib(xmlVisualizer, "sy", visualizer.sy);
		}
	}
	
	auto editorOptionsElem = editorElem->FirstChildElement("editorOptions");
	
	if (editorOptionsElem != nullptr)
	{
		EditorOptions defaultOptions;
		
		editorOptions.comment = stringAttrib(editorOptionsElem, "comment", editorOptions.comment.c_str());
		editorOptions.menuIsVisible = boolAttrib(editorOptionsElem, "menuIsVisible", defaultOptions.menuIsVisible);
		editorOptions.realTimePreview = boolAttrib(editorOptionsElem, "realTimePreview", defaultOptions.realTimePreview);
		editorOptions.autoHideUi = boolAttrib(editorOptionsElem, "autoHideUi", defaultOptions.autoHideUi);
		editorOptions.showBackground = boolAttrib(editorOptionsElem, "showBackground", defaultOptions.showBackground);
		editorOptions.showGrid = boolAttrib(editorOptionsElem, "showGrid", defaultOptions.showGrid);
		editorOptions.snapToGrid = boolAttrib(editorOptionsElem, "snapToGrid", defaultOptions.snapToGrid);
		editorOptions.showOneShotActivity = boolAttrib(editorOptionsElem, "showOneShotActivity", defaultOptions.showOneShotActivity);
		editorOptions.showContinuousActivity = boolAttrib(editorOptionsElem, "showContinuousActivity", defaultOptions.showContinuousActivity);
		editorOptions.showCpuHeat = boolAttrib(editorOptionsElem, "showCpuHeat", defaultOptions.showCpuHeat);
		
		const std::string defaultBackgroundColor = toColor(defaultOptions.backgroundColor).toHexString(true);
		const std::string defaultGridColor = toColor(defaultOptions.gridColor).toHexString(true);
		
		editorOptions.backgroundColor = toParticleColor(Color::fromHex(stringAttrib(editorOptionsElem, "backgroundColor", defaultBackgroundColor.c_str())));
		editorOptions.gridColor = toParticleColor(Color::fromHex(stringAttrib(editorOptionsElem, "gridColor", defaultGridColor.c_str())));
		
		editorOptions.cpuHeatColors = ParticleColorCurve();
		auto cpuHeatColorsElem = editorOptionsElem->FirstChildElement("cpuHeatColors");
		if (cpuHeatColorsElem != nullptr)
		{
			editorOptions.cpuHeatColors.load(cpuHeatColorsElem);
		}
	}
	
	return true;
}

bool GraphEdit::saveXml(tinyxml2::XMLPrinter & editorElem) const
{
	editorElem.PushAttribute("nextZKey", nextZKey);
	
	editorElem.OpenElement("dragAndZoom");
	{
		editorElem.PushAttribute("x", dragAndZoom.desiredFocusX);
		editorElem.PushAttribute("y", dragAndZoom.desiredFocusY);
		editorElem.PushAttribute("zoom", dragAndZoom.desiredZoom);
	}
	editorElem.CloseElement();
	
	editorElem.OpenElement("nodeDatas");
	{
		for (auto & nodeDataItr : nodeDatas)
		{
			auto nodeId = nodeDataItr.first;
			auto node = tryGetNode(nodeId);
			auto nodeData = nodeDataItr.second;
			
			Assert(node != nullptr);
			if (node == nullptr)
				continue;
			
			editorElem.OpenElement("nodeData");
			{
				editorElem.PushAttribute("nodeId", nodeId);
				editorElem.PushAttribute("x", nodeData.x);
				editorElem.PushAttribute("y", nodeData.y);
				if (!nodeData.displayName.empty())
					editorElem.PushAttribute("displayName", nodeData.displayName.c_str());
				editorElem.PushAttribute("zKey", nodeData.zKey);
				if (nodeData.isFolded)
					editorElem.PushAttribute("folded", nodeData.isFolded);
			}
			editorElem.CloseElement();
		}
	}
	editorElem.CloseElement();
	
	editorElem.OpenElement("visualizers");
	{
		for (auto & visualizerItr : visualizers)
		{
			auto & visualizer = visualizerItr.second;
			
			editorElem.OpenElement("visualizer");
			{
				editorElem.PushAttribute("id", visualizer.id);
				editorElem.PushAttribute("x", visualizer.x);
				editorElem.PushAttribute("y", visualizer.y);
				editorElem.PushAttribute("zKey", visualizer.zKey);
				
				editorElem.PushAttribute("nodeId", visualizer.nodeId);
				if (!visualizer.srcSocketName.empty())
					editorElem.PushAttribute("srcSocketName", visualizer.srcSocketName.c_str());
				if (!visualizer.dstSocketName.empty())
					editorElem.PushAttribute("dstSocketName", visualizer.dstSocketName.c_str());
				editorElem.PushAttribute("sx", visualizer.sx);
				editorElem.PushAttribute("sy", visualizer.sy);
			}
			editorElem.CloseElement();
		}
	}
	editorElem.CloseElement();
	
	editorElem.OpenElement("editorOptions");
	{
		editorElem.PushAttribute("comment", editorOptions.comment.c_str());
		editorElem.PushAttribute("menuIsVisible", editorOptions.menuIsVisible);
		editorElem.PushAttribute("realTimePreview", editorOptions.realTimePreview);
		editorElem.PushAttribute("autoHideUi", editorOptions.autoHideUi);
		editorElem.PushAttribute("showBackground", editorOptions.showBackground);
		editorElem.PushAttribute("showGrid", editorOptions.showGrid);
		editorElem.PushAttribute("snapToGrid", editorOptions.snapToGrid);
		editorElem.PushAttribute("showOneShotActivity", editorOptions.showOneShotActivity);
		editorElem.PushAttribute("showContinuousActivity", editorOptions.showContinuousActivity);
		editorElem.PushAttribute("showCpuHeat", editorOptions.showCpuHeat);
		
		const std::string backgroundColor = toColor(editorOptions.backgroundColor).toHexString(true);
		const std::string gridColor = toColor(editorOptions.gridColor).toHexString(true);
		
		editorElem.PushAttribute("backgroundColor", backgroundColor.c_str());
		editorElem.PushAttribute("gridColor", gridColor.c_str());
		
		if (editorOptions.cpuHeatColors.numKeys > 0)
		{
			editorElem.OpenElement("cpuHeatColors");
			{
				editorOptions.cpuHeatColors.save(&editorElem);
			}
			editorElem.CloseElement();
		}
	}
	editorElem.CloseElement();
	
	return true;
}

void GraphEdit::nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
{
	Assert(tryGetNode(nodeId) != nullptr);
	
	// allocate nodeData
	nodeDatas[nodeId];
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->nodeAdd(nodeId, typeName);
	}
}

void GraphEdit::nodeRemove(const GraphNodeId nodeId)
{
	Assert(tryGetNode(nodeId) != nullptr);
	Assert(tryGetNodeData(nodeId) != nullptr);
	
	if (selectedNodes.count(nodeId) != 0)
		selectedNodes.erase(nodeId);
	
	nodeDatas.erase(nodeId);
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->nodeRemove(nodeId);
	}
	
	// remove visualizers referenced by node
	
	for (auto i = visualizers.begin(); i != visualizers.end(); )
	{
		auto & visualizer = i->second;
		
		if (visualizer.nodeId == nodeId)
			i = visualizers.erase(i);
		else
			++i;
	}
}

void GraphEdit::linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	Assert(tryGetNode(srcNodeId) != nullptr);
	Assert(tryGetNode(dstNodeId) != nullptr);
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
}

void GraphEdit::linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	Assert(tryGetNode(srcNodeId) != nullptr);
	Assert(tryGetNode(dstNodeId) != nullptr);
	
	if (selectedLinks.count(linkId) != 0)
		selectedLinks.erase(linkId);
	
	for (auto i = selectedLinkRoutePoints.begin(); i != selectedLinkRoutePoints.end(); )
	{
		auto routePoint = *i;
		
		if (routePoint->linkId == linkId)
			i = selectedLinkRoutePoints.erase(i);
		else
			++i;
	}
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
		
		//
		
		auto srcNode = tryGetNode(srcNodeId);
		Assert(srcNode != nullptr);
		
		auto srcSocket = tryGetInputSocket(srcNodeId, srcSocketIndex);
		Assert(srcSocket != nullptr);
		
		if (srcNode != nullptr && srcSocket != nullptr)
		{
			auto valueText = srcNode->inputValues.find(srcSocket->name);
			if (valueText != srcNode->inputValues.end())
				realTimeConnection->setSrcSocketValue(srcNodeId, srcSocket->index, srcSocket->name, valueText->second);
		}
	}
}

//

GraphUi::PropEdit::PropEdit(GraphEdit_TypeDefinitionLibrary * _typeLibrary, GraphEdit * _graphEdit)
	: typeLibrary(nullptr)
	, graphEdit(_graphEdit)
	, graph(nullptr)
	, nodeId(kGraphNodeIdInvalid)
	, currentNodeId(kGraphNodeIdInvalid)
	, uiColors(nullptr)
{
	typeLibrary = _typeLibrary;
	
	uiColors = new ParticleColor[kMaxUiColors];
}

GraphUi::PropEdit::~PropEdit()
{
	delete[] uiColors;
	uiColors = nullptr;
}

void GraphUi::PropEdit::setGraph(Graph * _graph)
{
	graph = _graph;
}

void GraphUi::PropEdit::setNode(const GraphNodeId _nodeId)
{
	if (_nodeId != nodeId)
	{
		//logDebug("setNode: %d", _nodeId);
		
		nodeId = _nodeId;
	}
}

static bool doMenuItem(const GraphEdit & graphEdit, std::string & valueText, const std::string & name, const std::string & defaultValue, const std::string & editor, const float dt, const int index, ParticleColor * uiColors, const int maxUiColors, const GraphEdit_EnumDefinition * enumDefinition, bool & pressed)
{
	pressed = false;
	
	if (editor == "none")
	{
	}
	else if (editor == "textbox")
	{
		doTextBox(valueText, name.c_str(), dt);
		
		return valueText != defaultValue;
	}
	else if (editor == "textbox_int")
	{
		int value = Parse::Int32(valueText);
		
		auto result = doTextBox(value, name.c_str(), dt);
		
		valueText = String::ToString(value);
		
		return (value != Parse::Int32(defaultValue)) && result != kUiTextboxResult_EditingCompleteCleared;
	}
	else if (editor == "textbox_float")
	{
		float value = Parse::Float(valueText);
		
		auto result = doTextBox(value, name.c_str(), dt);
		
		valueText = String::FormatC("%f", value);
		
		return (value != Parse::Float(defaultValue)) && result != kUiTextboxResult_EditingCompleteCleared;
	}
	else if (editor == "checkbox")
	{
		bool value = Parse::Bool(valueText);
		
		doCheckBox(value, name.c_str(), false);
		
		valueText = value ? "1" : "0";
		
		return value != Parse::Bool(defaultValue);
	}
	else if (editor == "enum")
	{
		Assert(enumDefinition != nullptr);
		
		int value = 0;
		
		std::vector<EnumValue> enumValues;
		
		for (auto & elem : enumDefinition->enumElems)
		{
			const int index = enumValues.size();
			
			if (elem.valueText == valueText)
				value = index;
			
			enumValues.push_back(EnumValue(index, elem.name));
		}
		
		doDropdown(value, name.c_str(), enumValues);
		
		if (value < 0 || value >= enumDefinition->enumElems.size())
			valueText.clear();
		else
			valueText = enumDefinition->enumElems[value].valueText;
		
		return valueText != defaultValue;
	}
	else if (editor == "colorpicker")
	{
		if (index < maxUiColors)
		{
			ParticleColor & particleColor = uiColors[index];
			
			if (g_uiState->activeColor != &particleColor)
			{
				Color color = Color::fromHex(valueText.c_str());
				particleColor = toParticleColor(color);
			}
			
			doParticleColor(particleColor, name.c_str());
			
			Color color = toColor(particleColor);
			valueText = color.toHexString(true);
			
			return color.toRGBA() != color.fromHex(defaultValue.c_str()).toRGBA();
		}
		
		return false;
	}
	else if (editor == "button")
	{
		if (doButton(name.c_str()))
		{
			pressed = true;
		}
		
		return false;
	}
	else
	{
		Assert(false);
	}
	
	return false;
}

void GraphUi::PropEdit::doMenus(UiState * uiState, const float dt)
{
	if (!graphEdit->enabled(GraphEdit::kFlag_NodeProperties) || nodeId == kGraphNodeIdInvalid)
	{
		if (g_doActions)
		{
			pushMenu("propEdit");
			g_menu->reset();
			popMenu();
		}
		
		return;
	}
	
	const int kPadding = 10;
	
	g_drawX = GRAPHEDIT_SX - uiState->sx - kPadding;
	g_drawY = kPadding;
	
	uiState->textBoxTextOffset = 80;
	
	pushMenu("propEdit");
	
	if (nodeId != currentNodeId)
	{
		currentNodeId = nodeId;
		
		g_menu->reset();
	}
	
	GraphNode * node = tryGetNode();
	GraphEdit::NodeData * nodeData = (node == nullptr) ? nullptr : graphEdit->tryGetNodeData(node->id);
	
	if (node != nullptr && nodeData != nullptr)
	{
		const GraphEdit_TypeDefinition * typeDefinition = typeLibrary->tryGetTypeDefinition(node->typeName);
		
		if (typeDefinition != nullptr)
		{
			std::string headerText = typeDefinition->typeName;
			
			if (!typeDefinition->displayName.empty())
				headerText = typeDefinition->displayName;
			
			doLabel(headerText.c_str(), 0.f);
			
			doTextBox(nodeData->displayName, "display name", dt);
			
			int menuItemIndex = 0;
			
			auto & inputSockets = getInputSockets(*typeDefinition, *nodeData);
			
			for (auto & inputSocket : inputSockets)
			{
				auto valueTextItr = node->inputValues.find(inputSocket.name);
				
				const bool isPreExisting = valueTextItr != node->inputValues.end();
				
				std::string oldValueText;
				
				if (isPreExisting)
					oldValueText = valueTextItr->second;
				else
					oldValueText = inputSocket.defaultValue;
				
				std::string newValueText = oldValueText;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(inputSocket.typeName);
				
				const GraphEdit_EnumDefinition * enumDefinition = typeLibrary->tryGetEnumDefinition(inputSocket.enumName);
				
				bool pressed = false;
				
				const bool hasValue = doMenuItem(*graphEdit, newValueText,
					inputSocket.displayName.empty() ? inputSocket.name : inputSocket.displayName,
					inputSocket.defaultValue,
					enumDefinition != nullptr
					? "enum"
					: valueTypeDefinition != nullptr
					? valueTypeDefinition->editor
					: "textbox",
					dt, menuItemIndex, uiColors, kMaxUiColors, enumDefinition, pressed);
				
				menuItemIndex++;
				
				if (isPreExisting)
				{
					if (!hasValue)
					{
						node->inputValues.erase(inputSocket.name);
						
						if (graphEdit->realTimeConnection)
						{
							graphEdit->realTimeConnection->clearSrcSocketValue(nodeId, inputSocket.index, inputSocket.name);
						}
					}
					else
						valueTextItr->second = newValueText;
				}
				else
				{
					if (hasValue)
						node->inputValues[inputSocket.name] = newValueText;
				}
				
				if (graphEdit->realTimeConnection)
				{
					if (newValueText != oldValueText && hasValue)
					{
						graphEdit->realTimeConnection->setSrcSocketValue(nodeId, inputSocket.index, inputSocket.name, newValueText);
					}
					
					if (pressed)
					{
						graphEdit->realTimeConnection->handleSrcSocketPressed(nodeId, inputSocket.index, inputSocket.name);
					}
				}
			}
			
			auto & outputSockets = getOutputSockets(*typeDefinition, *nodeData);
			
			for (auto & outputSocket : outputSockets)
			{
				if (!outputSocket.isEditable)
					continue;
				
				std::string & newValueText = node->editorValue;
				
				const std::string oldValueText = newValueText;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(outputSocket.typeName);
				
				bool pressed = false;
				
				const bool hasValue = doMenuItem(*graphEdit, newValueText,
					outputSocket.displayName.empty() ? outputSocket.name : outputSocket.displayName,
					"",
					valueTypeDefinition != nullptr
					? valueTypeDefinition->editor
					: "textbox",
					dt, menuItemIndex, uiColors, kMaxUiColors, nullptr, pressed);
				
				if (graphEdit->realTimeConnection)
				{
					if (newValueText != oldValueText && !(!hasValue && oldValueText.empty()))
					{
						graphEdit->realTimeConnection->setDstSocketValue(nodeId, outputSocket.index, outputSocket.name, newValueText);
					}
					
					if (pressed)
					{
						graphEdit->realTimeConnection->handleDstSocketPressed(nodeId, outputSocket.index, outputSocket.name);
					}
				}
				
				menuItemIndex++;
			}
		}
	}
	
	if (uiState->activeColor)
		doColorWheel(*uiState->activeColor, "__colorPicker", dt);
	if (uiState->activeElem == nullptr)
		uiState->activeColor = nullptr;
	
	popMenu();
}

GraphNode * GraphUi::PropEdit::tryGetNode()
{
	if (graph == nullptr)
		return nullptr;
	else
		return graph->tryGetNode(nodeId);
}

//

GraphUi::NodeTypeNameSelect::NodeTypeNameSelect(GraphEdit * _graphEdit)
	: graphEdit(_graphEdit)
	, typeName()
	, showSuggestions(false)
	, history()
{
}

GraphUi::NodeTypeNameSelect::~NodeTypeNameSelect()
{
}

static uint32_t fuzzyStringDistance(const std::string & s1, const std::string & s2)
{
	const uint32_t len1 = s1.size();
	const uint32_t len2 = s2.size();
	
	uint32_t ** d = (uint32_t**)alloca(sizeof(uint32_t*) * (len1 + 1));
	for (uint32_t i = 0; i < len1 + 1; ++i)
		d[i] = (uint32_t*)alloca(4 * (len2 + 1));
	
	if (len1 >= 1 && len2 >= 1)
		d[0][0] = 0;
	
	for (uint32_t i = 1; i <= len1; ++i)
		d[i][0] = i;
	for (uint32_t i = 1; i <= len2; ++i)
		d[0][i] = i;

	for (uint32_t i = 1; i <= len1; ++i)
	{
		for (uint32_t j = 1; j <= len2; ++j)
		{
			// note that std::min({arg1, arg2, arg3}) works only in C++11,
			// for C++98 use std::min(std::min(arg1, arg2), arg3)
			d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 20) });
		}
	}
	
	return d[len1][len2];
}

static uint32_t literalStringDistance(const std::string & s1, const std::string & s2)
{
	const uint32_t len1 = s1.size();
	const uint32_t len2 = s2.size();

	const uint32_t len = std::min(len1, len2);

	if (len == 0)
		return UINT32_MAX;

	bool equal = true;

	for (uint32_t i = 0; i < len; ++i)
		equal &= s1[i] == s2[i];

	if (equal == false)
		return UINT32_MAX;

	const uint32_t score = len2 > len1 ? len2 - len1 : len1 - len2;
	
	return score;
}

struct TypeNameAndScore
{
	std::string typeName;
	uint32_t score;
	
	bool operator<(const TypeNameAndScore & other) const
	{
		if (score != other.score)
			return score < other.score;
		else
			return typeName < other.typeName;
	}
};

void calculateTypeNamesAndScores(const std::string & typeName, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary, std::vector<TypeNameAndScore> & typeNamesAndScores)
{
	int index = 0;
	
	for (auto & typeDefinitionItr : typeDefinitionLibrary->typeDefinitions)
	{
		auto & typeDefinition = typeDefinitionItr.second;
		
		const uint32_t fuzzyScore =
			typeDefinition.typeName[0] == typeName[0]
			? 1000 + fuzzyStringDistance(typeDefinition.typeName, typeName)
			: UINT32_MAX;
		
		const uint32_t literalScore = literalStringDistance(
			typeDefinition.displayName.empty()
			? typeDefinition.typeName
			: typeDefinition.displayName,
			typeName);
		
		const uint32_t score = std::min(fuzzyScore, literalScore);
		
		typeNamesAndScores.resize(index + 1);
		typeNamesAndScores[index].typeName = typeDefinition.typeName;
		typeNamesAndScores[index].score = score;
		
		++index;
	}
	
	std::sort(typeNamesAndScores.begin(), typeNamesAndScores.end());
}

void GraphUi::NodeTypeNameSelect::doMenus(UiState * uiState, const float dt)
{
	if (!graphEdit->enabled(GraphEdit::kFlag_NodeAdd))
	{
		if (g_doActions)
		{
			typeName.clear();
			showSuggestions = false;
		}
		
		return;
	}
	
	pushMenu("nodeTypeSelect");
	{
		doLabel("insert", 0.f);
		
		if (doTextBox(typeName, "type", dt))
		{
			typeName = findClosestMatch(typeName);
			selectTypeName(typeName);
			
			uiState->reset();
			showSuggestions = false;
		}
		
		if (!typeName.empty() && showSuggestions)
		{
			// list recommendations
			
			std::vector<TypeNameAndScore> typeNamesAndScores;
			
			calculateTypeNamesAndScores(typeName, graphEdit->typeDefinitionLibrary, typeNamesAndScores);
			
			const int count = std::min(5, int(typeNamesAndScores.size()));
			
			doBreak();
			for (int i = 0; i < count; ++i)
			{
				char name[32];
				sprintf_s(name, sizeof(name), "f%02d", i);
				pushMenu(name);
				{
					if (doButton(typeNamesAndScores[i].typeName.c_str()))
					{
						selectTypeName(typeNamesAndScores[i].typeName);
						
						uiState->reset();
						showSuggestions = false;
					}
				}
				popMenu();
			}
			
			if (!history.empty())
			{
				doBreak();
				int index = 0;
				auto historyCopy = history;
				for (auto & historyItem : historyCopy)
				{
					char name[32];
					sprintf_s(name, sizeof(name), "m%02d", index);
					pushMenu(name);
					{
						if (doButton(historyItem.c_str()))
						{
							uiState->reset();
							selectTypeName(historyItem);
						}
					}
					popMenu();
					
					++index;
				}
			}
		}
		
		doBreak();
		
		if (g_doActions)
		{
			if (g_menu->getElem("type").clicked)
				showSuggestions = true;
			if (g_uiState->activeElem == nullptr)
				showSuggestions = false;
		}
	}
	popMenu();
}

std::string GraphUi::NodeTypeNameSelect::findClosestMatch(const std::string & typeName) const
{
	std::vector<TypeNameAndScore> typeNamesAndScores;
			
	calculateTypeNamesAndScores(typeName, graphEdit->typeDefinitionLibrary, typeNamesAndScores);
	
	if (typeNamesAndScores.empty())
		return typeName;
	else
		return typeNamesAndScores.front().typeName;
}

void GraphUi::NodeTypeNameSelect::selectTypeName(const std::string & _typeName)
{
	typeName = _typeName;
	
	if (graphEdit->tryAddNode(typeName, graphEdit->dragAndZoom.focusX, graphEdit->dragAndZoom.focusY, true, nullptr))
	{
		// update history
		
		addToHistory(typeName);
	}
}

void GraphUi::NodeTypeNameSelect::addToHistory(const std::string & typeName)
{
	for (auto i = history.begin(); i != history.end(); )
	{
		if ((*i) == typeName)
			i = history.erase(i);
		else
			++i;
	}

	history.push_front(typeName);

	while (history.size() > kMaxHistory)
		history.pop_back();
}

std::string & GraphUi::NodeTypeNameSelect::getNodeTypeName()
{
	return typeName;
}
