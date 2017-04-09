#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "../avpaint/video.h"

#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeFsfx.h"
#include "vfxNodes/vfxNodeLeapMotion.h"
#include "vfxNodes/vfxNodeMouse.h"
#include "vfxNodes/vfxNodeOsc.h"
#include "vfxNodes/vfxNodeOscPrimitives.h"
#include "vfxNodes/vfxNodePicture.h"
#include "vfxNodes/vfxNodeVideo.h"

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
- add editorValue to node inputs and outputs. let get*** methods use this value when plug is not connected
- let graph editor set editorValue for nodes. only when editor is set on type definition
+ add socket connection selection. remove connection on BACKSPACE
+ add multiple node selection
- on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
- add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
- remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values
- add zoom in/out
	+ add basic implementation
	- improve zoom in and out behavior
	- save/load zoom and focus position to/from XML
	+ add option to quickly reset drag and zoom values
- add sine, saw, triangle and square oscillators
+ save/load link ids
+ save/load next alloc ids for nodes and links
+ free literal values on graph free
+ recreate DatGui when loading graph / current node gets freed
- prioritize input between DatGui and graph editor. do hit test on DatGui
- add 'color' type name
- implement OSC node
- implement Leap Motion node

todo : fsfx :
- let FSFX use fsfx.vs vertex shader. don't require effects to have their own vertex shader
- expose uniforms/inputs from FSFX pixel shader
- iterate FSFX pixel shaders and generate type definitions based on FSFX name and exposed uniforms

reference :
- http://www.dsperados.com (company based in Utrecht ? send to Stijn)

*/

#define GFX_SX 1024
#define GFX_SY 768

extern void testDatGui();
extern void testNanovg();

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
	
	virtual void draw() const override
	{
	#if 0
		const VfxImageBase * image = getImage();
		
		if (image != nullptr)
		{
			gxSetTexture(image->getTexture());
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			drawRect(0, 0, GFX_SX, GFX_SY);
			popBlend();
			gxSetTexture(0);
		}
	#endif
	}
};

struct VfxNodeIntLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	int value;
	
	VfxNodeIntLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Int, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Int32(node.editorValue);
	}
};

struct VfxNodeFloatLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float value;
	
	VfxNodeFloatLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Float(node.editorValue);
	}
};

struct VfxNodeStringLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	std::string value;
	
	VfxNodeStringLiteral()
		: VfxNodeBase()
		, value()
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_String, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = node.editorValue;
	}
};

struct VfxNodeMath : VfxNodeBase
{
	enum Input
	{
		kInput_A,
		kInput_B,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_R,
		kOutput_COUNT
	};
	
	enum Type
	{
		kType_Unknown,
		kType_Add,
		kType_Sub,
		kType_Mul,
		kType_Sin,
		kType_Cos,
		kType_Abs,
		kType_Min,
		kType_Max,
		kType_Sat,
		kType_Neg,
		kType_Sqrt,
		kType_Pow,
		kType_Exp,
		kType_Mod,
		kType_Fract,
		kType_Floor,
		kType_Ceil,
		kType_Round,
		kType_Sign,
		kType_Hypot
	};
	
	Type type;
	float result;
	
	VfxNodeMath(Type _type)
		: VfxNodeBase()
		, type(kType_Unknown)
		, result(0.f)
	{
		type = _type;
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_A, kVfxPlugType_Float);
		addInput(kInput_B, kVfxPlugType_Float);
		addOutput(kOutput_R, kVfxPlugType_Float, &result);
	}
	
	virtual void tick(const float dt) override
	{
		const float a = getInputFloat(kInput_A, 0.f);
		const float b = getInputFloat(kInput_B, 0.f);
		
		float r;
		
		switch (type)
		{
		case kType_Unknown:
			r = 0.f;
			break;
			
		case kType_Add:
			r = a + b;
			break;
		
		case kType_Sub:
			r = a - b;
			break;
			
		case kType_Mul:
			r = a * b;
			break;
			
		case kType_Sin:
			r = std::sin(a);
			break;
			
		case kType_Cos:
			r = std::cos(a);
			break;
			
		case kType_Abs:
			r = std::abs(a);
			break;
			
		case kType_Min:
			r = std::min(a, b);
			break;
			
		case kType_Max:
			r = std::max(a, b);
			break;
			
		case kType_Sat:
			r = std::max(0.f, std::min(1.f, a));
			break;
			
		case kType_Neg:
			r = -a;
			break;
			
		case kType_Sqrt:
			r = std::sqrt(a);
			break;
			
		case kType_Pow:
			r = std::pow(a, b);
			break;
			
		case kType_Exp:
			r = std::exp(a);
			break;
			
		case kType_Mod:
			r = std::fmod(a, b);
			break;
			
		case kType_Fract:
			if (a >= 0.f)
				r = a - std::floor(a);
			else
				r = a - std::ceil(a);
			break;
			
		case kType_Floor:
			r = std::floor(a);
			break;
			
		case kType_Ceil:
			r = std::ceil(a);
			break;
			
		case kType_Round:
			r = std::round(a);
			break;
			
		case kType_Sign:
			r = a < 0.f ? -1.f : +1.f;
			break;
			
		case kType_Hypot:
			r = std::hypot(a, b);
			break;
		}
		
		result = r;
	}
};

