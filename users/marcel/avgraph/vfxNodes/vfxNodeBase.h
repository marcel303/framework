#pragma once

#include "Debugging.h"
#include "Mat4x4.h"
#include <stdint.h>
#include <string>
#include <vector>

struct GraphNode;
class Surface;

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
};

//

struct VfxTransform
{
	Mat4x4 matrix;
	
	VfxTransform();
};

//

enum VfxTriggerDataType
{
	kVfxTriggerDataType_None,
	kVfxTriggerDataType_Bool,
	kVfxTriggerDataType_Int,
	kVfxTriggerDataType_Float
};

struct VfxTriggerData
{
	VfxTriggerDataType type;
	
	union
	{
		bool boolValue;
		int intValue;
		float floatValue;
		uint8_t mem[8];
	};
	
	VfxTriggerData();
	
	void setBool(const bool value)
	{
		type = kVfxTriggerDataType_Bool;
		boolValue = value;
	}
	
	void setInt(const int value)
	{
		type = kVfxTriggerDataType_Int;
		intValue = value;
	}
	
	void setFloat(const float value)
	{
		type = kVfxTriggerDataType_Float;
		floatValue = value;
	}
	
	bool asBool() const
	{
		switch (type)
		{
		case kVfxTriggerDataType_None:
			return false;
		case kVfxTriggerDataType_Bool:
			return boolValue;
		case kVfxTriggerDataType_Int:
			return intValue != 0;
		case kVfxTriggerDataType_Float:
			return floatValue != 0.f;
		}
	}
	
	int asInt() const
	{
		switch (type)
		{
		case kVfxTriggerDataType_None:
			return 0;
		case kVfxTriggerDataType_Bool:
			return boolValue ? 1 : 0;
		case kVfxTriggerDataType_Int:
			return intValue;
		case kVfxTriggerDataType_Float:
			return std::round(floatValue);
		}
	}
	
	float asFloat() const
	{
		switch (type)
		{
		case kVfxTriggerDataType_None:
			return 0.f;
		case kVfxTriggerDataType_Bool:
			return boolValue ? 1.f : 0.f;
		case kVfxTriggerDataType_Int:
			return float(intValue);
		case kVfxTriggerDataType_Float:
			return floatValue;
		}
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

	static void interleave1(const Channel * channel1, uint8_t * _dst, const int dstPitch, const int sx, const int sy);
	static void interleave3(const Channel * channel1, const Channel * channel2, const Channel * channel3, uint8_t * dst, const int dstPitch, const int sx, const int sy);
	static void interleave4(const Channel * channel1, const Channel * channel2, const Channel * channel3, const Channel * channel4, uint8_t * dst, const int dstPitch, const int sx, const int sy);
};

//

struct VfxChannel
{
	const float * data;
	
	VfxChannel()
		: data(nullptr)
	{
	}
};

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
	void free();
};

// fixme : for some reason this is not allowed to be a struct member in XCode. it starts to complain about an undefined reference to this constant. I have no idea why .. I do this all over the place and it should be allowed ?
static const int kMaxVfxChannels = 16;

struct VfxChannels
{
	VfxChannel channels[kMaxVfxChannels];
	
	int size;
	int numChannels;
	
	VfxChannels()
		: channels()
		, size(0)
		, numChannels(0)
	{
	}
	
	void setData(const float * const * data, const int size, const int numChannels);
	void setDataContiguous(const float * data, const int size, const int numChannels);
	void reset();
};

//

enum VfxPlugType
{
	kVfxPlugType_None,
	kVfxPlugType_Bool,
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_Transform,
	kVfxPlugType_String,
	kVfxPlugType_Color,
	kVfxPlugType_Image,
	kVfxPlugType_ImageCpu,
	kVfxPlugType_Channels,
	kVfxPlugType_Trigger
};

struct VfxPlug
{
	VfxPlugType type;
	bool isValid;
	void * mem;
	
	VfxPlug()
		: type(kVfxPlugType_None)
		, isValid(true)
		, mem(nullptr)
	{
	}
	
	void connectTo(VfxPlug & dst);
	void connectTo(void * dstMem, const VfxPlugType dstType);
	
	void disconnect()
	{
		mem = nullptr;
	}
	
	bool isConnected() const
	{
		return mem != nullptr;
	}
	
	bool getBool() const
	{
		Assert(type == kVfxPlugType_Bool);
		return *((bool*)mem);
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
	
	const VfxTransform & getTransform() const
	{
		Assert(type == kVfxPlugType_Transform);
		return *((VfxTransform*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(type == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	const VfxColor & getColor() const
	{
		Assert(type == kVfxPlugType_Color);
		return *((VfxColor*)mem);
	}
	
	VfxImageBase * getImage() const
	{
		Assert(type == kVfxPlugType_Image);
		return (VfxImageBase*)mem;
	}
	
	VfxImageCpu * getImageCpu() const
	{
		Assert(type == kVfxPlugType_ImageCpu);
		return (VfxImageCpu*)mem;
	}
	
	VfxChannels * getChannels() const
	{
		Assert(type == kVfxPlugType_Channels);
		return (VfxChannels*)mem;
	}
	
	VfxTriggerData & getTriggerData() const
	{
		Assert(type == kVfxPlugType_Trigger);
		return *((VfxTriggerData*)mem);
	}
	
	//
	
	bool & getRwBool()
	{
		Assert(type == kVfxPlugType_Bool);
		return *((bool*)mem);
	}
	
	int & getRwInt()
	{
		Assert(type == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float & getRwFloat()
	{
		Assert(type == kVfxPlugType_Float);
		return *((float*)mem);
	}
	
	VfxTransform & getRwTransform()
	{
		Assert(type == kVfxPlugType_Transform);
		return *((VfxTransform*)mem);
	}
	
	std::string & getRwString()
	{
		Assert(type == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	VfxColor & getRwColor()
	{
		Assert(type == kVfxPlugType_Color);
		return *((VfxColor*)mem);
	}
};

struct VfxNodeBase
{
	struct TriggerTarget
	{
		VfxNodeBase * srcNode;
		int srcSocketIndex;
		int dstSocketIndex;
		
		TriggerTarget();
	};
	
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	std::vector<TriggerTarget> triggerTargets;
	
	int lastTickTraversalId;
	int lastDrawTraversalId;
	bool editorIsTriggered; // only here for real-time connection with graph editor
	
	bool isPassthrough;
	
	int tickOrder;
	int tickTimeAvg;
	int drawTimeAvg;
	
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
	
	int getInputBool(const int index, const bool defaultValue) const
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
	
	const VfxTransform & getInputTransform(const int index, const VfxTransform & defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getTransform();
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
	
	const VfxImageCpu * getInputImageCpu(const int index, const VfxImageCpu * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getImageCpu();
	}
	
	const VfxTriggerData * getInputTriggerData(const int index) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return nullptr;
		else
			return &plug->getTriggerData();
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
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void tick(const float dt) { }
	virtual void handleTrigger(const int inputSocketIndex, const VfxTriggerData & data) { }
	virtual void draw() const { }
};
