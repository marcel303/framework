#include "Calc.h"
#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "../avpaint/video.h"

#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeComposite.h"
#include "vfxNodes/vfxNodeFsfx.h"
#include "vfxNodes/vfxNodeLeapMotion.h"
#include "vfxNodes/vfxNodeLiteral.h"
#include "vfxNodes/vfxNodeMapEase.h"
#include "vfxNodes/vfxNodeMapRange.h"
#include "vfxNodes/vfxNodeMath.h"
#include "vfxNodes/vfxNodeMouse.h"
#include "vfxNodes/vfxNodeOsc.h"
#include "vfxNodes/vfxNodeOscPrimitives.h"
#include "vfxNodes/vfxNodePicture.h"
#include "vfxNodes/vfxNodeVideo.h"

#include "../libparticle/ui.h"

using namespace tinyxml2;

/*

todo :
+ replace surface type inputs and outputs to image type
+ add VfxImageBase type. let VfxPlug use this type for image type inputs and outputs. has virtual getTexture method
+ add VfxImage_Surface type. let VfxNodeFsfx use this type
+ add VfxPicture type. type name = 'picture'
+ add VfxImage_Texture type. let VfxPicture use this type
+ add VfxVideo type. type name = 'video'
- add default value to socket definitions
	- add to XML
	- add ability to reset values to their default in UI
+ add editorValue to node inputs and outputs. let get*** methods use this value when plug is not connected
+ let graph editor set editorValue for nodes. only when editor is set on type definition
+ add socket connection selection. remove connection on BACKSPACE
+ add multiple node selection
# on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
# add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
+ remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values
- add zoom in/out
	+ add basic implementation
	- improve zoom in and out behavior
		- clamp max zoom level
		- improve font rendering so it's both resolution independent and supports sub-pixel translation
	+ save/load zoom and focus position to/from XML
	+ add option to quickly reset drag and zoom values
	+ use arrow keys to navigate workspace (when no nodes are selected)
+ add sine, saw, triangle and square oscillators
+ save/load link ids
+ save/load next alloc ids for nodes and links
+ free literal values on graph free
+ recreate DatGui when loading graph / current node gets freed
# prioritize input between DatGui and graph editor. do hit test on DatGui
+ add 'color' type name
+ implement OSC node
+ implement Leap Motion node
- add undo/redo support. just serialize/deserialize graph for every action?
	- note : serialize/deserialize entire graph doesn't work nicely with real-time connection
			 we will need to serialize node on remove and re-add/restore it during undo (also invoking real-time connection)
			 same for links and all other actions. we need to perform the opposite action on undo
+ UI element focus: graph editor vs property editor
+ add ability to collapse nodes, so they take up less space
	+ SPACE to toggle
	+ fix hit test
	+ fix link end point locations
+ passthrough toggle on selection: check if all passthrough. yes? disable passthrough, else enable
+ add socket output value editing, for node types that define it on their outputs. required for literals
- add enum value types. use combo box to select values
	- define enums for ease node type
- add ability to randomize input values
# fix white screen issue on Windows when GUI is visible
- add trigger support
+ add real-time connection
	+ editing values updates values in live version
	+ marking nodes passthrough gets reflected in live
	# disabling nodes and links gets reflected in live -> deferred until later
	+ adding nodes should add nodes in live
	+ removing nodes should remove nodes in live
	+ adding links should add nodes in live
	+ removing links should remove nodes in live
+ add reverse real-time connection
	+ let graph edit sample socket input and output values
		+ let graph edit show a graph of the values when hovering over a socket
+ make it possible to disable nodes
+ make it possible to disable links
- add drag and drop support string literals
+ integrate with UI from libparticle. it supports enums, better color picking, incrementing values up and down in checkboxes
+ add mouse up/down movement support to increment/decrement values of int/float text boxes
+ add option to specify (in UiState) how far text boxes indent their text fields
+ add history of last nodes added
- insert node on pressing enter in the node type name box
	+ or when pressing one of the suggestion buttons
- add suggestion based purely on matching first part of string (no fuzzy string comparison)
	- order of listing should be : pure matches, fuzzy matches, history. show history once type name text box is made active
	- clear type name text box when adding node
- automatically hide UI when mouse/keyboard is inactive for a while
+ remove 'editor' code
+ allocate literal values for unconnected plugs when live-editing change comes in for input
- show which nodes and links are actively traversed. add live-connection callback to query activity
- show min/max on valueplotter

todo : nodes :
- add ease node
	- value
	- ease type
	- ease param1
	- ease param2
	- mirror?
	- result
- add time node
- add timer node
- add sample.float node
- add sample.image node. outputs r/g/b/a. specify normalized vs screen coords?
- add impulse response node. measure input impulse response with oscilator at given frequency
- add sample and hold node. has trigger for input
- add doValuePlotter to ui framework
- add simplex noise node
- add binary counter node, outputting 4-8 bit values (1.f or 0.f)
- add delay node. 4 inputs for delay. take max for delay buffer. delay buffer filled at say fixed 120 hz. 4 outputs delayed values
- add quantize node

todo : fsfx :
- let FSFX use fsfx.vs vertex shader. don't require effects to have their own vertex shader
- expose uniforms/inputs from FSFX pixel shader
- iterate FSFX pixel shaders and generate type definitions based on FSFX name and exposed uniforms

reference :
+ http://www.dsperados.com (company based in Utrecht ? send to Stijn)

*/

