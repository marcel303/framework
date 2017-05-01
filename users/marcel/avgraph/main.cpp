#include "Calc.h"
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
- on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
- add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
+ remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values
- add zoom in/out
	+ add basic implementation
	- improve zoom in and out behavior
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
+ UI element focus: graph editor vs property editor
+ add ability to collapse nodes, so they take up less space
	+ SPACE to toggle
	+ fix hit test
	+ fix link end point locations
+ passthrough toggle on selection: check if all passthrough. yes? disable passthrough, else enable
- add socket output value editing, for node types that define it on their outputs. required for literals
- add enum value types. use combo box to select values
- add ability to randomize input values
# fix white screen issue on Windows when GUI is visible
- add trigger support
- add real-time connection
	+ editing values updates values in live version
	- marking nodes passthrough gets reflected in live
	- disabling nodes and links gets reflected in live
+ add reverse real-time connection
	+ let graph edit sample socket input and output values
		+ let graph edit show a graph of the values when hovering over a socket
+ make it possible to disable nodes
+ make it possible to disable links
- add drag and drop support string literals
+ integrate with UI from libparticle. it supports enums, better color picking, incrementing values up and down in checkboxes
+ add mouse up/down movement support to increment/decrement values of int/float text boxes
+ add option to specify (in UiState) how far text boxes indent their text fields
- add history of last nodes added
- insert node on pressing enter in the node type name box or when pressing one of the suggestion buttons
- add suggestion based purely on matching first part of string (no fuzzy string comparison)
	- order of listing should be : pure matches, fuzzy matches, history. show history once type name text box is made active
	- clear type name text box when adding node
- automatically hide UI when mouse/keyboard is inactive for a while
+ remove 'editor' code
- allocate literal values for unconnected plugs when live-editing change comes in for input

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

struct VfxNodeBoolLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	bool value;
	
	VfxNodeBoolLiteral()
		: VfxNodeBase()
		, value(false)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Bool, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Bool(node.editorValue);
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

struct VfxNodeTransformLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	VfxTransform value;
	
	VfxNodeTransformLiteral()
		: VfxNodeBase()
		, value()
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Transform, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		// todo : parse node.editorValue;
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

struct VfxNodeColorLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	Color value;
	
	VfxNodeColorLiteral()
		: VfxNodeBase()
		, value(1.f, 1.f, 1.f, 1.f)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Color, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Color::fromHex(node.editorValue.c_str());
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

