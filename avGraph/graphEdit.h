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

#pragma once

#include "graph.h"
#include "graph_typeDefinitionLibrary.h"
#include "graphEdit_realTimeConnection.h"
#include <functional>
#include <list>
#include <map>
#include <math.h>
#include <set>
#include <string>
#include <vector>

// forward declarations

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

//

class Window;

struct UiState;
struct ParticleColor;

namespace GraphUi
{
	struct PropEdit;
	struct NodeTypeNameSelect;
};

//

struct Graph_TypeDefinitionLibrary;

//

struct GraphEdit;
struct GraphEdit_NodeResourceEditorWindow;
struct GraphEdit_NodeTypeSelect;
struct GraphEdit_ResourceEditorBase;
struct GraphEdit_Visualizer;

//

extern int GRAPHEDIT_SX;
extern int GRAPHEDIT_SY;

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

/*
GraphEdit_Visualizer
- points to a node and src or dst socket
- copies the socket value on tick
- keeps a history of values on tick
- computes some channel related info for visualizing channels
- has a draw method to draw the visualizer
*/
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
			
			if (historySize < maxHistorySize)
				historySize = historySize + 1;
			
			history[nextWriteIndex] = value;
			
			nextWriteIndex++;
			
			if (nextWriteIndex == maxHistorySize)
				nextWriteIndex = 0;
		}
		
		float getGraphValue(const int offset) const
		{
			const int index = (nextWriteIndex - historySize + offset + maxHistorySize) % maxHistorySize;
			
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
	
	GraphEdit_ChannelData channelData;
	bool hasChannelDataMinMax;
	float channelDataMin;
	float channelDataMax;
	
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
		, channelData()
		, hasChannelDataMinMax(false)
		, channelDataMin(0.f)
		, channelDataMax(0.f)
	{
	}
	
	static const int kDefaultMaxTextureSx = 200;
	static const int kDefaultMaxTextureSy = 200;
	static const int kDefaultGraphSx = History::kMaxHistory;
	static const int kDefaultGraphSy = 50;
	static const int kDefaultChannelsSx = 200;
	static const int kDefaultChannelsSy = 100;
	
	void init(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex);
	void init();
	
	void tick(const GraphEdit & graphEdit, const float dt);
	void measure(const GraphEdit & graphEdit, const std::string & nodeName,
		const int graphSx, const int graphSy,
		const int maxTextureSx, const int maxTextureSy,
		const int channelsSx, const int channelsSy,
		int & sx, int & sy) const;
	void draw(const GraphEdit & graphEdit, const std::string & nodeName, const bool isSelected, const int * sx, const int * sy) const;
};

//

struct GraphEdit_ResourceEditorBase
{
	int initSx;
	int initSy;
	
	int x;
	int y;
	
	int sx;
	int sy;
	
	GraphEdit_ResourceEditorBase(const int _sx, const int _sy)
		: initSx(_sx)
		, initSy(_sy)
		, x(0)
		, y(0)
		, sx(0)
		, sy(0)
	{
	}
	
	virtual ~GraphEdit_ResourceEditorBase()
	{
	}
	
	void init(const int _x, const int _y, const int _sx, const int _sy)
	{
		sx = _sx;
		sy = _sy;
		afterSizeChanged();
		
		x = _x;
		y = _y;
		afterPositionChanged();
	}
	
	virtual void afterSizeChanged() { }
	virtual void afterPositionChanged() { }
	
	virtual bool tick(const float dt, const bool inputIsCaptured) = 0;
	virtual void draw() const = 0;
	
	virtual void setResource(const GraphNode & node, const char * type, const char * name) = 0;
	virtual bool serializeResource(std::string & text) const = 0;
};

//

#include "particle.h" // todo : remove ParticleColor dependency

struct SDL_Cursor;

//

struct GraphEdit : GraphEditConnection
{
	enum State
	{
		kState_Idle,
		kState_NodeSelect,
		kState_NodeDrag,
		kState_NodeResourceEdit,
		kState_InputSocketConnect,
		kState_OutputSocketConnect,
		kState_NodeResize,
		kState_NodeInsert,
		kState_TouchDecide,
		kState_TouchDrag,
		kState_TouchZoom,
		kState_Hidden,
		kState_HiddenIdle
	};
	
