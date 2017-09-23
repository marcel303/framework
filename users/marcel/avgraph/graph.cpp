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
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <algorithm>
#include <cmath>

#include "vfxProfiling.h" // fixme : remove !!

extern const int GFX_SX; // fixme : make property of graph editor
extern const int GFX_SY;

using namespace tinyxml2;

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
		
		const double dTreshold = cr;
		const double d = std::abs(cx * nx + cy * ny - nd);
		
		//printf("d = %f / %f\n", float(d), float(dTreshold));
		
		if (d <= dTreshold)
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

GraphNode::EditorVisualizer::EditorVisualizer()
	: nodeId(kGraphNodeIdInvalid)
	, srcSocketName()
	, srcSocketIndex(-1)
	, dstSocketName()
	, dstSocketIndex(-1)
	, visualizer(nullptr)
	, sx(250)
	, sy(250)
{
}

GraphNode::EditorVisualizer::EditorVisualizer(const EditorVisualizer & other)
	: nodeId(kGraphNodeIdInvalid)
	, srcSocketName()
	, srcSocketIndex(-1)
	, dstSocketName()
	, dstSocketIndex(-1)
	, visualizer(nullptr)
	, sx(0)
	, sy(0)
{
	*this = other;
}

GraphNode::EditorVisualizer::~EditorVisualizer()
{
	delete visualizer;
	visualizer = nullptr;
}

void GraphNode::EditorVisualizer::allocVisualizer()
{
	delete visualizer;
	visualizer = nullptr;
	
	if (nodeId != kGraphNodeIdInvalid)
	{
		visualizer = new GraphEdit_Visualizer();
		
		visualizer->init(
			nodeId,
			srcSocketName,
			srcSocketIndex,
			dstSocketName,
			dstSocketIndex);
	}
}

void GraphNode::EditorVisualizer::tick(const GraphEdit & graphEdit)
{
	visualizer->tick(graphEdit);
	
	updateSize(graphEdit);
}

void GraphNode::EditorVisualizer::updateSize(const GraphEdit & graphEdit)
{
	if (sx == 0 || sy == 0)
	{
		if (visualizer->hasValue)
		{
			int sxi = 0;
			int syi = 0;
			
			visualizer->measure(graphEdit, "measure",
				GraphEdit_Visualizer::kDefaultGraphSx, GraphEdit_Visualizer::kDefaultGraphSy,
				GraphEdit_Visualizer::kDefaultMaxTextureSx, GraphEdit_Visualizer::kDefaultMaxTextureSy,
				GraphEdit_Visualizer::kDefaultChannelsSx, GraphEdit_Visualizer::kDefaultChannelsSy,
				sxi, syi);
			
			sx = sxi;
			sy = syi;
		}
	}
}

void GraphNode::EditorVisualizer::operator=(const EditorVisualizer & other)
{
	nodeId = other.nodeId;
	srcSocketName = other.srcSocketName;
	srcSocketIndex = other.srcSocketIndex;
	dstSocketName = other.dstSocketName;
	dstSocketIndex = other.dstSocketIndex;
	sx = other.sx;
	sy = other.sy;
	
	allocVisualizer();
}

//

GraphNode::GraphNode()
	: id(kGraphNodeIdInvalid)
	, nodeType(kGraphNodeType_Regular)
	, typeName()
	, isEnabled(true)
	, isPassthrough(false)
	, resources()
	, editorName()
	, editorX(0.f)
	, editorY(0.f)
	, editorZKey(0)
	, editorIsFolded(false)
	, editorFoldAnimTime(0.f)
	, editorFoldAnimTimeRcp(0.f)
	, editorInputValues()
	, editorValue()
	, editorIsActiveAnimTime(0.f)
	, editorIsActiveAnimTimeRcp(0.f)
	, editorIsActiveContinuous(false)
	, editorVisualizer()
{
}

void GraphNode::tick(const GraphEdit & graphEdit, const float dt)
{
	editorFoldAnimTime = Calc::Max(0.f, editorFoldAnimTime - dt);
	editorIsActiveAnimTime = Calc::Max(0.f, editorIsActiveAnimTime - dt);
	
	if (nodeType == kGraphNodeType_Visualizer)
	{
		editorVisualizer.tick(graphEdit);
	}
}

const std::string & GraphNode::getDisplayName() const
{
	return !editorName.empty() ? editorName : typeName;
}

void GraphNode::setIsEnabled(const bool _isEnabled)
{
	isEnabled = _isEnabled;
}

void GraphNode::setIsPassthrough(const bool _isPassthrough)
{
	isPassthrough = _isPassthrough;
}

void GraphNode::setIsFolded(const bool isFolded)
{
	if (editorIsFolded == isFolded)
		return;
	
	editorIsFolded = isFolded;
	editorFoldAnimTime = isFolded ? .1f : .07f;
	editorFoldAnimTimeRcp = 1.f / editorFoldAnimTime;
}

