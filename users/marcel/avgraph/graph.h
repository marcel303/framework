#pragma once

#include "Mat4x4.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

struct UiState;
struct ParticleColor;

struct GraphEdit;

namespace GraphUi
{
	struct PropEdit;
	struct NodeTypeNameSelect;
};

//

struct GraphEdit_TypeDefinitionLibrary;
struct GraphEdit_Visualizer;

//

typedef unsigned int GraphNodeId;
typedef unsigned int GraphLinkId;

extern GraphNodeId kGraphNodeIdInvalid;
extern GraphLinkId kGraphLinkIdInvalid;

enum GraphNodeType
{
	kGraphNodeType_Regular,
	kGraphNodeType_Visualizer
};

struct GraphNode
{
	struct EditorVisualizer
	{
		GraphNodeId nodeId;
		std::string srcSocketName;
		int srcSocketIndex;
		std::string dstSocketName;
		int dstSocketIndex;
		
		GraphEdit_Visualizer * visualizer;
		
		mutable int sx;
		mutable int sy;
		
		EditorVisualizer();
		EditorVisualizer(const EditorVisualizer & other);
		~EditorVisualizer();
		
		void allocVisualizer();
		
		// nodes (and thus also visualizers) get copied around. we want to copy parameters but not the
		// dynamically allocated visualizer object. so we need a copy constructor/assignment operator
		// to address this and copy the parameters manually and allocate a new visualizer object
		void operator=(const EditorVisualizer & other);
	};
	
	GraphNodeId id;
	GraphNodeType nodeType;
	std::string typeName;
	bool isEnabled;
	bool isPassthrough;
	
	std::string editorName;
	float editorX;
	float editorY;
	bool editorIsFolded;
	float editorFoldAnimTime;
	float editorFoldAnimTimeRcp;
	
	std::map<std::string, std::string> editorInputValues;
	std::string editorValue;
	
	float editorIsActiveAnimTime; // real-time connection node activation animation
	float editorIsActiveAnimTimeRcp;
	bool editorIsActiveContinuous;
	
	EditorVisualizer editorVisualizer;
	
	GraphNode();
	
	void tick(const GraphEdit & graphEdit, const float dt);
	
	const std::string & getDisplayName() const;
	
	void setIsEnabled(const bool isEnabled);
	void setIsPassthrough(const bool isPassthrough);
	void setIsFolded(const bool isFolded);
	
	void setVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex);
};

struct GraphNodeSocketLink
{
	GraphLinkId id;
	bool isEnabled;
	
	GraphNodeId srcNodeId;
	std::string srcNodeSocketName;
	int srcNodeSocketIndex;
	
	GraphNodeId dstNodeId;
	std::string dstNodeSocketName;
	int dstNodeSocketIndex;
	
	GraphNodeSocketLink();
	
	void setIsEnabled(const bool isEnabled);
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
	std::map<GraphLinkId, GraphNodeSocketLink> links;
	
	GraphNodeId nextNodeId;
	GraphLinkId nextLinkId;
	
	GraphEditConnection * graphEditConnection;
	
	Graph();
	~Graph();
	
	GraphNodeId allocNodeId();
	GraphLinkId allocLinkId();
	
	void addNode(GraphNode & node);
	void removeNode(const GraphNodeId nodeId);
	
	void addLink(const GraphNodeSocketLink & link, const bool clearInputDuplicates);
	void removeLink(const GraphLinkId linkId);
	
	GraphNode * tryGetNode(const GraphNodeId nodeId);
	
	bool loadXml(const tinyxml2::XMLElement * xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	bool saveXml(tinyxml2::XMLPrinter & xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary) const;
};

//

struct GraphEdit_ValueTypeDefinition
{
	std::string typeName;
	
	// ui
	
	std::string editor;
	std::string editorMin;
	std::string editorMax;
	std::string visualizer;
	
	GraphEdit_ValueTypeDefinition()
		: typeName()
		, editor()
		, editorMin()
		, editorMax()
		, visualizer()
	{
	}
	
	bool loadXml(const tinyxml2::XMLElement * xmlType);
};

struct GraphEdit_TypeDefinition
{
	struct InputSocket;
	struct OutputSocket;
	
	struct InputSocket
	{
		std::string typeName;
		std::string name;
		std::string defaultValue;
		
		// ui
		
