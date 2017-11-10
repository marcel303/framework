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
#include <stdint.h>
#include <string>
#include <vector>

extern const int GFX_SX;
extern const int GFX_SY;

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

struct GraphEdit_NodeTypeSelect;
struct GraphEdit_ResourceEditorBase;
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
	struct Resource
	{
		std::string type;
		std::string name;
		std::string data;
	};
	
	GraphNodeId id;
	GraphNodeType nodeType;
	std::string typeName;
	bool isPassthrough;
	
	std::map<std::string, Resource> resources;
	
	// editor
	
	std::map<std::string, std::string> editorInputValues;
	std::string editorValue;
	
	GraphNode();
	
	void setIsPassthrough(const bool isPassthrough);
	
	void setResource(const char * type, const char * name, const char * data);
	void clearResource(const char * type, const char * name);
	const char * getResource(const char * type, const char * name, const char * defaultValue) const;
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
	
	std::map<std::string, std::string> params;
	
	// editor
	
	std::list<GraphLinkRoutePoint> editorRoutePoints;
	
	float editorIsActiveAnimTime; // real-time connection node activation animation
	float editorIsActiveAnimTimeRcp;
	
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
		, editorMin("0")
		, editorMax("1")
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
		bool hasDefaultValue;
		
		// ui
		
		int index;
		
		InputSocket()
			: typeName()
			, enumName()
			, name()
			, defaultValue()
			, hasDefaultValue(false)
			, index(-1)
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
		
		OutputSocket()
			: typeName()
			, name()
			, isEditable(false)
			, index(-1)
		{
		}
		
		bool canConnectTo(const GraphEdit_TypeDefinitionLibrary * typeDefintionLibrary, const InputSocket & socket) const;
	};
	
	struct ResourceEditor
	{
		GraphEdit_ResourceEditorBase * (*create)();
		
		ResourceEditor()
			: create(nullptr)
		{
		}
	};
	
	std::string typeName;
	
	std::vector<InputSocket> inputSockets;
	std::vector<OutputSocket> outputSockets;
	
	ResourceEditor resourceEditor;
	
	// ui
	
	std::string displayName;
	
	std::string resourceTypeName;
	
	GraphEdit_TypeDefinition()
		: typeName()
		, inputSockets()
		, outputSockets()
		, resourceEditor()
		, displayName()
		, resourceTypeName()
	{
	}
	
	void createUi();
	
	bool loadXml(const tinyxml2::XMLElement * xmlNode);
};

struct GraphEdit_LinkTypeDefinition
{
	struct Param
	{
		std::string typeName;
		std::string name;
		std::string defaultValue;
		
		Param()
			: typeName()
			, name()
			, defaultValue()
		{
		}
	};
	
	std::string srcTypeName;
	std::string dstTypeName;
	std::vector<Param> params;
};

struct GraphEdit_TypeDefinitionLibrary
{
	std::map<std::string, GraphEdit_ValueTypeDefinition> valueTypeDefinitions;
	std::map<std::string, GraphEdit_EnumDefinition> enumDefinitions;
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	std::map<std::pair<std::string, std::string>, GraphEdit_LinkTypeDefinition> linkTypeDefinitions;
	
	GraphEdit_TypeDefinitionLibrary()
		: valueTypeDefinitions()
		, enumDefinitions()
		, typeDefinitions()
		, linkTypeDefinitions()
	{
	}
	
	const GraphEdit_ValueTypeDefinition * tryGetValueTypeDefinition(const std::string & typeName) const;
	const GraphEdit_EnumDefinition * tryGetEnumDefinition(const std::string & typeName) const;
	const GraphEdit_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const;
	const GraphEdit_LinkTypeDefinition * tryGetLinkTypeDefinition(const std::string & srcTypeName, const std::string & dstTypeName) const;
	
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
	