void GraphNode::setVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex)
{
	nodeType = kGraphNodeType_Visualizer;
	
	editorVisualizer.nodeId = nodeId;
	editorVisualizer.srcSocketName = srcSocketName;
	editorVisualizer.srcSocketIndex = srcSocketIndex;
	editorVisualizer.dstSocketName = dstSocketName;
	editorVisualizer.dstSocketIndex = dstSocketIndex;
	editorVisualizer.allocVisualizer();
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

GraphNodeSocketLink::GraphNodeSocketLink()
	: id(kGraphLinkIdInvalid)
	, isEnabled(true)
	, srcNodeId(kGraphNodeIdInvalid)
	, srcNodeSocketName()
	, srcNodeSocketIndex(-1)
	, dstNodeId(kGraphNodeIdInvalid)
	, dstNodeSocketName()
	, dstNodeSocketIndex(-1)
	, params()
{
}

void GraphNodeSocketLink::setIsEnabled(const bool _isEnabled)
{
	isEnabled = _isEnabled;
}

//

Graph::Graph()
	: nodes()
	, links()
	, nextNodeId(1)
	, nextLinkId(1)
	, nextZKey(1)
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

int Graph::allocZKey()
{
	return nextZKey++;
}

void Graph::addNode(GraphNode & node)
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

void Graph::addLink(const GraphNodeSocketLink & link, const bool clearInputDuplicates)
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

GraphNodeSocketLink * Graph::tryGetLink(const GraphLinkId linkId)
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
	nextZKey = intAttrib(xmlGraph, "nextZKey", nextZKey);
	
	for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
	{
		GraphNode node;
		node.id = intAttrib(xmlNode, "id", node.id);
		node.nodeType = (GraphNodeType)intAttrib(xmlNode, "nodeType", node.nodeType);
		node.typeName = stringAttrib(xmlNode, "typeName", node.typeName.c_str());
		node.isEnabled = boolAttrib(xmlNode, "enabled", node.isEnabled);
		node.isPassthrough = boolAttrib(xmlNode, "passthrough", node.isPassthrough);
		node.editorName = stringAttrib(xmlNode, "editorName", node.editorName.c_str());
		node.editorX = floatAttrib(xmlNode, "editorX", node.editorX);
		node.editorY = floatAttrib(xmlNode, "editorY", node.editorY);
		node.editorIsFolded = boolAttrib(xmlNode, "folded", node.editorIsFolded);
		node.editorValue = stringAttrib(xmlNode, "editorValue", node.editorValue.c_str());
		
		if (node.nodeType == kGraphNodeType_Visualizer)
		{
			const XMLElement * xmlVisualizer = xmlNode->FirstChildElement("visualizer");
			Assert(xmlVisualizer != nullptr);
			
			if (xmlVisualizer != nullptr)
			{
				node.editorVisualizer.nodeId = intAttrib(xmlVisualizer, "nodeId", node.editorVisualizer.nodeId);
				node.editorVisualizer.srcSocketName = stringAttrib(xmlVisualizer, "srcSocketName", node.editorVisualizer.srcSocketName.c_str());
				node.editorVisualizer.dstSocketName = stringAttrib(xmlVisualizer, "dstSocketName", node.editorVisualizer.dstSocketName.c_str());
				node.editorVisualizer.sx = floatAttrib(xmlVisualizer, "sx", node.editorVisualizer.sx);
				node.editorVisualizer.sy = floatAttrib(xmlVisualizer, "sy", node.editorVisualizer.sy);
			}
		}
		
		for (const XMLElement * xmlInput = xmlNode->FirstChildElement("input"); xmlInput != nullptr; xmlInput = xmlInput->NextSiblingElement("input"))
		{
			const std::string socket = stringAttrib(xmlInput, "socket", "");
			const std::string value = stringAttrib(xmlInput, "value", "");
			
			node.editorInputValues[socket] = value;
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
		nextZKey = std::max(nextZKey, node.editorZKey + 1);
	}
	
	for (const XMLElement * xmlLink = xmlGraph->FirstChildElement("link"); xmlLink != nullptr; xmlLink = xmlLink->NextSiblingElement("link"))
	{
		GraphNodeSocketLink link;
		link.id = intAttrib(xmlLink, "id", link.id);
		link.isEnabled = boolAttrib(xmlLink, "enabled", link.isEnabled);
		link.srcNodeId = intAttrib(xmlLink, "srcNodeId", link.srcNodeId);
		link.srcNodeSocketName = stringAttrib(xmlLink, "srcNodeSocketName", link.srcNodeSocketName.c_str());
		link.dstNodeId = intAttrib(xmlLink, "dstNodeId", link.dstNodeId);
		link.dstNodeSocketName = stringAttrib(xmlLink, "dstNodeSocketName", link.dstNodeSocketName.c_str());
		
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
		
		if (link.srcNodeSocketIndex == -1)
		{
			printf("failed to find srcSocketIndex. srcNodeId=%d, srcSocketName=%s\n", link.srcNodeId, link.srcNodeSocketName.c_str());
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
		
		if (link.dstNodeSocketIndex == -1)
		{
			printf("failed to find dstSocketIndex. dstNodeId=%d, dstSocketName=%s\n", link.dstNodeId, link.dstNodeSocketName.c_str());
		}
	}
	
	for (auto & nodeItr : nodes)
	{
		auto & node = nodeItr.second;
		
		if (node.nodeType == kGraphNodeType_Visualizer)
		{
			auto linkedNode = tryGetNode(node.editorVisualizer.nodeId);
			Assert(linkedNode != nullptr);
			
			if (linkedNode != nullptr)
			{
				auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(linkedNode->typeName);
				Assert(typeDefinition != nullptr);
				
				if (typeDefinition != nullptr)
				{
					if (!node.editorVisualizer.srcSocketName.empty())
					{
						for (auto & inputSocket : typeDefinition->inputSockets)
						{
							if (inputSocket.name == node.editorVisualizer.srcSocketName)
							{
								node.editorVisualizer.srcSocketIndex = inputSocket.index;
								break;
							}
						}
					}
					
					if (!node.editorVisualizer.dstSocketName.empty())
					{
						for (auto & outputSocket : typeDefinition->outputSockets)
						{
							if (outputSocket.name == node.editorVisualizer.dstSocketName)
							{
								node.editorVisualizer.dstSocketIndex = outputSocket.index;
								break;
							}
						}
					}
				}
			}
			
			node.editorVisualizer.allocVisualizer();
		}
	}

	return true;
}

bool Graph::saveXml(XMLPrinter & xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary) const
{
	bool result = true;
	
	xmlGraph.PushAttribute("nextNodeId", nextNodeId);
	xmlGraph.PushAttribute("nextLinkId", nextLinkId);
	xmlGraph.PushAttribute("nextZKey", nextZKey);
	
	for (auto & nodeItr : nodes)
	{
		auto & node = nodeItr.second;
		
		xmlGraph.OpenElement("node");
		{
			xmlGraph.PushAttribute("id", node.id);
			
			if (node.nodeType != kGraphNodeType_Regular)
				xmlGraph.PushAttribute("nodeType", node.nodeType);
			
			xmlGraph.PushAttribute("typeName", node.typeName.c_str());
			
			if (!node.isEnabled)
				xmlGraph.PushAttribute("enabled", node.isEnabled);
			if (node.isPassthrough)
				xmlGraph.PushAttribute("passthrough", node.isPassthrough);
			
			if (!node.editorName.empty())
				xmlGraph.PushAttribute("editorName", node.editorName.c_str());
			
			xmlGraph.PushAttribute("editorX", node.editorX);
			xmlGraph.PushAttribute("editorY", node.editorY);
			
			if (node.editorIsFolded)
				xmlGraph.PushAttribute("folded", node.editorIsFolded);
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
			
			if (node.nodeType == kGraphNodeType_Visualizer)
			{
				xmlGraph.OpenElement("visualizer");
				{
					xmlGraph.PushAttribute("nodeId", node.editorVisualizer.nodeId);
					if (!node.editorVisualizer.srcSocketName.empty())
						xmlGraph.PushAttribute("srcSocketName", node.editorVisualizer.srcSocketName.c_str());
					if (!node.editorVisualizer.dstSocketName.empty())
						xmlGraph.PushAttribute("dstSocketName", node.editorVisualizer.dstSocketName.c_str());
					xmlGraph.PushAttribute("sx", node.editorVisualizer.sx);
					xmlGraph.PushAttribute("sy", node.editorVisualizer.sy);
				}
				xmlGraph.CloseElement();
			}
			
			for (auto input : node.editorInputValues)
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
			xmlGraph.PushAttribute("enabled", link.isEnabled);
			xmlGraph.PushAttribute("srcNodeId", link.srcNodeId);
			xmlGraph.PushAttribute("srcNodeSocketName", link.srcNodeSocketName.c_str());
			xmlGraph.PushAttribute("dstNodeId", link.dstNodeId);
			xmlGraph.PushAttribute("dstNodeSocketName", link.dstNodeSocketName.c_str());
			
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
		}
		xmlGraph.CloseElement();
	}
	
	return result;
}

//

#include "framework.h"
#include "../libparticle/particle.h"
#include "../libparticle/ui.h"

static bool selectionMod()
{
	return keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
}

static bool commandMod()
{
	return keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);
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
		
		elem.value = value;
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
	float py = 0.f;
	float pf = 0.f;
	
	py += 5.f;
	pf += 5.f;
	
	// (typeName label)
	
	py += 15.f;
	pf += 15.f;
	
	// (sockets)
	
	const float socketPaddingY = 20.f;
	const float socketRadius = 6.f;
	const float socketPyBegin = py;
	
	// setup input sockets
	
	{
		int index = 0;
		float px = 0.f;
		float socketPy = socketPyBegin;
		
		for (auto & inputSocket : inputSockets)
		{
			inputSocket.index = index;
			inputSocket.px = px;
			inputSocket.py = socketPy + socketPaddingY / 2.f;
			inputSocket.radius = socketRadius;
			
			++index;
			socketPy += socketPaddingY;
			py = std::max(py, socketPy);
		}
	}
	
	// setup output sockets
	
	{
		int index = 0;
		float px = 100.f;
		float socketPy = socketPyBegin;
		
		for (auto & outputSocket : outputSockets)
		{
			outputSocket.index = index;
			outputSocket.px = px;
			outputSocket.py = socketPy + socketPaddingY / 2.f;
			outputSocket.radius = socketRadius;
			
			++index;
			socketPy += socketPaddingY;
			py = std::max(py, socketPy);
		}
	}
	
	//
	
	py += 5.f;
	pf += 5.f;
	
	//
	
	sx = 100.f;
	sy = py;
	syFolded = pf;
}

bool GraphEdit_TypeDefinition::hitTest(const float x, const float y, const bool isFolded, HitTestResult & result) const
{
	result = HitTestResult();
	
	if (isFolded == false)
	{
		for (auto & inputSocket : inputSockets)
		{
			const float dx = x - inputSocket.px;
			const float dy = y - inputSocket.py;
			const float ds = std::hypotf(dx, dy);
			
			if (ds <= inputSocket.radius)
			{
				result.inputSocket = &inputSocket;
				return true;
			}
		}
		
		for (auto & outputSocket : outputSockets)
		{
			const float dx = x - outputSocket.px;
			const float dy = y - outputSocket.py;
			const float ds = std::hypotf(dx, dy);
			
			if (ds <= outputSocket.radius)
			{
				result.outputSocket = &outputSocket;
				return true;
			}
		}
	}
	
	if (x >= 0.f && y >= 0.f && x < sx && y < (isFolded ? syFolded : sy))
	{
		result.background = true;
		return true;
	}
	
	return false;
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
		socket.defaultValue = stringAttrib(xmlInput, "default", socket.defaultValue.c_str());
		
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

void GraphEdit_Visualizer::tick(const GraphEdit & graphEdit)
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
		setColor(63, 63, 127, 255);
	else
		setColor(31, 31, 31);
	drawRect(0, 0, sx, sy);
	
	if (isSelected)
		setColor(255, 255, 255);
	else
		setColor(191, 191, 191);
	drawRectLine(0, 0, sx, sy);
	
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
		const int xOffset = history.kMaxHistory - history.historySize;
		
		setColor(127, 127, 255);
		gxBegin(GL_QUADS);
		{
			for (int i = 0; i < history.historySize; ++i)
			{
				const float value = history.getGraphValue(i);
				
				const float plotX1 = graphX + (i + 0 + xOffset) * graphSx / history.maxHistorySize;
				const float plotX2 = graphX + (i + 1 + xOffset) * graphSx / history.maxHistorySize;
				const float plotY = (graphMin == graphMax) ? .5f : (value - graphMax) / (graphMin - graphMax);
				
				const int x1 = plotX1;
				const int y1 = y + graphSy;
				const int x2 = plotX2;
				const int y2 = y + plotY * graphSy;
				
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
		drawRectLine(graphX, y, graphX + graphSx, y + graphSy);
		
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
			glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ZERO, GL_ZERO);
			
			setColor(colorWhite);
			gxSetTexture(texture);
			{
				drawRect(textureX, y + textureY, textureX + textureSx, y + textureY + textureSy);
			}
			gxSetTexture(0);
			
			glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
		}
		
		setColor(colorWhite);
		drawRectLine(textureX, y + textureY, textureX + textureSx, y + textureY + textureSy);
		
		y += textureAreaSy;
	}
	
	//
	
	if (hasChannels)
	{
		const int kDataBorder = 2;
		
		y += kElemPadding;
		
		const int channelsDataSx = channelsSx - kDataBorder * 2;
		const int channelsDataSy = channelsSy - kDataBorder * 2;
		
		const int channelsEdgeX = (sx - channelsSx) / 2;
		const int channelsDataX = (sx - channelsDataSx) / 2;
		
		const int dataY = y + kDataBorder;
		
		float min = 0.f;
		float max = 0.f;
		
		bool isFirst = true;
		
		if (hasChannelDataMinMax)
		{
			isFirst = false;
			
			min = channelDataMin;
			max = channelDataMax;
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
					
					min = std::min(min, value);
					max = std::max(max, value);
				}
			}
		}
		
		if (min != max)
		{
			hasChannelDataMinMax = true;
			channelDataMin = min;
			channelDataMax = max;
		}
					
		setColor(127, 127, 255);
		
		const float strokeSize = 1.f * graphEdit.dragAndZoom.zoom;
		
		for (int c = 0; c < channelData.channels.size(); ++c)
		{
			auto & channel = channelData.channels[c];
			
			float lastX = 0.f;
			float lastY = 0.f;
			
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
						// connected lines between each sample
						
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
		
		setColor(colorWhite);
		drawRectLine(channelsEdgeX, y, channelsEdgeX + channelsSx, y + channelsSy);
		
		y += channelsSy;
	}
	
	//
	
	y += kPadding;
	//fassert(y == sy); // fixme
}

