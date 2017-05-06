#include "Calc.h"
#include "Debugging.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <cmath>

extern const int GFX_SX; // fixme : make property of graph editor
extern const int GFX_SY;

using namespace tinyxml2;

//

GraphNodeId kGraphNodeIdInvalid = 0;
GraphLinkId kGraphLinkIdInvalid = 0;

//

static const int kGridSize = 16;

//

static bool areCompatibleSocketLinkTypeNames(const std::string & srcTypeName, const std::string & dstTypeName)
{
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

//

GraphNode::GraphNode()
	: id(kGraphNodeIdInvalid)
	, typeName()
	, isEnabled(true)
	, editorName()
	, editorX(0.f)
	, editorY(0.f)
	, editorIsFolded(false)
	, editorFoldAnimTime(0.f)
	, editorFoldAnimTimeRcp(0.f)
	, editorInputValues()
	, editorValue()
	, editorIsPassthrough(false)
	, editorIsActiveAnimTime(0.f)
	, editorIsActiveAnimTimeRcp(0.f)
	, editorIsActiveContinuous(false)
{
}

void GraphNode::tick(const float dt)
{
	editorFoldAnimTime = Calc::Max(0.f, editorFoldAnimTime - dt);
	editorIsActiveAnimTime = Calc::Max(0.f, editorIsActiveAnimTime - dt);
}

void GraphNode::setIsEnabled(const bool _isEnabled)
{
	isEnabled = _isEnabled;
}

void GraphNode::setIsPassthrough(const bool isPassthrough)
{
	editorIsPassthrough = isPassthrough;
}

void GraphNode::setIsFolded(const bool isFolded)
{
	if (editorIsFolded == isFolded)
		return;
	
	editorIsFolded = isFolded;
	editorFoldAnimTime = isFolded ? .1f : .07f;
	editorFoldAnimTimeRcp = 1.f / editorFoldAnimTime;
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

void Graph::addNode(GraphNode & node)
{
	Assert(node.id != kGraphNodeIdInvalid);
	Assert(nodes.find(node.id) == nodes.end());
	
	nodes.insert(std::pair<GraphNodeId, GraphNode>(node.id, node));
	
	if (graphEditConnection != nullptr)
		graphEditConnection->nodeAdd(node.id, node.typeName);
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
			if (graphEditConnection)
			{
				graphEditConnection->linkRemove(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
			}
			
			linkItr = links.erase(linkItr);
		}
		else if (link.dstNodeId == nodeId)
		{
			if (graphEditConnection)
			{
				graphEditConnection->linkRemove(link.id, link.srcNodeId, link.srcNodeSocketIndex, link.dstNodeId, link.dstNodeSocketIndex);
			}
			
			linkItr = links.erase(linkItr);
		}
		else
			++linkItr;
	}
	
	if (graphEditConnection)
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

bool Graph::loadXml(const XMLElement * xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	nextNodeId = intAttrib(xmlGraph, "nextNodeId", nextNodeId);
	nextLinkId = intAttrib(xmlGraph, "nextLinkId", nextLinkId);
	
	for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
	{
		GraphNode node;
		node.id = intAttrib(xmlNode, "id", node.id);
		node.typeName = stringAttrib(xmlNode, "typeName", node.typeName.c_str());
		node.isEnabled = boolAttrib(xmlNode, "enabled", node.isEnabled);
		node.editorName = stringAttrib(xmlNode, "editorName", node.editorName.c_str());
		node.editorX = floatAttrib(xmlNode, "editorX", node.editorX);
		node.editorY = floatAttrib(xmlNode, "editorY", node.editorY);
		node.editorIsFolded = boolAttrib(xmlNode, "folded", node.editorIsFolded);
		node.editorValue = stringAttrib(xmlNode, "editorValue", node.editorValue.c_str());
		node.editorIsPassthrough = boolAttrib(xmlNode, "editorIsPassthrough", node.editorIsPassthrough);
		
		for (const XMLElement * xmlInput = xmlNode->FirstChildElement("input"); xmlInput != nullptr; xmlInput = xmlInput->NextSiblingElement("input"))
		{
			const std::string socket = stringAttrib(xmlInput, "socket", "");
			const std::string value = stringAttrib(xmlInput, "value", "");
			
			node.editorInputValues[socket] = value;
		}
		
		addNode(node);
		
		nextNodeId = std::max(nextNodeId, node.id + 1);
	}
	
	for (const XMLElement * xmlLink = xmlGraph->FirstChildElement("link"); xmlLink != nullptr; xmlLink = xmlLink->NextSiblingElement("link"))
	{
		GraphNodeSocketLink link;
		link.id = intAttrib(xmlLink, "id", link.id);
		link.isEnabled = boolAttrib(xmlLink, "enabled", link.isEnabled);
		link.srcNodeId = intAttrib(xmlLink, "srcNodeId", link.srcNodeId);
		link.srcNodeSocketName = stringAttrib(xmlLink, "srcNodeSocketName", link.srcNodeSocketName.c_str());
		link.srcNodeSocketIndex = intAttrib(xmlLink, "srcNodeSocketIndex", link.srcNodeSocketIndex);
		link.dstNodeId = intAttrib(xmlLink, "dstNodeId", link.dstNodeId);
		link.dstNodeSocketName = stringAttrib(xmlLink, "dstNodeSocketName", link.dstNodeSocketName.c_str());
		link.dstNodeSocketIndex = intAttrib(xmlLink, "dstNodeSocketIndex", link.dstNodeSocketIndex);
		
		addLink(link, false);
		
		nextLinkId = std::max(nextLinkId, link.id + 1);
	}
	
#if 0
	// todo : remove this once conversion is complete : fixup socket input and output names
	
	for (auto & linkItr : links)
	{
		auto & link = linkItr.second;
		
		auto srcNode = tryGetNode(link.srcNodeId);
		
		if (srcNode)
		{
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
			
			if (typeDefinition)
			{
				if (link.srcNodeSocketIndex >= 0 && link.srcNodeSocketIndex < typeDefinition->inputSockets.size())
				{
					link.srcNodeSocketName = typeDefinition->inputSockets[link.srcNodeSocketIndex].name;
					printf("srcNodeSocketName: %s\n", link.srcNodeSocketName.c_str());
				}
			}
		}
		
		auto dstNode = tryGetNode(link.dstNodeId);
		
		if (dstNode)
		{
			auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
			
			if (typeDefinition)
			{
				if (link.dstNodeSocketIndex >= 0 && link.dstNodeSocketIndex < typeDefinition->outputSockets.size())
				{
					link.dstNodeSocketName = typeDefinition->outputSockets[link.dstNodeSocketIndex].name;
					printf("dstNodeSocketName: %s\n", link.dstNodeSocketName.c_str());
				}
			}
		}
	}
#else
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
	}
#endif

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
			xmlGraph.PushAttribute("enabled", node.isEnabled);
			xmlGraph.PushAttribute("editorName", node.editorName.c_str());
			xmlGraph.PushAttribute("editorX", node.editorX);
			xmlGraph.PushAttribute("editorY", node.editorY);
			xmlGraph.PushAttribute("folded", node.editorIsFolded);
			xmlGraph.PushAttribute("editorValue", node.editorValue.c_str());
			xmlGraph.PushAttribute("editorIsPassthrough", node.editorIsPassthrough);
			
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
			xmlGraph.PushAttribute("srcNodeSocketIndex", link.srcNodeSocketIndex);
			xmlGraph.PushAttribute("dstNodeId", link.dstNodeId);
			xmlGraph.PushAttribute("dstNodeSocketName", link.dstNodeSocketName.c_str());
			xmlGraph.PushAttribute("dstNodeSocketIndex", link.dstNodeSocketIndex);
		}
		xmlGraph.CloseElement();
	}
	
	return result;
}

