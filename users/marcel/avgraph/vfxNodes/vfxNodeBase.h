#pragma once

#include "framework.h"
#include "graph.h"
#include "Mat4x4.h"
#include <stdint.h>
#include <string.h>

struct VfxTransform
{
	VfxTransform()
	{
		matrix.MakeIdentity();
	}
	
	Mat4x4 matrix;
};

enum VfxTriggerDataType
{
	kVfxTriggerDataType_None,
	kVfxTriggerDataType_Bool,
	kVfxTriggerDataType_Int
};

struct VfxTriggerData
{
	VfxTriggerDataType type;
	
	union
	{
		bool boolValue;
		int intValue;
		uint8_t mem[8];
	};
	
	VfxTriggerData()
		: type(kVfxTriggerDataType_None)
	{
		memset(mem, 0, sizeof(mem));
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
		}
	}
};

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
	kVfxPlugType_Bool,
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_Transform,
	kVfxPlugType_String,
	kVfxPlugType_Color,
	kVfxPlugType_Image,
	kVfxPlugType_Surface,
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
		if (type == kVfxPlugType_Trigger)
			return ((VfxTriggerData*)mem)->asBool();
		Assert(type == kVfxPlugType_Bool);
		return *((bool*)mem);
	}
	
	int getInt() const
	{
		if (type == kVfxPlugType_Trigger)
			return ((VfxTriggerData*)mem)->asInt();
		Assert(type == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float getFloat() const
	{
		if (type == kVfxPlugType_Trigger)
			return ((VfxTriggerData*)mem)->asFloat();
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
	
	const Color & getColor() const
	{
		Assert(type == kVfxPlugType_Color);
		return *((Color*)mem);
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
	
	Color & getRwColor()
	{
		Assert(type == kVfxPlugType_Color);
		return *((Color*)mem);
	}
};

struct VfxNodeBase
{
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	
	int lastTickTraversalId;
	int lastDrawTraversalId;
	
	bool isPassthrough;
	
	VfxNodeBase()
		: inputs()
		, outputs()
		, predeps()
		, lastTickTraversalId(-1)
		, lastDrawTraversalId(-1)
		, isPassthrough(false)
	{
	}
	
	virtual ~VfxNodeBase()
	{
	}
	
	void traverseTick(const int traversalId, const float dt)
	{
		Assert(lastTickTraversalId != traversalId);
		lastTickTraversalId = traversalId;
		
		for (auto predep : predeps)
		{
			if (predep->lastTickTraversalId != traversalId)
				predep->traverseTick(traversalId, dt);
		}
		
		tick(dt);
	}
	
	void traverseDraw(const int traversalId)
	{
		Assert(lastDrawTraversalId != traversalId);
		lastDrawTraversalId = traversalId;
		
		for (auto predep : predeps)
		{
			if (predep->lastDrawTraversalId != traversalId)
				predep->traverseDraw(traversalId);
		}
		
		draw();
	}
	
	void trigger(const int outputSocketIndex)
	{
		Assert(outputSocketIndex >= 0 && outputSocketIndex < outputs.size());
		if (outputSocketIndex >= 0 && outputSocketIndex < outputs.size())
		{
			auto & outputSocket = outputs[outputSocketIndex];
			Assert(outputSocket.type == kVfxPlugType_Trigger);
			if (outputSocket.type == kVfxPlugType_Trigger)
			{
				// todo : iterate the list of outgoing connections, resolve nodes, call handleTrigger on nodes with correct inputSocketIndex
			}
		}
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
	
	const Surface * getInputSurface(const int index, const Surface * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getSurface();
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
	virtual void handleTrigger(const int inputSocketIndex) { }
	virtual void draw() const { }
};
