#include "Calc.h"
#include "Debugging.h"
#include "graph.h"
#include "ofxDatGUi/ofxDatGui.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <cmath>

using namespace tinyxml2;

extern std::string insertNodeTypeName;

//

GraphNodeId kGraphNodeIdInvalid = 0;
GraphLinkId kGraphLinkIdInvalid = 0;

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
	, editorX(0.f)
	, editorY(0.f)
	, editorIsFolded(false)
	, editorInputValues()
	, editorValue()
	, editorIsPassthrough(false)
{
}

void GraphNode::togglePassthrough()
{
	editorIsPassthrough = !editorIsPassthrough;
}

//

GraphNodeSocketLink::GraphNodeSocketLink()
	: id(kGraphLinkIdInvalid)
	, srcNodeId(kGraphNodeIdInvalid)
	, srcNodeSocketIndex(-1)
	, dstNodeId(kGraphNodeIdInvalid)
	, dstNodeSocketIndex(-1)
{
}

//

Graph::Graph()
	: nodes()
	, links()
	, nextNodeId(1)
	, nextLinkId(1)
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
}

void Graph::removeNode(const GraphNodeId nodeId)
{
	Assert(nodeId != kGraphNodeIdInvalid);
	Assert(nodes.find(nodeId) != nodes.end());
	
	nodes.erase(nodeId);
	
	for (auto linkItr = links.begin(); linkItr != links.end(); )
	{
		auto & link = linkItr->second;
		
		if (link.srcNodeId == nodeId)
			linkItr = links.erase(linkItr);
		else if (link.dstNodeId == nodeId)
			linkItr = links.erase(linkItr);
		else
			++linkItr;
	}
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
				i = links.erase(i);
			}
			else
			{
				++i;
			}
		}
	}
	
	links[link.id] = link;
}