//

GraphEdit::GraphEdit(GraphEdit_TypeDefinitionLibrary * _typeDefinitionLibrary)
	: graph(nullptr)
	, typeDefinitionLibrary(nullptr)
	, typeDefinition_visualizer()
	, realTimeConnection(nullptr)
	, selectedNodes()
	, selectedLinks()
	, selectedLinkRoutePoints()
	, state(kState_Idle)
	, nodeSelect()
	, nodeDrag()
	, socketConnect()
	, nodeResize()
	, nodeDoubleClickTime(0.f)
	, touches()
	, mousePosition()
	, dragAndZoom()
	, realTimeSocketCapture()
	, documentInfo()
	, editorOptions()
	, propertyEditor(nullptr)
	, nodeTypeNameSelect(nullptr)
	, nodeEditor()
	, uiState(nullptr)
	, cursorHand(nullptr)
	, idleTime(0.f)
	, hideTime(1.f)
{
	graph = new Graph();
	
	graph->graphEditConnection = this;
	
	typeDefinitionLibrary = _typeDefinitionLibrary;
	
	typeDefinition_visualizer.inputSockets.resize(1);
	typeDefinition_visualizer.createUi();
	
	propertyEditor = new GraphUi::PropEdit(_typeDefinitionLibrary, this);
	
	nodeTypeNameSelect = new GraphUi::NodeTypeNameSelect(this);
	
	uiState = new UiState();
	
	const int kPadding = 10;
	uiState->sx = 200;
	uiState->x = kPadding;
	uiState->y = kPadding;
	uiState->textBoxTextOffset = 50;
	
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

GraphNodeSocketLink * GraphEdit::tryGetLink(const GraphLinkId id) const
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
	
	if (node == nullptr)
		return nullptr;
	
	const GraphEdit_TypeDefinition * typeDefinition = nullptr;
	
	if (node->nodeType == kGraphNodeType_Regular)
	{
		typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
	}
	else if (node->nodeType == kGraphNodeType_Visualizer)
	{
		typeDefinition = &typeDefinition_visualizer;
	}
	else
	{
		Assert(false);
	}
	
	if (typeDefinition == nullptr)
		return nullptr;
	if (socketIndex < 0 || socketIndex >= typeDefinition->inputSockets.size())
		return nullptr;
	return &typeDefinition->inputSockets[socketIndex];
}

