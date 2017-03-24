#include "Debugging.h"
#include "graph.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <cmath>

using namespace tinyxml2;

//

GraphNodeId kGraphNodeIdInvalid = 0;

//

static bool areCompatibleSocketLinkTypeNames(const std::string & srcTypeName, const std::string & dstTypeName)
{
	if (srcTypeName == dstTypeName)
		return true;
	
	if (srcTypeName == "int")
	{
		if (dstTypeName == "int" || dstTypeName == "intLiteral")
			return true;
	}
	
	if (srcTypeName == "intLiteral")
	{
		if (dstTypeName == "intLiteral")
			return true;
	}
	
	if (srcTypeName == "float")
	{
		if (dstTypeName == "float" || dstTypeName == "floatLiteral")
			return true;
	}
	
	if (srcTypeName == "floatLiteral")
	{
		if (dstTypeName == "floatLiteral")
			return true;
	}
	
	if (srcTypeName == "string")
	{
		if (dstTypeName == "string" || dstTypeName == "stringLiteral")
			return true;
	}
	
	if (srcTypeName == "stringLiteral")
	{
		if (dstTypeName == "stringLiteral")
			return true;
	}
	
	return false;
}

//

GraphNode::GraphNode()
	: id(kGraphNodeIdInvalid)
	, type()
	, editorX(0.f)
	, editorY(0.f)
{
}

//

Graph::Graph()
	: nodes()
	, links()
	, nextId(1)
{
}

Graph::~Graph()
{
}

GraphNodeId Graph::allocId()
{
	GraphNodeId result = nextId;
	
	nextId++;
	
	Assert(nodes.find(result) == nodes.end());
	
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
	
	for (auto i = links.begin(); i != links.end(); )
	{
		if (i->srcNodeId == nodeId)
			i = links.erase(i);
		else if (i->dstNodeId == nodeId)
			i = links.erase(i);
		else
			++i;
	}
}

bool Graph::loadXml(const char * xml)
{
	return false;
}

bool Graph::saveXml(std::string & xml) const
{
	return false;
}

//

#include "framework.h"

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
	
	py += 5.f;
	
	// (typeName label)
	
	py += 15.f;
	
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
	
	//
	
	sx = 100.f;
	sy = py;
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

void GraphEdit_TypeDefinition::loadXml(const tinyxml2::XMLElement * xmlType)
{
	typeName = stringAttrib(xmlType, "typeName", "");
	
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
		socket.displayName = stringAttrib(xmlInput, "displayName", socket.displayName.c_str());
		
		inputSockets.push_back(socket);
	}
	
	for (auto xmlOutput = xmlType->FirstChildElement("output"); xmlOutput != nullptr; xmlOutput = xmlOutput->NextSiblingElement("output"))
	{
		OutputSocket socket;
		socket.typeName = stringAttrib(xmlOutput, "typeName", socket.typeName.c_str());
		socket.displayName = stringAttrib(xmlOutput, "displayName", socket.displayName.c_str());
		
		outputSockets.push_back(socket);
	}
}

//

void GraphEdit_TypeDefinitionLibrary::loadXml(const XMLElement * xmlLibrary)
{
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

GraphEdit::GraphEdit()
	: graph(nullptr)
	, typeDefinitionLibrary(nullptr)
	, selectedNodes()
	, state(kState_Idle)
{
	graph = new Graph();
}

GraphEdit::~GraphEdit()
{
	selectedNodes.clear();
	
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
	auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->type);
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
	auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node->type);
	if (typeDefinition == nullptr)
		return nullptr;
	if (socketIndex < 0 || socketIndex >= typeDefinition->outputSockets.size())
		return nullptr;
	return &typeDefinition->outputSockets[socketIndex];
}

bool GraphEdit::hitTest(const float x, const float y, HitTestResult & result) const
{
	result = HitTestResult();
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		
		const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.type);
		
		if (typeDefinition == nullptr)
		{
			// todo : complain ?
		}
		else
		{
			GraphEdit_TypeDefinition::HitTestResult hitTestResult;
			
			if (typeDefinition->hitTest(mouse.x - node.editorX, mouse.y - node.editorY, hitTestResult))
			{
				result.hasNode = true;
				result.node = &node;
				result.nodeHitTestResult = hitTestResult;
				return true;
			}
		}
	}
	
	return false;
}