		int index;
		float px;
		float py;
		float radius;
		
		InputSocket()
			: typeName()
			, name()
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
		std::string name;
		bool isEditable;
		
		// ui
		
		int index;
		float px;
		float py;
		float radius;
		
		OutputSocket()
			: typeName()
			, name()
			, isEditable(false)
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
		const InputSocket * inputSocket;
		const OutputSocket * outputSocket;
		bool background;
		
		HitTestResult()
			: inputSocket(nullptr)
			, outputSocket(nullptr)
			, background(false)
		{
		}
	};
	
	std::string typeName;
	
	std::vector<InputSocket> inputSockets;
	std::vector<OutputSocket> outputSockets;
	
	// ui
	
	std::string displayName;
	
	float sx;
	float sy;
	float syFolded;
	
	GraphEdit_TypeDefinition()
		: typeName()
		, inputSockets()
		, outputSockets()
		, displayName()
		, sx(0.f)
		, sy(0.f)
		, syFolded(0.f)
	{
	}
	
	void createUi();
	
	bool hitTest(const float x, const float y, const bool isFolded, HitTestResult & result) const;
	
	bool loadXml(const tinyxml2::XMLElement * xmlNode);
};

struct GraphEdit_TypeDefinitionLibrary
{
	std::map<std::string, GraphEdit_ValueTypeDefinition> valueTypeDefinitions;
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	
	GraphEdit_TypeDefinitionLibrary()
		: valueTypeDefinitions()
		, typeDefinitions()
	{
	}
	
	// todo : move to cpp
	const GraphEdit_ValueTypeDefinition * tryGetValueTypeDefinition(const std::string & typeName) const
	{
		auto i = valueTypeDefinitions.find(typeName);
		
		if (i != valueTypeDefinitions.end())
			return &i->second;
		else
			return nullptr;
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
	
	bool loadXml(const tinyxml2::XMLElement * xmlLibrary);
};

//

#include <list>

struct GraphEdit_UndoBuffer
{
	std::string xml;
	
	GraphEdit_UndoBuffer()
		: xml()
	{
	}
	
	void makeSnapshot(const Graph & graph)
	{
		//tinyxml2::XMLPrinter printer;
		
		//graph.saveXml(printer);
	}
	
	void restoreSnapshot(Graph & graph)
	{
		//graph.loadXml();
	}
};

struct GraphEdit_UndoHistory
{
	std::list<GraphEdit_UndoBuffer> undoBuffers;
	
	GraphEdit_UndoHistory()
		: undoBuffers()
	{
	}
	
	void onInit(const Graph & graph)
	{
		undoBuffers.clear();
		
		undoBuffers.push_back(GraphEdit_UndoBuffer());
		
		GraphEdit_UndoBuffer & undoBuffer = undoBuffers.back();
		
		undoBuffer.makeSnapshot(graph);
	}
	
	void commit(const Graph & graph)
	{
	}
};

//

struct GraphEdit_Visualizer
{
	struct History
	{
		const static int kMaxHistory = 100;
		
		float * history;
		int maxHistorySize;
		
		int historySize;
		int nextWriteIndex;
		
		float min;
		float max;
		
		History()
			: history(nullptr)
			, maxHistorySize(0)
			, historySize(0)
			, nextWriteIndex(0)
			, min(0.f)
			, max(0.f)
		{
		}
		
		~History()
		{
			resize(0);
		}
		
		void resize(const int _maxHistorySize)
		{
			delete[] history;
			history = nullptr;
			maxHistorySize = 0;
			
			if (_maxHistorySize > 0)
			{
				history = new float[_maxHistorySize];
				maxHistorySize = _maxHistorySize;
				historySize = 0;
				nextWriteIndex = 0;
				min = 0.f;
				max = 0.f;
			}
		}
		
		void add(const float value)
		{
			if (historySize == 0)
			{
				min = value;
				max = value;
			}
			else
			{
				if (value < min)
					min = value;
				if (value > max)
					max = value;
			}
			
			if (historySize < kMaxHistory)
				historySize = historySize + 1;
			
			history[nextWriteIndex] = value;
			
			nextWriteIndex++;
			
			if (nextWriteIndex == kMaxHistory)
				nextWriteIndex = 0;
		}
		
