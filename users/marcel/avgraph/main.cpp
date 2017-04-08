#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "../avpaint/video.h"

#include "testDatGui.h"

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

todo : fsfx :
- let FSFX use fsfx.vs vertex shader. don't require effects to have their own vertex shader
- expose uniforms/inputs from FSFX pixel shader
- iterate FSFX pixel shaders and generate type definitions based on FSFX name and exposed uniforms

reference :
- http://www.dsperados.com (company based in Utrecht ? send to Stijn)

*/

#define GFX_SX 1024
#define GFX_SY 768

struct VfxImageBase
{
	VfxImageBase()
	{
	}
	
	virtual ~VfxImageBase()
	{
	}
	
	virtual GLuint getTexture() const = 0;
};

struct VfxImage_Texture : VfxImageBase
{
	GLuint texture;
	
	VfxImage_Texture()
		: VfxImageBase()
		, texture(0)
	{
	}
	
	virtual GLuint getTexture() const override
	{
		return texture;
	}
};

struct VfxImage_Surface : VfxImageBase
{
	Surface * surface;
	
	VfxImage_Surface(Surface * _surface)
		: VfxImageBase()
		, surface(nullptr)
	{
		surface = _surface;
	}
	
	virtual GLuint getTexture() const override
	{
		if (surface)
			return surface->getTexture();
		else
			return 0;
	}
};

enum VfxPlugType
{
	kVfxPlugType_None,
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_String,
	kVfxPlugType_Image,
	kVfxPlugType_Surface
};

struct VfxPlug
{
	VfxPlugType type;
	void * mem;
	
	VfxPlug()
		: type(kVfxPlugType_None)
		, mem(nullptr)
	{
	}
	
	void connectTo(VfxPlug & dst)
	{
		if (dst.type != type)
		{
			logError("node connection failed. type mismatch");
		}
		else
		{
			mem = dst.mem;
		}
	}
	
	void connectTo(void * dstMem, const VfxPlugType dstType)
	{
		if (dstType != type)
		{
			logError("node connection failed. type mismatch");
		}
		else
		{
			mem = dstMem;
		}
	}
	
	bool isConnected() const
	{
		return mem != nullptr;
	}
	