const GraphEdit_TypeDefinition::OutputSocket * GraphEdit::tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const
{
	auto node = tryGetNode(nodeId);
	
	if (node == nullptr)
		return nullptr;
	auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
	if (typeDefinition == nullptr)
		return nullptr;
	if (socketIndex < 0 || socketIndex >= typeDefinition->outputSockets.size())
		return nullptr;
	return &typeDefinition->outputSockets[socketIndex];
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
	
	auto inputSocket = tryGetInputSocket(link.srcNodeId, link.srcNodeSocketIndex);
	auto outputSocket = tryGetOutputSocket(link.dstNodeId, link.dstNodeSocketIndex);
	
	if (srcNode == nullptr ||
		dstNode == nullptr ||
		inputSocket == nullptr ||
		outputSocket == nullptr)
	{
		return false;
	}
	else
	{
		auto srcTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
		auto dstTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
		
		const float srcSy = srcTypeDefinition == nullptr ? 0.f : srcTypeDefinition->syFolded;
		const float dstSy = dstTypeDefinition == nullptr ? 0.f : dstTypeDefinition->syFolded;
		
		const float srcX = srcNode->editorX + inputSocket->px;
		const float srcY = srcNode->editorY + (srcNode->editorIsFolded ? srcSy/2.f : inputSocket->py);
		const float dstX = dstNode->editorX + outputSocket->px;
		const float dstY = dstNode->editorY + (dstNode->editorIsFolded ? dstSy/2.f : outputSocket->py);
		
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

bool GraphEdit::hitTest(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	for (auto nodeItr = graph->nodes.rbegin(); nodeItr != graph->nodes.rend(); ++nodeItr)
	{
		auto & node = nodeItr->second;
		
		if (node.nodeType == kGraphNodeType_Regular)
		{
			const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
			
			if (typeDefinition == nullptr)
			{
				// todo : complain ?
			}
			else
			{
				GraphEdit_TypeDefinition::HitTestResult hitTestResult;
				
				if (typeDefinition->hitTest(x - node.editorX, y - node.editorY, node.editorIsFolded, hitTestResult))
				{
					result.hasNode = true;
					result.node = &node;
					result.nodeHitTestResult = hitTestResult;
					return true;
				}
			}
		}
		else if (node.nodeType == kGraphNodeType_Visualizer)
		{
			if (testRectOverlap(
				x, y,
				x, y,
				node.editorX,
				node.editorY,
				node.editorX + node.editorVisualizer.sx,
				node.editorY + node.editorVisualizer.sy))
			{
				GraphEdit_TypeDefinition::HitTestResult hitTestResult;
				
				const int borderSize = 6;
				
				if (testRectOverlap(
					x, y,
					x, y,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 0,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 0,
					node.editorX + borderSize * +1 + node.editorVisualizer.sx * 0,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 1))
				{
					hitTestResult.borderL = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					node.editorX + borderSize * -1 + node.editorVisualizer.sx * 1,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 0,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 1,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 1))
				{
					hitTestResult.borderR = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 0,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 0,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 1,
					node.editorY + borderSize * +1 + node.editorVisualizer.sy * 0))
				{
					hitTestResult.borderT = true;
				}
				
				if (testRectOverlap(
					x, y,
					x, y,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 0,
					node.editorY + borderSize * -1 + node.editorVisualizer.sy * 1,
					node.editorX + borderSize * +0 + node.editorVisualizer.sx * 1,
					node.editorY + borderSize * +0 + node.editorVisualizer.sy * 1))
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
				
				result.hasNode = true;
				result.node = &node;
				result.nodeHitTestResult = hitTestResult;
				
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
			
			if (testLineOverlap(x1, y1, x2, y2, x, y, 10.f))
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

bool GraphEdit::tick(const float dt, const bool _inputIsCaptured)
{
	cpuTimingBlock(GraphEdit_Tick);
	
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
				
				realTimeSocketCapture.visualizer.tick(*this);
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
				
				realTimeSocketCapture.visualizer.tick(*this);
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
	
	if (nodeEditor.editor != nullptr)
	{
		inputIsCaptured |= nodeEditor.editor->tick(dt, inputIsCaptured);
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
	
	if (inputIsCaptured == false)
	{
		inputIsCaptured |= propertyEditor->tick(dt);
	}
	
	makeActive(uiState, true, false);
	
	if (inputIsCaptured == false)
	{
		doMenu(dt);
		
		doLinkParams(dt);
		
		doEditorOptions(dt);
		
		nodeTypeNameSelect->doMenus(uiState, dt);
		
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
	
	SDL_Cursor * cursor = SDL_GetDefaultCursor();
	
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
					}
					
					if (hitTestResult.nodeHitTestResult.outputSocket)
					{
						highlightedSockets.dstNodeId = hitTestResult.node->id;
						highlightedSockets.dstNodeSocket = hitTestResult.nodeHitTestResult.outputSocket;
					}
					
					if (hitTestResult.nodeHitTestResult.borderL ||
						hitTestResult.nodeHitTestResult.borderR ||
						hitTestResult.nodeHitTestResult.borderT ||
						hitTestResult.nodeHitTestResult.borderB)
					{
						cursor = cursorHand;
					}
				}
				
				if (hitTestResult.hasLink)
				{
					highlightedLinks.insert(hitTestResult.link->id);
				}
				
				if (hitTestResult.hasLinkRoutePoint)
				{
					highlightedLinkRoutePoints.insert(hitTestResult.linkRoutePoint);
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
						if (hitTestResult.nodeHitTestResult.inputSocket)
						{
							socketConnect.srcNodeId = hitTestResult.node->id;
							socketConnect.srcNodeSocket = hitTestResult.nodeHitTestResult.inputSocket;
							
							state = kState_InputSocketConnect;
							break;
						}
						
						if (hitTestResult.nodeHitTestResult.outputSocket)
						{
							socketConnect.dstNodeId = hitTestResult.node->id;
							socketConnect.dstNodeSocket = hitTestResult.nodeHitTestResult.outputSocket;
							
							state = kState_OutputSocketConnect;
							break;
						}
						
						if (appendSelection == false)
						{
							// todo : implement the following bahvior: click + hold/move = drag, click + release = select single and discard the rest of the selection
							
							if (selectedNodes.count(hitTestResult.node->id) == 0)
							{
								selectNode(hitTestResult.node->id, true);
								
								hitTestResult.node->editorZKey = graph->allocZKey();
							}
						}
						else
						{
							if (selectedNodes.count(hitTestResult.node->id) == 0)
								selectNode(hitTestResult.node->id, false);
							else
								selectedNodes.erase(hitTestResult.node->id);
						}
						
						if (hitTestResult.nodeHitTestResult.borderL ||
							hitTestResult.nodeHitTestResult.borderR ||
							hitTestResult.nodeHitTestResult.borderT ||
							hitTestResult.nodeHitTestResult.borderB)
						{
							nodeResize.nodeId = hitTestResult.node->id;
							nodeResize.dragL = hitTestResult.nodeHitTestResult.borderL;
							nodeResize.dragR = hitTestResult.nodeHitTestResult.borderR;
							nodeResize.dragT = hitTestResult.nodeHitTestResult.borderT;
							nodeResize.dragB = hitTestResult.nodeHitTestResult.borderB;
							
							state = kState_NodeResize;
							break;
						}
						
						if (hitTestResult.nodeHitTestResult.background)
						{
							for (auto nodeId : selectedNodes)
							{
								auto node = tryGetNode(nodeId);
								
								if (node != nullptr)
								{
									auto & offset = nodeDrag.offsets[nodeId];
									
									offset[0] = mousePosition.x - node->editorX;
									offset[1] = mousePosition.y - node->editorY;
								}
							}
							
							if (appendSelection == false && nodeDoubleClickTime > 0.f)
							{
								nodeDoubleClickTime = 0.f;
								
								nodeEditBegin(hitTestResult.node->id);
								
								state = kState_NodeEdit;
								break;
							}
							else
							{
								nodeDoubleClickTime = .4f;
								
								state = kState_NodeDrag;
								break;
							}
						}
					}
					
					if (hitTestResult.hasLink)
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
					
					if (hitTestResult.hasLinkRoutePoint)
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
						
						state = kState_NodeDrag;
						break;
					}
				}
				else
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
						if (hitTestResult.nodeHitTestResult.inputSocket)
						{
							tryAddVisualizer(
								hitTestResult.node->id,
								hitTestResult.nodeHitTestResult.inputSocket->name,
								hitTestResult.nodeHitTestResult.inputSocket->index,
								String::Empty, -1,
								mousePosition.x, mousePosition.y, true);
						}
						
						if (hitTestResult.nodeHitTestResult.outputSocket)
						{
							tryAddVisualizer(
								hitTestResult.node->id,
								String::Empty, -1,
								hitTestResult.nodeHitTestResult.outputSocket->name,
								hitTestResult.nodeHitTestResult.outputSocket->index,
								mousePosition.x, mousePosition.y, true);
						}
					}
					
					if (hitTestResult.hasLink)
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
			}
			
			if (keyboard.wentDown(SDLK_i))
			{
				std::string typeName = nodeTypeNameSelect->getNodeTypeName();
				
				tryAddNode(typeName, mousePosition.x, mousePosition.x, true);
			}
			
			if (keyboard.wentDown(SDLK_a))
			{
				if (commandMod())
					selectAll();
				else
				{
					for (auto nodeId : selectedNodes)
					{
						auto node = tryGetNode(nodeId);
						
						if (node != nullptr)
						{
							snapToGrid(*node);
						}
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_o) || keyboard.wentDown(SDLK_0))
			{
				if (commandMod())
				{
					dragAndZoom.desiredZoom = 1.f;
					
					if (selectionMod())
					{
						dragAndZoom.desiredFocusX = 0.f;
						dragAndZoom.desiredFocusY = 0.f;
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_MINUS) && commandMod())
			{
				dragAndZoom.desiredZoom /= 1.5f;
			}
			
			if (keyboard.wentDown(SDLK_EQUALS) && commandMod())
			{
				dragAndZoom.desiredZoom *= 1.5f;
			}
			
			if (keyboard.wentDown(SDLK_d))
			{
				std::set<GraphNodeId> newSelectedNodes;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					Assert(node != nullptr);
					if (node != nullptr)
					{
						GraphNode newNode = *node;
						newNode.id = graph->allocNodeId();
						newNode.isEnabled = true;
						newNode.isPassthrough = false;
						newNode.editorX = node->editorX + kGridSize;
						newNode.editorY = node->editorY + kGridSize;
						newNode.editorZKey = graph->allocZKey();
						newNode.editorInputValues.clear();
						newNode.editorValue.clear();
						
						graph->addNode(newNode);
						
						newSelectedNodes.insert(newNode.id);
						
						if (commandMod())
						{
							auto newNodePtr = tryGetNode(newNode.id);
							Assert(newNodePtr != nullptr);
							
							// deep copy node including values
							
							auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(newNode.typeName);
							Assert(typeDefinition != nullptr);
							
							// let real-time editing know the socket values have changed
							for (auto & inputValueItr : node->editorInputValues)
							{
								auto & srcSocketName = inputValueItr.first;
								auto & srcSocketValue = inputValueItr.second;
								
								newNodePtr->editorInputValues[srcSocketName] = srcSocketValue;
								
								if (typeDefinition != nullptr)
								{
									for (auto & srcSocket : typeDefinition->inputSockets)
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
							
							// fixme : copy editor value too and let real-time connection know its value is changed
							//newNode.editorValue = node->editorValue;
						}
					}
				}
				
				if (!newSelectedNodes.empty())
				{
					selectedNodes = newSelectedNodes;
					selectedLinks.clear();
					selectedLinkRoutePoints.clear();
				}
			}
			
			if (keyboard.wentDown(SDLK_p))
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
						node->setIsPassthrough(allPassthrough ? false : true);
						
						if (realTimeConnection != nullptr)
						{
							if (node->nodeType == kGraphNodeType_Regular)
							{
								realTimeConnection->setNodeIsPassthrough(node->id, node->isPassthrough);
							}
						}
					}
				}
			}
			
			if (selectedNodes.empty() && selectedLinkRoutePoints.empty())
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
						auto node = tryGetNode(nodeId);
						
						Assert(node);
						if (node)
						{
							node->editorX += moveX;
							node->editorY += moveY;
						}
					}
					
					for (auto routePoint : selectedLinkRoutePoints)
					{
						routePoint->x += moveX;
						routePoint->y += moveY;
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_SPACE))
			{
				bool anyUnfolded = false;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr && !node->editorIsFolded)
					{
						anyUnfolded = true;
					}
				}
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr)
					{
						node->setIsFolded(anyUnfolded ? true : false);
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_e))
			{
				bool anyEnabled = false;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr && node->isEnabled)
					{
						anyEnabled = true;
					}
				}
				
				for (auto linkId : selectedLinks)
				{
					auto link = tryGetLink(linkId);
					
					if (link != nullptr && link->isEnabled)
					{
						anyEnabled = true;
					}
				}
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr)
					{
						node->setIsEnabled(anyEnabled ? false : true);
					}
				}
				
				for (auto linkId : selectedLinks)
				{
					auto link = tryGetLink(linkId);
					
					if (link != nullptr)
					{
						link->setIsEnabled(anyEnabled ? false : true);
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_BACKSPACE))
			{
				auto nodesToRemove = selectedNodes;
				for (auto nodeId : nodesToRemove)
				{
					graph->removeNode(nodeId);
				}
				
				Assert(selectedNodes.empty());
				selectedNodes.clear();
				
				auto linksToRemove = selectedLinks;
				for (auto linkId : linksToRemove)
				{
					if (graph->tryGetLink(linkId) != nullptr)
						graph->removeLink(linkId);
				}
				
				Assert(selectedLinks.empty());
				selectedLinks.clear();
				
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
			
			if (keyboard.isDown(SDLK_LCTRL) || mouse.isDown(BUTTON_RIGHT))
			{
				dragAndZoom.focusX -= mouse.dx / std::max(1.f, dragAndZoom.zoom);
				dragAndZoom.focusY -= mouse.dy / std::max(1.f, dragAndZoom.zoom);
				dragAndZoom.desiredFocusX = dragAndZoom.focusX;
				dragAndZoom.desiredFocusY = dragAndZoom.focusY;
			}
			
			if (keyboard.isDown(SDLK_LALT))
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
				
				if (node.nodeType == kGraphNodeType_Regular)
				{
					auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
					
					Assert(typeDefinition != nullptr);
					if (typeDefinition != nullptr)
					{
						if (testRectOverlap(
							nodeSelect.beginX,
							nodeSelect.beginY,
							nodeSelect.endX,
							nodeSelect.endY,
							node.editorX,
							node.editorY,
							node.editorX + typeDefinition->sx,
							node.editorY + (node.editorIsFolded ? typeDefinition->syFolded : typeDefinition->sy)))
						{
							nodeSelect.nodeIds.insert(node.id);
						}
					}
				}
				else if (node.nodeType == kGraphNodeType_Visualizer)
				{
					if (testRectOverlap(
						nodeSelect.beginX,
						nodeSelect.beginY,
						nodeSelect.endX,
						nodeSelect.endY,
						node.editorX,
						node.editorY,
						node.editorX + node.editorVisualizer.sx,
						node.editorY + node.editorVisualizer.sy))
					{
						nodeSelect.nodeIds.insert(node.id);
					}
				}
				else
				{
					Assert(false);
				}
			}
			
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
			
			for (auto i : selectedNodes)
			{
				auto node = tryGetNode(i);
				
				if (node != nullptr)
				{
					auto offsetItr = nodeDrag.offsets.find(i);
					
					if (offsetItr != nodeDrag.offsets.end())
					{
						auto & offset = offsetItr->second;
						
						node->editorX = mousePosition.x - offset[0];
						node->editorY = mousePosition.y - offset[1];
					}
				}
			}
			
			for (auto & routePoint : selectedLinkRoutePoints)
			{
				routePoint->x += mousePosition.dx;
				routePoint->y += mousePosition.dy;
			}
		}
		break;
		
	case kState_NodeEdit:
		{
			if (mouse.wentDown(BUTTON_RIGHT))
			{
				nodeEditEnd();
				
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
			GraphNode * node = tryGetNode(nodeResize.nodeId);
			
			Assert(node != nullptr);
			if (node != nullptr)
			{
				const float dragX = mouse.dx;
				const float dragY = mouse.dy;
				
				if (node->nodeType == kGraphNodeType_Visualizer)
				{
					if (nodeResize.dragL)
					{
						node->editorX += dragX;
						node->editorVisualizer.sx -= dragX;
					}
					
					if (nodeResize.dragR)
					{
						node->editorVisualizer.sx += dragX;
					}
					
					if (nodeResize.dragT)
					{
						node->editorY += dragY;
						node->editorVisualizer.sy -= dragY;
					}
					
					if (nodeResize.dragB)
					{
						node->editorVisualizer.sy += dragY;
					}
					
					cursor = cursorHand;
				}
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				nodeResize = NodeResize();
				
				state = kState_Idle;
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
			auto & node = nodeItr.second;
			
			//
			
			if (node.nodeType == kGraphNodeType_Regular && node.isEnabled)
			{
				const int activity = realTimeConnection == nullptr ? 0 : realTimeConnection->nodeIsActive(node.id);
				
				if (activity & GraphEdit_RealTimeConnection::kActivity_OneShot)
				{
					node.editorIsActiveAnimTime = .2f;
					node.editorIsActiveAnimTimeRcp = 1.f / node.editorIsActiveAnimTime;
				}
				
				node.editorIsActiveContinuous = (activity & GraphEdit_RealTimeConnection::kActivity_Continuous) != 0;
			}
			
			//
			
			node.tick(*this, dt);
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
	
	SDL_SetCursor(cursor);
	
	inputIsCaptured &= (state != kState_Hidden);
	
	return inputIsCaptured;
}

bool GraphEdit::tickTouches()
{
	if (state != kState_Idle && state != kState_TouchDrag && state != kState_TouchZoom)
	{
		touches = Touches();
		
		return false;
	}
	
	const float sx = GFX_SX;
	const float sy = GFX_SY;
	
	for (auto & event : framework.events)
	{
		if (event.type == SDL_FINGERDOWN)
		{
			//logDebug("touch down: fingerId=%d", event.tfinger.fingerId);
			
			if (state == kState_Idle)
			{
				if (touches.finger1.id == 0)
				{
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
					
					if (std::abs(std::abs(dragAndZoom.desiredZoom) - 1.f) < .1f)
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
					
					if (std::abs(std::abs(dragAndZoom.desiredZoom) - 1.f) < .1f)
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
						
						dragAndZoom.zoom += zoomDelta * (std::abs(dragAndZoom.zoom) + .5f);
						dragAndZoom.desiredZoom = dragAndZoom.zoom;
					}
				
					// update movement
					
					dragAndZoom.focusX -= delta[0] * dragSpeed;
					dragAndZoom.focusY -= delta[1] * dragSpeed;
					
					dragAndZoom.desiredFocusX = dragAndZoom.focusX;
					dragAndZoom.desiredFocusY = dragAndZoom.focusY;
				}
			}
		}
	}
	
	return
		state == kState_TouchDrag ||
		state == kState_TouchZoom;
}

void GraphEdit::tickMouseScroll(const float dt)
{
	const int kScrollBorder = 100;
	const float kScrollSpeed = 600.f;
	
	int tX = 0;
	int tY = 0;
	
	if (mouse.x < kScrollBorder)
		tX = mouse.x - kScrollBorder;
	if (mouse.y < kScrollBorder)
		tY = mouse.y - kScrollBorder;
	if (mouse.x > GFX_SX - 1 - kScrollBorder)
		tX = mouse.x - (GFX_SX - 1 - kScrollBorder);
	if (mouse.y > GFX_SY - 1 - kScrollBorder)
		tY = mouse.y - (GFX_SY - 1 - kScrollBorder);
	
	dragAndZoom.desiredFocusX += tX / float(kScrollBorder) * kScrollSpeed * dt;
	dragAndZoom.desiredFocusY += tY / float(kScrollBorder) * kScrollSpeed * dt;
}

void GraphEdit::tickKeyboardScroll()
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
	
	dragAndZoom.desiredFocusX += moveX / dragAndZoom.zoom * kGridSize * 8;
	dragAndZoom.desiredFocusY += moveY / dragAndZoom.zoom * kGridSize * 10;
}