		float getGraphValue(const int offset) const
		{
			const int index = (nextWriteIndex - historySize + offset + kMaxHistory) % kMaxHistory;
			
			return history[index];
		}
		
		bool getRange(float & _min, float & _max) const
		{
			if (historySize == 0)
				return false;
			else
			{
				_min = min;
				_max = max;
				
				return true;
			}
		}
	};
	
	GraphNodeId nodeId;
	std::string srcSocketName;
	int srcSocketIndex;
	std::string dstSocketName;
	int dstSocketIndex;
	
	std::string value;
	bool hasValue;
	
	History history;
	
	uint32_t texture;
	
	GraphEdit_Visualizer()
		: nodeId(kGraphNodeIdInvalid)
		, srcSocketName()
		, srcSocketIndex(-1)
		, dstSocketName()
		, dstSocketIndex(-1)
		, value()
		, hasValue(false)
		, history()
		, texture(0)
	{
	}
	
	void init(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex);
	
	void tick(const GraphEdit & graphEdit);
	void draw(const GraphEdit & graphEdit, const std::string & nodeName, const bool isSelected, int * sx, int * sy) const;
};

//

struct GraphEdit_RealTimeConnection
{
	enum ActivityFlags
	{
		kActivity_Inactive    = 0,
		kActivity_OneShot     = 1 << 0,
		kActivity_Continuous  = 1 << 1
	};
	
	virtual ~GraphEdit_RealTimeConnection()
	{
	}
	
	virtual void loadBegin()
	{
	}
	
	virtual void loadEnd(GraphEdit & graphEdit)
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
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough)
	{
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value)
	{
	}
	
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value)
	{
		return false;
	}
	
	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value)
	{
	}
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value)
	{
		return false;
	}
	
	virtual int nodeIsActive(const GraphNodeId nodeId)
	{
		return kActivity_Inactive;
	}
	
	virtual int linkIsActive(const GraphLinkId linkId)
	{
		return kActivity_Inactive;
	}
};

//

#include "../libparticle/particle.h" // todo : remove ParticleColor dependency

//

struct GraphEdit : GraphEditConnection
{
	enum State
	{
		kState_Idle,
		kState_NodeSelect,
		kState_NodeDrag,
		kState_InputSocketConnect,
		kState_OutputSocketConnect,
		kState_Hidden
	};
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		GraphEdit_TypeDefinition::HitTestResult nodeHitTestResult;
		
		bool hasLink;
		GraphNodeSocketLink * link;
		
		HitTestResult()
			: hasNode(false)
			, node(nullptr)
			, nodeHitTestResult()
			, hasLink(false)
			, link(nullptr)
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
	
	struct GraphEditMouse
	{
		float uiX;
		float uiY;
		float x;
		float y;
		
		GraphEditMouse()
			: uiX(0.f)
			, uiY(0.f)
			, x(0.f)
			, y(0.f)
		{
		}
	};
	
	struct DragAndZoom
	{
		float zoom;
		float focusX;
		float focusY;
		float desiredZoom;
		float desiredFocusX;
		float desiredFocusY;
		
		Mat4x4 transform;
		Mat4x4 invTransform;
		
		DragAndZoom()
			: zoom(1.f)
			, focusX(0.f)
			, focusY(0.f)
			, desiredZoom(1.f)
			, desiredFocusX(0.f)
			, desiredFocusY(0.f)
			, transform(true)
			, invTransform(true)
		{
			updateTransform();
		}
		
		void tick(const float dt)
		{
			const float falloff = std::powf(.01f, dt);
			const float t1 = falloff;
			const float t2 = 1.f - falloff;
			zoom = zoom * t1 + desiredZoom * t2;
			focusX = focusX * t1 + desiredFocusX * t2;
			focusY = focusY * t1 + desiredFocusY * t2;
			
			updateTransform();
		}
		
		void updateTransform()
		{
			transform = Mat4x4(true).Translate(1024/2, 768/2, 0).Scale(zoom, zoom, 1.f).Translate(-focusX, -focusY, 0.f);
			invTransform = transform.Invert();
		}
	};
	
	struct RealTimeSocketCapture
	{
		GraphEdit_Visualizer visualizer;
		
		RealTimeSocketCapture()
			: visualizer()
		{
		}
	};
	
	struct DocumentInfo
	{
		std::string filename;
	};
	
