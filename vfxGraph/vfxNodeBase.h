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

#include "Debugging.h"
#include "vfxProfiling.h"
#include <stdint.h>
#include <string>
#include <vector>

#if ENABLE_VFXGRAPH_PROFILING
	#include "vfxNodes/openglGpuTimer.h"
#endif

#define EXTENDED_INPUTS 1

struct GraphEdit_ResourceEditorBase;
struct GraphEdit_TypeDefinitionLibrary;
struct GraphNode;
class Surface;

struct VfxTransform;

//

struct VfxColor
{
	float r;
	float g;
	float b;
	float a;
	
	VfxColor()
		: r(0.f)
		, g(0.f)
		, b(0.f)
		, a(0.f)
	{
	}
	
	VfxColor(const float _r, const float _g, const float _b, const float _a)
		: r(_r)
		, g(_g)
		, b(_b)
		, a(_a)
	{
	}
	
	void setRgba(const float _r, const float _g, const float _b, const float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
};

//

struct VfxImageBase
{
	VfxImageBase()
	{
	}
	
	virtual ~VfxImageBase()
	{
	}
	
	virtual int getSx() const = 0;
	virtual int getSy() const = 0;
	virtual uint32_t getTexture() const = 0;
};

struct VfxImage_Texture : VfxImageBase
{
	uint32_t texture;
	
	VfxImage_Texture();
	
	virtual int getSx() const override;
	virtual int getSy() const override;
	virtual uint32_t getTexture() const override;
};

//

struct VfxImageCpu
{
	struct Channel
	{
		const uint8_t * data;
		int stride;
		int pitch;
		
		Channel()
			: data(nullptr)
			, stride(0)
			, pitch(0)
		{
		}
	};
	
	int sx;
	int sy;
	int numChannels;
	int alignment;
	bool isInterleaved;
	bool isPlanar;
	
	Channel channel[4];
	
	VfxImageCpu();
	
	void setDataInterleaved(const uint8_t * data, const int sx, const int sy, const int numChannels, const int alignment, const int pitch);
	void setDataR8(const uint8_t * r, const int sx, const int sy, const int alignment, const int pitch);
	void setDataRGBA8(const uint8_t * rgba, const int sx, const int sy, const int alignment, const int pitch);
	void reset();
	
	int getMemoryUsage() const;

	static void interleave1(const Channel * channel1, uint8_t * _dst, const int dstPitch, const int sx, const int sy);
	static void interleave3(const Channel * channel1, const Channel * channel2, const Channel * channel3, uint8_t * dst, const int dstPitch, const int sx, const int sy);
	static void interleave4(const Channel * channel1, const Channel * channel2, const Channel * channel3, const Channel * channel4, uint8_t * dst, const int dstPitch, const int sx, const int sy);
};

//

struct VfxImageCpuData
{
	uint8_t * data;
	
	VfxImageCpu image;
	
	VfxImageCpuData();
	~VfxImageCpuData();

	void alloc(const int sx, const int sy, const int numChannels, const bool interleaved);
	void allocOnSizeChange(const int sx, const int sy, const int numChannels, const bool interleaved);
	void allocOnSizeChange(const VfxImageCpu & reference);
	void free();
};

//

struct VfxChannelData
{
	float * data;
	int size;
	
	VfxChannelData()
		: data(nullptr)
		, size(0)
	{
	}
	
	~VfxChannelData()
	{
		free();
	}
	
	void alloc(const int size);
	void allocOnSizeChange(const int size);
	void free();
};

struct VfxChannel
{
	const float * data;
	bool continuous; // hints whether data should be treated as individual samples, or continuous. it's similar to nearest vs linear texture sampling in OpenGL
	
	int size;
	
	int sx;
	int sy;
	
	VfxChannel()
		: data(nullptr)
		, continuous(false)
		, size(0)
		, sx(0)
		, sy(0)
	{
	}
	
	void setData(const float * data, const bool continuous, const int size);
	void setData2D(const float * data, const bool continuous, const int sx, const int sy);
	void reset();
	
	float * dataRw() const
	{
		return const_cast<float*>(data);
	}
};

struct VfxChannelZipper
{
	const static int kMaxChannels = 16;
	
	VfxChannelZipper(std::initializer_list<const VfxChannel*> _channels)
		: numChannels(0)
		, sx(0)
		, sy(0)
		, x(0)
		, y(0)
	{
		ctor(_channels.begin(), _channels.size());
	}
	