#define FILENAME "graph2.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testDatGui();
extern void testNanovg();
extern void testFourier2d();
extern void testChaosGame();
extern void testImpulseResponseMeasurement();

struct VfxNodeDisplay : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_COUNT
	};
	
	VfxNodeDisplay()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, 0);
		addInput(kInput_Image, kVfxPlugType_Image);
	}
	
	const VfxImageBase * getImage() const
	{
		return getInputImage(kInput_Image, nullptr);
	}
};

struct VfxNodeTransform2d : VfxNodeBase
{
	enum Input
	{
		kInput_X,
		kInput_Y,
		kInput_Scale,
		kInput_ScaleX,
		kInput_ScaleY,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Transform,
		kOutput_COUNT
	};
	
	VfxTransform transform;
	
	VfxNodeTransform2d()
		: VfxNodeBase()
		, transform()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_X, kVfxPlugType_Float);
		addInput(kInput_Y, kVfxPlugType_Float);
		addInput(kInput_Scale, kVfxPlugType_Float);
		addInput(kInput_ScaleX, kVfxPlugType_Float);
		addInput(kInput_ScaleY, kVfxPlugType_Float);
		addInput(kInput_Angle, kVfxPlugType_Float);
		addOutput(kOutput_Transform, kVfxPlugType_Transform, &transform);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		// todo : parse node.editorValue;
	}
	
	virtual void tick(const float dt) override
	{
		const float x = getInputFloat(kInput_X, 0.f);
		const float y = getInputFloat(kInput_Y, 0.f);
		const float scale = getInputFloat(kInput_Scale, 1.f);
		const float scaleX = getInputFloat(kInput_ScaleX, 1.f);
		const float scaleY = getInputFloat(kInput_ScaleY, 1.f);
		const float angle = getInputFloat(kInput_Angle, 0.f);
		
		Mat4x4 t;
		Mat4x4 s;
		Mat4x4 r;
		
		t.MakeTranslation(x, y, 0.f);
		s.MakeScaling(scale * scaleX, scale * scaleY, 1.f);
		r.MakeRotationZ(Calc::DegToRad(angle));
		
		transform.matrix = t * r * s;
	}
};

struct VfxGraph
{
	struct ValueToFree
	{
		enum Type
		{
			kType_Unknown,
			kType_Bool,
			kType_Int,
			kType_Float,
			kType_Transform,
			kType_String,
			kType_Color
		};
		
		Type type;
		void * mem;
		
		ValueToFree()
			: type(kType_Unknown)
			, mem(nullptr)
		{
		}
		
		ValueToFree(const Type _type, void * _mem)
			: type(_type)
			, mem(_mem)
		{
		}
	};
	
	std::map<GraphNodeId, VfxNodeBase*> nodes;
	
	GraphNodeId displayNodeId;
	
	mutable int nextTickTraversalId;
	mutable int nextDrawTraversalId;
	
	Graph * graph; // todo : remove ?
	