//

#include "framework.h"
#include "../libparticle/particle.h"
#include "../libparticle/ui.h"


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
	typeName = stringAttrib(xmlType, "typeName", "");
	
	editor = stringAttrib(xmlType, "editor", "textbox");
	editorMin = stringAttrib(xmlType, "editorMin", "0");
	editorMax = stringAttrib(xmlType, "editorMax", "1");
	visualizer = stringAttrib(xmlType, "visualizer", "");
	
	return true;
}

bool GraphEdit_TypeDefinition::InputSocket::canConnectTo(const GraphEdit_TypeDefinition::OutputSocket & socket) const
{
	if (!areCompatibleSocketLinkTypeNames(typeName, socket.typeName))
		return false;
	
	return true;
}

bool GraphEdit_TypeDefinition::OutputSocket::canConnectTo(const GraphEdit_TypeDefinition::InputSocket & socket) const
{
	if (!areCompatibleSocketLinkTypeNames(socket.typeName, typeName))
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
	typeName = stringAttrib(xmlType, "typeName", "");
	
	displayName = stringAttrib(xmlType, "displayName", "");
	
	for (auto xmlInput = xmlType->FirstChildElement("input"); xmlInput != nullptr; xmlInput = xmlInput->NextSiblingElement("input"))
	{
		InputSocket socket;
		socket.typeName = stringAttrib(xmlInput, "typeName", socket.typeName.c_str());
		socket.name = stringAttrib(xmlInput, "name", socket.name.c_str());
		
		inputSockets.push_back(socket);
	}
	
	for (auto xmlOutput = xmlType->FirstChildElement("output"); xmlOutput != nullptr; xmlOutput = xmlOutput->NextSiblingElement("output"))
	{
		OutputSocket socket;
		socket.typeName = stringAttrib(xmlOutput, "typeName", socket.typeName.c_str());
		socket.name = stringAttrib(xmlOutput, "name", socket.name.c_str());
		socket.isEditable = boolAttrib(xmlOutput, "editable", socket.isEditable);
		
		outputSockets.push_back(socket);
	}
	
	return true;
}

//