	enum Flags
	{
		kFlag_None = 0,
		kFlag_SaveLoad = 1 << 0,
		kFlag_EditorOptions = 1 << 1,
		kFlag_NodeAdd = 1 << 2,
		kFlag_NodeRemove = 1 << 3,
		kFlag_NodeProperties = 1 << 4,
		kFlag_NodeResourceEdit = 1 << 5,
		kFlag_NodeDrag = 1 << 6,
		kFlag_LinkAdd = 1 << 7,
		kFlag_LinkRemove = 1 << 8,
		kFlag_LinkEdit = 1 << 9,
		kFlag_Drag = 1 << 10,
		kFlag_Zoom = 1 << 11,
		kFlag_ToggleIsPassthrough = 1 << 12,
		kFlag_ToggleIsFolded = 1 << 13,
		kFlag_SetCursor = 1 << 14,
		kFlag_Select = 1 << 15,
		kFlag_All = ~0
	};
	
	struct NodeData
	{
		struct DynamicSockets
		{
			bool hasDynamicSockets;
			
			std::vector<Graph_TypeDefinition::InputSocket> inputSockets;
			std::vector<Graph_TypeDefinition::OutputSocket> outputSockets;
			
			std::vector<Graph_EnumDefinition> enumDefinitions;
			
			DynamicSockets()
				: hasDynamicSockets(false)
				, inputSockets()
				, outputSockets()
			{
			}
			
			void reset()
			{
				if (hasDynamicSockets)
				{
					hasDynamicSockets = false;
					
					inputSockets.clear();
					outputSockets.clear();
					
					enumDefinitions.clear();
				}
			}
			
			void update(
				const Graph_TypeDefinition & typeDefinition,
				const std::vector<GraphEdit_RealTimeConnection::DynamicInput> & newInputs,
				const std::vector<GraphEdit_RealTimeConnection::DynamicOutput> & newOutputs);
		};
	
		float x;
		float y;
		int zKey;
		
		std::string displayName;
		
		bool isFolded;
		float foldAnimProgress;
		float foldAnimTimeRcp;
		bool isCloseToConnectionSite; // true when a new connection is being made near this node

		float isActiveAnimTime; // real-time connection node activation animation
		float isActiveAnimTimeRcp;
		bool isActiveContinuous;
		
		DynamicSockets dynamicSockets;
		
		NodeData()
			: x(0.f)
			, y(0.f)
			, zKey(0)
			, displayName()
			, isFolded(false)
			, foldAnimProgress(1.f)
			, foldAnimTimeRcp(0.f)
			, isCloseToConnectionSite(false)
			, isActiveAnimTime(0.f)
			, isActiveAnimTimeRcp(0.f)
			, isActiveContinuous(false)
			, dynamicSockets()
		{
		}
		
		void setIsFolded(const bool isFolded);
	};
	
	struct EditorVisualizer : GraphEdit_Visualizer
	{
		GraphNodeId id;
		
		float x;
		float y;
		
		float sx;
		float sy;
		
		int zKey;
		
		EditorVisualizer();
		
		void tick(const GraphEdit & graphEdit, const float dt);
		void updateSize(const GraphEdit & graphEdit);
	};
	
	struct EditorComment
	{
		GraphNodeId id;
		
		float x;
		float y;
		
		float sx;
		float sy;
		
		std::string caption;
		ParticleColor color;
		
		EditorComment()
			: id(kGraphNodeIdInvalid)
			, x(0)
			, y(0)
			, sx(200)
			, sy(100)
			, caption()
			, color(1.f, 1.f, .9f, .9f)
		{
		}
		
		void draw() const;
	};
	
	struct NodeHitTestResult
	{
		const Graph_TypeDefinition::InputSocket * inputSocket;
		const Graph_TypeDefinition::OutputSocket * outputSocket;
		bool background;
		
		NodeHitTestResult()
			: inputSocket(nullptr)
			, outputSocket(nullptr)
			, background(false)
		{
		}
	};
	
	struct VisualizerHitTestResult
	{
		bool background;
		bool borderL;
		bool borderR;
		bool borderT;
		bool borderB;
		
		VisualizerHitTestResult()
			: background(false)
			, borderL(false)
			, borderR(false)
			, borderT(false)
			, borderB(false)
		{
		}
	};
	
	struct CommentHitTestResult
	{
		bool background;
		bool borderL;
		bool borderR;
		bool borderT;
		bool borderB;
		