void GraphEdit::tick(const float dt)
{
	highlightedSockets = SocketSelection();
	
	switch (state)
	{
	case kState_Idle:
		{
			HitTestResult hitTestResult;
			
			if (hitTest(mouse.x, mouse.y, hitTestResult))
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
			}
		
			if (mouse.wentDown(BUTTON_LEFT))
			{
				selectedNodes.clear();
				
				HitTestResult hitTestResult;
				
				if (hitTest(mouse.x, mouse.y, hitTestResult))
				{
					if (hitTestResult.hasNode)
					{
						selectedNodes.insert(hitTestResult.node->id);
						
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
						
						if (hitTestResult.nodeHitTestResult.background)
						{
							state = kState_NodeDrag;
							break;
						}
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_i))
			{
				char typeName[256];
				sprintf(typeName, "effect_%03d", rand() % 5);
				
				if (keyboard.isDown(SDLK_LSHIFT))
					sprintf(typeName, "intLiteral");
				if (keyboard.isDown(SDLK_RSHIFT))
					sprintf(typeName, "fsfx");
				
				GraphNode node;
				node.id = graph->allocId();
				node.type = typeName;
				node.editorX = mouse.x;
				node.editorY = mouse.y;
				
				graph->addNode(node);
			}
			
			if (keyboard.wentDown(SDLK_BACKSPACE))
			{
				for (auto nodeId : selectedNodes)
				{
					graph->removeNode(nodeId);
				}
				
				selectedNodes.clear();
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
			
			if (hitTest(mouse.x, mouse.y, hitTestResult))
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
			
			if (hitTest(mouse.x, mouse.y, hitTestResult))
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
	}
}

void GraphEdit::socketConnectEnd()
{
	if (socketConnect.srcNodeId != kGraphNodeIdInvalid && socketConnect.dstNodeId != kGraphNodeIdInvalid)
	{
		GraphNodeSocketLink link;
		link.srcNodeId = socketConnect.srcNodeId;
		link.srcNodeSocketIndex = socketConnect.srcNodeSocket->index;
		link.dstNodeId = socketConnect.dstNodeId;
		link.dstNodeSocketIndex = socketConnect.dstNodeSocket->index;
		graph->links.push_back(link);
	}
}

void GraphEdit::draw() const
{
	// traverse links and draw
	
	for (auto & link : graph->links)
	{
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
				setColor(255, 255, 0);
				hqLine(
					srcNode->editorX + inputSocket->px, srcNode->editorY + inputSocket->py, 2.f,
					dstNode->editorX + outputSocket->px, dstNode->editorY + outputSocket->py, 2.f);
			}
			hqEnd();
		}
	}
	
	// traverse nodes and draw
	
	for (auto & nodeItr : graph->nodes)
	{
		auto & node = nodeItr.second;
		
		const auto typeDefinition = typeDefinitionLibrary->tryGetTypeDefinition(node.type);
		
		if (typeDefinition == nullptr)
		{
			// todo : draw error node ?
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
						mouse.x, mouse.y, 1.5f);
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
						mouse.x, mouse.y, 1.5f);
				}
				hqEnd();
			}
		}
		break;
	}
}

void GraphEdit::drawTypeUi(const GraphNode & node, const GraphEdit_TypeDefinition & definition) const
{
	setColor(63, 63, 63, 255);
	drawRect(0.f, 0.f, definition.sx, definition.sy);
	
	if (selectedNodes.count(node.id) != 0)
		setColor(255, 255, 255, 255);
	else
		setColor(127, 127, 127, 255);
	drawRectLine(0.f, 0.f, definition.sx, definition.sy);
	
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(definition.sx/2, 12, 14, 0.f, 0.f, "%s", definition.typeName.c_str());
	
	for (auto & editor : definition.editors)
	{
		setFont("calibri.ttf");
		setColor(255, 255, 255);
		drawText(editor.editorX + editor.editorSx/2, editor.editorY + editor.editorSy/2, 12, 0.f, 0.f, "%s", editor.typeName.c_str());
		
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
		drawText(inputSocket.px + inputSocket.radius + 2, inputSocket.py, 12, +1.f, 0.f, "%s", inputSocket.displayName.c_str());
	}
	
	for (auto & outputSocket : definition.outputSockets)
	{
		setFont("calibri.ttf");
		setColor(255, 255, 255);
		drawText(outputSocket.px - outputSocket.radius - 2, outputSocket.py, 12, -1.f, 0.f, "%s", outputSocket.displayName.c_str());
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