void GraphEdit::nodeSelectEnd()
{
	// fixme : apply append behavior
	
	selectedNodes = nodeSelect.nodeIds;
	selectedLinks.clear(); // todo : also select links
	selectedLinkRoutePoints.clear(); // todo : also select route points
	
	nodeSelect = NodeSelect();
}

void GraphEdit::nodeDragEnd()
{
	if (editorOptions.snapToGrid)
	{
		for (auto i : selectedNodes)
		{
			auto node = tryGetNode(i);
				
			if (node != nullptr)
			{
				snapToGrid(*node);
			}
		}
		
		for (auto & routePoint : selectedLinkRoutePoints)
		{
			snapToGrid(*routePoint);
		}
	}
	
	nodeDrag = NodeDrag();
}

// todo : remove test node editor and add a mechanism to create node specific node editors
struct TestNodeEditor : GraphEdit_NodeEditorBase
{
	mutable UiState state;
	
	float value;
	
	TestNodeEditor()
		: state()
		, value(0.f)
	{
		state.sx = 300;
		state.opacity = 1.f;
	}
	
	void doUi(const bool doActions, const bool doDraw, const float dt)
	{
		makeActive(&state, doActions, doDraw);
		
		pushMenu("editor");
		{
			if (doButton("randomize"))
			{
				value = random(0.f, 1.f);
			}
			
			auto text = String::FormatC("value: %.2f", value);
			
			doLabel(text.c_str(), 0.f);
		}
		popMenu();
	}
	
	virtual void getSize(int & sx, int & sy) const override
	{
		sx = state.sx;
		sy = 50;
	}
	
	virtual void setPosition(const int x, const int y) override
	{
		state.x = x;
		state.y = y;
	}
	
	virtual bool tick(const float dt, const bool inputIsCaptured) override
	{
		doUi(true, false, dt);
		
		return state.activeElem != nullptr;
	}
	
	virtual void draw() const override
	{
		const_cast<TestNodeEditor*>(this)->doUi(false, true, 0.f);
	}
	
	virtual void setResource(const GraphNode & node, const char * type, const char * name, const char * text) override
	{
		value = Parse::Float(text);
	}
	
	virtual bool serializeResource(std::string & text) const override
	{
		text = String::FormatC("%f", value);
		
		return true;
	}
};

#define TEST_TIMELINE_EDITOR 1

#if TEST_TIMELINE_EDITOR
	#include "editor_vfxTimeline.h"
#endif