	int getInt() const
	{
		Assert(type == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float getFloat() const
	{
		Assert(type == kVfxPlugType_Float);
		return *((float*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(type == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	VfxImageBase * getImage() const
	{
		Assert(type == kVfxPlugType_Image);
		return (VfxImageBase*)mem;
	}
	
	Surface * getSurface() const
	{
		Assert(type == kVfxPlugType_Surface);
		return *((Surface**)mem);
	}
};

struct VfxNodeBase
{
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	
	int lastTraversalId;
	
	VfxNodeBase()
		: inputs()
		, outputs()
		, predeps()
		, lastTraversalId(-1)
	{
	}
	
	virtual ~VfxNodeBase()
	{
	}
	
	void traverse(const int traversalId)
	{
		Assert(lastTraversalId != traversalId);
		lastTraversalId = traversalId;
		
		for (auto predep : predeps)
		{
			if (predep->lastTraversalId != traversalId)
				predep->traverse(traversalId);
		}
		
		draw();
	}
	
	void resizeSockets(const int numInputs, const int numOutputs)
	{
		inputs.resize(numInputs);
		outputs.resize(numOutputs);
	}
	
	void addInput(const int index, VfxPlugType type)
	{
		Assert(index >= 0 && index < inputs.size());
		if (index >= 0 && index < inputs.size())
		{
			inputs[index].type = type;
		}
	}
	
	void addOutput(const int index, VfxPlugType type, void * mem)
	{
		Assert(index >= 0 && index < outputs.size());
		if (index >= 0 && index < outputs.size())
		{
			outputs[index].type = type;
			outputs[index].mem = mem;
		}
	}
	
	VfxPlug * tryGetInput(const int index)
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	VfxPlug * tryGetOutput(const int index)
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	const VfxPlug * tryGetInput(const int index) const
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	const VfxPlug * tryGetOutput(const int index) const
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	int getInputInt(const int index, const int defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getInt();
	}
	
	float getInputFloat(const int index, const float defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getFloat();
	}
	
	const char * getInputString(const int index, const char * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getString().c_str();
	}
	
	const VfxImageBase * getInputImage(const int index, const VfxImageBase * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getImage();
	}
	
	const Surface * getInputSurface(const int index, const Surface * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getSurface();
	}
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void tick(const float dt) { }
	virtual void draw() const { }
};

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

struct VfxNodePicture : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture * image;
	
	VfxNodePicture()
		: VfxNodeBase()
		, image(nullptr)
	{
		image = new VfxImage_Texture();
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source, kVfxPlugType_String);
		addOutput(kOutput_Image, kVfxPlugType_Image, image);
	}
	
	virtual ~VfxNodePicture()
	{
		delete image;
		image = nullptr;
	}
	
	virtual void init(const GraphNode & node) override
	{
		const char * filename = getInputString(kInput_Source, nullptr);
		
		if (filename == nullptr)
		{
			image->texture = 0;
		}
		else
		{
			image->texture = getTexture(filename);
		}
	}
};

struct VfxNodeVideo : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture * image;
	
	MediaPlayer * mediaPlayer;
	
	VfxNodeVideo()
		: VfxNodeBase()
		, image(nullptr)
		, mediaPlayer(nullptr)
	{
		image = new VfxImage_Texture();
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source, kVfxPlugType_String);
		addOutput(kOutput_Image, kVfxPlugType_Image, image);
		
		mediaPlayer = new MediaPlayer();
		mediaPlayer->openAsync("video6.mpg", false);
	}
	
	~VfxNodeVideo()
	{
		delete mediaPlayer;
		mediaPlayer = nullptr;
		
		delete image;
		image = nullptr;
	}
	
	virtual void tick(const float dt) override
	{
		mediaPlayer->tick(mediaPlayer->context);
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt;
		
		image->texture = mediaPlayer->getTexture();
	}
};

struct VfxNodeFsfx : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Shader,
		kInput_Param1,
		kInput_Param2,
		kInput_Param3,
		kInput_Param4,
		kInput_Opacity,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture * image;
	
	bool isPassthrough;
	
	VfxNodeFsfx()
		: VfxNodeBase()
		, surface(nullptr)
		, image(nullptr)
		, isPassthrough(false)
	{
		surface = new Surface(GFX_SX, GFX_SY, true);
		
		surface->clear();
		surface->swapBuffers();
		surface->clear();
		surface->swapBuffers();
		
		image = new VfxImage_Texture();
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Image, kVfxPlugType_Image);
		addInput(kInput_Shader, kVfxPlugType_String);
		addInput(kInput_Param1, kVfxPlugType_Float);
		addInput(kInput_Param2, kVfxPlugType_Float);
		addInput(kInput_Param3, kVfxPlugType_Float);
		addInput(kInput_Param4, kVfxPlugType_Float);
		addInput(kInput_Opacity, kVfxPlugType_Float);
		addOutput(kOutput_Image, kVfxPlugType_Image, image);
	}
	
	virtual ~VfxNodeFsfx() override
	{
		delete image;
		image = nullptr;
		
		delete surface;
		surface = nullptr;
	}
	
	virtual void init(const GraphNode & node) override
	{
		isPassthrough = node.editorIsPassthrough;
	}
	
	virtual void draw() const override
	{
		if (isPassthrough)
		{
			const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
			const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
			image->texture = inputTexture;
			return;
		}
		
		const std::string & shaderName = getInputString(kInput_Shader, "");
		
		if (shaderName.empty())
		{
			// todo : warn ?
		}
		else
		{
			const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
			const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
			Shader shader(shaderName.c_str());
			
			if (shader.isValid())
			{
				pushBlend(BLEND_OPAQUE);
				setShader(shader);
				{
					shader.setImmediate("screenSize", surface->getWidth(), surface->getHeight());
					shader.setTexture("colormap", 0, inputTexture);
					shader.setImmediate("param1", getInputFloat(kInput_Param1, 0.f));
					shader.setImmediate("param2", getInputFloat(kInput_Param2, 0.f));
					shader.setImmediate("param3", getInputFloat(kInput_Param3, 0.f));
					shader.setImmediate("param4", getInputFloat(kInput_Param4, 0.f));
					shader.setImmediate("opacity", getInputFloat(kInput_Opacity, 1.f));
					shader.setImmediate("time", framework.time);
					surface->postprocess();
				}
				clearShader();
				popBlend();
			}
			else
			{
				pushSurface(surface);
				{
					if (inputImage == nullptr)
					{
						pushBlend(BLEND_OPAQUE);
						setColor(0, 127, 0);
						drawRect(0, 0, GFX_SX, GFX_SY);
						popBlend();
					}
					else
					{
						pushBlend(BLEND_OPAQUE);
						gxSetTexture(inputTexture);
						setColorMode(COLOR_ADD);
						setColor(127, 0, 127);
						drawRect(0, 0, GFX_SX, GFX_SY);
						setColorMode(COLOR_MUL);
						gxSetTexture(0);
						popBlend();
					}
				}
				popSurface();
			}
		}
		
		image->texture = surface->getTexture();
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

struct VfxNodeOscSine : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float phase;
	float value;
	
	VfxNodeOscSine()
		: VfxNodeBase()
		, phase(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Frequency, kVfxPlugType_Float);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void tick(const float dt) override
	{
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		
		value = std::sin(phase * 2.f * float(M_PI));
		
		phase = std::fmod(phase + dt * frequency, 1.f);
	}
};

struct VfxNodeOscSaw : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float phase;
	float value;
	