bool GraphEdit_TypeDefinitionLibrary::loadXml(const XMLElement * xmlLibrary)
{
	bool result = true;
	
	for (auto xmlType = xmlLibrary->FirstChildElement("valueType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("valueType"))
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		
		result &= typeDefinition.loadXml(xmlType);
		
		// todo : check typeName doesn't exist yet
		
		valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	for (auto xmlType = xmlLibrary->FirstChildElement("type"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("type"))
	{
		GraphEdit_TypeDefinition typeDefinition;
		
		result &= typeDefinition.loadXml(xmlType);
		typeDefinition.createUi();
		
		// todo : check typeName doesn't exist yet
		
		typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	return result;
}

//

GraphEdit::GraphEdit(GraphEdit_TypeDefinitionLibrary * _typeDefinitionLibrary)
	: graph(nullptr)
	, typeDefinitionLibrary(nullptr)
	, realTimeConnection(nullptr)
	, selectedNodes()
	, selectedLinks()
	, state(kState_Idle)
	, nodeSelect()
	, socketConnect()
	, mousePosition()
	, dragAndZoom()
	, realTimeSocketCapture()
	, documentInfo()
	, editorOptions()
	, propertyEditor(nullptr)
	, nodeTypeNameSelect(nullptr)
	, uiState(nullptr)
{
	graph = new Graph();
	
	graph->graphEditConnection = this;
	
	typeDefinitionLibrary = _typeDefinitionLibrary;
	
	propertyEditor = new GraphUi::PropEdit(_typeDefinitionLibrary, this);
	
	nodeTypeNameSelect = new GraphUi::NodeTypeNameSelect(this);
	
	uiState = new UiState();
	
	const int kPadding = 10;
	uiState->sx = 200;
	uiState->x = kPadding;
	uiState->y = kPadding;
	uiState->textBoxTextOffset = 50;
	
	//
	
	propertyEditor->setGraph(graph);
}

GraphEdit::~GraphEdit()
{
	selectedNodes.clear();
	
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
	auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
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

bool GraphEdit::hitTest(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	for (auto nodeItr = graph->nodes.rbegin(); nodeItr != graph->nodes.rend(); ++nodeItr)
	{
		auto & node = nodeItr->second;
		
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
	
	for (auto linkItr = graph->links.rbegin(); linkItr != graph->links.rend(); ++linkItr)
	{
		auto & link = linkItr->second;
		
		auto srcNode = tryGetNode(link.srcNodeId);
		auto dstNode = tryGetNode(link.dstNodeId);
		
		auto srcNodeSocket = tryGetInputSocket(link.srcNodeId, link.srcNodeSocketIndex);
		auto dstNodeSocket = tryGetOutputSocket(link.dstNodeId, link.dstNodeSocketIndex);
		
		Assert(srcNode != nullptr && dstNode != nullptr && srcNodeSocket != nullptr && dstNodeSocket != nullptr);
		if (srcNode != nullptr && dstNode != nullptr && srcNodeSocket != nullptr && dstNodeSocket != nullptr)
		{
			auto srcTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
			auto dstTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
			
			const int srcY = srcTypeDefinition == nullptr ? 0 : srcNode->editorIsFolded ? srcTypeDefinition->syFolded/2 : srcNodeSocket->py;
			const int dstY = dstTypeDefinition == nullptr ? 0 : dstNode->editorIsFolded ? dstTypeDefinition->syFolded/2 : dstNodeSocket->py;
			
			if (testLineOverlap(
				srcNode->editorX + srcNodeSocket->px,
				srcNode->editorY + srcY,
				dstNode->editorX + dstNodeSocket->px,
				dstNode->editorY + dstY,
				x, y, 10.f))
			{
				result.hasLink = true;
				result.link = &link;
				return true;
			}
		}
	}
	
	return false;
}

bool GraphEdit::tick(const float dt)
{
	{
		mousePosition.uiX = mouse.x;
		mousePosition.uiY = mouse.y;
		
		const Vec2 srcMousePosition(mouse.x, mouse.y);
		const Vec2 dstMousePosition = dragAndZoom.invTransform * srcMousePosition;
		mousePosition.x = dstMousePosition[0];
		mousePosition.y = dstMousePosition[1];
	}
	
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
				auto srcSocketIndex = srcSocket->index;
				
				if (nodeId != realTimeSocketCapture.nodeId || srcSocketIndex != realTimeSocketCapture.srcSocketIndex)
				{
					//logDebug("reset realTimeSocketCapture");
					realTimeSocketCapture = RealTimeSocketCapture();
				}
				
				realTimeSocketCapture.nodeId = nodeId;
				realTimeSocketCapture.srcSocketIndex = srcSocketIndex;
				
				std::string value;
				bool hasValue;
				
				hasValue = realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocket->name, value);
				
				if (hasValue)
				{
					//logDebug("real time srcSocket value: %s", value.c_str());
				}
				
				realTimeSocketCapture.value = value;
				realTimeSocketCapture.hasValue = hasValue;
				
				//
				
				auto valueTypeDefinition = typeDefinitionLibrary->tryGetValueTypeDefinition(srcSocket->typeName);
				
				if (valueTypeDefinition != nullptr)
				{
					if (valueTypeDefinition->visualizer == "valueplotter")
					{
						const float valueAsFloat = Parse::Float(value);
						
						realTimeSocketCapture.history.add(valueAsFloat);
					}
					
					if (valueTypeDefinition->visualizer == "opengl-texture")
					{
						realTimeSocketCapture.texture = Parse::Int32(value);
					}
				}
			}
			
			if (result.hasNode && result.nodeHitTestResult.outputSocket != nullptr)
			{
				isValid = true;
				
				auto dstSocket = result.nodeHitTestResult.outputSocket;
				
				auto nodeId = result.node->id;
				auto dstSocketIndex = dstSocket->index;
				
				if (nodeId != realTimeSocketCapture.nodeId || dstSocketIndex != realTimeSocketCapture.dstSocketIndex)
				{
					//logDebug("reset realTimeSocketCapture");
					realTimeSocketCapture = RealTimeSocketCapture();
				}
				
				realTimeSocketCapture.nodeId = nodeId;
				realTimeSocketCapture.dstSocketIndex = dstSocketIndex;
				
				std::string value;
				bool hasValue;
				
				hasValue = realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocket->name, value);
				
				if (hasValue)
				{
					//logDebug("real time srcSocket value: %s", value.c_str());
				}
				
				realTimeSocketCapture.value = value;
				realTimeSocketCapture.hasValue = hasValue;
				
				//
				
				auto valueTypeDefinition = typeDefinitionLibrary->tryGetValueTypeDefinition(dstSocket->typeName);
				
				if (valueTypeDefinition != nullptr)
				{
					if (valueTypeDefinition->visualizer == "valueplotter")
					{
						const float valueAsFloat = Parse::Float(value);
						
						realTimeSocketCapture.history.add(valueAsFloat);
					}
					
					if (valueTypeDefinition->visualizer == "opengl-texture")
					{
						realTimeSocketCapture.texture = Parse::Int32(value);
					}
				}
			}
		}
		
		if (isValid == false && realTimeSocketCapture.nodeId != kGraphNodeIdInvalid)
		{
			//logDebug("reset realTimeSocketCapture");
			realTimeSocketCapture = RealTimeSocketCapture();
		}
	}
	
	//
	
	bool inputIsCaptured = (state != kState_Idle);
	
	if (inputIsCaptured == false)
	{
		inputIsCaptured |= propertyEditor->tick(dt);
	}
	
	makeActive(uiState, true, false);
	
	if (inputIsCaptured == false)
	{
		doMenu(dt);
		
		doEditorOptions(dt);
		
		nodeTypeNameSelect->doMenus(uiState, dt);
		
		inputIsCaptured |= uiState->activeElem != nullptr;
	}
	
	//
	
	highlightedSockets = SocketSelection();
	highlightedLinks.clear();
	
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
				}
				
				if (hitTestResult.hasLink)
				{
					highlightedLinks.insert(hitTestResult.link->id);
				}
			}
		
			if (mouse.wentDown(BUTTON_LEFT))
			{
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
						
						if (selectedNodes.count(hitTestResult.node->id) == 0)
						{
							selectNode(hitTestResult.node->id);
						}
						
						if (hitTestResult.nodeHitTestResult.background)
						{
							state = kState_NodeDrag;
							break;
						}
					}
					
					if (hitTestResult.hasLink)
					{
						if (selectedLinks.count(hitTestResult.link->id) == 0)
						{
							selectLink(hitTestResult.link->id);
						}
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
			
			if (keyboard.wentDown(SDLK_i))
			{
				std::string typeName = nodeTypeNameSelect->getNodeTypeName();
				
				tryAddNode(typeName, mousePosition.x, mousePosition.x, true);
			}
			
			if (keyboard.wentDown(SDLK_a))
			{
				if (keyboard.isDown(SDLK_LGUI))
					selectAll();
				else
				{
					for (auto nodeId : selectedNodes)
					{
						auto node = tryGetNode(nodeId);
						
						if (node != nullptr)
						{
							node->editorX = std::round(node->editorX / float(kGridSize)) * kGridSize;
							node->editorY = std::round(node->editorY / float(kGridSize)) * kGridSize;
						}
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_o))
			{
				if (keyboard.isDown(SDLK_LGUI))
				{
					dragAndZoom.desiredZoom = 1.f;
					dragAndZoom.desiredFocusX = 0.f;
					dragAndZoom.desiredFocusY = 0.f;
				}
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
						GraphNode newNode;
						newNode.id = graph->allocNodeId();
						newNode.typeName = node->typeName;
						newNode.editorX = node->editorX + kGridSize;
						newNode.editorY = node->editorY + kGridSize;
						
						if (keyboard.isDown(SDLK_LGUI))
						{
							// deep copy node including values
							newNode.editorInputValues = node->editorInputValues;
							newNode.editorValue = node->editorValue;
							
							// todo : deep copy doesn't work nicely with real-time editing for now. we need to tell the run-time the socket values
						}
						
						graph->addNode(newNode);
						
						newSelectedNodes.insert(newNode.id);
					}
				}
				
				if (!newSelectedNodes.empty())
				{
					selectedNodes = newSelectedNodes;
					selectedLinks.clear();
				}
			}
			
			if (keyboard.wentDown(SDLK_p))
			{
				bool allPassthrough = true;
				
				for (auto nodeId : selectedNodes)
				{
					auto node = tryGetNode(nodeId);
					
					if (node != nullptr)
						allPassthrough &= node->editorIsPassthrough;
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
							realTimeConnection->setNodeIsPassthrough(node->id, node->editorIsPassthrough);
						}
					}
				}
			}
			
			if (selectedNodes.empty())
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
				
				dragAndZoom.desiredFocusX += moveX * kGridSize * 8;
				dragAndZoom.desiredFocusY += moveY * kGridSize * 10;
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
				for (auto nodeId : selectedNodes)
				{
					graph->removeNode(nodeId);
				}
				
				selectedNodes.clear();
				
				for (auto linkId : selectedLinks)
				{
					graph->removeLink(linkId);
				}
				
				selectedLinks.clear();
			}
			
			if (keyboard.wentDown(SDLK_TAB))
			{
				state = kState_Hidden;
				break;
			}
			
			// drag and zoom
			
			if (keyboard.isDown(SDLK_LCTRL))
			{
				dragAndZoom.desiredFocusX -= mouse.dx / dragAndZoom.zoom;
				dragAndZoom.desiredFocusY -= mouse.dy / dragAndZoom.zoom;
				dragAndZoom.focusX = dragAndZoom.desiredFocusX;
				dragAndZoom.focusY = dragAndZoom.desiredFocusY;
			}
			
			if (keyboard.isDown(SDLK_LALT))
			{
				dragAndZoom.desiredZoom += mouse.dy / 100.f;
				dragAndZoom.zoom = dragAndZoom.desiredZoom;
			}
			
			dragAndZoom.tick(dt);
		}
		break;
		
	case kState_NodeSelect:
		{
			nodeSelect.endX = mousePosition.x;
			nodeSelect.endY = mousePosition.y;
			
			// hit test nodes
			
			nodeSelect.nodeIds.clear();
			
			for (auto & nodeItr : graph->nodes)
			{
				auto & node = nodeItr.second;
				
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
			if (mouse.wentUp(BUTTON_LEFT))
			{
				state = kState_Idle;
				break;
			}
			
			for (auto i : selectedNodes)
			{
				auto node = tryGetNode(i);
				
				if (node != nullptr)
				{
					node->editorX += mouse.dx;
					node->editorY += mouse.dy;
				}
			}
		}
		break;
		
	case kState_InputSocketConnect:
		{
			socketConnect.dstNodeId = kGraphNodeIdInvalid;
			socketConnect.dstNodeSocket = nullptr;
			
			HitTestResult hitTestResult;
			
			if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
			{
				if (hitTestResult.hasNode &&
					hitTestResult.node->id != socketConnect.srcNodeId &&
					hitTestResult.nodeHitTestResult.outputSocket)
				{
					if (socketConnect.srcNodeSocket->canConnectTo(*hitTestResult.nodeHitTestResult.outputSocket))
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
			socketConnect.srcNodeId = kGraphNodeIdInvalid;
			socketConnect.srcNodeSocket = nullptr;
			
			HitTestResult hitTestResult;
			
			if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
			{
				if (hitTestResult.hasNode &&
					hitTestResult.node->id != socketConnect.dstNodeId &&
					hitTestResult.nodeHitTestResult.inputSocket)
				{
					if (socketConnect.dstNodeSocket->canConnectTo(*hitTestResult.nodeHitTestResult.inputSocket))
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
			
	case kState_Hidden:
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				state = kState_Idle;
				break;
			}
		}
		break;
	}
	
	if (graph != nullptr)
	{
		for (auto & nodeItr : graph->nodes)
		{
			auto & node = nodeItr.second;
			
			//
			
			const int activity = realTimeConnection == nullptr ? 0 : realTimeConnection->nodeIsActive(node.id);
			
			if (activity & GraphEdit_RealTimeConnection::kActivity_OneShot)
			{
				node.editorIsActiveAnimTime = .2f;
				node.editorIsActiveAnimTimeRcp = 1.f / node.editorIsActiveAnimTime;
			}
			
			node.editorIsActiveContinuous = (activity & GraphEdit_RealTimeConnection::kActivity_Continuous) != 0;
			
			//
			
			node.tick(dt);
		}
	}
	
	return inputIsCaptured;
}

void GraphEdit::nodeSelectEnd()
{
	selectedNodes = nodeSelect.nodeIds;
	selectedLinks.clear(); // todo : also select links
	
	nodeSelect = NodeSelect();
}

void GraphEdit::socketConnectEnd()
{
	if (socketConnect.srcNodeId != kGraphNodeIdInvalid && socketConnect.dstNodeId != kGraphNodeIdInvalid)
	{
		GraphNodeSocketLink link;
		link.id = graph->allocLinkId();
		link.srcNodeId = socketConnect.srcNodeId;
		link.srcNodeSocketName = socketConnect.srcNodeSocket->name;
		link.srcNodeSocketIndex = socketConnect.srcNodeSocket->index;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketName = socketConnect.dstNodeSocket->name;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		
		graph->addLink(link, true);
		
		selectLink(link.id);
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
		}
		
		if (doButton("save", size * 1, size, false))
		{
			save(documentInfo.filename.c_str());
		}
		
		if (doButton("save as", size * 2, size, true))
		{
			save(documentInfo.filename.c_str());
		}
		
		doTextBox(documentInfo.filename, "filename", dt);
		
		doBreak();
	}
	popMenu();
}

void GraphEdit::doEditorOptions(const float dt)
{
	pushMenu("editorOptions");
	{
		doLabel("editor options", 0.f);
		
		if (doDrawer(editorOptions.menuIsVisible, "options"))
		{
			doCheckBox(editorOptions.showBackground, "show background", false);
			doCheckBox(editorOptions.showGrid, "show grid", false);
			doParticleColor(editorOptions.backgroundColor, "background color");
			doParticleColor(editorOptions.gridColor, "grid color");
			doCheckBox(editorOptions.snapToGrid, "snap to grid", false);
			doCheckBox(editorOptions.showOneShotActivity, "show one-shot activity", false);
			doCheckBox(editorOptions.showContinuousActivity, "show continuous activity", false);
			
			if (uiState->activeColor == &editorOptions.gridColor ||
				uiState->activeColor == &editorOptions.backgroundColor)
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
	else
	{
		GraphNode node;
		node.id = graph->allocNodeId();
		node.typeName = typeName;
		node.editorX = x;
		node.editorY = y;
		
		graph->addNode(node);
		
		selectNode(node.id);
		
		return true;
	}
}

void GraphEdit::selectNode(const GraphNodeId nodeId)
{
	Assert(selectedNodes.count(nodeId) == 0);
	selectedNodes.clear();
	selectedLinks.clear();
	
	selectedNodes.insert(nodeId);
}

void GraphEdit::selectLink(const GraphLinkId linkId)
{
	Assert(selectedLinks.count(linkId) == 0);
	selectedNodes.clear();
	selectedLinks.clear();
	
	selectedLinks.insert(linkId);
}

void GraphEdit::selectNodeAll()
{
	selectedNodes.clear();
	//selectedLinks.clear();
	
	for (auto & nodeItr : graph->nodes)
		selectedNodes.insert(nodeItr.first);
}

void GraphEdit::selectLinkAll()
{
	//selectedNodes.clear();
	selectedLinks.clear();
	
	for (auto & linkItr : graph->links)
		selectedLinks.insert(linkItr.first);
}

void GraphEdit::selectAll()
{
	selectNodeAll();
	selectLinkAll();
}

void GraphEdit::draw() const
{
	if (state == kState_Hidden)
		return;
	
	gxPushMatrix();
	gxMultMatrixf(dragAndZoom.transform.m_v);
	
	// draw background and grid
	
	{
		const Vec2 p1 = dragAndZoom.invTransform * Vec2(0.f, 0.f);
		const Vec2 p2 = dragAndZoom.invTransform * Vec2(GFX_SX, GFX_SY);
		
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
			const int cx2 = std::floor(p2[0] / kGridSize);
			const int cy2 = std::floor(p2[1] / kGridSize);
			
			setColorf(
				editorOptions.gridColor.rgba[0],
				editorOptions.gridColor.rgba[1],
				editorOptions.gridColor.rgba[2],
				editorOptions.gridColor.rgba[3]);
			hqBegin(HQ_LINES);
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

	// traverse links and draw
	
	for (auto & linkItr : graph->links)
	{
		auto linkId = linkItr.first;
		auto & link = linkItr.second;
		
		auto srcNode = tryGetNode(link.srcNodeId);
		auto dstNode = tryGetNode(link.dstNodeId);
		
		auto inputSocket = tryGetInputSocket(link.srcNodeId, link.srcNodeSocketIndex);
		auto outputSocket = tryGetOutputSocket(link.dstNodeId, link.dstNodeSocketIndex);
		
		if (srcNode == nullptr ||
			dstNode == nullptr ||
			inputSocket == nullptr ||
			outputSocket == nullptr)
		{
			// todo : error
		}
		else
		{
			hqBegin(HQ_LINES);
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
				
				auto srcTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(srcNode->typeName);
				auto dstTypeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(dstNode->typeName);
				
				const float srcSy = srcTypeDefinition == nullptr ? 0.f : srcTypeDefinition->syFolded;
				const float dstSy = dstTypeDefinition == nullptr ? 0.f : dstTypeDefinition->syFolded;
				
				const float srcX = srcNode->editorX + inputSocket->px;
				const float srcY = srcNode->editorY + (srcNode->editorIsFolded ? srcSy/2.f : inputSocket->py);
				const float dstX = dstNode->editorX + outputSocket->px;
				const float dstY = dstNode->editorY + (dstNode->editorIsFolded ? dstSy/2.f : outputSocket->py);
				
				hqLine(
					srcX, srcY, 2.f,
					dstX, dstY, 4.f);
			}
			hqEnd();
		}
	}
	
	// traverse nodes and draw
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		
		const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefinition == nullptr)
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
				
				drawTypeUi(node, *typeDefinition);
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
	
	case kState_InputSocketConnect:
		{
			auto node = tryGetNode(socketConnect.srcNodeId);
			
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
		
	case kState_Hidden:
		break;
	}

	gxPopMatrix();
	
	switch (state)
	{
	case kState_Idle:
		{
			if (realTimeSocketCapture.nodeId != kGraphNodeIdInvalid)
			{
				gxPushMatrix();
				{
					gxTranslatef(mousePosition.uiX + 15, mousePosition.uiY, 0);
					
					const int kFontSize = 12;
					const int kPadding = 8;
					const int kElemPadding = 4;
					
					std::string caption;
					
					if (realTimeSocketCapture.srcSocketIndex != -1)
					{
						auto srcSocket = tryGetInputSocket(realTimeSocketCapture.nodeId, realTimeSocketCapture.srcSocketIndex);
						
						if (srcSocket != nullptr)
							caption = srcSocket->name;
					}
					
					if (realTimeSocketCapture.dstSocketIndex != -1)
					{
						auto dstSocket = tryGetOutputSocket(realTimeSocketCapture.nodeId, realTimeSocketCapture.dstSocketIndex);
						
						if (dstSocket != nullptr)
							caption = dstSocket->name;
					}
					
					float captionSx;
					float captionSy;
					measureText(kFontSize, captionSx, captionSy, "%s", caption.c_str());
					
					//
					
					const std::string & value = realTimeSocketCapture.value;
					const bool hasValue = realTimeSocketCapture.hasValue;
					
					float valueSx;
					float valueSy;
					measureText(kFontSize, valueSx, valueSy, "%s", value.c_str());
					
					//
					
					int sx = std::max(120, std::max(int(captionSx), int(valueSx)));
					int sy = kPadding;
					
					sy += kFontSize; // caption
					sy += kElemPadding + kFontSize; // value
					
					//
					
					float graphMin;
					float graphMax;
					
					const bool hasGraph =
						realTimeSocketCapture.history.getRange(graphMin, graphMax) &&
						graphMin != graphMax;
					
					const int graphSx = realTimeSocketCapture.history.kMaxHistory;
					const int graphSy = 50;
					
					if (hasGraph)
					{
						sx = std::max(sx, graphSx);
						sy += kElemPadding + graphSy;
					}
					
					//
					
					const bool hasTexture = realTimeSocketCapture.texture != 0;
					
					int textureSx = 200;
					int textureSy = 200;
					
					if (hasTexture)
					{
						GLint baseTextureSx;
						GLint baseTextureSy;
						gxSetTexture(realTimeSocketCapture.texture);
						glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &baseTextureSx);
						glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &baseTextureSy);
						gxSetTexture(0);
						
						const float scaleX = textureSx / float(baseTextureSx);
						const float scaleY = textureSy / float(baseTextureSy);
						const float scale = std::min(scaleX, scaleY);
						textureSx = std::floor(baseTextureSx * scale);
						textureSy = std::floor(baseTextureSy * scale);
						
						sx = std::max(sx, textureSx);
						sy += kElemPadding + textureSy;
					}
					
					//
					
					sx += kPadding * 2;
					sy += kPadding;
					
					//
					
					setColor(31, 31, 31);
					drawRect(0, 0, sx, sy);
					
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
						const int xOffset = realTimeSocketCapture.history.kMaxHistory - realTimeSocketCapture.history.historySize;
						
						for (int i = 0; i < realTimeSocketCapture.history.historySize; ++i)
						{
							const float value = realTimeSocketCapture.history.getGraphValue(i);
							
							const float plotX = graphX + i + xOffset;
							const float plotY = (value - graphMax) / (graphMin - graphMax);
							
							setColor(127, 127, 255);
							drawLine(plotX, y + graphSy, plotX, y + plotY * graphSy);
						}
						
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
						
						setColor(colorWhite);
						gxSetTexture(realTimeSocketCapture.texture);
						{
							drawRect(textureX, y, textureX + textureSx, y + textureSy);
						}
						gxSetTexture(0);
						
						setColor(colorWhite);
						drawRectLine(textureX, y, textureX + textureSx, y + textureSy);
						
						y += textureSy;
					}
					
					y += kPadding;
					fassert(y == sy);
				}
				gxPopMatrix();
			}
		}
		break;
		
	case kState_NodeSelect:
		break;
		
	case kState_NodeDrag:
		break;
		
	case kState_InputSocketConnect:
		break;
		
	case kState_OutputSocketConnect:
		break;
		
	case kState_Hidden:
		break;
	}
	
	if (propertyEditor != nullptr)
	{
		propertyEditor->draw();
	}
	
	makeActive(uiState, false, true);
	
	GraphEdit * self = const_cast<GraphEdit*>(this);
	
	self->doMenu(0.f);
	
	self->doEditorOptions(0.f);
	
	if (nodeTypeNameSelect != nullptr)
	{
		nodeTypeNameSelect->doMenus(uiState, 0.f);
	}
}