void Graph::removeLink(const GraphLinkId linkId)
{
	Assert(linkId != kGraphLinkIdInvalid);
	Assert(links.find(linkId) != links.end());
	
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

bool Graph::loadXml(const XMLElement * xmlGraph)
{
	nextNodeId = intAttrib(xmlGraph, "nextNodeId", nextNodeId);
	nextLinkId = intAttrib(xmlGraph, "nextLinkId", nextLinkId);
	
	for (const XMLElement * xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
	{
		GraphNode node;
		node.id = intAttrib(xmlNode, "id", node.id);
		node.typeName = stringAttrib(xmlNode, "typeName", node.typeName.c_str());
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
		link.srcNodeId = intAttrib(xmlLink, "srcNodeId", link.srcNodeId);
		link.srcNodeSocketIndex = intAttrib(xmlLink, "srcNodeSocketIndex", link.srcNodeSocketIndex);
		link.dstNodeId = intAttrib(xmlLink, "dstNodeId", link.dstNodeId);
		link.dstNodeSocketIndex = intAttrib(xmlLink, "dstNodeSocketIndex", link.dstNodeSocketIndex);
		
		addLink(link, false);
		
		nextLinkId = std::max(nextLinkId, link.id + 1);
	}
	
	return true;
}

bool Graph::saveXml(XMLPrinter & xmlGraph) const
{
	xmlGraph.PushAttribute("nextNodeId", nextNodeId);
	xmlGraph.PushAttribute("nextLinkId", nextLinkId);
	
	for (auto & nodeItr : nodes)
	{
		auto & node = nodeItr.second;
		
		xmlGraph.OpenElement("node");
		{
			xmlGraph.PushAttribute("id", node.id);
			xmlGraph.PushAttribute("typeName", node.typeName.c_str());
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
			xmlGraph.PushAttribute("srcNodeId", link.srcNodeId);
			xmlGraph.PushAttribute("srcNodeSocketIndex", link.srcNodeSocketIndex);
			xmlGraph.PushAttribute("dstNodeId", link.dstNodeId);
			xmlGraph.PushAttribute("dstNodeSocketIndex", link.dstNodeSocketIndex);
		}
		xmlGraph.CloseElement();
	}
	
	return true;
}

//

#include "framework.h"

void GraphEdit_ValueTypeDefinition::loadXml(const XMLElement * xmlType)
{
	typeName = stringAttrib(xmlType, "typeName", "");
	
	editor = stringAttrib(xmlType, "editor", "textbox");
	editorMin = stringAttrib(xmlType, "editorMin", "0");
	editorMax = stringAttrib(xmlType, "editorMax", "1");
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
	
	// (editors)
	
	for (auto & editor : editors)
	{
		py += 2.f;
		
		editor.editorX = 0.f;
		editor.editorY = py;
		editor.editorSx = 100.f;
		editor.editorSy = 20.f;
		
		py += editor.editorSy;
		
		py += 2.f;
	}
	
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

bool GraphEdit_TypeDefinition::hitTest(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	for (auto & editor : editors)
	{
		if (x >= editor.editorX &&
			y >= editor.editorY &&
			x < editor.editorX + editor.editorSx &&
			y < editor.editorY + editor.editorSy)
		{
			result.editor = &editor;
			return true;
		}
	}
	
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
	
	if (x >= 0.f && y >= 0.f && x < sx && y < sy)
	{
		result.background = true;
		return true;
	}
	
	return false;
}

void GraphEdit_TypeDefinition::loadXml(const XMLElement * xmlType)
{
	typeName = stringAttrib(xmlType, "typeName", "");
	
	displayName = stringAttrib(xmlType, "displayName", "");
	
	for (auto xmlEditor = xmlType->FirstChildElement("editor"); xmlEditor != nullptr; xmlEditor = xmlEditor->NextSiblingElement("editor"))
	{
		GraphEdit_Editor editor;
		editor.typeName = stringAttrib(xmlEditor, "typeName", editor.typeName.c_str());
		editor.outputSocketIndex = intAttrib(xmlEditor, "output", editor.outputSocketIndex);
		
		editors.push_back(editor);
	}
	
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
}

//

void GraphEdit_TypeDefinitionLibrary::loadXml(const XMLElement * xmlLibrary)
{
	for (auto xmlType = xmlLibrary->FirstChildElement("valueType"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("valueType"))
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		
		typeDefinition.loadXml(xmlType);
		
		// todo : check typeName doesn't exist yet
		
		valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	for (auto xmlType = xmlLibrary->FirstChildElement("type"); xmlType != nullptr; xmlType = xmlType->NextSiblingElement("type"))
	{
		GraphEdit_TypeDefinition typeDefinition;
		
		typeDefinition.loadXml(xmlType);
		typeDefinition.createUi();
		
		// todo : check typeName doesn't exist yet
		
		typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

//

bool GraphEdit::SocketValueEdit::processKeyboard()
{
	bool result = false;
	
	for (int i = 0; i <= 9; ++i)
	{
		const SDLKey key = SDLKey(SDLK_0 + i);
		
		if (keyboard.wentDown(key))
		{
			keyboardText.push_back('0' + i);
			
			result = true;
		}
	}
	
	if (keyboard.wentDown(SDLK_PERIOD))
	{
		keyboardText.push_back('.');
		
		result = true;
	}
	
	if (keyboard.wentDown(SDLK_BACKSPACE))
	{
		keyboardText.pop_back();
		
		result = true;
	}
	
	return result;
}

//

GraphEdit::GraphEdit()
	: graph(nullptr)
	, typeDefinitionLibrary(nullptr)
	, selectedNodes()
	, selectedLinks()
	, state(kState_Idle)
	, nodeSelect()
	, socketConnect()
	, socketValueEdit()
	, mousePosition()
	, dragAndZoom()
	, propertyEditor(nullptr)
	, dummyGui(nullptr)
{
	graph = new Graph();
	
	propertyEditor = new GraphUi::PropEdit(nullptr);
	
	dummyGui = new ofxDatGui();
}

GraphEdit::~GraphEdit()
{
	selectedNodes.clear();
	
	delete dummyGui;
	dummyGui = nullptr;
	
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

bool GraphEdit::hitTestUi(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	if (propertyEditor->datGui != nullptr)
	{
		auto p = propertyEditor->datGui->getPosition();
		auto sx = propertyEditor->datGui->getWidth();
		auto sy = propertyEditor->datGui->getHeight();
		
		if (x > p.x && y > p.y && x < p.x + sx && y < p.y + sy)
		{
			result.hasPropEdit = true;
			return true;
		}
	}

	return false;
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
			
			if (typeDefinition->hitTest(x - node.editorX, y - node.editorY, hitTestResult))
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
			if (testLineOverlap(
				srcNode->editorX + srcNodeSocket->px,
				srcNode->editorY + srcNodeSocket->py,
				dstNode->editorX + dstNodeSocket->px,
				dstNode->editorY + dstNodeSocket->py,
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

void GraphEdit::tick(const float dt)
{
	bool enablePropEdit = false;
	
	{
		mousePosition.uiX = mouse.x;
		mousePosition.uiY = mouse.y;
		
		const Vec2 srcMousePosition(mouse.x, mouse.y);
		const Vec2 dstMousePosition = dragAndZoom.invTransform * srcMousePosition;
		mousePosition.x = dstMousePosition[0];
		mousePosition.y = dstMousePosition[1];
	}
	
	highlightedSockets = SocketSelection();
	highlightedLinks.clear();
	
	switch (state)
	{
	case kState_Idle:
		{
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
				
				if (hitTestUi(mousePosition.uiX, mousePosition.uiY, hitTestResult))
				{
					//
				}
				else if (hitTest(mousePosition.x, mousePosition.y, hitTestResult))
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
						
						if (hitTestResult.nodeHitTestResult.editor)
						{
							socketValueEdit.nodeId = hitTestResult.node->id;
							socketValueEdit.editor = hitTestResult.nodeHitTestResult.editor;
							state = kState_SocketValueEdit;
							break;
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
					
					if (hitTestResult.hasPropEdit)
					{
						state = kState_PropEdit;
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
			
			if (keyboard.wentDown(SDLK_i))
			{
				std::string typeName;
				
				if (insertNodeTypeName.empty())
				{
					std::vector<std::string> typeNames;
					for (auto i : typeDefinitionLibrary->typeDefinitions)
						typeNames.push_back(i.first);
					
					typeName = typeNames[rand() % typeNames.size()];
				}
				else
				{
					auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(insertNodeTypeName);
					
					if (typeDefinition != nullptr)
						typeName = insertNodeTypeName;
				}
				
				if (!typeName.empty())
				{
					GraphNode node;
					node.id = graph->allocNodeId();
					node.typeName = typeName;
					node.editorX = mousePosition.x;
					node.editorY = mousePosition.y;
					
					graph->addNode(node);
					
					selectNode(node.id);
				}
			}
			
			if (keyboard.wentDown(SDLK_a))
			{
				if (keyboard.isDown(SDLK_LGUI))
					selectAll();
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
						newNode.editorX = node->editorX + 20;
						newNode.editorY = node->editorY + 20;
						
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
						node->editorIsPassthrough = allPassthrough ? false : true;
					}
				}
			}
			
			int moveX = 0;
			int moveY = 0;
			
			if (keyboard.wentDown(SDLK_LEFT))
				moveX -= 10;
			if (keyboard.wentDown(SDLK_RIGHT))
				moveX += 10;
			if (keyboard.wentDown(SDLK_UP))
				moveY -= 10;
			if (keyboard.wentDown(SDLK_DOWN))
				moveY += 10;
			
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
						node->editorIsFolded = anyUnfolded ? true : false;
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
			
			enablePropEdit = true;
		}
		break;
		
	case kState_NodeSelect:
		{
			nodeSelect.endX = mousePosition.x;
			nodeSelect.endY = mousePosition.y;
			
			// todo : hit test nodes
			
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
						node.editorY + typeDefinition->sy))
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
		
	case kState_SocketValueEdit:
		{
			auto node = tryGetNode(socketValueEdit.nodeId);
			
			if (node == nullptr)
			{
				// todo : complain
			}
			else
			{
				if (socketValueEdit.editor->typeName == "float")
				{
					float value = Parse::Float(node->editorValue);
					
					if (socketValueEdit.mode == SocketValueEdit::kMode_Idle)
					{
						if (mouse.dx || mouse.dy)
						{
							socketValueEdit.mode = SocketValueEdit::kMode_MouseAbsolute;
							break;
						}
						
						if (mouse.wentUp(BUTTON_LEFT))
						{
							socketValueEdit.mode = SocketValueEdit::kMode_MouseRelative;
							break;
						}
					}
					
					if (socketValueEdit.mode == SocketValueEdit::kMode_MouseAbsolute)
					{
						const float x1 = node->editorX + socketValueEdit.editor->editorX;
						const float x2 = x1 + socketValueEdit.editor->editorSx;
						value = (mousePosition.x - x1) / (x2 - x1);
						
						if (mouse.wentUp(BUTTON_LEFT))
						{
							socketValueEditEnd();
							
							state = kState_Idle;
							break;
						}
					}
					
					if (socketValueEdit.mode == SocketValueEdit::kMode_MouseRelative)
					{
						value += -mouse.dy * .01f;
						
						if (socketValueEdit.processKeyboard())
						{
							socketValueEdit.mode = SocketValueEdit::kMode_Keyboard;
							break;
						}
						
						if (mouse.wentDown(BUTTON_LEFT))
						{
							socketValueEditEnd();
							
							state = kState_Idle;
							break;
						}
					}
					
					if (socketValueEdit.mode == SocketValueEdit::kMode_Keyboard)
					{
						socketValueEdit.processKeyboard();
						
						value = Parse::Float(socketValueEdit.keyboardText);
						
						if (keyboard.wentDown(SDLK_RETURN))
						{
							socketValueEditEnd();
							
							state = kState_Idle;
							break;
						}
					}
					
					value = Calc::Clamp(value, 0.f, 1.f);
					
					char valueText[256];
					sprintf_s(valueText, sizeof(valueText), "%f", value);
					
					node->editorValue = valueText;
				}
				
				if (socketValueEdit.editor->typeName == "string")
				{
					if (keyboard.wentDown(SDLK_BACKSPACE))
					{
						if (!node->editorValue.empty())
							node->editorValue.pop_back();
					}
					
					for (int i = 0; i < 256; ++i)
					{
						SDLKey key = SDLKey(i);
						
						int c = i;
						
						if (keyboard.isDown(SDLK_LSHIFT))
							c = toupper(c);
					
						if (isprint(c) && keyboard.wentDown(key))
						{
							node->editorValue.push_back(c);
						}
					}
					
					if (mouse.wentDown(BUTTON_LEFT) || keyboard.wentDown(SDLK_RETURN))
					{
						socketValueEditEnd();
						
						state = kState_Idle;
						break;
					}
				}
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
	
	if (propertyEditor != nullptr)
	{
		if (propertyEditor->datGui != nullptr)
		{
			//logDebug("enablePropEdit: %d", int(enablePropEdit));
			
			ofxDatGui * focusGui = nullptr;
			
			if (enablePropEdit)
				focusGui = propertyEditor->datGui;
			else
				focusGui = dummyGui;
			
			//if (focusGui != nullptr && !focusGui->getFocused())
			//	focusGui->focus();
		}
		
		propertyEditor->tick(dt);
	}
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
		link.srcNodeSocketIndex = socketConnect.srcNodeSocket->index;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		
		graph->addLink(link, true);
		
		selectLink(link.id);
	}
	
	socketConnect = SocketConnect();
}

void GraphEdit::socketValueEditEnd()
{
	socketValueEdit = SocketValueEdit();
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
				const bool isSelected = selectedLinks.count(linkId) != 0;
				const bool isHighlighted = highlightedLinks.count(linkId) != 0;
				
				if (isSelected)
					setColor(127, 127, 255);
				else if (isHighlighted)
					setColor(255, 255, 255);
				else
					setColor(255, 255, 0);
				
				hqLine(
					srcNode->editorX + inputSocket->px, srcNode->editorY + inputSocket->py, 2.f,
					dstNode->editorX + outputSocket->px, dstNode->editorY + outputSocket->py, 4.f);
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
	
	case kState_SocketValueEdit:
		break;
	
	case kState_Hidden:
		break;
	}
	
#if 0
	// todo : remove
	
	for (int x = 0; x < GFX_SX; x += 10)
	{
		for (int y = 0; y < GFX_SY; y += 10)
		{
			HitTestResult hitTestResult;
			
			if (hitTest(x, y, hitTestResult))
			{
				if (hitTestResult.hasLink)
				{
					setColor(colorYellow);
					fillCircle(x, y, 5.f, 10);
				}
				else
				{
					setColor(colorRed);
					fillCircle(x, y, 5.f, 10);
				}
			}
			else
			{
				setColor(colorBlue);
				fillCircle(x, y, 5.f, 10);
			}
		}
	}
#endif

	gxPopMatrix();
	
	if (propertyEditor != nullptr)
	{
		propertyEditor->draw();
	}
}

void GraphEdit::drawTypeUi(const GraphNode & node, const GraphEdit_TypeDefinition & definition) const
{
	const bool isSelected = selectedNodes.count(node.id) != 0;
	const bool isFolded = node.editorIsFolded;
	
	if (isSelected)
		setColor(63, 63, 127, 255);
	else
		setColor(63, 63, 63, 255);
	
	drawRect(0.f, 0.f, definition.sx, isFolded ? definition.syFolded : definition.sy);
	
	if (isSelected)
		setColor(255, 255, 255, 255);
	else
		setColor(127, 127, 127, 255);
	drawRectLine(0.f, 0.f, definition.sx, isFolded ? definition.syFolded : definition.sy);
	
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(definition.sx/2, 12, 14, 0.f, 0.f, "%s", definition.displayName.empty() ? definition.typeName.c_str() : definition.displayName.c_str());
	
	if (node.editorIsPassthrough)
	{
		setFont("calibri.ttf");
		setColor(127, 127, 255);
		drawText(definition.sx - 8, 12, 14, -1.f, 0.f, "P");
	}
	
	if (isFolded == false)
	{
		for (auto & editor : definition.editors)
		{
			if (socketValueEdit.nodeId == node.id && socketValueEdit.editor == &editor)
			{
				setColor(63, 63, 127);
				drawRect(
					editor.editorX,
					editor.editorY,
					editor.editorX + editor.editorSx,
					editor.editorY + editor.editorSy);
			}
			
			if (editor.typeName == "float")
			{
				const float value = Parse::Float(node.editorValue);
				
				setColor(255, 0, 0);
				drawRect(
					editor.editorX,
					editor.editorY,
					editor.editorX + editor.editorSx * value,
					editor.editorY + editor.editorSy);
				
				setFont("calibri.ttf");
				setColor(255, 255, 255);
				drawText(
					editor.editorX + editor.editorSx/2,
					editor.editorY + editor.editorSy/2,
					12, 0.f, 0.f, "%s : %.2f", editor.typeName.c_str(), value);
			}
			
			if (editor.typeName == "string")
			{
				setFont("calibri.ttf");
				setColor(255, 255, 255);
				drawText(
					editor.editorX + editor.editorSx/2,
					editor.editorY + editor.editorSy/2,
					12, 0.f, 0.f, "%s", node.editorValue.c_str());
			}
			
			setColor(127, 127, 127, 255);
			drawRectLine(
				editor.editorX,
				editor.editorY,
				editor.editorX + editor.editorSx,
				editor.editorY + editor.editorSy);
		}
		
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

//

void GraphUi::TextEdit::tick(const float dt)
{
	if (hasFocus)
	{
		std::string newEditText = editText;
		
		if (keyboard.wentDown(SDLK_BACKSPACE))
		{
			if (!newEditText.empty())
			{
				newEditText.pop_back();
			}
		}
		
		for (int c = '0'; c <= '9'; ++c)
		{
			if (keyboard.wentDown((SDLKey)c))
			{
				newEditText.push_back(c);
			}
		}
		
		if (newEditText != editText)
		{
			editText = newEditText;
			
			if (onChange != nullptr)
			{
				onChange(*this);
			}
		}
	}
}

void GraphUi::TextEdit::draw() const
{
	gxPushMatrix();
	{
		gxTranslatef(px, py, 0.f);
		
		if (hasFocus)
		{
			if (editTextIsValid)
				setColor(colorBlue);
			else
				setColor(colorRed);
		}
		else
		{
			setColor(colorBlack);
		}
		drawRect(0.f, 0.f, sx, sy);
		setColor(colorWhite);
		drawRectLine(0.f, 0.f, sx, sy);
		
		setFont("calibri.ttf");
		setColor(colorWhite);
		drawText(sx/2.f, sy/2.f, 12, 0.f, 0.f, "%s", editText.c_str());
	}
	gxPopMatrix();
}

void GraphUi::TextEdit::setHasFocus(const bool _hasFocus)
{
	if (_hasFocus != hasFocus)
	{
		logDebug("setHasFocus: %d -> %d", int(hasFocus), int(_hasFocus));
		
		hasFocus = _hasFocus;
		
		if (hasFocus)
		{
			if (onChange != nullptr)
			{
				onChange(*this);
			}
		}
		else
		{
			if (editTextIsValid)
				realText = editText;
			else
			{
				editText = realText;
				editTextIsValid = true;
			}
		}
	}
}

bool GraphUi::TextEdit::hitTest(const float x, const float y) const
{
	return
		x >= px &&
		y >= py &&
		x < px + sx &&
		y < py + sy;
}

GraphUi::PropEdit::PropEdit(GraphEdit_TypeDefinitionLibrary * _typeLibrary)
	: typeLibrary(nullptr)
	, graph(nullptr)
	, nodeId(kGraphNodeIdInvalid)
	, hasFocus(true)
	, datGui(nullptr)
{
	typeLibrary = _typeLibrary;
}

GraphUi::PropEdit::~PropEdit()
{
	delete datGui;
	datGui = nullptr;
}

void GraphUi::PropEdit::tick(const float dt)
{
	// todo : move event triggering to ofMain
	
	for (auto & e : keyboard.events)
	{
		if (e.type == SDL_KEYDOWN)
		{
			ofKeyEventArgs args;
			
			int c = e.key.keysym.sym;
			
			int caps = 0;
			
			if (e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
				caps = +1;
			if (e.key.keysym.mod & (KMOD_CAPS))
				caps = caps ? -1 : +1;
			
			if (caps == -1)
				c = tolower(c);
			if (caps == +1)
				c = toupper(c);
			
			args.key = c;
			
			ofEvents().keyPressed.notify(args);
		}
	}
	
	{
		ofMouseEventArgs args;
		ofEvents().mouseScrolled.notify(args);
	}
	
	ofEventArgs e;
	
	ofEvents().update.notify(e);
}

void GraphUi::PropEdit::draw() const
{
	// todo : move event triggering to ofMain
	
	ofEventArgs e;
	
	ofEvents().draw.notify(e);
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
		logDebug("setNode: %d", _nodeId);
		
		nodeId = _nodeId;
		
		createUi();
	}
}

void GraphUi::PropEdit::createUi()
{
	delete datGui;
	datGui = nullptr;
	
	datGui = new ofxDatGui(ofxDatGuiAnchor::TOP_RIGHT);
	
	ofxDatGui * gui = datGui;
	
	std::string headerText;
	
	//gui->addButton("save");
	//gui->addButton("load");
	//gui->addBreak();
	
	GraphNode * node = tryGetNode();
	
	if (node != nullptr)
	{
		gui->addLabel(node->typeName);
		
		const GraphEdit_TypeDefinition * typeDefinition = typeLibrary->tryGetTypeDefinition(node->typeName);
		
		if (typeDefinition != nullptr)
		{
			headerText = typeDefinition->typeName;
			
			if (!typeDefinition->displayName.empty())
				headerText = typeDefinition->displayName;
			
			for (auto & inputSocket : typeDefinition->inputSockets)
			{
				const auto valueItr = node->editorInputValues.find(inputSocket.name);
				const std::string valueText = valueItr == node->editorInputValues.end() ? "" : valueItr->second;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(inputSocket.typeName);
				
				ofxDatGuiComponent * component = nullptr;
				
				if (valueTypeDefinition == nullptr)
				{
					component = gui->addTextInput(inputSocket.name, valueText);
				}
				else
				{
					if (valueTypeDefinition->editor == "textbox")
					{
						component = gui->addTextInput(inputSocket.name, valueText);
					}
					else if (valueTypeDefinition->editor == "slider")
					{
						component = gui->addSlider(
							inputSocket.name,
							Parse::Float(valueTypeDefinition->editorMin),
							Parse::Float(valueTypeDefinition->editorMax),
							Parse::Float(valueText));
					}
					else if (valueTypeDefinition->editor == "colorpicker")
					{
						auto colorPicker = gui->addColorPicker(inputSocket.name, ofColor::fromHex(valueText.c_str()));
						
						component = colorPicker;
					}
				}
				
				if (component != nullptr)
				{
					component->setIndex(0);
					component->setName(inputSocket.name);
				}
			}
			
			for (auto & outputSocket : typeDefinition->outputSockets)
			{
				if (!outputSocket.isEditable)
					continue;
				
				const std::string & valueText = node->editorValue;
				
				const GraphEdit_ValueTypeDefinition * valueTypeDefinition = typeLibrary->tryGetValueTypeDefinition(outputSocket.typeName);
				
				ofxDatGuiComponent * component = nullptr;
				
				if (valueTypeDefinition == nullptr)
				{
					component = gui->addTextInput(outputSocket.name, valueText);
				}
				else
				{
					if (valueTypeDefinition->editor == "textbox")
					{
						component = gui->addTextInput(outputSocket.name, valueText);
					}
					else if (valueTypeDefinition->editor == "slider")
					{
						component = gui->addSlider(
							outputSocket.name,
							Parse::Float(valueTypeDefinition->editorMin),
							Parse::Float(valueTypeDefinition->editorMax),
							Parse::Float(valueText));
					}
					else if (valueTypeDefinition->editor == "colorpicker")
					{
						auto colorPicker = gui->addColorPicker(outputSocket.name, ofColor::fromHex(valueText.c_str()));
						
						component = colorPicker;
					}
				}
				
				if (component != nullptr)
				{
					component->setIndex(1);
					component->setName(outputSocket.name);
				}
			}
		}
	}
	
	gui->addBreak();
	
	//gui->addHeader(headerText);
	gui->addFooter();
	
	gui->onTextInputEvent(this, &PropEdit::onTextInputEvent);
	gui->onSliderEvent(this, &PropEdit::onSliderEvent);
	gui->onColorPickerEvent(this, &PropEdit::onColorPickerEvent);
	
	datGui->setTheme(new ofxDatGuiThemeAutumn(), true);
}

GraphNode * GraphUi::PropEdit::tryGetNode()
{
	if (graph == nullptr)
		return nullptr;
	else
		return graph->tryGetNode(nodeId);
}

void GraphUi::PropEdit::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
	GraphNode * node = tryGetNode();
	
	if (node != nullptr)
	{
		if (e.target->getIndex() == 0)
			node->editorInputValues[e.target->getName()] = e.text;
		else
			node->editorValue = e.text;
	}
}

void GraphUi::PropEdit::onSliderEvent(ofxDatGuiSliderEvent e)
{
	GraphNode * node = tryGetNode();
	
	if (node != nullptr)
	{
		const std::string text = String::ToString(e.value);
		
		if (e.target->getIndex() == 0)
			node->editorInputValues[e.target->getName()] = text;
		else
			node->editorValue = text;
	}
}

void GraphUi::PropEdit::onColorPickerEvent(ofxDatGuiColorPickerEvent e)
{
	GraphNode * node = tryGetNode();
	
	if (node != nullptr)
	{
		const std::string hexString = String::Format("%02x%02x%02x%02x",
			e.color.r,
			e.color.g,
			e.color.b,
			e.color.a);
		
		if (e.target->getIndex() == 0)
			node->editorInputValues[e.target->getName()] = hexString;
		else
			node->editorValue = hexString;
	}
}