	VfxNodeOscSaw()
		: VfxNodeBase()
		, phase(.5f) // we start at phase=.5 so our saw wave starts at 0.0 instead of -1.0
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Frequency, kVfxPlugType_Float);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void tick(const float dt) override
	{
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		
		value = -1.f + 2.f * phase;
		
		phase = std::fmod(phase + dt * frequency, 1.f);
	}
};

struct VfxNodeOscTriangle : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float phase;
	float value;
	
	VfxNodeOscTriangle()
		: VfxNodeBase()
		, phase(.25f) // we start at phase=.25 so our triangle wave starts at 0.0 instead of -1.0
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Frequency, kVfxPlugType_Float);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void tick(const float dt) override
	{
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		
		value = 1.f - std::abs(phase * 4.f - 2.f);
		
		phase = std::fmod(phase + dt * frequency, 1.f);
	}
};

struct VfxNodeOscSquare : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float phase;
	float value;
	
	VfxNodeOscSquare()
		: VfxNodeBase()
		, phase(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Frequency, kVfxPlugType_Float);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void tick(const float dt) override
	{
		const float frequency = getInputFloat(kInput_Frequency, 0.f);
		
		value = 0.f; // todo
		
		phase = std::fmod(phase + dt * frequency, 1.f);
	}
};

struct VfxNodeMouse : VfxNodeBase
{
	enum Output
	{
		kOutput_X,
		kOutput_Y,
		kOutput_ButtonLeft,
		kOutput_ButtonRight,
		kOutput_COUNT
	};
	
	float x;
	float y;
	float buttonLeft;
	float buttonRight;
	
	VfxNodeMouse()
		: x(0.f)
		, y(0.f)
		, buttonLeft(0.f)
		, buttonRight(0.f)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_X, kVfxPlugType_Float, &x);
		addOutput(kOutput_Y, kVfxPlugType_Float, &y);
		addOutput(kOutput_ButtonLeft, kVfxPlugType_Float, &buttonLeft);
		addOutput(kOutput_ButtonRight, kVfxPlugType_Float, &buttonRight);
	}
	
	virtual void tick(const float dt)
	{
		x = mouse.x / float(GFX_SX);
		y = mouse.y / float(GFX_SY);
		buttonLeft = mouse.isDown(BUTTON_LEFT) ? 1.f : 0.f;
		buttonRight = mouse.isDown(BUTTON_RIGHT) ? 1.f : 0.f;
	}
};

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
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		testDatGui();
		
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
		
		GraphUi::PropEdit propEdit(typeDefinitionLibrary);
		
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
				propEdit.setGraph(nullptr);
				
				delete graphEdit->graph;
				graphEdit->graph = nullptr;
				
				//
				
				graphEdit->graph = new Graph();
				
				//
				
				XMLDocument document;
				document.LoadFile("graph.xml");
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				if (xmlGraph != nullptr)
					graphEdit->graph->loadXml(xmlGraph);
				
				//
				
				propEdit.setGraph(graphEdit->graph);
				
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
				
				propEdit.setNode(nodeId);
			}
			
			propEdit.tick(dt);
			
			framework.beginDraw(31, 31, 31, 255);
			{
				if (vfxGraph != nullptr)
				{
					vfxGraph->tick(framework.timeStep);
					vfxGraph->draw();
				}
				
				graphEdit->draw();
				
				propEdit.draw();
			}
			framework.endDraw();
		}
		
		delete graphEdit;
		graphEdit = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