void GraphEdit::nodeEditBegin(const GraphNodeId nodeId)
{
	nodeEditEnd();
	
	//
	
	// todo : how to solve the issue of updating all graph instances with the newly edited data ?
	//        perhaps add the notion of graph-local resources. it would be easiest to solve if each
	//        graph instance just references the data instead of owning it
	
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
				nodeEditor.nodeId = nodeId;
				nodeEditor.resourceTypeName = resourceTypeName;
			#if TEST_TIMELINE_EDITOR
				if (resourceTypeName == "timeline")
					nodeEditor.editor = new Editor_VfxTimeline();
			#else
				nodeEditor.editor = new TestNodeEditor();
			#endif
				
				Assert(nodeEditor.editor != nullptr);
				if (nodeEditor.editor != nullptr)
				{
					// deserialize resource data if set
					
					auto resourceData = node->getResource(resourceTypeName.c_str(), "editorData", nullptr);
					
					// todo : separate setResource and setResourceData calls ?
					
					//if (resourceData != nullptr)
					{
						nodeEditor.editor->setResource(*node, resourceTypeName.c_str(), "editorData", resourceData);
					}
					
					// calculate the position based on size and move the editor over there
					
					int x;
					int y;
					nodeEditor.editor->getPositionForViewCenter(x, y);
					
					nodeEditor.editor->setPosition(x, y);
				}
			}
		}
	}
}

void GraphEdit::nodeEditSave()
{
	if (nodeEditor.editor != nullptr)
	{
		auto node = tryGetNode(nodeEditor.nodeId);
		
		if (node != nullptr)
		{
			std::string resourceData;
			
			if (nodeEditor.editor->serializeResource(resourceData))
			{
				node->setResource(nodeEditor.resourceTypeName.c_str(), "editorData", resourceData.c_str());
			}
			else
			{
				node->clearResource(nodeEditor.resourceTypeName.c_str(), "editorData");
			}
		}
	}
}

void GraphEdit::nodeEditEnd()
{
	nodeEditSave();
	
	//
	
	delete nodeEditor.editor;
	nodeEditor.editor = nullptr;
	
	nodeEditor = NodeEditor();
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
		
		GraphNodeSocketLink link;
		link.id = graph->allocLinkId();
		link.srcNodeId = socketConnect.srcNodeId;
		link.srcNodeSocketName = socketConnect.srcNodeSocket->name;
		link.srcNodeSocketIndex = socketConnect.srcNodeSocket->index;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketName = socketConnect.dstNodeSocket->name;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		
		graph->addLink(link, clearInputDuplicates);
		
		selectLink(link.id, true);
	}
	else if (socketConnect.dstNodeId != kGraphNodeIdInvalid)
	{
		if (commandMod())
		{
			tryAddVisualizer(
				socketConnect.dstNodeId,
				String::Empty, -1,
				socketConnect.dstNodeSocket->name, socketConnect.dstNodeSocket->index,
				mousePosition.x, mousePosition.y, true);
		}
	}
	
	socketConnect = SocketConnect();
}

void GraphEdit::doMenu(const float dt)
{
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
		
		doBreak();
	}
	popMenu();
}