		CommentHitTestResult()
			: background(false)
			, borderL(false)
			, borderR(false)
			, borderT(false)
			, borderB(false)
		{
		}
	};
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		NodeHitTestResult nodeHitTestResult;
		
		bool hasLink;
		GraphLink * link;
		int linkSegmentIndex;
		
		bool hasLinkRoutePoint;
		GraphLinkRoutePoint * linkRoutePoint;
		
		bool hasVisualizer;
		EditorVisualizer * visualizer;
		VisualizerHitTestResult visualizerHitTestResult;
		
		bool hasComment;
		EditorComment * comment;
		CommentHitTestResult commentHitTestResult;
		
		HitTestResult()
			: hasNode(false)
			, node(nullptr)
			, nodeHitTestResult()
			, hasLink(false)
			, link(nullptr)
			, linkSegmentIndex(-1)
			, hasLinkRoutePoint(false)
			, linkRoutePoint(nullptr)
			, hasVisualizer(false)
			, visualizer(nullptr)
			, visualizerHitTestResult()
			, hasComment(false)
			, comment(nullptr)
			, commentHitTestResult()
		{
		}
	};
	
	struct SocketSelection
	{
		GraphNodeId srcNodeId;
		const Graph_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const Graph_TypeDefinition::OutputSocket * dstNodeSocket;
		
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
		
		bool hover;
		
		GraphEditMouse()
			: uiX(0.f)
			, uiY(0.f)
			, x(0.f)
			, y(0.f)
			, dx(0.f)
			, dy(0.f)
			, hover(false)
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
		
		Mat4x4 transform;    // graph coord to screen space
		Mat4x4 invTransform; // screen space to graph coord
		
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
			const float falloff = powf(.01f, dt);
			const float t1 = falloff;
			const float t2 = 1.f - falloff;
			zoom = zoom * t1 + desiredZoom * t2;
			focusX = focusX * t1 + desiredFocusX * t2;
			focusY = focusY * t1 + desiredFocusY * t2;
			
			updateTransform();
		}
		
		void updateTransform()
		{
			transform = Mat4x4(true)
				.Translate(GRAPHEDIT_SX/2, GRAPHEDIT_SY/2, 0)
				.Scale(zoom, zoom, 1.f)
				.Translate(-focusX, -focusY, 0.f);
			invTransform = transform.Invert();
		}
		
		bool animationIsDone() const
		{
			const float deltaZoom = zoom / desiredZoom;
			const float deltaFocusX = (focusX - desiredFocusX) * desiredZoom;
			const float deltaFocusY = (focusY - desiredFocusY) * desiredZoom;
			
			return
				fabsf(deltaZoom - 1.f) <= .001f &&
				fabsf(deltaFocusX) <= .01f &&
				fabsf(deltaFocusY) <= .01f;
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
		std::string comment;
		bool menuIsVisible;
		bool realTimePreview;
		bool autoHideUi;
		bool showBackground;
		bool showGrid;
		bool showSocketNames;
		bool showNodeImages; // todo : load/save to XML
		bool snapToGrid;
		bool showOneShotActivity;
		bool showContinuousActivity;
		bool showCpuHeat;
		bool drawLinksAsCurves;
		ParticleColor backgroundColor;
		ParticleColor gridColor;
		ParticleColorCurve cpuHeatColors;
		
		EditorOptions()
			: comment()
			, menuIsVisible(false)
			, realTimePreview(true)
			, autoHideUi(false)
			, showBackground(true)
			, showGrid(true)
			, showSocketNames(true)
			, showNodeImages(false)
			, snapToGrid(false)
			, showOneShotActivity(false)
			, showContinuousActivity(false)
			, showCpuHeat(false)
			, drawLinksAsCurves(true)
			, backgroundColor(0.f, 0.f, 0.f, .8f)
			, gridColor(1.f, 1.f, 1.f, .3f)
			, cpuHeatColors()
		{
		}
	};
	
	struct NodeResourceEditor
	{
		GraphNodeId nodeId;
		std::string resourceType;
		std::string resourceName;
		
		GraphEdit_ResourceEditorBase * resourceEditor;
		
		NodeResourceEditor()
			: nodeId(kGraphNodeIdInvalid)
			, resourceType()
			, resourceName()
			, resourceEditor(nullptr)
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
		
		std::set<GraphNodeId> visualizerIds;
		
		std::set<GraphNodeId> commentIds;
		
		NodeSelect()
			: beginX(0)
			, beginY(0)
			, endX(0)
			, endY(0)
			, nodeIds()
			, visualizerIds()
			, commentIds()
		{
		}
	};
	
	struct SocketConnect
	{
		GraphNodeId srcNodeId;
		const Graph_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const Graph_TypeDefinition::OutputSocket * dstNodeSocket;
		
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
	
	struct NodeInsert
	{
		float x;
		float y;
		
		GraphLinkId linkId;
		
		NodeInsert()
			: x(0.f)
			, y(0.f)
			, linkId(kGraphLinkIdInvalid)
		{
		}
	};
	
	struct Touches
	{
		struct FingerInfo
		{
			uint64_t id = 0;
			Vec2 position;
			bool hasPosition = false;
			Vec2 initialPosition;
		};
		
		FingerInfo finger1;
		FingerInfo finger2;
		
		// for zoom
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
	
	int nextZKey;
	
	std::map<GraphNodeId, NodeData> nodeDatas;
	
	std::map<GraphNodeId, EditorVisualizer> visualizers;
	std::map<GraphNodeId, EditorComment> comments;
	
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	Graph_TypeDefinition emptyTypeDefinition;
	
	GraphEdit_RealTimeConnection * realTimeConnection;
	
	std::set<GraphNodeId> selectedNodes;
	std::set<GraphLinkId> highlightedLinks;
	std::set<GraphLinkId> selectedLinks;
	std::set<GraphLinkRoutePoint*> highlightedLinkRoutePoints;
	std::set<GraphLinkRoutePoint*> selectedLinkRoutePoints;
	std::set<GraphNodeId> selectedVisualizers;
	std::set<GraphNodeId> selectedComments;
	
	SocketSelection highlightedSockets;
	SocketSelection selectedSockets;
	
	State state;
	
	int flags;
	
	NodeSelect nodeSelect;
	SocketConnect socketConnect;
	NodeResize nodeResize;
	NodeInsert nodeInsert;
	
	float nodeDoubleClickTime;
	
	std::function<void(const GraphNodeId)> handleNodeDoubleClicked;
	
	Touches touches;
	
	GraphEditMouse mousePosition;
	
	DragAndZoom dragAndZoom;
	
	RealTimeSocketCapture realTimeSocketCapture;
	
	DocumentInfo documentInfo;
	
	EditorOptions editorOptions;
	
	GraphUi::PropEdit * propertyEditor;
	
	GraphNodeId linkParamsEditorLinkId;
	
	GraphUi::NodeTypeNameSelect * nodeTypeNameSelect;
	
	GraphEdit_NodeTypeSelect * nodeInsertMenu;
	
	NodeResourceEditor nodeResourceEditor; // similar to nodeResourceEditorWindows, but editing happens inside the same window as the graph editor
	
	std::list<GraphEdit_NodeResourceEditorWindow*> nodeResourceEditorWindows;
	
	std::list<Notification> notifications;
	
	int displaySx;
	int displaySy;
	
	UiState * uiState;
	
	SDL_Cursor * cursorHand;
	
	bool animationIsDone;
	
	float idleTime;
	float hideTime;
	
	mutable int maxNodeCpuTime = 0;
	
	GraphEdit(
		const int displaySx,
		const int displaySy,
		Graph_TypeDefinitionLibrary * typeDefinitionLibrary,
		GraphEdit_RealTimeConnection * realTimeConnection = nullptr);
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	NodeData * tryGetNodeData(const GraphNodeId id) const;
	GraphLink * tryGetLink(const GraphLinkId id) const;
	const Graph_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const Graph_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	bool getLinkPath(const GraphLinkId linkId, LinkPath & path) const;
	const Graph_LinkTypeDefinition * tryGetLinkTypeDefinition(const GraphLinkId linkId) const;
	EditorVisualizer * tryGetVisualizer(const GraphNodeId id) const;
	EditorComment * tryGetComment(const GraphNodeId id) const;
	
	bool enabled(const int flag) const;
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	bool hitTestNode(const NodeData & nodeData, const Graph_TypeDefinition & typeDefinition, const float x, const float y, const bool socketsAreVisible, NodeHitTestResult & result) const;
	
	bool tick(const float dt, const bool inputIsCaptured);
	void tickVisualizers(const float dt);
	void tickNodeDatas(const float dt);
	bool tickTouches();
	void tickMouseScroll(const float dt);
	void tickKeyboardScroll();
	void tickNodeResourceEditorWindows();
	
	void nodeSelectEnd();
	void nodeDragEnd();
	bool nodeResourceEditBegin(const GraphNodeId nodeId);
	void nodeResourceEditSave();
	void nodeResourceEditEnd();
	void socketConnectEnd();
	
	void doMenu(const float dt);
	void doEditorOptions(const float dt);
	
	void doLinkParams(const float dt);
	void doCommentProperties(const float dt);
	
	bool isInputIdle() const;
	
	bool tryAddNode(const std::string & typeName, const float x, const float y, const bool select, GraphNodeId * nodeId);
	bool tryAddVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex, const float x, const float y, const bool select, EditorVisualizer ** visualizer);
	bool tryAddComment(const float x, const float y, const bool select, EditorComment ** comment);
	
	void updateDynamicSockets();
	void resolveSocketIndices(
		const GraphNodeId srcNodeId, const std::string & srcNodeSocketName, int & srcNodeSocketIndex,
		const GraphNodeId dstNodeId, const std::string & dstNodeSocketName, int & dstNodeSocketIndex);
	
	void selectNode(const GraphNodeId nodeId, const bool clearSelection);
	void selectLink(const GraphLinkId linkId, const bool clearSelection);
	void selectLinkRoutePoint(GraphLinkRoutePoint * routePoint, const bool clearSelection);
	void selectVisualizer(const GraphNodeId visualizerId, const bool clearSelection);
	void selectComment(const GraphNodeId commentId, const bool clearSelection);
	void selectNodeAll();
	void selectLinkAll();
	void selectLinkRoutePointAll();
	void selectVisualizerAll();
	void selectCommentAll();
	void selectAll();
	void deselectAll();
	
	void snapToGrid(float & x, float & y) const;
	
	void undo();
	void redo();
	
	void beginEditing();
	void cancelEditing();
	
	void showNotification(const char * format, ...);
	
	void draw() const;
	void drawNode(const GraphNode & node, const NodeData & nodeData, const Graph_TypeDefinition & typeDefinition, const char * displayName) const;
	void drawVisualizer(const EditorVisualizer & visualizer) const;
	void drawComment(const EditorComment & comment) const;
	
	bool load(const char * filename);
	bool save(const char * filename);
	
	bool loadXml(const tinyxml2::XMLElement * editorElem);
	bool saveXml(tinyxml2::XMLPrinter & editorElem) const;
	
	// GraphEditConnection
	
	virtual void nodeAdd(const GraphNode & node) override;
	virtual void nodeRemove(const GraphNodeId nodeId) override;
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	
	// add/remove
	
	void visualizerRemove(const GraphNodeId visualizerId);
	void commentRemove(const GraphNodeId commentId);
};