struct VfxNodeRange : VfxNodeBase
{
	enum Input
	{
		kInput_In,
		kInput_InMin,
		kInput_InMax,
		kInput_OutMin,
		kInput_OutMax,
		kInput_OutCurvePow,
		kInput_Clamp,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutputValue,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeRange()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_In, kVfxPlugType_Float);
		addInput(kInput_InMin, kVfxPlugType_Float);
		addInput(kInput_InMax, kVfxPlugType_Float);
		addInput(kInput_OutMin, kVfxPlugType_Float);
		addInput(kInput_OutMax, kVfxPlugType_Float);
		addInput(kInput_OutCurvePow, kVfxPlugType_Float);
		addInput(kInput_Clamp, kVfxPlugType_Bool);
		addOutput(kOutputValue, kVfxPlugType_Float, &outputValue);
	}
	
	virtual void tick(const float dt) override
	{
		const float in = getInputFloat(kInput_In, 0.f);
		const float inMin = getInputFloat(kInput_InMin, 0.f);
		const float inMax = getInputFloat(kInput_InMax, 1.f);
		const float outMin = getInputFloat(kInput_OutMin, 0.f);
		const float outMax = getInputFloat(kInput_OutMax, 1.f);
		const float outCurvePow = getInputFloat(kInput_OutCurvePow, 1.f);
		const bool clamp = getInputBool(kInput_Clamp, false);
		
		float t = (in - inMin) / (inMax - inMin);
		if (clamp)
			t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
		t = std::powf(t, outCurvePow);
		
		const float t1 = t;
		const float t2 = 1.f - t;
		
		outputValue = outMax * t1 + outMin * t2;
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
	
	bool isPassthrough;
	
	VfxNodeMath(Type _type)
		: VfxNodeBase()
		, type(kType_Unknown)
		, result(0.f)
		, isPassthrough(false)
	{
		type = _type;
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_A, kVfxPlugType_Float);
		addInput(kInput_B, kVfxPlugType_Float);
		addOutput(kOutput_R, kVfxPlugType_Float, &result);
	}
	
	virtual void init(const GraphNode & node) override
	{
		isPassthrough = node.editorIsPassthrough;
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
			if (isPassthrough)
				r = a;
			else
				r = std::sin(a);
			break;
			
		case kType_Cos:
			if (isPassthrough)
				r = a;
			else
				r = std::cos(a);
			break;
			
		case kType_Abs:
			if (isPassthrough)
				r = a;
			else
				r = std::abs(a);
			break;
			
		case kType_Min:
			r = std::min(a, b);
			break;
			
		case kType_Max:
			r = std::max(a, b);
			break;
			
		case kType_Sat:
			if (isPassthrough)
				r = a;
			else
				r = std::max(0.f, std::min(1.f, a));
			break;
			
		case kType_Neg:
			if (isPassthrough)
				r = a;
			else
				r = -a;
			break;
			
		case kType_Sqrt:
			if (isPassthrough)
				r = a;
			else
				r = std::sqrt(a);
			break;
			
		case kType_Pow:
			if (isPassthrough)
				r = a;
			else
				r = std::pow(a, b);
			break;
			
		case kType_Exp:
			if (isPassthrough)
				r = a;
			else
				r = std::exp(a);
			break;
			
		case kType_Mod:
			if (isPassthrough)
				r = a;
			else
				r = std::fmod(a, b);
			break;
			
		case kType_Fract:
			if (isPassthrough)
				r = a;
			else if (a >= 0.f)
				r = a - std::floor(a);
			else
				r = a - std::ceil(a);
			break;
			
		case kType_Floor:
			if (isPassthrough)
				r = a;
			else
				r = std::floor(a);
			break;
			
		case kType_Ceil:
			if (isPassthrough)
				r = a;
			else
				r = std::ceil(a);
			break;
			
		case kType_Round:
			if (isPassthrough)
				r = a;
			else
				r = std::round(a);
			break;
			
		case kType_Sign:
			if (isPassthrough)
				r = a;
			else
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

DefineMathNode(VfxNodeMathAdd, kType_Add);
DefineMathNode(VfxNodeMathSub, kType_Sub);
DefineMathNode(VfxNodeMathMul, kType_Mul);
DefineMathNode(VfxNodeMathSin, kType_Sin);
DefineMathNode(VfxNodeMathCos, kType_Cos);
DefineMathNode(VfxNodeMathAbs, kType_Abs);
DefineMathNode(VfxNodeMathMin, kType_Min);
DefineMathNode(VfxNodeMathMax, kType_Max);
DefineMathNode(VfxNodeMathSat, kType_Sat);
DefineMathNode(VfxNodeMathNeg, kType_Neg);
DefineMathNode(VfxNodeMathSqrt, kType_Sqrt);
DefineMathNode(VfxNodeMathPow, kType_Pow);
DefineMathNode(VfxNodeMathExp, kType_Exp);
DefineMathNode(VfxNodeMathMod, kType_Mod);
DefineMathNode(VfxNodeMathFract, kType_Fract);
DefineMathNode(VfxNodeMathFloor, kType_Floor);
DefineMathNode(VfxNodeMathCeil, kType_Ceil);
DefineMathNode(VfxNodeMathRound, kType_Round);
DefineMathNode(VfxNodeMathSign, kType_Sign);
DefineMathNode(VfxNodeMathHypot, kType_Hypot);

#undef DefineMathNode

struct VfxNodeComposite : VfxNodeBase
{
	enum Input
	{
		kInput_Image1,
		kInput_Transform1,
		kInput_Image2,
		kInput_Transform2,
		kInput_Image3,
		kInput_Transform3,
		kInput_Image4,
		kInput_Transform4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture image;
	
	VfxNodeComposite()
		: VfxNodeBase()
		, surface(nullptr)
		, image()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Image1, kVfxPlugType_Image);
		addInput(kInput_Transform1, kVfxPlugType_Transform);
		addInput(kInput_Image2, kVfxPlugType_Image);
		addInput(kInput_Transform2, kVfxPlugType_Transform);
		addInput(kInput_Image3, kVfxPlugType_Image);
		addInput(kInput_Transform3, kVfxPlugType_Transform);
		addInput(kInput_Image4, kVfxPlugType_Image);
		addInput(kInput_Transform4, kVfxPlugType_Transform);
		addOutput(kOutput_Image, kVfxPlugType_Image, &image);
		
		surface = new Surface(GFX_SX, GFX_SY, true);
	}
	
	virtual ~VfxNodeComposite() override
	{
		delete surface;
		surface = nullptr;
	}
	
	virtual void tick(const float dt) override
	{
		pushSurface(surface);
		{
			surface->clear();
			
			const int kNumImages = 4;
			
			const VfxImageBase * images[kNumImages] =
			{
				getInputImage(kInput_Image1, nullptr),
				getInputImage(kInput_Image2, nullptr),
				getInputImage(kInput_Image3, nullptr),
				getInputImage(kInput_Image4, nullptr)
			};
			
			const VfxTransform defaultTransform;
			
			const VfxTransform * transforms[kNumImages] =
			{
				&getInputTransform(kInput_Transform1, defaultTransform),
				&getInputTransform(kInput_Transform2, defaultTransform),
				&getInputTransform(kInput_Transform3, defaultTransform),
				&getInputTransform(kInput_Transform4, defaultTransform)
			};
			
			for (int i = 0; i < kNumImages; ++i)
			{
				if (images[i] == nullptr)
					continue;
				
				gxPushMatrix();
				{
					gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
					gxMultMatrixf(transforms[i]->matrix.m_v);
					gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);
					
					gxSetTexture(images[i]->getTexture());
					{
						setColor(colorWhite);
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					gxSetTexture(0);
				}
				gxPopMatrix();
			}
		}
		popSurface();
		
		image.texture = surface->getTexture();
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
		
		if (node.isEnabled == false)
		{
			continue;
		}
		
		VfxNodeBase * vfxNode = nullptr;
		
		if (node.typeName == "intBool")
		{
			vfxNode = new VfxNodeBoolLiteral();
		}
		else if (node.typeName == "intLiteral")
		{
			vfxNode = new VfxNodeIntLiteral();
		}
		else if (node.typeName == "floatLiteral")
		{
			vfxNode = new VfxNodeFloatLiteral();
		}
		else if (node.typeName == "transformLiteral")
		{
			vfxNode = new VfxNodeTransformLiteral();
		}
		else if (node.typeName == "stringLiteral")
		{
			vfxNode = new VfxNodeStringLiteral();
		}
		else if (node.typeName == "colorLiteral")
		{
			vfxNode = new VfxNodeColorLiteral();
		}
		DefineNodeImpl("transform.2d", VfxNodeTransform2d)
		DefineNodeImpl("math.range", VfxNodeRange)
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
		else if (node.typeName == "composite")
		{
			vfxNode = new VfxNodeComposite();
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
							if (vfxNodeInputs[i].type == kVfxPlugType_Bool)
							{
								bool * value = new bool();
								
								*value = Parse::Bool(inputValue);
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_Bool);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Bool, value));
							}
							else if (vfxNodeInputs[i].type == kVfxPlugType_Int)
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
							else if (vfxNodeInputs[i].type == kVfxPlugType_Transform)
							{
								VfxTransform * value = new VfxTransform();
								
								// todo : parse inputValue
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_Transform);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Transform, value));
							}
							else if (vfxNodeInputs[i].type == kVfxPlugType_String)
							{
								std::string * value = new std::string();
								
								*value = inputValue;
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_String);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
							}
							else if (vfxNodeInputs[i].type == kVfxPlugType_Color)
							{
								Color * value = new Color();
								
								*value = Color::fromHex(inputValue.c_str());
								
								vfxNodeInputs[i].connectTo(value, kVfxPlugType_Color);
								
								vfxGraph->valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Color, value));
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