void GraphEdit::doEditorOptions(const float dt)
{
	pushMenu("editorOptions");
	{
		if (doDrawer(editorOptions.menuIsVisible, "editor options"))
		{
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
	pushMenu("linkParams");
	{
		if (selectedLinks.size() == 1)
		{
			auto linkId = *selectedLinks.begin();
			
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
					
					// todo : detect if text has changed. if so, notify graph edit of change and let it take care of callbacks and undo/redo
					
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

bool GraphEdit::tryAddNode(const std::string & typeName, const int x, const int y, const bool select)
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
		node.editorX = x;
		node.editorY = y;
		node.editorZKey = graph->allocZKey();
		
		graph->addNode(node);
		
		if (select)
		{
			selectNode(node.id, true);
		}
		
		return true;
	}
}

bool GraphEdit::tryAddVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex, const int x, const int y, const bool select)
{
	if (nodeId == kGraphNodeIdInvalid)
	{
		return false;
	}
	else
	{
		GraphNode node;
		node.id = graph->allocNodeId();
		node.editorX = mousePosition.x;
		node.editorY = mousePosition.y;
		node.editorZKey = graph->allocZKey();
		node.setVisualizer(nodeId, srcSocketName, srcSocketIndex, dstSocketName, dstSocketIndex);
		
		node.editorVisualizer.sx = 0;
		node.editorVisualizer.sy = 0;
		
		graph->addNode(node);
		
	#if 0 // todo : remove ?
		GraphNodeSocketLink link;
		link.id = graph->allocLinkId();
		link.srcNodeId = node.id;
		link.srcNodeSocketName = "";
		link.srcNodeSocketIndex = 0;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketName = socketConnect.dstNodeSocket->name;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		graph->addLink(link, false);
	#endif
		
		if (select)
		{
			selectNode(node.id, true);
		}
		
		return true;
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
	}
	
	selectedLinkRoutePoints.insert(routePoint);
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
	for (auto & linkItr : graph->links)
	{
		auto & link = linkItr.second;
		
		for (auto & routePoint : link.editorRoutePoints)
		{
			selectedLinkRoutePoints.insert(&routePoint);
		}
	}
}

void GraphEdit::selectAll()
{
	selectNodeAll();
	selectLinkAll();
	selectLinkRoutePointAll();
}

void GraphEdit::snapToGrid(float & x, float & y) const
{
	x = std::round(x / float(kGridSize)) * kGridSize;
	y = std::round(y / float(kGridSize)) * kGridSize;
}

void GraphEdit::snapToGrid(GraphLinkRoutePoint & routePoint) const
{
	snapToGrid(routePoint.x, routePoint.y);
}

void GraphEdit::snapToGrid(GraphNode & node) const
{
	snapToGrid(node.editorX, node.editorY);
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
		
	case kState_NodeEdit:
		{
			nodeEditEnd();
			
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
	
	notifications.clear();
	
	Notification n;
	n.text = text;
	n.displayTime = 2.f;
	n.displayTimeRcp = 1.f / n.displayTime;
	
	notifications.push_back(n);
}

void GraphEdit::draw() const
{
	cpuTimingBlock(GraphEdit_Draw);
	
	if (state == kState_Hidden || (state == kState_HiddenIdle && hideTime == 0.f))
		return;
	
	gxPushMatrix();
	gxMultMatrixf(dragAndZoom.transform.m_v);
	gxTranslatef(.375f, .375f, 0.f); // avoid bad looking rectangles due to some diamond rule shenanigans in OpenGL by translating slightly
	
	pushFontMode(FONT_SDF);
	
	// draw background and grid
	
	{
		const Vec2 _p1 = dragAndZoom.invTransform * Vec2(0.f, 0.f);
		const Vec2 _p2 = dragAndZoom.invTransform * Vec2(GFX_SX, GFX_SY);
		
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
			
			if ((cx2 - cx1) < GFX_SX / 2)
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
				
				if (!isEnabled)
					setColor(191, 191, 191);
				else if (isSelected)
					setColor(127, 127, 255);
				else if (isHighlighted)
					setColor(255, 255, 255);
				else
					setColor(255, 255, 0);
				
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
	
#if 1 // todo : remove. test bezier control points
	gxBegin(GL_POINTS);
	{
		for (auto & linkItr : graph->links)
		{
			auto linkId = linkItr.first;
			auto & link = linkItr.second;
			
			LinkPath path;
			
			if (getLinkPath(linkId, path))
			{
				setColor(colorWhite);
				
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
	gxEnd();
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
	
	const int numNodes = graph->nodes.size();
	
	const GraphNode ** sortedNodes = (const GraphNode**)alloca(numNodes * sizeof(GraphNode*));
	
	int nodeIndex = 0;
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		
		sortedNodes[nodeIndex++] = &node;
	}
	
	std::sort(sortedNodes, sortedNodes + numNodes, [](const GraphNode *  n1, const GraphNode * n2) { return n1->editorZKey < n2->editorZKey; });
	
	for (int i = 0; i < numNodes; ++i)
	{
		auto & node = *sortedNodes[i];
		
		const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (node.nodeType == kGraphNodeType_Regular && typeDefinition == nullptr)
		{
			// todo : draw error node ?
			setColor(colorBlack);
			drawRectLine(node.editorX, node.editorY, node.editorX + 100, node.editorY + 20);
			setColor(colorRed);
			drawRectLine(node.editorX, node.editorY, node.editorX + 100, node.editorY + 20);
			setColor(colorWhite);
			setFont("calibri.ttf");
			drawText(node.editorX + 100/2, node.editorY + 20/2, 12, 0.f, 0.f, "%s", node.typeName.c_str());
		}
		else
		{
			gxPushMatrix();
			{
				gxTranslatef(node.editorX, node.editorY, 0.f);
				
				if (node.nodeType == kGraphNodeType_Regular)
				{
					drawNode(node, *typeDefinition);
				}
				else if (node.nodeType == kGraphNodeType_Visualizer)
				{
					drawVisualizer(node);
				}
				else
				{
					Assert(false);
				}
			}
			gxPopMatrix();
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
		
	case kState_NodeEdit:
		break;
	
	case kState_InputSocketConnect:
		{
			auto node = tryGetNode(socketConnect.srcNodeId);
			
			Assert(node != nullptr);
			if (node == nullptr)
			{
				// todo : error
			}
			else
			{
				hqBegin(HQ_LINES);
				{
					setColor(255, 255, 0);
					hqLine(
						node->editorX + socketConnect.srcNodeSocket->px,
						node->editorY + socketConnect.srcNodeSocket->py, 1.5f,
						mousePosition.x, mousePosition.y, 1.5f);
				}
				hqEnd();
			}
		}
		break;
	
	case kState_OutputSocketConnect:
		{
			auto node = tryGetNode(socketConnect.dstNodeId);
			
			Assert(node != nullptr);
			if (node == nullptr)
			{
				// todo : error
			}
			else
			{
				hqBegin(HQ_LINES);
				{
					setColor(255, 255, 0);
					hqLine(
						node->editorX + socketConnect.dstNodeSocket->px,
						node->editorY + socketConnect.dstNodeSocket->py, 1.5f,
						mousePosition.x, mousePosition.y, 1.5f);
				}
				hqEnd();
			}
		}
		break;
		
	case kState_NodeResize:
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
		
	case kState_NodeEdit:
		break;
		
	case kState_InputSocketConnect:
		break;
		
	case kState_OutputSocketConnect:
		break;
		
	case kState_NodeResize:
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
	
	if (propertyEditor != nullptr)
	{
		propertyEditor->draw();
	}
	
	makeActive(uiState, false, true);
	
	GraphEdit * self = const_cast<GraphEdit*>(this);
	
	self->doMenu(0.f);
	
	self->doLinkParams(0.f);
	
	self->doEditorOptions(0.f);
	
	if (nodeTypeNameSelect != nullptr)
	{
		nodeTypeNameSelect->doMenus(uiState, 0.f);
	}
	
	//
	
	if (state == kState_NodeEdit)
	{
		setColor(0, 0, 0, 127);
		drawRect(0, 0, GFX_SX, GFX_SY);
	}
	
	if (nodeEditor.editor != nullptr)
	{
		setColor(colorWhite);
		nodeEditor.editor->draw();
	}
	
	//
	
	if (!notifications.empty())
	{
		const int kWidth = 200;
		const int kHeight = 40;
		
		const Notification & n = notifications.front();
		
		const float t = n.displayTime * n.displayTimeRcp;
		const float tMoveUp = .05f;
		const float tMoveDown = .05f;
		
		float y = 1.f;
		
		if (t > (1.f - tMoveUp))
			y = 1.f - (t - (1.f - tMoveUp)) / tMoveUp;
		if (t < tMoveDown)
			y = t / tMoveDown;
		
		y *= 50;
		
		setColor(colorBlack);
		drawRect(GFX_SX/2 - kWidth, GFX_SY - y, GFX_SX/2 + kWidth, GFX_SY - y + kHeight);
		
		setColor(colorWhite);
		drawRectLine(GFX_SX/2 - kWidth, GFX_SY - y, GFX_SX/2 + kWidth, GFX_SY - y + kHeight);
		
		setColor(colorWhite);
		setFont("calibri.ttf");
		drawText(GFX_SX/2, GFX_SY - y + kHeight/2, 18, 0.f, 0.f, "%s", n.text.c_str());
	}
	
	HitTestResult hitTestResult;
	
	if (state == kState_Idle && hitTest(mousePosition.x, mousePosition.y, hitTestResult))
	{
		if (hitTestResult.hasNode &&
			hitTestResult.nodeHitTestResult.background &&
			hitTestResult.node->nodeType == kGraphNodeType_Regular)
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

void GraphEdit::drawNode(const GraphNode & node, const GraphEdit_TypeDefinition & definition) const
{
	vfxCpuTimingBlock(drawNode);
	
	const bool isEnabled = node.isEnabled;
	const bool isSelected = selectedNodes.count(node.id) != 0;
	const bool isFolded = node.editorIsFolded;
	
	const float nodeSy = Calc::Lerp(definition.syFolded, definition.sy, isFolded ? node.editorFoldAnimTime * node.editorFoldAnimTimeRcp : 1.f - node.editorFoldAnimTime * node.editorFoldAnimTimeRcp);
	
	Color color;
	
	if (!isEnabled)
		color = Color(191, 191, 191, 255);
	else if (isSelected)
		color = Color(63, 63, 127, 255);
	else
		color = Color(63, 63, 63, 255);
	
	if (editorOptions.showOneShotActivity)
	{
		const float activeAnim = node.editorIsActiveAnimTime * node.editorIsActiveAnimTimeRcp;
		color = color.interp(Color(63, 63, 255), activeAnim);
	}
	
	if (editorOptions.showContinuousActivity)
	{
		if (node.editorIsActiveContinuous)
		{
			color = color.interp(Color(63, 63, 255), .5f + .5f * (std::cos(framework.time * 8.f) + 1.f) / 2.f);
		}
	}
	
	if (editorOptions.showCpuHeat && realTimeConnection != nullptr && node.isEnabled && node.nodeType == kGraphNodeType_Regular)
	{
		const int timeUs = realTimeConnection->getNodeCpuTimeUs(node.id);
		const float t = timeUs / float(realTimeConnection->getNodeCpuHeatMax());
		
		ParticleColor particleColor;
		editorOptions.cpuHeatColors.sample(t, true, particleColor);
		
		color = color.interp(Color(particleColor.rgba[0], particleColor.rgba[1], particleColor.rgba[2]), particleColor.rgba[3]);
	}
	
#if 1
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	{
		const float border = 3.f;
		const float radius = 5.f;
		
		setColor(isSelected ? colorWhite : colorBlack);
		hqFillRoundedRect(-border/2, -border/2, definition.sx + border/2, nodeSy + border/2, radius + border/2);
		
		setColor(color);
		hqFillRoundedRect(0.f + border/2, 0.f + border/2, definition.sx - border/2, nodeSy - border/2, radius);
	}
	hqEnd();
#else
	setColor(color);
	drawRect(0.f, 0.f, definition.sx, nodeSy);
#endif
	
	if (isSelected)
		setColor(255, 255, 255, 255);
	else
		setColor(127, 127, 127, 255);
	//drawRectLine(0.f, 0.f, definition.sx, nodeSy);
	
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(definition.sx/2, 12, 14, 0.f, 0.f, "%s", !node.editorName.empty() ? node.editorName.c_str() : definition.displayName.empty() ? definition.typeName.c_str() : definition.displayName.c_str());
	
	if (node.isPassthrough)
	{
		setColor(127, 127, 255);
		drawText(definition.sx - 8, 12, 14, -1.f, 0.f, "P");
	}
	
	if (isFolded == false)
	{
		beginTextBatch();
		{
			for (auto & inputSocket : definition.inputSockets)
			{
				setColor(255, 255, 255);
				drawText(inputSocket.px + inputSocket.radius + 2, inputSocket.py, 12, +1.f, 0.f, "%s", inputSocket. name.c_str());
			}
			
			for (auto & outputSocket : definition.outputSockets)
			{
				setColor(255, 255, 255);
				drawText(outputSocket.px - outputSocket.radius - 2, outputSocket.py, 12, -1.f, 0.f, "%s", outputSocket.name.c_str());
			}
		}
		endTextBatch();
		
		hqBegin(HQ_FILLED_CIRCLES);
		{
			for (auto & inputSocket : definition.inputSockets)
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
				
				hqFillCircle(inputSocket.px, inputSocket.py, inputSocket.radius);
				
				if (node.editorInputValues.count(inputSocket.name) != 0)
				{
					setColor(255, 255, 255);
					hqFillCircle(inputSocket.px, inputSocket.py, inputSocket.radius / 3.f);
				}
			}
			
			for (auto & outputSocket : definition.outputSockets)
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
				
				hqFillCircle(outputSocket.px, outputSocket.py, outputSocket.radius);
			}
		}
		hqEnd();
	}
}

void GraphEdit::drawVisualizer(const GraphNode & node) const
{
	vfxCpuTimingBlock(drawVisualizer);
	
	const bool isSelected = selectedNodes.count(node.id) != 0;
	
	auto srcNode = tryGetNode(node.editorVisualizer.nodeId);
	
	const std::string & nodeName = srcNode != nullptr ? srcNode->getDisplayName() : String::Empty;
	
	const int visualizerSx = int(std::ceil(node.editorVisualizer.sx));
	const int visualizerSy = int(std::ceil(node.editorVisualizer.sy));
	node.editorVisualizer.visualizer->draw(*this, nodeName, isSelected, &visualizerSx, &visualizerSy);
	
#if 0
	setColor(255, 255, 255, 63); // fixme : remove this drawRect call! only here to test node resizing
	drawRect(0, 0, node.editorVisualizer.sx, node.editorVisualizer.sy);
#endif
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
			
			const XMLElement * xmlEditor = xmlGraph->FirstChildElement("editor");
			if (xmlEditor != nullptr)
			{
				result &= loadXml(xmlEditor);
			}
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
		
		nodeEditSave();
		
		XMLPrinter xmlGraph(file);;
		
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
	auto dragAndZoomElem = editorElem->FirstChildElement("dragAndZoom");
	
	if (dragAndZoomElem != nullptr)
	{
		dragAndZoom.desiredFocusX = floatAttrib(dragAndZoomElem, "x", 0.f);
		dragAndZoom.desiredFocusY = floatAttrib(dragAndZoomElem, "y", 0.f);
		dragAndZoom.desiredZoom = floatAttrib(dragAndZoomElem, "zoom", 1.f);
		
		dragAndZoom.focusX = dragAndZoom.desiredFocusX;
		dragAndZoom.focusY = dragAndZoom.desiredFocusY;
		dragAndZoom.zoom = dragAndZoom.desiredZoom;
	}
	
	auto editorOptionsElem = editorElem->FirstChildElement("editorOptions");
	
	if (editorOptionsElem != nullptr)
	{
		EditorOptions defaultOptions;
		
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
	editorElem.OpenElement("dragAndZoom");
	{
		editorElem.PushAttribute("x", dragAndZoom.desiredFocusX);
		editorElem.PushAttribute("y", dragAndZoom.desiredFocusY);
		editorElem.PushAttribute("zoom", dragAndZoom.desiredZoom);
	}
	editorElem.CloseElement();
	
	editorElem.OpenElement("editorOptions");
	{	
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
	auto node = tryGetNode(nodeId);
	Assert(node != nullptr);
	
	if (realTimeConnection != nullptr && node->nodeType == kGraphNodeType_Regular)
	{
		realTimeConnection->nodeAdd(nodeId, typeName);
	}
}

void GraphEdit::nodeRemove(const GraphNodeId nodeId)
{
	auto node = tryGetNode(nodeId);
	Assert(node != nullptr);
	
	if (selectedNodes.count(nodeId) != 0)
		selectedNodes.erase(nodeId);
	
	if (realTimeConnection != nullptr && node->nodeType == kGraphNodeType_Regular)
	{
		realTimeConnection->nodeRemove(nodeId);
	}
}

void GraphEdit::linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	auto srcNode = tryGetNode(srcNodeId);
	auto dstNode = tryGetNode(dstNodeId);
	
	Assert(srcNode != nullptr);
	Assert(dstNode != nullptr);
	
	if (realTimeConnection != nullptr &&
		srcNode->nodeType == kGraphNodeType_Regular &&
		dstNode->nodeType == kGraphNodeType_Regular)
	{
		realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
}

void GraphEdit::linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	auto srcNode = tryGetNode(srcNodeId);
	auto dstNode = tryGetNode(dstNodeId);
	
	Assert(srcNode != nullptr);
	Assert(dstNode != nullptr);
	
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
	
	if (realTimeConnection != nullptr &&
		srcNode->nodeType == kGraphNodeType_Regular &&
		dstNode->nodeType == kGraphNodeType_Regular)
	{
		realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
		
		//
		
		auto srcSocket = tryGetInputSocket(srcNodeId, srcSocketIndex);
		Assert(srcSocket != nullptr);
		
		if (srcSocket != nullptr)
		{
			auto valueText = srcNode->editorInputValues.find(srcSocket->name);
			if (valueText != srcNode->editorInputValues.end())
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
	, uiState(nullptr)
	, uiColors(nullptr)
{
	typeLibrary = _typeLibrary;
	
	uiState = new UiState();
	
	const int kPadding = 10;
	uiState->sx = 200;
	uiState->x = GFX_SX - uiState->sx - kPadding;
	uiState->y = kPadding;
	uiState->textBoxTextOffset = 80;
	
	uiColors = new ParticleColor[kMaxUiColors];
}

GraphUi::PropEdit::~PropEdit()
{
	delete[] uiColors;
	uiColors = nullptr;
	
	delete uiState;
	uiState = nullptr;
}

bool GraphUi::PropEdit::tick(const float dt)
{	
	if (nodeId == kGraphNodeIdInvalid)
	{
		return false;
	}
	else
	{
		doMenus(true, false, dt);
		
		return uiState->activeElem != nullptr;
	}
}

void GraphUi::PropEdit::draw() const
{
	const_cast<PropEdit*>(this)->doMenus(false, true, 0.f);
}

void GraphUi::PropEdit::setGraph(Graph * _graph)
{
	graph = _graph;
	
	createUi();
}

void GraphUi::PropEdit::setNode(const GraphNodeId _nodeId)
{
	if (_nodeId != nodeId)
	{
		//logDebug("setNode: %d", _nodeId);
		
		nodeId = _nodeId;
		
		createUi();
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
		
		int value = Parse::Int32(valueText);
		
		std::vector<EnumValue> enumValues;
		
		for (auto & elem : enumDefinition->enumElems)
		{
			enumValues.push_back(EnumValue(elem.value, elem.name));
		}
		
		doDropdown(value, name.c_str(), enumValues);
		
		valueText = String::ToString(value);
		
		return value != Parse::Int32(defaultValue);
	}
	else if (editor == "slider")
	{
		// todo : add doSlider
		
		doTextBox(valueText, name.c_str(), dt);
		
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
	else if (editor == "custom")
	{
		if (graphEdit.realTimeConnection != nullptr)
		{
			graphEdit.realTimeConnection->doEditor(valueText, name, defaultValue, g_doActions, g_doDraw, dt);
			
			return valueText != defaultValue;
		}
		
		return false;
	}
	else
	{
		Assert(false);
	}
	
	return false;
}

void GraphUi::PropEdit::doMenus(const bool doActions, const bool doDraw, const float dt)
{
	makeActive(uiState, doActions, doDraw);
	pushMenu("propEdit");
	
	GraphNode * node = tryGetNode();
	
	if (node != nullptr)
	{
		// todo : would be nice to change node type on the fly
		
		//doTextBox(node->typeName, "node type", dt);
		
		const GraphEdit_TypeDefinition * typeDefinition = typeLibrary->tryGetTypeDefinition(node->typeName);
		
		if (typeDefinition != nullptr)
		{
			std::string headerText = typeDefinition->typeName;
			
			if (!typeDefinition->displayName.empty())
				headerText = typeDefinition->displayName;
			
			doLabel(headerText.c_str(), 0.f);
			
			doTextBox(node->editorName, "display name", dt);
			
			int menuItemIndex = 0;
			
			for (auto & inputSocket : typeDefinition->inputSockets)
			{
				auto valueTextItr = node->editorInputValues.find(inputSocket.name);
				
				const bool isPreExisting = valueTextItr != node->editorInputValues.end();
				
				std::string oldValueText;
				
				if (isPreExisting)
					oldValueText = valueTextItr->second;
				else
					oldValueText = inputSocket.defaultValue;
				
				std::string newValueText = oldValueText;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(inputSocket.typeName);
				
				const GraphEdit_EnumDefinition * enumDefinition = typeLibrary->tryGetEnumDefinition(inputSocket.enumName);
				
				bool pressed = false;
				
				const bool hasValue = doMenuItem(*graphEdit, newValueText, inputSocket.name, inputSocket.defaultValue,
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
						node->editorInputValues.erase(inputSocket.name);
						
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
						node->editorInputValues[inputSocket.name] = newValueText;
				}
				
				// todo : detect if text has changed. if so, notify graph edit of change and let it take care of callbacks and undo/redo
				
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
			
			for (auto & outputSocket : typeDefinition->outputSockets)
			{
				if (!outputSocket.isEditable)
					continue;
				
				std::string & newValueText = node->editorValue;
				
				const std::string oldValueText = newValueText;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(outputSocket.typeName);
				
				bool pressed = false;
				
				const bool hasValue = doMenuItem(*graphEdit, newValueText, outputSocket.name, "",
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

void GraphUi::PropEdit::createUi()
{
	uiState->reset();
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
	
	for (auto & typeDefenition : typeDefinitionLibrary->typeDefinitions)
	{
		const std::string & typeNameToMatch = typeDefenition.second.typeName;
		
		if (typeNameToMatch[0] != typeName[0])
			continue;
		
		const uint32_t score = fuzzyStringDistance(typeNameToMatch, typeName);
		
		typeNamesAndScores.resize(index + 1);
		typeNamesAndScores[index].typeName = typeNameToMatch;
		typeNamesAndScores[index].score = score;
		
		++index;
	}
	
	std::sort(typeNamesAndScores.begin(), typeNamesAndScores.end());
}

void GraphUi::NodeTypeNameSelect::doMenus(UiState * uiState, const float dt)
{
	pushMenu("nodeTypeSelect");
	{
		doLabel("insert", 0.f);
		
		if (doTextBox(typeName, "type", dt))
		{
			typeName = findClosestMatch(typeName);
			selectTypeName(typeName);
		}
		
		if (!typeName.empty())
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
						uiState->reset();
						selectTypeName(typeNamesAndScores[i].typeName);
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
	
	if (graphEdit->tryAddNode(typeName, graphEdit->dragAndZoom.focusX, graphEdit->dragAndZoom.focusY, true))
	{
		// update history
		
		for (auto i = history.begin(); i != history.end(); )
		{
			if ((*i) == typeName)
				i = history.erase(i);
			else
				++i;
		}
		
		history.push_front(_typeName);
		
		while (history.size() > kMaxHistory)
			history.pop_back();
	}
}

std::string & GraphUi::NodeTypeNameSelect::getNodeTypeName()
{
	return typeName;
}