//

namespace GraphUi
{
	struct PropEdit
	{
		Graph_TypeDefinitionLibrary * typeLibrary;
		GraphEdit * graphEdit;
		Graph * graph;
		GraphNodeId nodeId;
		GraphNodeId currentNodeId;
		
		static const int kMaxUiColors = 32;
		ParticleColor * uiColors;
		
		PropEdit(Graph_TypeDefinitionLibrary * _typeLibrary, GraphEdit * graphEdit);
		~PropEdit();
		
		void setGraph(Graph * graph);
		void setNode(const GraphNodeId _nodeId);
		
		void doMenus(UiState * uiState, const float dt);
		
		GraphNode * tryGetNode();
	};
	
	struct NodeTypeNameSelect
	{
		static const int kMaxHistory = 5;
		
		GraphEdit * graphEdit;
		
		std::string typeName;
		
		bool showSuggestions;
		
		std::list<std::string> history;
		
		NodeTypeNameSelect(GraphEdit * graphEdit);
		~NodeTypeNameSelect();
		
		void doMenus(UiState * uiState, const float dt);
		std::string findClosestMatch(const std::string & typeName) const;
		void selectTypeName(const std::string & typeName);
		void addToHistory(const std::string & typeName);
		
		std::string & getNodeTypeName();
	};
}