struct RealTimeConnection : GraphEdit_RealTimeConnection
{
	VfxGraph * vfxGraph;
	
	RealTimeConnection()
		: GraphEdit_RealTimeConnection()
		, vfxGraph(nullptr)
	{
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
		
		if (vfxGraph == nullptr)
			return;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		if (nodeItr == vfxGraph->nodes.end())
			return;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		if (input == nullptr)
			return;
		
		// todo : create literal node type ?
		
		if (input->isConnected() == false)
			return;
		
		setPlugValue(input, value);
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
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto input = node->tryGetInput(srcSocketIndex);
		
		if (input == nullptr)
			return false;
		
		if (input->isConnected() == false)
			return false;
		
		return getPlugValue(input, value);
	}
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		if (vfxGraph == nullptr)
			return false;
		
		auto nodeItr = vfxGraph->nodes.find(nodeId);
		
		if (nodeItr == vfxGraph->nodes.end())
			return false;
		
		auto node = nodeItr->second;
		
		auto output = node->tryGetOutput(dstSocketIndex);
		
		if (output == nullptr)
			return false;
		
		return getPlugValue(output, value);
	}
};

#include "Timer.h"

static void testChaosGame()
{
	const int kNumIterations = 100000;
	
	int64_t * output = new int64_t[kNumIterations * 2];
	
	for (int i = 0; i < kNumIterations * 2; ++i)
		output[i] = 0;
	
	unsigned int randomSeed = 1;
	unsigned int randomPrime = 16807;
	
	const int kNumPoints = 4;
	
	int64_t pointX[kNumPoints];
	int64_t pointY[kNumPoints];
	
	pointX[0] = 0ll   << 32ll;
	pointY[0] = 0ll   << 32ll;
	
	pointX[1] = 600ll << 32ll;
	pointY[1] = 0ll   << 32ll;
	
	pointX[2] = 300ll << 32ll;
	pointY[2] = 600ll << 32ll;
	
	for (int i = 3; i < kNumPoints; ++i)
	{
		pointX[i] = int64_t(rand() % 600) << 32ll;
		pointY[i] = int64_t(rand() % 600) << 32ll;
	}
	
	do
	{
		framework.process();
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			auto t1 = g_TimerRT.TimeUS_get();
			
			pointX[2] = int64_t(mouse.x) << 32ll;
			pointY[2] = int64_t(mouse.y) << 32ll;
			
			int64_t x = pointX[0];
			int64_t y = pointY[0];
			
			int64_t * __restrict outputItr = output;
			
			for (int i = 0; i < kNumIterations; ++i)
			{
				//randomSeed *= randomPrime;
				
				//const int index = randomSeed % 3;
				
				const int index = rand() % kNumPoints;
				
				x = (x * 32 + pointX[index] * 40) >> 6;
				y = (y * 32 + pointY[index] * 40) >> 6;
				
				*outputItr++ = x;
				*outputItr++ = y;
			}
			
			auto t2 = g_TimerRT.TimeUS_get();
			
			printf("eval took %lldus\n", (t2 - t1));
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const float scale = mouse.isDown(BUTTON_LEFT) ? 1.f : (.1f + mouse.x / float(GFX_SX) * (20.f - .1f));
			const float xOffset = mouse.isDown(BUTTON_LEFT) ? 0.f : mouse.y;
			gxScalef(scale, scale, 1.f);
			gxTranslatef(-xOffset, 0.f, 0.f);
			
			setBlend(BLEND_ADD);
			setColorf(.3f, .2f, .1f, 1.f);
			//gxBegin(GL_POINTS);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				const int64_t * __restrict outputItr = output;
				
				//const float percentage = std::fmod(framework.time / 4.f, 1.f);
				//const int numPointsToDraw = kNumIterations * percentage;
				const int numPointsToDraw = kNumIterations;
				
				for (int i = 0; i < numPointsToDraw; ++i)
				{
					const int x = outputItr[0] >> 32;
					const int y = outputItr[1] >> 32;
					
					//gxVertex2f(x, y);
					hqFillCircle(x, y, .4f);
					
					outputItr += 2;
				}
			}
			//gxEnd();
			hqEnd();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete[] output;
	output = nullptr;
}

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
				delete graphEdit->graph;
				graphEdit->graph = nullptr;
				
				graphEdit->propertyEditor->setGraph(nullptr);
				
				//
				
				graphEdit->graph = new Graph();
				
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