	VfxChannelZipper(const VfxChannel * const * _channels, const int _numChannels)
		: numChannels(0)
		, sx(0)
		, sy(0)
		, x(0)
		, y(0)
	{
		ctor(_channels, _numChannels);
	}
	
	void ctor(const VfxChannel * const * _channels, const int _numChannels)
	{
		for (int i = 0; i < _numChannels; ++i)
		{
			const VfxChannel * channel = _channels[i];
			
			if (numChannels < kMaxChannels)
			{
				channels[numChannels] = channel;
				
				if (channel != nullptr)
				{
					sx = std::max(sx, channel->sx);
					sy = std::max(sy, channel->sy);
				}
				
				numChannels++;
			}
		}
	}
	
	int size() const
	{
		return sx * sy;
	}
	
	bool doneX() const
	{
		return x == sx;
	}
	
	void nextX()
	{
		Assert(!doneX());
		
		x = x + 1;
	}
	
	bool doneY() const
	{
		return y == sy;
	}
	
	void nextY()
	{
		Assert(!doneY());
		
		y = y + 1;
		
		if (!doneY())
		{
			x = 0;
		}
	}
	
	bool done() const
	{
		return doneX() && doneY();
	}
	
	void next()
	{
		nextX();
		
		if (doneX())
		{
			nextY();
		}
	}
	
	void restart()
	{
		x = 0;
		y = 0;
	}
	
	float read(const int channelIndex, const float defaultValue) const
	{
		auto channel = channels[channelIndex];
		
		if (channel == nullptr || channel->size == 0)
		{
			return defaultValue;
		}
		else
		{
			const int rx = x % channel->sx;
			const int ry = y % channel->sy;
			
			const float result = channel->data[ry * channel->sx + rx];
			
			return result;
		}
	}
	
	const VfxChannel * channels[kMaxChannels];
	int numChannels;
	
	int sx;
	int sy;
	
	int x;
	int y;
};

//

enum VfxPlugType
{
	kVfxPlugType_None,
	kVfxPlugType_DontCare,
	kVfxPlugType_Bool,
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_String,
	kVfxPlugType_Color,
	kVfxPlugType_Image,
	kVfxPlugType_ImageCpu,
	kVfxPlugType_Channel,
	kVfxPlugType_Trigger,
	kVfxPlugType_Draw
};

#if EXTENDED_INPUTS

struct VfxFloatArray
{
	struct Elem
	{
		float * value;
		
		// per-link range support
		bool hasRange;
		float inMin;
		float inMax;
		float outMin;
		float outMax;
		
		Elem()
			: value(nullptr)
			, hasRange(false)
			, inMin(0.f)
			, inMax(1.f)
			, outMin(0.f)
			, outMax(1.f)
		{
		}
	};
	
	float sum;
	std::vector<Elem> elems;
	float * immediateValue;
	
	int lastUpdateTick;
	
	VfxFloatArray()
		: sum(0.f)
		, elems()
		, immediateValue(nullptr)
		, lastUpdateTick(-1)
	{
	}
	
	void update();
	
	float * get();
};

#endif

struct VfxPlug
{
	VfxPlugType type;
	bool isValid;
	bool isReferencedByLink;
	int referencedByRealTimeConnectionTick;
	void * mem;
	VfxPlugType memType;
	
#if EXTENDED_INPUTS
	mutable VfxFloatArray floatArray;
#endif

	bool editorIsTriggered; // only here for real-time connection with graph editor
	
	VfxPlug()
		: type(kVfxPlugType_None)
		, isValid(true)
		, isReferencedByLink(false)
		, referencedByRealTimeConnectionTick(-1)
		, mem(nullptr)
		, memType(kVfxPlugType_None)
	#if EXTENDED_INPUTS
		, floatArray()
	#endif
		, editorIsTriggered(false)
	{
	}
	
	void connectTo(VfxPlug & dst);
	void connectTo(void * dstMem, const VfxPlugType dstType, const bool isImmediate);
	
	void setMap(const void * dst, const float inMin, const float inMax, const float outMin, const float outMax);
	void clearMap(const void * dst);
	
	void disconnect();
	void disconnect(const void * dstMem);
	
	bool isConnected() const;
	
	bool isReferenced() const;
	
	bool getBool() const
	{
		Assert(memType == kVfxPlugType_Bool);
		return *((bool*)mem);
	}
	