	bool hasChannels() const
	{
		return !channels.empty();
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
	
	GraphEdit_ChannelData channelData;
	mutable bool hasChannelDataMinMax;
	mutable float channelDataMin;
	mutable float channelDataMax;
	
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
	
	void tick(const GraphEdit & graphEdit);
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
	virtual ~GraphEdit_ResourceEditorBase()
	{
	}
	
	virtual void getSize(int & sx, int & sy) const = 0;
	virtual void setPosition(const int x, const int y) = 0;
	
	virtual bool tick(const float dt, const bool inputIsCaptured) = 0;
	virtual void draw() const = 0;
	
	// todo : introduce the concept of a resource path ? perhaps name node-specific resources "<type>:node/<id>"
	virtual void setResource(const GraphNode & node, const char * type, const char * name) = 0;
	virtual bool serializeResource(std::string & text) const = 0;
	
	//
	
	void getPositionForViewCenter(int & x, int & y) const
	{
		int sx;
		int sy;
		
		getSize(sx, sy);
		
		x = (GFX_SX - sx) / 2;
		y = (GFX_SY - sy) / 2;
	}
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
	
	struct DynamicInput
	{
		std::string name;
		std::string type;
	};
	
	struct DynamicOutput
	{
		std::string name;
		std::string type;
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
	
	virtual void saveBegin(GraphEdit & graphEdit)
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
	
	virtual void setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value)
	{
	}
	
	virtual void clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name)
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
	