#define DefineMathNode(name, type) \
	struct name : VfxNodeMath \
	{ \
		name() \
			: VfxNodeMath(type) \
		{ \
		} \
	};

DefineMathNode(VfxNodeAdd, kType_Add);
DefineMathNode(VfxNodeSub, kType_Sub);
DefineMathNode(VfxNodeMul, kType_Mul);
DefineMathNode(VfxNodeSin, kType_Sin);
DefineMathNode(VfxNodeCos, kType_Cos);
DefineMathNode(VfxNodeAbs, kType_Abs);
DefineMathNode(VfxNodeMin, kType_Min);
DefineMathNode(VfxNodeMax, kType_Max);
DefineMathNode(VfxNodeSat, kType_Sat);
DefineMathNode(VfxNodeNeg, kType_Neg);
DefineMathNode(VfxNodeSqrt, kType_Sqrt);
DefineMathNode(VfxNodePow, kType_Pow);
DefineMathNode(VfxNodeExp, kType_Exp);
DefineMathNode(VfxNodeMod, kType_Mod);
DefineMathNode(VfxNodeFract, kType_Fract);
DefineMathNode(VfxNodeFloor, kType_Floor);
DefineMathNode(VfxNodeCeil, kType_Ceil);
DefineMathNode(VfxNodeRound, kType_Round);
DefineMathNode(VfxNodeSign, kType_Sign);
DefineMathNode(VfxNodeHypot, kType_Hypot);

#undef DefineMathNode

struct VfxGraph
{
	struct ValueToFree
	{
		enum Type
		{
			kType_Unknown,
			kType_Int,
			kType_Float,
			kType_String
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
	
	Graph * graph; // todo : remove ?
	
	std::vector<ValueToFree> valuesToFree;
	
	VfxGraph()
		: nodes()
		, displayNodeId(kGraphNodeIdInvalid)
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
			case ValueToFree::kType_Int:
				delete (int*)i.mem;
				break;
			case ValueToFree::kType_Float:
				delete (float*)i.mem;
				break;
			case ValueToFree::kType_String:
				delete (std::string*)i.mem;
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
	
	void tick(const float dt)
	{
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			node->tick(dt);
		}
	}
	
	void draw() const
	{
		static int traversalId = 0; // fixme
		traversalId++;
		
		// todo : start at output to screen and traverse to leafs and back up again to draw
		
		if (displayNodeId != kGraphNodeIdInvalid)
		{
			auto nodeItr = nodes.find(displayNodeId);
			Assert(nodeItr != nodes.end());
			if (nodeItr != nodes.end())
			{
				auto node = nodeItr->second;
				
				VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
				
				displayNode->traverse(traversalId);
				
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
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			node->draw();
		}
	}
};

static VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
#define DefineNodeImpl(_typeName, _type) \
	else if (node.typeName == _typeName) \
		vfxNode = new _type();

	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		VfxNodeBase * vfxNode = nullptr;
		
