#pragma once

#include "framework.h"
#include "graph.h"
#include <stdint.h>
#include <string.h>

enum VfxTriggerDataType
{
	kVfxTriggerDataType_None,
	kVfxTriggerDataType_Int
};

struct VfxTriggerData
{
	VfxTriggerDataType type;
	
	union
	{
		int intValue;
		uint8_t mem[8];
	};
	
	VfxTriggerData()
		: type(kVfxTriggerDataType_None)
	{
		memset(mem, 0, sizeof(mem));
	}
	
	int asInt() const
	{
		switch (type)
		{
		case kVfxTriggerDataType_None:
			return 0;
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
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_String,
	kVfxPlugType_Image,
	kVfxPlugType_Surface,
	kVfxPlugType_Trigger
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
	virtual void handleTrigger(const int inputSocketIndex) { }
	virtual void draw() const { }
};