void GraphEdit::drawTypeUi(const GraphNode & node, const GraphEdit_TypeDefinition & definition) const
{
	const bool isEnabled = node.isEnabled;
	const bool isSelected = selectedNodes.count(node.id) != 0;
	const bool isFolded = node.editorIsFolded;
	
	const float nodeSy = Calc::Lerp(definition.syFolded, definition.sy, isFolded ? node.editorFoldAnimTime * node.editorFoldAnimTimeRcp : 1.f - node.editorFoldAnimTime * node.editorFoldAnimTimeRcp);
	
	if (!isEnabled)
		setColor(191, 191, 191, 255);
	else if (isSelected)
		setColor(63, 63, 127, 255);
	else
		setColor(63, 63, 63, 255);
	drawRect(0.f, 0.f, definition.sx, nodeSy);
	
	if (editorOptions.showOneShotActivity)
	{
		const float activeAnim = node.editorIsActiveAnimTime * node.editorIsActiveAnimTimeRcp;
		setColor(63, 63, 255, 255 * activeAnim);
		drawRect(0.f, 0.f, definition.sx, nodeSy);
	}
	
	if (editorOptions.showContinuousActivity)
	{
		if (node.editorIsActiveContinuous)
		{
			setColor(63, 63, 255, 127 + 127 * (std::cos(framework.time * 8.f) + 1.f) / 2.f);
			drawRect(0.f, 0.f, definition.sx, nodeSy);
		}
	}
	
	if (isSelected)
		setColor(255, 255, 255, 255);
	else
		setColor(127, 127, 127, 255);
	drawRectLine(0.f, 0.f, definition.sx, nodeSy);
	
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(definition.sx/2, 12, 14, 0.f, 0.f, "%s", !node.editorName.empty() ? node.editorName.c_str() : definition.displayName.empty() ? definition.typeName.c_str() : definition.displayName.c_str());
	
	if (node.editorIsPassthrough)
	{
		setFont("calibri.ttf");
		setColor(127, 127, 255);
		drawText(definition.sx - 8, 12, 14, -1.f, 0.f, "P");
	}
	
	if (isFolded == false)
	{
		for (auto & inputSocket : definition.inputSockets)
		{
			setFont("calibri.ttf");
			setColor(255, 255, 255);
			drawText(inputSocket.px + inputSocket.radius + 2, inputSocket.py, 12, +1.f, 0.f, "%s", inputSocket. name.c_str());
		}
		
		for (auto & outputSocket : definition.outputSockets)
		{
			setFont("calibri.ttf");
			setColor(255, 255, 255);
			drawText(outputSocket.px - outputSocket.radius - 2, outputSocket.py, 12, -1.f, 0.f, "%s", outputSocket.name.c_str());
		}
		
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
				else if (state == kState_OutputSocketConnect && node.id != socketConnect.dstNodeId && inputSocket.canConnectTo(*socketConnect.dstNodeSocket))
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
				else if (state == kState_InputSocketConnect && node.id != socketConnect.srcNodeId && outputSocket.canConnectTo(*socketConnect.srcNodeSocket))
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
		editorOptions.showBackground = boolAttrib(editorOptionsElem, "showBackground", defaultOptions.showBackground);
		editorOptions.showGrid = boolAttrib(editorOptionsElem, "showGrid", defaultOptions.showGrid);
		editorOptions.snapToGrid = boolAttrib(editorOptionsElem, "snapToGrid", defaultOptions.snapToGrid);
		editorOptions.showOneShotActivity = boolAttrib(editorOptionsElem, "showOneShotActivity", defaultOptions.showOneShotActivity);
		editorOptions.showContinuousActivity = boolAttrib(editorOptionsElem, "showContinuousActivity", defaultOptions.showContinuousActivity);
		
		const std::string defaultBackgroundColor = toColor(defaultOptions.backgroundColor).toHexString(true);
		const std::string defaultGridColor = toColor(defaultOptions.gridColor).toHexString(true);
		
		editorOptions.backgroundColor = toParticleColor(Color::fromHex(stringAttrib(editorOptionsElem, "backgroundColor", defaultBackgroundColor.c_str())));
		editorOptions.gridColor = toParticleColor(Color::fromHex(stringAttrib(editorOptionsElem, "gridColor", defaultGridColor.c_str())));
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
		editorElem.PushAttribute("showBackground", editorOptions.showBackground);
		editorElem.PushAttribute("showGrid", editorOptions.showGrid);
		editorElem.PushAttribute("snapToGrid", editorOptions.snapToGrid);
		editorElem.PushAttribute("showOneShotActivity", editorOptions.showOneShotActivity);
		editorElem.PushAttribute("showContinuousActivity", editorOptions.showContinuousActivity);
		
		const std::string backgroundColor = toColor(editorOptions.backgroundColor).toHexString(true);
		const std::string gridColor = toColor(editorOptions.gridColor).toHexString(true);
		
		editorElem.PushAttribute("backgroundColor", backgroundColor.c_str());
		editorElem.PushAttribute("gridColor", gridColor.c_str());
	}
	editorElem.CloseElement();
	
	return true;
}

