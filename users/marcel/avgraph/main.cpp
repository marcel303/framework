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
- add live-connection callback so the graph editor can show which nodes and links are actively traversed

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
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeRange()
		: VfxNodeBase()
		, outputValue(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_In, kVfxPlugType_Float);
		addInput(kInput_InMin, kVfxPlugType_Float);
		addInput(kInput_InMax, kVfxPlugType_Float);
		addInput(kInput_OutMin, kVfxPlugType_Float);
		addInput(kInput_OutMax, kVfxPlugType_Float);
		addInput(kInput_OutCurvePow, kVfxPlugType_Float);
		addInput(kInput_Clamp, kVfxPlugType_Bool);
		addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	}
	
	virtual void tick(const float dt) override
	{
		const float in = getInputFloat(kInput_In, 0.f);
		
		if (isPassthrough)
		{
			outputValue = in;
			return;
		}
		
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

#include "Ease.h"

struct VfxNodeEase : VfxNodeBase
{
	enum Input
	{
		kInput_Value,
		kInput_Type,
		kInput_Param,
		kInput_Mirror,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeEase()
		: VfxNodeBase()
		, outputValue(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_Type, kVfxPlugType_Int);
		addInput(kInput_Param, kVfxPlugType_Float);
		addInput(kInput_Mirror, kVfxPlugType_Bool);
		addOutput(kOutput_Result, kVfxPlugType_Float, &outputValue);
	}
	
	virtual void tick(const float dt) override
	{
		float value = getInputFloat(kInput_Value, 0.f);
		
		if (isPassthrough)
		{
			outputValue = value;
			return;
		}
		
		int type = getInputInt(kInput_Type, 0);
		const float param = getInputFloat(kInput_Param, 0.f);
		const bool mirror = getInputBool(kInput_Mirror, false);
		
		if (type < 0 || type >= kEaseType_Count)
			type = kEaseType_Linear;
		
		if (mirror)
		{
			value = std::fmod(std::abs(value), 2.f);
			
			if (value > 1.f)
				value = 2.f - value;
		}
		
		value = value < 0.f ? 0.f : value > 1.f ? 1.f : value;
		
		outputValue = EvalEase(value, (EaseType)type, param);
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
	DefineNodeImpl("math.range", VfxNodeRange)
	DefineNodeImpl("ease", VfxNodeEase)
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

#include "image.h"

static void fft(double * dreal, double * dimag, const int size, const int log2n, const bool inverse, const bool normalize)
{
	const double pi2 = M_PI * 2.0;
	const double scale = 1.0 / size;
	
	int n = 1;
	
	for (int k = 0; k < log2n; ++k)
	{
		const int n2 = n;
		
		n <<= 1;
		
		const double angle = (inverse) ? pi2 / n : -pi2 / n;
		double wtmp = std::sin(0.5 * angle);
		double wpr = -2.0 * wtmp * wtmp;
		double wpi = std::sin(angle);
		double wr = 1.0;
		double wi = 0.0;

		for (int m = 0; m < n2; ++m)
		{
			for (int i = m; i < size; i += n)
			{
				const int j = i + n2;
				
				const double tcreal = wr * dreal[j] - wi * dimag[j];
				const double tcimag = wr * dimag[j] + wi * dreal[j];
				
				dreal[j] = dreal[i] - tcreal;
				dimag[j] = dimag[i] - tcimag;
				
				dreal[i] += tcreal;
				dimag[i] += tcimag;
			}
			
			wtmp = wr;
			wr = wtmp * wpr - wi * wpi + wr;
			wi = wi * wpr + wtmp * wpi + wi;
		}
	}
	
	if (normalize)
	{
		for (int i = 0; i < n; ++i)
		{
			dreal[i] *= scale;
			dimag[i] *= scale;
		}
	}
}

static int reverseBits(const int value, const int numBits)
{
	int smask = 1 << (numBits - 1);
	int dmask = 1;
	
	int result = 0;
	
	for (int i = 0; i < numBits; ++i, smask >>= 1, dmask <<= 1)
	{
		if (value & smask)
			result |= dmask;
	}
	
	return result;
}

static int upperPowerOf2(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static int integerLog2(int v)
{
	int r = 0;
	while (v > 1)
	{
		v >>= 1;
		r++;
	}
	return r;
}

#include "Timer.h"

static void testFourier2d()
{
	//ImageData * image = loadImage("rainbow-pow2.png");
	//ImageData * image = loadImage("rainbow-small.png");
	//ImageData * image = loadImage("rainbow.png");
	//ImageData * image = loadImage("rainbow.jpg");
	ImageData * image = loadImage("picture.jpg");
	//ImageData * image = loadImage("wordcloud.png");
	//ImageData * image = loadImage("antigrain.png");
	
	if (image == nullptr)
	{
		logDebug("failed to load image!");
		return;
	}
	
	// perform fast fourier transform on 64 samples of complex data
	
	const int kN = 64;
	const int kLog2N = integerLog2(kN);
	
	double dreal[2][kN];
	double dimag[2][kN];
	
	int a = 0; // we need to reverse bits of data indices before doing the fft. to do so we ping-pong between two sets of data, dreal/dimag[0] and dreal/dimag[1]
	
	for (int i = 0; i < kN; ++i)
	{
		dreal[a][i] = std::cos(i / 100.0);
		dimag[a][i] = 0.0;
	}
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[i], dimag[i]);
	}
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	fft(dreal[a], dimag[a], kN, kLog2N, false, false);
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	fft(dreal[a], dimag[a], kN, kLog2N, true, true);
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	// perform 2d fast fourier transform on image data

	const int imageSx = image->sx;
	const int imageSy = image->sy;
	const int transformSx = upperPowerOf2(imageSx);
	const int transformSy = upperPowerOf2(imageSy);

	double * freal = new double[transformSx * transformSy];
	double * fimag = new double[transformSx * transformSy];
	
	{
		// note : the 2d fourier transform is seperable, meaning we could do the transform
		//        on all samples at once, or first for each row, and then for each column
		//        of the already transformed results of the previous row-passes
		
		const int numBitsX = integerLog2(transformSx);
		const int numBitsY = integerLog2(transformSy);
		
		auto fast_t1 = g_TimerRT.TimeUS_get();
		
		// perform FFT on each row
		
		for (int y = 0; y < imageSy; ++y)
		{
			double * __restrict rreal = &freal[y * transformSx];
			double * __restrict rimag = &fimag[y * transformSx];
			
			auto line = image->getLine(y);
			
			for (int x = 0; x < imageSx; ++x)
			{
				const auto & pixel = line[x];
				const double value = (pixel.r + pixel.g + pixel.b) / 3.0;
				
				const int index = reverseBits(x, numBitsX);
				rreal[index] = value;
				rimag[index] = 0.0;
			}
			
			for (int x = imageSx; x < transformSx; ++x)
			{
				const int index = reverseBits(x, numBitsX);
				rreal[index] = 0.0;
				rimag[index] = 0.0;
			}
			
			fft(rreal, rimag, transformSx, numBitsX, false, false);
		}
		
		// perform FFT on each column
		
		// complex values for current column
		double * creal = new double[transformSy];
		double * cimag = new double[transformSy];
		
		for (int x = 0; x < transformSx; ++x)
		{
			double * __restrict frealc = &freal[x];
			double * __restrict fimagc = &fimag[x];
			
			for (int y = 0; y < imageSy; ++y)
			{
				const int index = reverseBits(y, numBitsY);
				
				creal[index] = *frealc;
				cimag[index] = *fimagc;
				
				frealc += transformSx;
				fimagc += transformSx;
			}
			
			for (int y = imageSy; y < transformSy; ++y)
			{
				const int index = reverseBits(y, numBitsY);
				creal[index] = 0.0;
				cimag[index] = 0.0;
			}
			
			fft(creal, cimag, transformSy, numBitsY, false, false);
			
			//
			
			frealc = &freal[x];
			fimagc = &fimag[x];
			
			const double scale = 1.0 / double(transformSx * transformSy);
			
			for (int y = 0; y < transformSy; ++y)
			{
				*frealc = creal[y] * scale;
				*fimagc = cimag[y] * scale;
				
				frealc += transformSx;
				fimagc += transformSx;
			}
		}
		
		delete[] creal;
		delete[] cimag;
		
		auto fast_t2 = g_TimerRT.TimeUS_get();
		printf("fast fourier transform took %gms\n", (fast_t2 - fast_t1) / 1000.0);
	}
	
#if 0
	auto ref_t1 = g_TimerRT.TimeUS_get();
	for (int y = 0; y < image->sy; ++y)
	{
		printf("%d/%d\n", y + 1, image->sy);
		
		for (int x = 0; x < image->sx; ++x)
		{
			double coss = 0.0;
			double sins = 0.0;
			
			for (int v = 0; v < image->sy; ++v)
			{
				double phaseV = v * y / double(image->sy);
				
				for (int u = 0; u < image->sx; ++u)
				{
					double phaseU = u * x / double(image->sx);
					
					double phase = (2.0 * M_PI) * (phaseV + phaseU);
					
					double cosv = std::cos(phase);
					double sinv = std::sin(phase);
					
					double value = (image->getLine(v)[u].r + image->getLine(v)[u].g + image->getLine(v)[u].b) / 3.0;
					
					coss += cosv * value;
					sins += sinv * value;
				}
			}
			
			freal[y * transformSx + x] = coss / double(image->sx * image->sy);
			fimag[y * transformSx + x] = sins / double(image->sx * image->sy);
		}
	}
	auto ref_t2 = g_TimerRT.TimeUS_get();
	printf("reference fourier transform took %lluus\n", (ref_t2 - ref_t1));
#endif
	
#if 0
	for (int y = 0; y < image->sy; ++y)
	{
		for (int x = 0; x < image->sx; ++x)
		{
			double & fc = f[y * transformSx * 2 + x * 2 + 0];
			double & fs = f[y * transformSx * 2 + x * 2 + 1];
			
			const int sx = (x + image->sx/2) % image->sx;
			const int sy = (y + image->sy/2) % image->sy;
			
			const double dx = sx - image->sx/2;
			const double dy = sy - image->sy/2;
			const double ds = std::hypot(dx, dy);
			
			const double w = std::pow(ds / 50.0, 4.0);
			const double s = w < 0.0 ? 0.0 : w > 1.0 ? 1.0 : w;
			
			fc *= s;
			fs *= s;
		}
	}
	
	// todo : reconstruct image from coefficients
#endif
	
	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const double scaleX = 100.0 * mouse.x / double(GFX_SX);
			const double scaleY = 100.0 * mouse.y / double(GFX_SX);
			
			const int displayScaleX = std::max(1, GFX_SX / transformSx);
			const int displayScaleY = std::max(1, GFX_SY / transformSy);
			const int displayScale = std::min(displayScaleX, displayScaleY);
			gxScalef(displayScale, displayScale, 1);
			
			//hqBegin(HQ_FILLED_CIRCLES);
			gxBegin(GL_QUADS);
			{
				for (int y = 0; y < transformSy; ++y)
				{
					for (int x = 0; x < transformSx; ++x)
					{
						// sample from the middle as it looks nicer when presenting the result ..
						
						const int sx = (x + transformSx/2) % transformSx;
						const int sy = (y + transformSy/2) % transformSy;
						
						const double c = freal[sy * transformSx + sx];
						const double s = fimag[sy * transformSx + sx];
						const double m = std::log10(10.0 + std::hypot(c, s)) - 1.0;
						
						const double r = c * scaleX;
						const double g = s * scaleX;
						const double b = m * scaleY;
						
						//gxColor4f(r, g, b, 1.f);
						gxColor4f(b, b, b, 1.f);
						//hqFillCircle(x + .5f, y + .5f, .4f);
						gxVertex2f(x+0, y+0);
						gxVertex2f(x+1, y+0);
						gxVertex2f(x+1, y+1);
						gxVertex2f(x+0, y+1);
					}
				}
			}
			//hqEnd();
			gxEnd();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete[] freal;
	delete[] fimag;
}

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

static void testImpulseResponseMeasurement()
{
	// todo : generate signal
	
	// todo : do impulse response measurement over time
	
	do
	{
		framework.process();
		
		const int kNumSamples = 1000;
		
		double samples[kNumSamples];
		
		double responseX[kNumSamples];
		double responseY[kNumSamples];
		
		double twoPi = M_PI * 2.0;
		
		double samplePhase = 0.0;
		double sampleStep = twoPi / 87.65;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			samples[i] = std::cos(samplePhase);
			
			samplePhase = std::fmod(samplePhase + sampleStep, twoPi);
		}
		
		double measurementPhase = twoPi * (mouse.y / double(GFX_SY));
		double measurementStep = twoPi / (150.0 * mouse.x / double(GFX_SX));
		
		if (keyboard.isDown(SDLK_a))
			measurementStep = sampleStep;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			const double x = std::cos(measurementPhase);
			const double y = std::sin(measurementPhase);
			
			responseX[i] = samples[i] * x;
			responseY[i] = samples[i] * y;
			
			measurementPhase = std::fmod(measurementPhase + measurementStep, twoPi);
		}
		
		double sumS = 0.f;
		double sumX = 0.f;
		double sumY = 0.f;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			sumS += std::fabs(samples[i]);
			sumX += responseX[i];
			sumY += responseY[i];
		}
		
		const double avgX = sumX / kNumSamples;
		const double avgY = sumY / kNumSamples;
		
		double impulseResponse = std::hypot(avgX, avgY) * 2.0;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX/2, GFX_SY/2, 24, 0, 0, "impulse response: %f (%f^8)", impulseResponse, std::pow(impulseResponse, 8.0));
			drawText(GFX_SX/2, GFX_SY/2 + 30, 24, 0, 0, "sampleFreq=%f, measurementFreq=%f", 1.f / sampleStep, 1.f / measurementStep);
			drawText(GFX_SX/2, GFX_SY/2 + 60, 24, 0, 0, "sumS=%f, sumX=%f, sumY=%f", sumS, sumX, sumY);
			
			gxPushMatrix();
			{
				gxTranslatef(0, GFX_SY/2, 0);
				gxScalef(GFX_SX / float(kNumSamples), 1.f, 0.f);
				
				setColor(255, 255, 255);
				gxBegin(GL_POINTS);
				{
					for (int i = 0; i < kNumSamples; ++i)
					{
						gxVertex2f(i, samples[i] * 20.f);
					}
				}
				gxEnd();
				
				setColor(colorRed);
				gxBegin(GL_POINTS);
				{
					float rsum = 0.f;
					
					for (int i = 0; i < kNumSamples; ++i)
					{
						rsum += responseX[i];
						
						gxVertex2f(i, rsum);
						
						gxVertex2f(i, responseX[i] * 20.f);
					}
				}
				gxEnd();
				
				setColor(colorGreen);
				gxBegin(GL_POINTS);
				{
					float rsum = 0.f;
					
					for (int i = 0; i < kNumSamples; ++i)
					{
						rsum += responseY[i];
						
						gxVertex2f(i, rsum);
						
						gxVertex2f(i, responseY[i] * 20.f);
					}
				}
				gxEnd();
			}
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
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
