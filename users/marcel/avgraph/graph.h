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
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

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
		
		float sx;
		float sy;
		
		EditorVisualizer();
		EditorVisualizer(const EditorVisualizer & other);
		~EditorVisualizer();
		
		void allocVisualizer();
		
		void tick(const GraphEdit & graphEdit);
		
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
	int editorZKey;
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
	
	// editor
	
	std::list<GraphLinkRoutePoint> editorRoutePoints;
	
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
	int nextZKey;
	
	GraphEditConnection * graphEditConnection;
	
	Graph();
	~Graph();
	
	GraphNodeId allocNodeId();
	GraphLinkId allocLinkId();
	int allocZKey();
	
	void addNode(GraphNode & node);
	void removeNode(const GraphNodeId nodeId);
	
	void addLink(const GraphNodeSocketLink & link, const bool clearInputDuplicates);
	void removeLink(const GraphLinkId linkId);
	
	GraphNode * tryGetNode(const GraphNodeId nodeId);
	GraphNodeSocketLink * tryGetLink(const GraphLinkId linkId);
	
	bool loadXml(const tinyxml2::XMLElement * xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	bool saveXml(tinyxml2::XMLPrinter & xmlGraph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary) const;
};

//

struct GraphEdit_ValueTypeDefinition
{
	std::string typeName;
	bool multipleInputs;
	
	// ui
	
	std::string editor;
	std::string editorMin;
	std::string editorMax;
	std::string visualizer;
	bool typeValidation;
	
	GraphEdit_ValueTypeDefinition()
		: typeName()
		, multipleInputs(false)
		, editor()
		, editorMin()
		, editorMax()
		, visualizer()
		, typeValidation(true)
	{
	}
	
	bool loadXml(const tinyxml2::XMLElement * xmlType);
};

struct GraphEdit_EnumDefinition
{
	struct Elem
	{
		int value;
		std::string name;
	};
	
	std::string enumName;
	std::vector<Elem> enumElems;
	
	GraphEdit_EnumDefinition()
		: enumName()
		, enumElems()
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
		std::string enumName;
		std::string name;
		std::string defaultValue;
		
		// ui
		
		int index;
		float px;
		float py;
		float radius;
		
		InputSocket()
			: typeName()
			, enumName()
			, name()
			, index(-1)
			, px(0.f)
			, py(0.f)
			, radius(0.f)
		{
		}
		
		bool canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const OutputSocket & socket) const;
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
		
		bool canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const InputSocket & socket) const;
	};
	
	struct HitTestResult
	{
		const InputSocket * inputSocket;
		const OutputSocket * outputSocket;
		bool background;
		bool borderL;
		bool borderR;
		bool borderT;
		bool borderB;
		
		HitTestResult()
			: inputSocket(nullptr)
			, outputSocket(nullptr)
			, background(false)
			, borderL(false)
			, borderR(false)
			, borderT(false)
			, borderB(false)
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
	std::map<std::string, GraphEdit_EnumDefinition> enumDefinitions;
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	
	GraphEdit_TypeDefinitionLibrary()
		: valueTypeDefinitions()
		, enumDefinitions()
		, typeDefinitions()
	{
	}
	
	const GraphEdit_ValueTypeDefinition * tryGetValueTypeDefinition(const std::string & typeName) const;
	const GraphEdit_EnumDefinition * tryGetEnumDefinition(const std::string & typeName) const;
	const GraphEdit_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const;
	
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

struct GraphEdit_ChannelData
{
	struct Channel
	{
		const float * values;
		int numValues;
		bool continuous;
		
		Channel()
			: values(nullptr)
			, numValues(0)
			, continuous(false)
		{
		}
	};
	
	std::vector<Channel> channels;
	
	GraphEdit_ChannelData()
		: channels()
	{
	}
	
	void addChannel(const float * values, const int numValues, const bool continuous)
	{
		Channel channel;
		channel.values = values;
		channel.numValues = numValues;
		channel.continuous = continuous;
		
		channels.push_back(channel);
	}
	
	void clear()
	{
		channels.clear();
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
	
	// source
	
	GraphNodeId nodeId;
	std::string srcSocketName;
	int srcSocketIndex;
	std::string dstSocketName;
	int dstSocketIndex;
	
	// raw value
	
	std::string value;
	bool hasValue;
	
	// interpreted values
	
	History history;
	
	uint32_t texture;
	
	GraphEdit_ChannelData channels;
	
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
		, channels()
	{
	}
	
	static const int kDefaultMaxTextureSx = 200;
	static const int kDefaultMaxTextureSy = 200;
	static const int kDefaultGraphSx = History::kMaxHistory;
	static const int kDefaultGraphSy = 50;
	static const int kDefaultChannelsSx = 200;
	static const int kDefaultChannelsSy = 100;
	
	void init(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex);
	
	void tick(const GraphEdit & graphEdit);
	void measure(const GraphEdit & graphEdit, const std::string & nodeName,
		const int graphSx, const int graphSy,
		const int maxTextureSx, const int maxTextureSy,
		const int channelsSx, const int channelsSy,
		int & sx, int & sy) const;
	void draw(const GraphEdit & graphEdit, const std::string & nodeName, const bool isSelected, const int * sx, const int * sy) const;
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
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels)
	{
		return false;
	}
	
	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels)
	{
		return false;
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual void handleDstSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines)
	{
		return false;
	}
	
	virtual bool doEditor(std::string & valueText, const std::string & name, const std::string & defaultValue, const bool doActions, const bool doDraw, const float dt)
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
	
	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const
	{
		return 0;
	}
};

//

#include "../libparticle/particle.h" // todo : remove ParticleColor dependency

