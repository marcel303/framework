#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

typedef unsigned int GraphNodeId;

extern GraphNodeId kGraphNodeIdInvalid;

struct GraphNode
{
	GraphNodeId id;
	std::string type;
	
	float editorX;
	float editorY;
	
	GraphNode();
};

struct GraphNodeSocketLink
{
	GraphNodeId srcNodeId;
	int srcNodeSocketIndex;
	
	GraphNodeId dstNodeId;
	int dstNodeSocketIndex;
	
	// todo : move to cpp
	GraphNodeSocketLink()
		: srcNodeId(kGraphNodeIdInvalid)
		, srcNodeSocketIndex(-1)
		, dstNodeId(kGraphNodeIdInvalid)
		, dstNodeSocketIndex(-1)
	{
	}
};

struct Graph
{
	std::map<GraphNodeId, GraphNode> nodes;
	std::vector<GraphNodeSocketLink> links;
	
	GraphNodeId nextId;
	
	Graph();
	~Graph();
	
	GraphNodeId allocId();
	
	void addNode(GraphNode & node);
	void removeNode(const GraphNodeId nodeId);
	
	bool loadXml(const tinyxml2::XMLElement * xmlGraph);
	bool saveXml(tinyxml2::XMLPrinter & xmlGraph) const;
};

//

struct GraphEdit_Editor
{
	std::string typeName;
	int outputSocketIndex;
	
	// editor
	
	float editorX;
	float editorY;
	float editorSx;
	float editorSy;
	
	GraphEdit_Editor()
		: typeName()
		, outputSocketIndex(-1)
		, editorX(0.f)
		, editorY(0.f)
		, editorSx(0.f)
		, editorSy(0.f)
	{
	}
};

//

struct GraphEdit_TypeDefinition
{
	struct InputSocket;
	struct OutputSocket;
	
	struct InputSocket
	{
		std::string typeName;
		
		// ui
		
		std::string displayName;
		
		int index;
		float px;
		float py;
		float radius;
		
		InputSocket()
			: typeName()
			, displayName()
			, index(-1)
			, px(0.f)
			, py(0.f)
			, radius(0.f)
		{
		}
		
		bool canConnectTo(const OutputSocket & socket) const;
	};
	
	struct OutputSocket
	{
		std::string typeName;
		
		// ui
		
		std::string displayName;
		
		int index;
		float px;
		float py;
		float radius;
		
		OutputSocket()
			: typeName()
			, displayName()
			, index(-1)
			, px(0.f)
			, py(0.f)
			, radius(0.f)
		{
		}
		
		bool canConnectTo(const InputSocket & socket) const;
	};
	
	struct HitTestResult
	{
		const GraphEdit_Editor * editor;
		const InputSocket * inputSocket;
		const OutputSocket * outputSocket;
		bool background;
		
		// todo : move to cpp
		HitTestResult()
			: editor(nullptr)
			, inputSocket(nullptr)
			, outputSocket(nullptr)
			, background(false)
		{
		}
	};
	
	std::string typeName;
	
	std::vector<GraphEdit_Editor> editors;
	std::vector<InputSocket> inputSockets;
	std::vector<OutputSocket> outputSockets;
	
	// ui
	
	float sx;
	float sy;
	
	// todo : move to cpp
	GraphEdit_TypeDefinition()
		: typeName()
		, editors()
		, inputSockets()
		, outputSockets()
		, sx(0.f)
		, sy(0.f)
	{
	}
	
	void createUi();
	
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	void loadXml(const tinyxml2::XMLElement * xmlLibrary);
};

struct GraphEdit_TypeDefinitionLibrary
{
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	
	// todo : move to cpp
	GraphEdit_TypeDefinitionLibrary()
		: typeDefinitions()
	{
	}
	
	// todo : move to cpp
	const GraphEdit_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const
	{
		auto i = typeDefinitions.find(typeName);
		
		if (i != typeDefinitions.end())
			return &i->second;
		else
			return nullptr;
	}
	
	void loadXml(const tinyxml2::XMLElement * xmlLibrary);
};

//

struct GraphEdit
{
	enum State
	{
		kState_Idle,
		kState_NodeDrag,
		kState_InputSocketConnect,
		kState_OutputSocketConnect
	};
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		GraphEdit_TypeDefinition::HitTestResult nodeHitTestResult;
		
		HitTestResult()
			: hasNode(false)
			, node(nullptr)
			, nodeHitTestResult()
		{
		}
	};
	
	struct SocketSelection
	{
		GraphNodeId srcNodeId;
		const GraphEdit_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const GraphEdit_TypeDefinition::OutputSocket * dstNodeSocket;
		
		SocketSelection()
			: srcNodeId(kGraphNodeIdInvalid)
			, srcNodeSocket(nullptr)
			, dstNodeId(kGraphNodeIdInvalid)
			, dstNodeSocket(nullptr)
		{
		}
	};
	
	struct SocketConnect
	{
		GraphNodeId srcNodeId;
		const GraphEdit_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const GraphEdit_TypeDefinition::OutputSocket * dstNodeSocket;
		
		SocketConnect()
			: srcNodeId(kGraphNodeIdInvalid)
			, srcNodeSocket(nullptr)
			, dstNodeId(kGraphNodeIdInvalid)
			, dstNodeSocket(nullptr)
		{
		}
	};
	
	Graph * graph;
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::set<GraphNodeId> selectedNodes;
	
	SocketSelection highlightedSockets;
	SocketSelection selectedSockets;
	
	State state;
	
	SocketConnect socketConnect;
	
	GraphEdit();
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	const GraphEdit_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const GraphEdit_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;

	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	void tick(const float dt);
	void socketConnectEnd();
	
	void draw() const;
	void drawTypeUi(const GraphNode & node, const GraphEdit_TypeDefinition & typeDefinition) const;
};