	std::vector<ValueToFree> valuesToFree;
	
	VfxGraph()
		: nodes()
		, displayNodeId(kGraphNodeIdInvalid)
		, nextTickTraversalId(0)
		, nextDrawTraversalId(0)
		, graph(nullptr)
		, valuesToFree()
	{
	}
	
	~VfxGraph()
	{
		destroy();
	}
	
	void destroy()
	{
		graph = nullptr;
		
		displayNodeId = kGraphNodeIdInvalid;
		
		for (auto i : valuesToFree)
		{
			switch (i.type)
			{
			case ValueToFree::kType_Bool:
				delete (bool*)i.mem;
				break;
			case ValueToFree::kType_Int:
				delete (int*)i.mem;
				break;
			case ValueToFree::kType_Float:
				delete (float*)i.mem;
				break;
			case ValueToFree::kType_Transform:
				delete (VfxTransform*)i.mem;
				break;
			case ValueToFree::kType_String:
				delete (std::string*)i.mem;
				break;
			case ValueToFree::kType_Color:
				delete (Color*)i.mem;
				break;
			default:
				Assert(false);
				break;
			}
		}
		
		valuesToFree.clear();
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			delete node;
			node = nullptr;
		}
		
		nodes.clear();
	}
	
	void connectToInputLiteral(VfxPlug & input, const std::string & inputValue)
	{
		if (input.type == kVfxPlugType_Bool)
		{
			bool * value = new bool();
			
			*value = Parse::Bool(inputValue);
			
			input.connectTo(value, kVfxPlugType_Bool);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Bool, value));
		}
		else if (input.type == kVfxPlugType_Int)
		{
			int * value = new int();
			
			*value = Parse::Int32(inputValue);
			
			input.connectTo(value, kVfxPlugType_Int);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Int, value));
		}
		else if (input.type == kVfxPlugType_Float)
		{
			float * value = new float();
			
			*value = Parse::Float(inputValue);
			
			input.connectTo(value, kVfxPlugType_Float);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Float, value));
		}
		else if (input.type == kVfxPlugType_Transform)
		{
			VfxTransform * value = new VfxTransform();
			
			// todo : parse inputValue
			
			input.connectTo(value, kVfxPlugType_Transform);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Transform, value));
		}
		else if (input.type == kVfxPlugType_String)
		{
			std::string * value = new std::string();
			
			*value = inputValue;
			
			input.connectTo(value, kVfxPlugType_String);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
		}
		else if (input.type == kVfxPlugType_Color)
		{
			Color * value = new Color();
			
			*value = Color::fromHex(inputValue.c_str());
			
			input.connectTo(value, kVfxPlugType_Color);
			
			valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Color, value));
		}
		else
		{
			logWarning("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
		}
	}
	
	void tick(const float dt)
	{
		// todo : use traversalId, start update at display node
		
		if (displayNodeId != kGraphNodeIdInvalid)
		{
			auto nodeItr = nodes.find(displayNodeId);
			Assert(nodeItr != nodes.end());
			if (nodeItr != nodes.end())
			{
				auto node = nodeItr->second;
				
				VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
				
				displayNode->traverseTick(nextTickTraversalId, dt);
			}
		}
		
		// todo : process nodes that aren't connected to the display node ?
		//        perhaps process them as islands, following predeps again ?
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			if (node->lastTickTraversalId != nextTickTraversalId)
			{
				node->lastTickTraversalId = nextTickTraversalId;
				
				node->tick(dt);
			}
		}
		
		++nextTickTraversalId;
	}
	
	void draw() const
	{
		// start traversal at the display node and traverse to leafs following predeps and and back up the tree again to draw
		
		if (displayNodeId != kGraphNodeIdInvalid)
		{
			auto nodeItr = nodes.find(displayNodeId);
			Assert(nodeItr != nodes.end());
			if (nodeItr != nodes.end())
			{
				auto node = nodeItr->second;
				
				VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
				
				displayNode->traverseDraw(nextDrawTraversalId);
				
				const VfxImageBase * image = displayNode->getImage();
				
				if (image != nullptr)
				{
					gxSetTexture(image->getTexture());
					pushBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					popBlend();
					gxSetTexture(0);
				}
			}
		}
		
		++nextDrawTraversalId;
	}
};

static VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph)
{
	VfxNodeBase * vfxNode = nullptr;
	
#define DefineNodeImpl(_typeName, _type) \
	else if (typeName == _typeName) \
		vfxNode = new _type();
	
	if (typeName == "intBool")
	{
		vfxNode = new VfxNodeBoolLiteral();
	}
	else if (typeName == "intLiteral")
	{
		vfxNode = new VfxNodeIntLiteral();
	}
	else if (typeName == "floatLiteral")
	{
		vfxNode = new VfxNodeFloatLiteral();
	}
	else if (typeName == "transformLiteral")
	{
		vfxNode = new VfxNodeTransformLiteral();
	}
	else if (typeName == "stringLiteral")
	{
		vfxNode = new VfxNodeStringLiteral();
	}
	else if (typeName == "colorLiteral")
	{
		vfxNode = new VfxNodeColorLiteral();
	}
	DefineNodeImpl("transform.2d", VfxNodeTransform2d)
	DefineNodeImpl("math.range", VfxNodeMapRange)
	DefineNodeImpl("ease", VfxNodeMapEase)
	DefineNodeImpl("math.add", VfxNodeMathAdd)
	DefineNodeImpl("math.sub", VfxNodeMathSub)
	DefineNodeImpl("math.mul", VfxNodeMathMul)
	DefineNodeImpl("math.sin", VfxNodeMathSin)
	DefineNodeImpl("math.cos", VfxNodeMathCos)
	DefineNodeImpl("math.abs", VfxNodeMathAbs)
	DefineNodeImpl("math.min", VfxNodeMathMin)
	DefineNodeImpl("math.max", VfxNodeMathMax)
	DefineNodeImpl("math.sat", VfxNodeMathSat)
	DefineNodeImpl("math.neg", VfxNodeMathNeg)
	DefineNodeImpl("math.sqrt", VfxNodeMathSqrt)
	DefineNodeImpl("math.pow", VfxNodeMathPow)
	DefineNodeImpl("math.exp", VfxNodeMathExp)
	DefineNodeImpl("math.mod", VfxNodeMathMod)
	DefineNodeImpl("math.fract", VfxNodeMathFract)
	DefineNodeImpl("math.floor", VfxNodeMathFloor)
	DefineNodeImpl("math.ceil", VfxNodeMathCeil)
	DefineNodeImpl("math.round", VfxNodeMathRound)
	DefineNodeImpl("math.sign", VfxNodeMathSign)
	DefineNodeImpl("math.hypot", VfxNodeMathHypot)
	DefineNodeImpl("osc.sine", VfxNodeOscSine)
	DefineNodeImpl("osc.saw", VfxNodeOscSaw)
	DefineNodeImpl("osc.triangle", VfxNodeOscTriangle)
	DefineNodeImpl("osc.square", VfxNodeOscSquare)
	else if (typeName == "display")
	{
		vfxNode = new VfxNodeDisplay();
		
		// fixme : move display node id handling out of here. remove nodeId and vfxGraph passed in to this function
		Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
		vfxGraph->displayNodeId = nodeId;
	}
	else if (typeName == "mouse")
	{
		vfxNode = new VfxNodeMouse();
	}
	else if (typeName == "leap")
	{
		vfxNode = new VfxNodeLeapMotion();
	}
	else if (typeName == "osc")
	{
		vfxNode = new VfxNodeOsc();
	}
	else if (typeName == "composite")
	{
		vfxNode = new VfxNodeComposite();
	}
	else if (typeName == "picture")
	{
		vfxNode = new VfxNodePicture();
	}
	else if (typeName == "video")
	{
		vfxNode = new VfxNodeVideo();
	}
	else if (typeName == "fsfx")
	{
		vfxNode = new VfxNodeFsfx();
	}
	else
	{
		logError("unknown node type: %s", typeName.c_str());
	}
	
#undef DefineNodeImpl

	return vfxNode;
}

static VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		if (node.isEnabled == false)
		{
			continue;
		}
		
		VfxNodeBase * vfxNode = createVfxNode(node.id, node.typeName, vfxGraph);
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			vfxNode->isPassthrough = node.editorIsPassthrough;
			
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		if (link.isEnabled == false)
		{
			continue;
		}
		
		auto srcNodeItr = vfxGraph->nodes.find(link.srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(link.dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
		}
		else
		{
			auto srcNode = srcNodeItr->second;
			auto dstNode = dstNodeItr->second;
			
			auto input = srcNode->tryGetInput(link.srcNodeSocketIndex);
			auto output = dstNode->tryGetOutput(link.dstNodeSocketIndex);
			
			Assert(input != nullptr && output != nullptr);
			if (input == nullptr || output == nullptr)
			{
				if (input == nullptr)
					logError("input node socket doesn't exist");
				if (output == nullptr)
					logError("output node socket doesn't exist");
			}
			else
			{
				input->connectTo(*output);
				
				// note : this may add the same node multiple times to the list of predeps. note that this
				//        is ok as nodes will be traversed once through the travel id + it works nicely
				//        with the live connection as we can just remove the predep and still have one or
				//        references to the predep if the predep was referenced more than once
				srcNode->predeps.push_back(dstNode);
			}
		}
	}
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefintion = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefintion == nullptr)
			continue;
		
		auto vfxNodeItr = vfxGraph->nodes.find(node.id);
		
		if (vfxNodeItr == vfxGraph->nodes.end())
			continue;
		
		VfxNodeBase * vfxNode = vfxNodeItr->second;
		
		auto & vfxNodeInputs = vfxNode->inputs;
		
		for (auto inputValueItr : node.editorInputValues)
		{
			const std::string & inputName = inputValueItr.first;
			const std::string & inputValue = inputValueItr.second;
			
			for (size_t i = 0; i < typeDefintion->inputSockets.size(); ++i)
			{
				if (typeDefintion->inputSockets[i].name == inputName)
				{
					if (i < vfxNodeInputs.size())
					{
						if (vfxNodeInputs[i].isConnected() == false)
						{
							vfxGraph->connectToInputLiteral(vfxNodeInputs[i], inputValue);
						}
					}
				}
			}
		}
	}
	
	for (auto vfxNodeItr : vfxGraph->nodes)
	{
		auto nodeId = vfxNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto vfxNode = vfxNodeItr.second;
		
		vfxNode->init(node);
	}
	
	return vfxGraph;
}

struct RealTimeConnection : GraphEdit_RealTimeConnection
{
	VfxGraph * vfxGraph;
	
	RealTimeConnection()
		: GraphEdit_RealTimeConnection()
		, vfxGraph(nullptr)
	{
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		logDebug("nodeAdd");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr == vfxGraph->nodes.end());
		if (nodeItr != vfxGraph->nodes.end())
			return;
		
		//
		
		GraphNode node;
		node.id = nodeId;
		node.typeName = typeName;
		
		//
		
		VfxNodeBase * vfxNode = createVfxNode(nodeId, typeName, vfxGraph);
		vfxNode->initSelf(node);
		
		vfxGraph->nodes[node.id] = vfxNode;
		
		//
		