	int getInt() const
	{
		Assert(memType == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float getFloat() const
	{
		Assert(memType == kVfxPlugType_Float);
	#if EXTENDED_INPUTS
		if (mem == nullptr)
			return *floatArray.get();
	#endif
		return *((float*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(memType == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	const VfxColor & getColor() const
	{
		Assert(memType == kVfxPlugType_Color);
		return *((VfxColor*)mem);
	}
	
	VfxImageBase * getImage() const
	{
		Assert(memType == kVfxPlugType_Image);
		return (VfxImageBase*)mem;
	}
	
	VfxImageCpu * getImageCpu() const
	{
		Assert(memType == kVfxPlugType_ImageCpu);
		return (VfxImageCpu*)mem;
	}
	
	VfxChannel * getChannel() const
	{
		Assert(memType == kVfxPlugType_Channel);
		return (VfxChannel*)mem;
	}
	
	//
	
	bool & getRwBool()
	{
		Assert(memType == kVfxPlugType_Bool);
		return *((bool*)mem);
	}
	
	int & getRwInt()
	{
		Assert(memType == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float & getRwFloat()
	{
		Assert(memType == kVfxPlugType_Float);
	#if EXTENDED_INPUTS
		if (mem == nullptr)
			return *floatArray.get();
	#endif
		return *((float*)mem);
	}
	
	std::string & getRwString()
	{
		Assert(memType == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	VfxColor & getRwColor()
	{
		Assert(memType == kVfxPlugType_Color);
		return *((VfxColor*)mem);
	}
};

struct VfxNodeDescription
{
	std::vector<std::string> lines;
	
	void add(const char * format, ...);
	void add(const char * name, const VfxImageBase & image);
	void add(const char * name, const VfxImageCpu & image);
	void add(const VfxChannel & channel);
	void addOpenglTexture(const char * name, const uint32_t id);
	
	void newline();
};

struct VfxNodeBase
{
	enum Flag
	{
		kFlag_CustomTraverseTick = 1 << 0,
		kFlag_CustomTraverseDraw = 1 << 1
	};
	
	struct TriggerTarget
	{
		VfxNodeBase * srcNode;
		int srcSocketIndex;
		int dstSocketIndex;
		
		TriggerTarget();
	};
	
	struct DynamicInput
	{
		std::string name;
		VfxPlugType type;
	};
	
	struct DynamicOutput
	{
		std::string name;
		VfxPlugType type;
		void * mem;
	};
	
	int id;
	
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	std::vector<TriggerTarget> triggerTargets;
	std::vector<DynamicInput> dynamicInputs;
	std::vector<DynamicOutput> dynamicOutputs;
	
	int flags;
	
	int lastTickTraversalId;
	int lastDrawTraversalId;
	bool editorIsTriggered; // only here for real-time connection with graph editor
	mutable std::string editorIssue;
	
	bool isPassthrough;
	
	int tickTimeAvg;
	int drawTimeAvg;
	int gpuTimeAvg;
	
#if ENABLE_VFXGRAPH_PROFILING
	OpenglGpuTimer gpuTimer; // todo : remove !
#endif
	
	VfxNodeBase();
	
	virtual ~VfxNodeBase()
	{
	}
	
	void traverseTick(const int traversalId, const float dt);
	void traverseDraw(const int traversalId);
	
	void trigger(const int outputSocketIndex);
	
	void resizeSockets(const int numInputs, const int numOutputs)
	{
		inputs.resize(numInputs);
		outputs.resize(numOutputs);
	}
	
	void addInput(const int index, VfxPlugType type)
	{
		Assert(index >= 0 && index < (int)inputs.size());
		if (index >= 0 && index < (int)inputs.size())
		{
			inputs[index].type = type;
		}
	}
	
	void addOutput(const int index, VfxPlugType type, void * mem)
	{
		Assert(index >= 0 && index < (int)outputs.size());
		Assert(type == kVfxPlugType_Trigger || mem != nullptr);
		Assert(type != kVfxPlugType_Trigger || mem == nullptr);
		if (index >= 0 && index < (int)outputs.size())
		{
			outputs[index].type = type;
			outputs[index].mem = mem;
			outputs[index].memType = type;
		}
	}
	
	void reconnectDynamicInputs(const int dstNodeId = -1);
	void setDynamicInputs(const DynamicInput * newInputs, const int numInputs);
	void setDynamicOutputs(const DynamicOutput * newOutputs, const int numOutputs);
	
	VfxPlug * tryGetInput(const int index)
	{
		Assert(index >= 0 && index <= (int)inputs.size());
		if (index < 0 || index >= (int)inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	VfxPlug * tryGetOutput(const int index)
	{
		Assert(index >= 0 && index <= (int)outputs.size());
		if (index < 0 || index >= (int)outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	const VfxPlug * tryGetInput(const int index) const
	{
		Assert(index >= 0 && index <= (int)inputs.size());
		if (index < 0 || index >= (int)inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	const VfxPlug * tryGetOutput(const int index) const
	{
		Assert(index >= 0 && index <= (int)outputs.size());
		if (index < 0 || index >= (int)outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	bool getInputBool(const int index, const bool defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getBool();
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
	
	const VfxColor * getInputColor(const int index, const VfxColor * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return &plug->getColor();
	}
	
	const VfxImageBase * getInputImage(const int index, const VfxImageBase * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getImage();
	}
	
	const VfxImageCpu * getInputImageCpu(const int index, const VfxImageCpu * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getImageCpu();
	}
	
	const VfxChannel * getInputChannel(const int index, const VfxChannel * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getChannel();
	}
	
	void setOuputIsValid(const int index, const bool isValid)
	{
		VfxPlug * plug = tryGetOutput(index);
		
		if (plug == nullptr)
		{
		}
		else
		{
			plug->isValid = isValid;
		}
	}
	
	void clearEditorIssue() const
	{
		editorIssue.clear();
	}
	
	void setEditorIssue(const char * text) const
	{
		editorIssue = text;
	}
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void tick(const float dt) { }
	virtual void handleTrigger(const int inputSocketIndex) { }
	virtual void draw() const { }
	virtual void beforeDraw() const { }
	virtual void afterDraw() const { }
	virtual void customTraverseTick(const int traversalId, const float dt) { }
	virtual void customTraverseDraw(const int traversalId) const { }
	virtual void beforeSave(GraphNode & node) const { }
	
	virtual void getDescription(VfxNodeDescription & d) { } 
};

//

struct VfxEnumTypeRegistration
{
	struct Elem
	{
		std::string name;
		std::string valueText;
	};
	
	VfxEnumTypeRegistration * next;
	
	std::string enumName;
	int nextValue;
	
	std::vector<Elem> elems;
	
	VfxEnumTypeRegistration();
	
	void elem(const char * name, const int value = -1);
	void elem(const char * name, const char * valueText);
};

#define VFX_ENUM_TYPE(name) \
	struct name ## __registration : VfxEnumTypeRegistration \
	{ \
		name ## __registration() \
		{ \
			enumName = # name; \
			init(); \
		} \
		void init(); \
	}; \
	extern name ## __registration name ## __registrationInstance; \
	name ## __registration name ## __registrationInstance; \
	void name ## __registration :: init()

//

struct VfxNodeTypeRegistration
{
	struct Input
	{
		std::string typeName;
		std::string name;
		std::string displayName;
		std::string enumName;
		std::string defaultValue;
	};
	
	struct Output
	{
		std::string typeName;
		std::string name;
		std::string displayName;
		bool isEditable;
		
		Output()
			: typeName()
			, name()
			, displayName()
			, isEditable(false)
		{
		}
	};
	
	VfxNodeTypeRegistration * next;
	
	VfxNodeBase * (*create)();
	
	GraphEdit_ResourceEditorBase * (*createResourceEditor)();
	
	std::string typeName;
	std::string displayName;
	
	std::string resourceTypeName;
	
	std::string author;
	std::string copyright;
	std::string description;
	std::string helpText;
	
	std::vector<Input> inputs;
	std::vector<Output> outputs;
	
	VfxNodeTypeRegistration();
	
	void in(const char * name, const char * typeName, const char * defaultValue = "", const char * displayName = "");
	void inEnum(const char * name, const char * enumName, const char * defaultValue = "", const char * displayName = "");
	void out(const char * name, const char * typeName, const char * displayName = "");
	void outEditable(const char * name);
};

#define VFX_NODE_TYPE(type) \
	struct type ## __registration : VfxNodeTypeRegistration \
	{ \
		type ## __registration() \
		{ \
			create = []() -> VfxNodeBase* { return new type(); }; \
			init(); \
		} \
		void init(); \
	}; \
	extern type ## __registration type ## __registrationInstance; \
	type ## __registration type ## __registrationInstance; \
	void type ## __registration :: init()

//

extern VfxEnumTypeRegistration * g_vfxEnumTypeRegistrationList;
extern VfxNodeTypeRegistration * g_vfxNodeTypeRegistrationList;

void createVfxValueTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary);
void createVfxEnumTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxEnumTypeRegistration * registrationList);
void createVfxNodeTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxNodeTypeRegistration * registrationList);

void createVfxTypeDefinitionLibrary(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxEnumTypeRegistration * enumRegistrationList, const VfxNodeTypeRegistration * nodeRegistrationList);