	virtual bool getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues)
	{
		return false;
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines)
	{
		return false;
	}
	
	virtual int nodeIsActive(const GraphNodeId nodeId)
	{
		return kActivity_Inactive;
	}
	
	virtual int linkIsActive(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
		return kActivity_Inactive;
	}
	
	virtual bool getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const
	{
		return false;
	}
	
	virtual int getNodeCpuHeatMax() const
	{
		return 1000;
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
		kState_NodeResourceEdit,
		kState_InputSocketConnect,
		kState_OutputSocketConnect,
		kState_NodeResize,
		kState_NodeInsert,
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
		kFlag_NodeDrag = 1 << 5,
		kFlag_LinkAdd = 1 << 6,
		kFlag_LinkRemove = 1 << 7,
		kFlag_Drag = 1 << 8,
		kFlag_Zoom = 1 << 9,
		kFlag_ToggleIsPassthrough = 1 << 10,
		kFlag_ToggleIsFolded = 1 << 11,
		kFlag_SetCursor = 1 << 12,
		kFlag_Select = 1 << 13,
		kFlag_All = ~0
	};
	
	struct NodeData
	{
		struct DynamicSockets
		{
			struct Input
			{
				std::string name;
				std::string type;
			};
			
			struct Output
			{
				std::string name;
				std::string type;
			};
			
			std::vector<Input> inputs;
			std::vector<Output> outputs;
			
			void reset()
			{
				inputs.clear();
				outputs.clear();
			}
			
			void update(
				std::vector<GraphEdit_RealTimeConnection::DynamicInput> & newInputs,
				std::vector<GraphEdit_RealTimeConnection::DynamicOutput> & newOutputs)
			{
				inputs.resize(newInputs.size());
				outputs.resize(newOutputs.size());
				
				for (int i = 0; i < newInputs.size(); ++i)
				{
					auto & newInput = newInputs[i];
					auto & input = inputs[i];
					
					input.name = newInput.name;
					input.type = newInput.type;
				}
				
				for (int i = 0; i < newOutputs.size(); ++i)
				{
					auto & newOutput = newOutputs[i];
					auto & output = outputs[i];
					
					output.name = newOutput.name;
					output.type = newOutput.type;
				}
			}
		};
		
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
			void updateSize(const GraphEdit & graphEdit);
			
			// nodes (and thus also visualizers) get copied around. we want to copy parameters but not the
			// dynamically allocated visualizer object. so we need a copy constructor/assignment operator
			// to address this and copy the parameters manually and allocate a new visualizer object
			void operator=(const EditorVisualizer & other);
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
		
		EditorVisualizer visualizer;
		
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
			, visualizer()
		{
		}
		
		void setIsFolded(const bool isFolded);
		
		void setVisualizer(const GraphNodeId nodeId, const std::string & srcSocketName, const int srcSocketIndex, const std::string & dstSocketName, const int dstSocketIndex);
	};
	
	struct NodeHitTestResult
	{
		const GraphEdit_TypeDefinition::InputSocket * inputSocket;
		const GraphEdit_TypeDefinition::OutputSocket * outputSocket;
		bool background;
		bool borderL;
		bool borderR;
		bool borderT;
		bool borderB;
		
		NodeHitTestResult()
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
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		NodeHitTestResult nodeHitTestResult;
		
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
			transform = Mat4x4(true).Translate(GFX_SX/2, GFX_SY/2, 0).Scale(zoom, zoom, 1.f).Translate(-focusX, -focusY, 0.f);
			invTransform = transform.Invert();
		}
		
		bool animationIsDone() const
		{
			const float deltaZoom = zoom / desiredZoom;
			const float deltaFocusX = (focusX - desiredFocusX) * desiredZoom;
			const float deltaFocusY = (focusY - desiredFocusY) * desiredZoom;
			
			return
				std::abs(deltaZoom - 1.f) <= .001f &&
				std::abs(deltaFocusX) <= .01f &&
				std::abs(deltaFocusY) <= .01f;
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
	
	struct NodeResourceEditor
	{
		GraphNodeId nodeId;
		std::string resourceTypeName; // todo : should be path ?
		GraphEdit_ResourceEditorBase * resourceEditor;
		
		NodeResourceEditor()
			: nodeId(kGraphNodeIdInvalid)
			, resourceTypeName()
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
	
	struct NodeInsert
	{
		GraphEdit_NodeTypeSelect * menu;
		
		float x;
		float y;
		
		NodeInsert()
			: menu(nullptr)
			, x(0.f)
			, y(0.f)
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
	
	int nextZKey;
	
	std::map<GraphNodeId, NodeData> nodeDatas;
	
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
	
	int flags;
	
	NodeSelect nodeSelect;
	NodeDrag nodeDrag;
	SocketConnect socketConnect;
	NodeResize nodeResize;
	NodeInsert nodeInsert;
	
	float nodeDoubleClickTime;
	
	Touches touches;
	
	GraphEditMouse mousePosition;
	
	DragAndZoom dragAndZoom;
	
	RealTimeSocketCapture realTimeSocketCapture;
	
	DocumentInfo documentInfo;
	
	EditorOptions editorOptions;
	
	GraphUi::PropEdit * propertyEditor;
	
	GraphNodeId linkParamsEditorLinkId;
	
	GraphUi::NodeTypeNameSelect * nodeTypeNameSelect;
	
	NodeResourceEditor nodeResourceEditor;
	
	std::list<Notification> notifications;
	
	UiState * uiState;
	
	SDL_Cursor * cursorHand;
	
	bool animationIsDone;
	
	float idleTime;
	float hideTime;
	
	GraphEdit(GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	NodeData * tryGetNodeData(const GraphNodeId id) const;
	GraphNodeSocketLink * tryGetLink(const GraphLinkId id) const;
	const GraphEdit_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const GraphEdit_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	bool getLinkPath(const GraphLinkId linkId, LinkPath & path) const;
	const GraphEdit_LinkTypeDefinition * tryGetLinkTypeDefinition(const GraphLinkId linkId) const;
	
	bool enabled(const int flag) const;
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	bool hitTestNode(const GraphEdit_TypeDefinition & typeDefinition, const float x, const float y, const bool socketsAreVisible, NodeHitTestResult & result) const;
	
	bool tick(const float dt, const bool inputIsCaptured);
	void tickNodeDatas(const float dt);
	bool tickTouches();
	void tickMouseScroll(const float dt);
	void tickKeyboardScroll();
	
	void nodeSelectEnd();
	void nodeDragEnd();
	bool nodeResourceEditBegin(const GraphNodeId nodeId);
	void nodeResourceEditSave();
	void nodeResourceEditEnd();
	void socketConnectEnd();
	
	void doMenu(const float dt);
	void doEditorOptions(const float dt);
	
	void doLinkParams(const float dt);
	
	bool isInputIdle() const;
	
	bool tryAddNode(const std::string & typeName, const float x, const float y, const bool select);
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
	void snapToGrid(NodeData & nodeData) const;
	
	void undo();
	void redo();
	
	void beginEditing();
	void cancelEditing();
	
	void showNotification(const char * format, ...);
	
	void draw() const;
	void drawNode(const GraphNode & node, const NodeData & nodeData, const GraphEdit_TypeDefinition & typeDefinition) const;
	void drawVisualizer(const GraphNode & node, const NodeData & nodeData) const;
	
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
		GraphNodeId currentNodeId;
		
		static const int kMaxUiColors = 32;
		ParticleColor * uiColors;
		
		PropEdit(GraphEdit_TypeDefinitionLibrary * _typeLibrary, GraphEdit * graphEdit);
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