		vfxNode->init(node);
	}
	
	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		logDebug("nodeRemove");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		// all links should be removed at this point. there should be no remaining predeps or connected nodes
		Assert(node->predeps.empty());
		//for (auto & input : node->inputs)
		//	Assert(!input.isConnected()); // may be a literal value node with a non-accounted for (in the graph) connection when created directly from socket value
		// todo : iterate all other nodes, to ensure there are no nodes with references back to this node?
		
		delete node;
		node = nullptr;
		
		vfxGraph->nodes.erase(nodeItr);
	}
	
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		logDebug("linkAdd");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
			
			return;
		}

		auto srcNode = srcNodeItr->second;
		auto dstNode = dstNodeItr->second;
		
		auto input = srcNode->tryGetInput(srcSocketIndex);
		auto output = dstNode->tryGetOutput(dstSocketIndex);
		
		Assert(input != nullptr && output != nullptr);
		if (input == nullptr || output == nullptr)
		{
			if (input == nullptr)
				logError("input node socket doesn't exist");
			if (output == nullptr)
				logError("output node socket doesn't exist");
			
			return;
		}
		
		input->connectTo(*output);
		
		// note : this may add the same node multiple times to the list of predeps. note that this
		//        is ok as nodes will be traversed once through the travel id + it works nicely
		//        with the live connection as we can just remove the predep and still have one or
		//        references to the predep if the predep was referenced more than once
		srcNode->predeps.push_back(dstNode);
	}
	
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		logDebug("linkRemove");
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(srcNodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return;
		
		Assert(input->isConnected());
		input->disconnect();
		
		{
			// attempt to remove dst node from predeps
			
			auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
			
			Assert(dstNodeItr != vfxGraph->nodes.end());
			if (dstNodeItr != vfxGraph->nodes.end())
			{
				auto dstNode = dstNodeItr->second;
				
				bool foundPredep = false;
				
				for (auto i = node->predeps.begin(); i != node->predeps.end(); ++i)
				{
					VfxNodeBase * vfxNode = *i;
					
					if (vfxNode == dstNode)
					{
						node->predeps.erase(i);
						foundPredep = true;
						break;
					}
				}
				
				Assert(foundPredep);
			}
		}
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		logDebug("setNodeIsPassthrough called for nodeId=%d, isPassthrough=%d", int(nodeId), int(isPassthrough));
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		node->isPassthrough = isPassthrough;
	}
	
	static bool setPlugValue(VfxPlug * plug, const std::string & value)
	{
		switch (plug->type)
		{
		case kVfxPlugType_None:
			return false;
		case kVfxPlugType_Bool:
			plug->getRwBool() = Parse::Bool(value);
			return true;
		case kVfxPlugType_Int:
			plug->getRwInt() = Parse::Int32(value);
			return true;
		case kVfxPlugType_Float:
			plug->getRwFloat() = Parse::Float(value);
			return true;
			
		case kVfxPlugType_Transform:
			return false;
			
		case kVfxPlugType_String:
			plug->getRwString() = value;
			return true;
		case kVfxPlugType_Color:
			plug->getRwColor() = Color::fromHex(value.c_str());
			return true;
			
		case kVfxPlugType_Image:
			return false;
		case kVfxPlugType_Surface:
			return false;
		case kVfxPlugType_Trigger:
			return false;
		}
		
		Assert(false); // all cases should be handled explicitly
		return false;
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		logDebug("setSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
		
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return;
		
		if (input->isConnected())
		{
			setPlugValue(input, value);
		}
		else
		{
			vfxGraph->connectToInputLiteral(*input, value);
		}
	}
	
	static bool getPlugValue(VfxPlug * plug, std::string & value)
	{
		switch (plug->type)
		{
		case kVfxPlugType_None:
			return false;
		case kVfxPlugType_Bool:
			value = String::ToString(plug->getBool());
			return true;
		case kVfxPlugType_Int:
			value = String::ToString(plug->getInt());
			return true;
		case kVfxPlugType_Float:
			value = String::FormatC("%f", plug->getFloat());
			return true;
		case kVfxPlugType_Transform:
			return false;
		case kVfxPlugType_String:
			value = plug->getString();
			return true;
		case kVfxPlugType_Color:
			{
				const Color & color = plug->getColor();
				value = color.toHexString(true);
				return true;
			}
		case kVfxPlugType_Image:
			value = String::ToString(plug->getImage()->getTexture());
			return true;
		case kVfxPlugType_Surface:
			return false;
		case kVfxPlugType_Trigger:
			return false;
		}
		
		Assert(false); // all cases should be handled explicitly
		return false;
	}
	
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		Assert(input != nullptr);
		if (input == nullptr)
			return false;
		
		if (input->isConnected() == false)
			return false;
		
		return getPlugValue(input, value);
	}
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto output = node->tryGetOutput(dstSocketIndex);
		
		Assert(output != nullptr);
		if (output == nullptr)
			return false;
		
		return getPlugValue(output, value);
	}
	
	virtual bool nodeIsActive(const GraphNodeId nodeId) override
	{
		Assert(vfxGraph != nullptr);
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		Assert(nodeItr != vfxGraph->nodes.end());
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		return node->lastDrawTraversalId + 1 == vfxGraph->nextDrawTraversalId;
	}
	
	virtual bool linkIsActive(const GraphLinkId linkId) override
	{
		return false;
	}
};

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		//testDatGui();
		
		//testNanovg();
		
		//testChaosGame();
		
		//testFourier2d();
		
		//testImpulseResponseMeasurement();
		
		//
		
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		{
			XMLDocument * document = new XMLDocument();
			
			if (document->LoadFile("types.xml") == XML_SUCCESS)
			{
				const XMLElement * xmlLibrary = document->FirstChildElement("library");
				
				if (xmlLibrary != nullptr)
				{
					typeDefinitionLibrary->loadXml(xmlLibrary);
				}
			}
			
			delete document;
			document = nullptr;
		}
		
		//
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection();
		
		//
		
		GraphEdit * graphEdit = new GraphEdit();
		
		graphEdit->typeDefinitionLibrary = typeDefinitionLibrary;
		
		graphEdit->realTimeConnection = realTimeConnection;
		
		if (graphEdit->propertyEditor != nullptr)
		{
			graphEdit->propertyEditor->typeLibrary = typeDefinitionLibrary;
			
			graphEdit->propertyEditor->setGraph(graphEdit->graph);
		}

		VfxGraph * vfxGraph = nullptr;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			if (graphEdit->tick(dt))
			{
			}
			else if (keyboard.wentDown(SDLK_s))
			{
				FILE * file = fopen(FILENAME, "wt");
				
				XMLPrinter xmlGraph(file);;
				
				xmlGraph.OpenElement("graph");
				{
					graphEdit->graph->saveXml(xmlGraph, typeDefinitionLibrary);
					
					xmlGraph.OpenElement("editor");
					{
						graphEdit->saveXml(xmlGraph);
					}
					xmlGraph.CloseElement();
				}
				xmlGraph.CloseElement();
				
				fclose(file);
				file = nullptr;
			}
			else if (keyboard.wentDown(SDLK_l))
			{
				graphEdit->realTimeConnection = nullptr;
				
				delete graphEdit->graph;
				graphEdit->graph = nullptr;
				
				graphEdit->propertyEditor->setGraph(nullptr);
				
				//
				
				graphEdit->graph = new Graph();
				graphEdit->graph->graphEditConnection = graphEdit;
				
				// fixme : we should delete/new the property editor here ..
				
				delete graphEdit->propertyEditor;
				graphEdit->propertyEditor = nullptr;
				
				graphEdit->propertyEditor = new GraphUi::PropEdit(typeDefinitionLibrary, graphEdit);
				
				//
				
				XMLDocument document;
				document.LoadFile(FILENAME);
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				if (xmlGraph != nullptr)
				{
					graphEdit->graph->loadXml(xmlGraph, typeDefinitionLibrary);
					
					const XMLElement * xmlEditor = xmlGraph->FirstChildElement("editor");
					if (xmlEditor != nullptr)
					{
						graphEdit->loadXml(xmlEditor);
					}
				}
				
				// fixme : it's a bit messy to manually assign it here. we shouldn't have to do this
				
				if (graphEdit->propertyEditor != nullptr)
				{
					graphEdit->propertyEditor->typeLibrary = typeDefinitionLibrary;
					
					graphEdit->propertyEditor->setGraph(graphEdit->graph);
				}
				
				//
				
				delete vfxGraph;
				vfxGraph = nullptr;
				
				vfxGraph = constructVfxGraph(*graphEdit->graph, typeDefinitionLibrary);
				
			#if 0
				delete vfxGraph;
				vfxGraph = nullptr;
			#endif
				
				realTimeConnection->vfxGraph = vfxGraph;
				graphEdit->realTimeConnection = realTimeConnection;
			}
			
			if (!graphEdit->selectedNodes.empty())
			{
				const GraphNodeId nodeId = *graphEdit->selectedNodes.begin();
				
				graphEdit->propertyEditor->setNode(nodeId);
			}
			
			framework.beginDraw(31, 31, 31, 255);
			{
				if (vfxGraph != nullptr)
				{
					vfxGraph->tick(framework.timeStep);
					vfxGraph->draw();
				}
				
				graphEdit->draw();
			}
			framework.endDraw();
		}
		
		delete graphEdit;
		graphEdit = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete typeDefinitionLibrary;
		typeDefinitionLibrary = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