struct SDL_Cursor;

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
		kState_NodeResize,
		kState_TouchDrag,
		kState_TouchZoom,
		kState_Hidden,
		kState_HiddenIdle
	};
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		GraphEdit_TypeDefinition::HitTestResult nodeHitTestResult;
		
		bool hasLink;
		GraphNodeSocketLink * link;
		int linkSegmentIndex;
		
		bool hasLinkRoutePoint;
		GraphLinkRoutePoint * linkRoutePoint;
		
		HitTestResult()
			: hasNode(false)
			, node(nullptr)
			, nodeHitTestResult()
			, hasLink(false)
			, link(nullptr)
			, linkSegmentIndex(-1)
			, hasLinkRoutePoint(false)
			, linkRoutePoint(nullptr)
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
		float dx;
		float dy;
		
		GraphEditMouse()
			: uiX(0.f)
			, uiY(0.f)
			, x(0.f)
			, y(0.f)
			, dx(0.f)
			, dy(0.f)
		{
		}
	};
	
	struct LinkPath
	{
		struct Point
		{
			float x;
			float y;
			
			Point()
				: x(0.f)
				, y(0.f)
			{
			}
		};
		
		std::vector<Point> points;
		
		LinkPath()
			: points()
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
		bool realTimePreview;
		bool autoHideUi;
		bool showBackground;
		bool showGrid;
		bool snapToGrid;
		bool showOneShotActivity;
		bool showContinuousActivity;
		bool showCpuHeat;
		ParticleColor backgroundColor;
		ParticleColor gridColor;
		ParticleColorCurve cpuHeatColors;
		
		EditorOptions()
			: menuIsVisible(false)
			, realTimePreview(true)
			, autoHideUi(false)
			, showBackground(true)
			, showGrid(true)
			, snapToGrid(false)
			, showOneShotActivity(false)
			, showContinuousActivity(false)
			, showCpuHeat(false)
			, backgroundColor(0.f, 0.f, 0.f, .8f)
			, gridColor(1.f, 1.f, 1.f, .3f)
			, cpuHeatColors()
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
	
	struct NodeDrag
	{
		std::map<GraphNodeId, Vec2> offsets;
		
		NodeDrag()
			: offsets()
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
	
	struct NodeResize
	{
		GraphNodeId nodeId;
		
		bool dragL;
		bool dragR;
		bool dragT;
		bool dragB;
		
		NodeResize()
			: nodeId(kGraphNodeIdInvalid)
			, dragL(false)
			, dragR(false)
			, dragT(false)
			, dragB(false)
		{
		}
	};
	
	struct Touches
	{
		struct FingerInfo
		{
			uint64_t id;
			Vec2 position;
			Vec2 initialPosition;
		};
		
		FingerInfo finger1;
		FingerInfo finger2;
		
		float initialDistance;
		float distance;
		
		Touches()
			: finger1()
			, finger2()
			, initialDistance(0.f)
			, distance(0.f)
		{
		}
		
		float getDistance() const
		{
			return (finger2.position - finger1.position).CalcSize();
		}
	};
	
	// UI
	
	struct Notification
	{
		std::string text;
		float displayTime;
		float displayTimeRcp;
		
		Notification()
			: text()
			, displayTime(0.f)
			, displayTimeRcp(0.f)
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
	std::set<GraphLinkRoutePoint*> highlightedLinkRoutePoints;
	std::set<GraphLinkRoutePoint*> selectedLinkRoutePoints;
	
	SocketSelection highlightedSockets;
	SocketSelection selectedSockets;
	
	State state;
	
	NodeSelect nodeSelect;
	NodeDrag nodeDrag;
	SocketConnect socketConnect;
	NodeResize nodeResize;
	
	Touches touches;
	
	GraphEditMouse mousePosition;
	
	DragAndZoom dragAndZoom;
	
	RealTimeSocketCapture realTimeSocketCapture;
	
	DocumentInfo documentInfo;
	
	EditorOptions editorOptions;
	
	GraphUi::PropEdit * propertyEditor;
	
	GraphUi::NodeTypeNameSelect * nodeTypeNameSelect;
	
	std::list<Notification> notifications;
	
	UiState * uiState;
	
	SDL_Cursor * cursorHand;
	
	float idleTime;
	float hideTime;
	
	GraphEdit(GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	GraphNodeSocketLink * tryGetLink(const GraphLinkId id) const;
	const GraphEdit_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const GraphEdit_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	bool getLinkPath(const GraphLinkId linkId, LinkPath & path) const;
	
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	bool tick(const float dt);
	bool tickTouches();
	void tickMouseScroll(const float dt);
	void tickKeyboardScroll();
	
	void nodeSelectEnd();
	void nodeDragEnd();
	void socketConnectEnd();
	
	void doMenu(const float dt);
	void doEditorOptions(const float dt);
	
	bool isInputIdle() const;
	
	bool tryAddNode(const std::string & typeName, const int x, const int y, const bool select);
	bool tryAddVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex, const int x, const int y, const bool select);
	
	void selectNode(const GraphNodeId nodeId, const bool clearSelection);
	void selectLink(const GraphLinkId linkId, const bool clearSelection);
	void selectLinkRoutePoint(GraphLinkRoutePoint * routePoint, const bool clearSelection);
	void selectNodeAll();
	void selectLinkAll();
	void selectLinkRoutePointAll();
	void selectAll();
	
	void snapToGrid(float & x, float & y) const;
	void snapToGrid(GraphLinkRoutePoint & routePoint) const;
	void snapToGrid(GraphNode & node) const;
	
	void undo();
	void redo();
	
	void showNotification(const char * format, ...);
	
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
		std::string findClosestMatch(const std::string & typeName) const;
		void selectTypeName(const std::string & typeName);
		
		std::string & getNodeTypeName();
	};
}