void GraphEdit::nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
{
	if (realTimeConnection)
		realTimeConnection->nodeAdd(nodeId, typeName);
}

void GraphEdit::nodeRemove(const GraphNodeId nodeId)
{
	if (realTimeConnection)
		realTimeConnection->nodeRemove(nodeId);
}

void GraphEdit::linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (realTimeConnection)
		realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
}

void GraphEdit::linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (realTimeConnection)
		realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
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

static bool doMenuItem(std::string & valueText, const std::string & name, const std::string & editor, const float dt, const int index, ParticleColor * uiColors, const int maxUiColors)
{
	if (editor == "textbox")
	{
		doTextBox(valueText, name.c_str(), dt);
		
		return !valueText.empty();
	}
	else if (editor == "textbox_int")
	{
		int value = Parse::Int32(valueText);
		
		doTextBox(value, name.c_str(), dt);
		
		valueText = String::ToString(value);
		
		return value != 0;
	}
	else if (editor == "textbox_float")
	{
		float value = Parse::Float(valueText);
		
		doTextBox(value, name.c_str(), dt);
		
		valueText = String::FormatC("%f", value);
		
		return value != 0.f;
	}
	else if (editor == "checkbox")
	{
		bool value = Parse::Bool(valueText);
		
		doCheckBox(value, name.c_str(), false);
		
		valueText = value ? "1" : "0";
		
		return value != false;
	}
	else if (editor == "slider")
	{
		// todo : add doSlider
		
		doTextBox(valueText, name.c_str(), dt);
		
		return !valueText.empty();
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
		}
		
		return true;
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
				
				std::string newValueText = oldValueText;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(inputSocket.typeName);
				
				const bool hasValue = doMenuItem(newValueText, inputSocket.name, valueTypeDefinition == nullptr ? "textbox" : valueTypeDefinition->editor, dt, menuItemIndex, uiColors, kMaxUiColors);
				
				menuItemIndex++;
				
				if (isPreExisting)
				{
					if (!hasValue)
						node->editorInputValues.erase(inputSocket.name);
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
					if (newValueText != oldValueText && !(!hasValue && oldValueText.empty()))
					{
						graphEdit->realTimeConnection->setSrcSocketValue(nodeId, inputSocket.index, inputSocket.name, newValueText);
					}
				}
			}
			
			for (auto & outputSocket : typeDefinition->outputSockets)
			{
				if (!outputSocket.isEditable)
					continue;
				
				std::string & valueText = node->editorValue;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(outputSocket.typeName);
				
				doMenuItem(valueText, outputSocket.name, valueTypeDefinition == nullptr ? "textbox" : valueTypeDefinition->editor, dt, menuItemIndex, uiColors, kMaxUiColors);
				
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

void GraphUi::NodeTypeNameSelect::doMenus(UiState * uiState, const float dt)
{
	pushMenu("nodeTypeSelect");
	{
		doLabel("insert", 0.f);
		
		doTextBox(typeName, "type", dt);
		
		if (!typeName.empty())
		{
			// list recommendations
			
			// todo : store this list in the type definition library ?
			
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
			
			std::vector<TypeNameAndScore> typeNamesAndScores;
			
			int index = 0;
			
			for (auto & typeDefenition : graphEdit->typeDefinitionLibrary->typeDefinitions)
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

void GraphUi::NodeTypeNameSelect::selectTypeName(const std::string & _typeName)
{
	typeName = _typeName;
	
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
	
	//
	
	graphEdit->tryAddNode(typeName, graphEdit->dragAndZoom.focusX, graphEdit->dragAndZoom.focusY, true);
}

std::string & GraphUi::NodeTypeNameSelect::getNodeTypeName()
{
	return typeName;
}