	struct EditorOptions
	{
		bool menuIsVisible;
		bool showBackground;
		bool showGrid;
		bool snapToGrid;
		bool showOneShotActivity;
		bool showContinuousActivity;
		ParticleColor backgroundColor;
		ParticleColor gridColor;
		
		EditorOptions()
			: menuIsVisible(false)
			, showBackground(true)
			, showGrid(true)
			, snapToGrid(false)
			, showOneShotActivity(false)
			, showContinuousActivity(false)
			, backgroundColor(0.f, 0.f, 0.f, .8f)
			, gridColor(1.f, 1.f, 1.f, .3f)
		{
		}
	};
	
	// state support structures
	
	struct NodeSelect
	{
		int beginX;
		int beginY;
		int endX;
		int endY;
		
		std::set<GraphNodeId> nodeIds;
		
		NodeSelect()
			: beginX(0)
			, beginY(0)
			, endX(0)
			, endY(0)
			, nodeIds()
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
	GraphEdit_TypeDefinition typeDefinition_visualizer;
	
	GraphEdit_RealTimeConnection * realTimeConnection;
	
	std::set<GraphNodeId> selectedNodes;
	std::set<GraphLinkId> highlightedLinks;
	std::set<GraphLinkId> selectedLinks;
	
	SocketSelection highlightedSockets;
	SocketSelection selectedSockets;
	
	State state;
	
	NodeSelect nodeSelect;
	SocketConnect socketConnect;
	
	GraphEditMouse mousePosition;
	
	DragAndZoom dragAndZoom;
	
	RealTimeSocketCapture realTimeSocketCapture;
	
	DocumentInfo documentInfo;
	
	EditorOptions editorOptions;
	
	GraphUi::PropEdit * propertyEditor;
	
	GraphUi::NodeTypeNameSelect * nodeTypeNameSelect;
	
	UiState * uiState;
	
	GraphEdit(GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	GraphNodeSocketLink * tryGetLink(const GraphLinkId id) const;
	const GraphEdit_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const GraphEdit_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	bool tick(const float dt);
	void nodeSelectEnd();
	void socketConnectEnd();
	
	void doMenu(const float dt);
	void doEditorOptions(const float dt);
	
	bool tryAddNode(const std::string & typeName, const int x, const int y, const bool select);
	
	void selectNode(const GraphNodeId nodeId);
	void selectLink(const GraphLinkId linkId);
	void selectNodeAll();
	void selectLinkAll();
	void selectAll();
	
	void undo();
	void redo();
	
	void draw() const;
	void drawNode(const GraphNode & node, const GraphEdit_TypeDefinition & typeDefinition) const;
	void drawVisualizer(const GraphNode & node) const;
	
	bool load(const char * filename);
	bool save(const char * filename);
	
	bool loadXml(const tinyxml2::XMLElement * editorElem);
	bool saveXml(tinyxml2::XMLPrinter & editorElem) const;
	
	// GraphEditConnection
	
	virtual void nodeAdd(const GraphNodeId, const std::string & typeName) override;
	virtual void nodeRemove(const GraphNodeId nodeId) override;
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
};

//

namespace GraphUi
{
	struct PropEdit
	{
		GraphEdit_TypeDefinitionLibrary * typeLibrary;
		GraphEdit * graphEdit;
		Graph * graph;
		GraphNodeId nodeId;
		
		UiState * uiState;
		
		static const int kMaxUiColors = 32;
		ParticleColor * uiColors;
		
		PropEdit(GraphEdit_TypeDefinitionLibrary * _typeLibrary, GraphEdit * graphEdit);
		~PropEdit();
		
		bool tick(const float dt);
		void draw() const;
		
		void setGraph(Graph * graph);
		void setNode(const GraphNodeId _nodeId);
		
		void doMenus(const bool doActions, const bool doDraw, const float dt);
		void createUi();
		
		GraphNode * tryGetNode();
	};
	
	struct NodeTypeNameSelect
	{
		static const int kMaxHistory = 5;
		
		GraphEdit * graphEdit;
		
		std::string typeName;
		
		std::list<std::string> history;
		
		NodeTypeNameSelect(GraphEdit * graphEdit);
		~NodeTypeNameSelect();
		
		void doMenus(UiState * uiState, const float dt);
		void selectTypeName(const std::string & typeName);
		
		std::string & getNodeTypeName();
	};
}