		if (node.typeName == "intLiteral")
		{
			vfxNode= new VfxNodeIntLiteral();
		}
		else if (node.typeName == "floatLiteral")
		{
			vfxNode = new VfxNodeFloatLiteral();
		}
		else if (node.typeName == "stringLiteral")
		{
			vfxNode = new VfxNodeStringLiteral();
		}
		DefineNodeImpl("math.add", VfxNodeAdd)
		DefineNodeImpl("math.sub", VfxNodeSub)
		DefineNodeImpl("math.mul", VfxNodeMul)
		DefineNodeImpl("math.sin", VfxNodeSin)
		DefineNodeImpl("math.cos", VfxNodeCos)
		DefineNodeImpl("math.abs", VfxNodeAbs)
		DefineNodeImpl("math.min", VfxNodeMin)
		DefineNodeImpl("math.max", VfxNodeMax)
		DefineNodeImpl("math.sat", VfxNodeSat)
		DefineNodeImpl("math.neg", VfxNodeNeg)
		DefineNodeImpl("math.sqrt", VfxNodeSqrt)
		DefineNodeImpl("math.pow", VfxNodePow)
		DefineNodeImpl("math.exp", VfxNodeExp)
		DefineNodeImpl("math.mod", VfxNodeMod)
		DefineNodeImpl("math.fract", VfxNodeFract)
		DefineNodeImpl("math.floor", VfxNodeFloor)
		DefineNodeImpl("math.ceil", VfxNodeCeil)
		DefineNodeImpl("math.round", VfxNodeRound)
		DefineNodeImpl("math.sign", VfxNodeSign)
		DefineNodeImpl("math.hypot", VfxNodeHypot)
		DefineNodeImpl("osc.sine", VfxNodeOscSine)
		DefineNodeImpl("osc.saw", VfxNodeOscSaw)
		DefineNodeImpl("osc.triangle", VfxNodeOscTriangle)
		DefineNodeImpl("osc.square", VfxNodeOscSquare)
		else if (node.typeName == "display")
		{
			auto vfxDisplayNode = new VfxNodeDisplay();
			
			Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
			vfxGraph->displayNodeId = node.id;
			
			vfxNode = vfxDisplayNode;
		}
		else if (node.typeName == "mouse")
		{
			vfxNode = new VfxNodeMouse();
		}
		else if (node.typeName == "leap")
		{
			vfxNode = new VfxNodeLeapMotion();
		}
		else if (node.typeName == "osc")
		{
			vfxNode = new VfxNodeOsc();
		}
		else if (node.typeName == "picture")
		{
			vfxNode = new VfxNodePicture();
		}
		else if (node.typeName == "video")
		{
			vfxNode = new VfxNodeVideo();
		}
		else if (node.typeName == "fsfx")
		{
			vfxNode = new VfxNodeFsfx();
		}
		else
		{
			logError("unknown node type: %s", node.typeName.c_str());
		}
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
#undef DefineNodeImpl
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
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
							if (vfxNodeInputs[i].type == kVfxPlugType_Int)
							{
								int * value = new int();
								
								*value = Parse::Int32(inputValue);
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_Int);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Int, value));
							}
							else if (vfxNodeInputs[i].type == kVfxPlugType_Float)
							{
								float * value = new float();
								
								*value = Parse::Float(inputValue);
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_Float);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Float, value));
							}
							else if (vfxNodeInputs[i].type == kVfxPlugType_String)
							{
								std::string * value = new std::string();
								
								*value = inputValue;
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_String);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
							}
							else
							{
								logWarning("cannot instantiate literal for non-supported type %d, value=%s", vfxNodeInputs[i].type, inputValue.c_str());
							}
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

int main(int argc, char * argv[])
{
	for (float phase = 0.f; phase <= 1.f; phase += .02f)
	{
		//const float phase2 = std::fmod(phase + .25f, 1.f);
		const float phase2 = std::fmod(phase + .5f, 1.f);
		
		//const float value = 1.f - std::abs(phase2 * 4.f - 2.f);
		const float value = -1.f + 2.f * phase2;
		
		printf("value: %f\n", value);
	}
	
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		//testDatGui();
		
		//testNanovg();
		
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
		
		GraphEdit * graphEdit = new GraphEdit();
		
		graphEdit->typeDefinitionLibrary = typeDefinitionLibrary;
		
		VfxGraph * vfxGraph = nullptr;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			graphEdit->tick(dt);
			
			if (keyboard.wentDown(SDLK_s))
			{
				FILE * file = fopen("graph.xml", "wt");
				
				XMLPrinter xmlGraph(file);;
				
				xmlGraph.OpenElement("graph");
				{
					graphEdit->graph->saveXml(xmlGraph);
				}
				xmlGraph.CloseElement();
				
				fclose(file);
				file = nullptr;
			}
			
			if (keyboard.wentDown(SDLK_l))
			{
				delete graphEdit->graph;
				graphEdit->graph = nullptr;
				
				graphEdit->propertyEditor->setGraph(nullptr);
				
				//
				
				graphEdit->graph = new Graph();
				
				graphEdit->propertyEditor = new GraphUi::PropEdit(typeDefinitionLibrary);
				
				//
				
				XMLDocument document;
				document.LoadFile("graph.xml");
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				if (xmlGraph != nullptr)
					graphEdit->graph->loadXml(xmlGraph);
				
				//
				
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
		
		framework.shutdown();
	}

	return 0;
}
