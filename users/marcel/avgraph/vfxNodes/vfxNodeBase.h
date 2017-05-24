#pragma once

#include "framework.h"
#include "graph.h"
#include "Mat4x4.h"
#include <stdint.h>
#include <string.h>

class Surface;

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
	
	VfxTriggerData()
		: type(kVfxTriggerDataType_None)
	{
		memset(mem, 0, sizeof(mem));
	}
	
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
	
	virtual int getSx() const override
	{
		int result = 0;
		
		if (texture != 0)
		{
			// todo : nicely restore previously bound texture ?
			gxSetTexture(texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &result);
			gxSetTexture(0);
		}
		
		return result;
	}
	
	virtual int getSy() const override
	{
		int result = 0;
		
		if (texture != 0)
		{
			// todo : nicely restore previously bound texture ?
			gxSetTexture(texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &result);
			gxSetTexture(0);
		}
		
		return result;
	}
	
	virtual GLuint getTexture() const override
	{
		return texture;
	}
};

struct VfxImageCpu
{
	struct Channel
	{
		const uint8_t * data;
		int stride;
		int pitch;
		
		Channel()
			: data(nullptr)
			, pitch(0)
		{
		}
	};
	
	int sx;
	int sy;
	
	Channel interleaved;
	
	Channel channel[4];
	
	VfxImageCpu()
		: sx(0)
		, sy(0)
		, channel()
	{
	}
	
	void setDataR8(const uint8_t * r, const int _sx, const int _sy, const int _pitch)
	{
		sx = _sx;
		sy = _sy;
		
		const int stride = 1;
		const int pitch = _pitch == 0 ? sx * 1 : _pitch;
		
		interleaved.data = r;
		interleaved.stride = stride;
		interleaved.pitch = pitch;
		
		for (int i = 0; i < 4; ++i)
		{
			channel[i].data = r;
			channel[i].stride = 1;
			channel[i].pitch = pitch;
		}
	}
	
	void setDataRGBA8(const uint8_t * rgba, const int _sx, const int _sy, const int _pitch)
	{
		sx = _sx;
		sy = _sy;
		
		const int stride = 4;
		const int pitch = _pitch == 0 ? sx * 4 : _pitch;
		
		interleaved.data = rgba;
		interleaved.stride = stride;
		interleaved.pitch = pitch;
		
		for (int i = 0; i < 4; ++i)
		{
			channel[i].data = rgba + i;
			channel[i].stride = stride;
			channel[i].pitch = pitch;
		}
	}
	
	static void interleave1(const Channel * channel1, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
	{
		const int dstPitch = _dstPitch == 0 ? sx * 1 : _dstPitch;
		
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict src1 = channel1->data + y * channel1->pitch;
				  uint8_t * __restrict dst  = _dst + y * dstPitch;
			
			if (channel1->stride == 1)
			{
				memcpy(dst, src1, sx);
			}
			else
			{
				for (int x = 0; x < sx; ++x)
				{
					*dst++ = *src1;
					
					src1 += channel1->stride;
				}
			}
		}
	}
	
	static void interleave3(const Channel * channel1, const Channel * channel2, const Channel * channel3, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
	{
		const int dstPitch = _dstPitch == 0 ? sx * 3 : _dstPitch;
		
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict src1 = channel1->data + y * channel1->pitch;
			const uint8_t * __restrict src2 = channel2->data + y * channel2->pitch;
			const uint8_t * __restrict src3 = channel3->data + y * channel3->pitch;
				  uint8_t * __restrict dst  = _dst + y * dstPitch;
			
			for (int x = 0; x < sx; ++x)
			{
				*dst++ = *src1;
				*dst++ = *src2;
				*dst++ = *src3;
				
				src1 += channel1->stride;
				src2 += channel2->stride;
				src3 += channel3->stride;
			}
		}
	}
	
	static void interleave4(const Channel * channel1, const Channel * channel2, const Channel * channel3, const Channel * channel4, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
	{
		const int dstPitch = _dstPitch == 0 ? sx * 4 : _dstPitch;
		
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict src1 = channel1->data + y * channel1->pitch;
			const uint8_t * __restrict src2 = channel2->data + y * channel2->pitch;
			const uint8_t * __restrict src3 = channel3->data + y * channel3->pitch;
			const uint8_t * __restrict src4 = channel4->data + y * channel4->pitch;
				  uint8_t * __restrict dst  = _dst + y * dstPitch;
			
			for (int x = 0; x < sx; ++x)
			{
				*dst++ = *src1;
				*dst++ = *src2;
				*dst++ = *src3;
				*dst++ = *src4;
				
				src1 += channel1->stride;
				src2 += channel2->stride;
				src3 += channel3->stride;
				src4 += channel4->stride;
			}
		}
	}
	
	void reset()
	{
		sx = 0;
		sy = 0;
		
		interleaved = Channel();
		
		for (int i = 0; i < 4; ++i)
		{
			channel[i] = Channel();
		}
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
	kVfxPlugType_ImageCpu,
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
	
	VfxImageCpu * getImageCpu() const
	{
		Assert(type == kVfxPlugType_ImageCpu);
		return (VfxImageCpu*)mem;
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
	
	Color & getRwColor()
	{
		Assert(type == kVfxPlugType_Color);
		return *((Color*)mem);
	}
};

struct VfxNodeBase
{
	struct TriggerTarget
	{
		VfxNodeBase * srcNode;
		int srcSocketIndex;
		int dstSocketIndex;
		
		TriggerTarget()
			: srcNode(nullptr)
			, srcSocketIndex(-1)
			, dstSocketIndex(-1)
		{
		}
	};
	
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	std::vector<TriggerTarget> triggerTargets;
	
	int lastTickTraversalId;
	int lastDrawTraversalId;
	bool editorIsTriggered; // only here for real-time connection with graph editor
	
	bool isPassthrough;
	
	VfxNodeBase()
		: inputs()
		, outputs()
		, predeps()
		, triggerTargets()
		, lastTickTraversalId(-1)
		, lastDrawTraversalId(-1)
		, editorIsTriggered(false)
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
		editorIsTriggered = true;
		
		Assert(outputSocketIndex >= 0 && outputSocketIndex < outputs.size());
		if (outputSocketIndex >= 0 && outputSocketIndex < outputs.size())
		{
			auto & outputSocket = outputs[outputSocketIndex];
			Assert(outputSocket.type == kVfxPlugType_Trigger);
			if (outputSocket.type == kVfxPlugType_Trigger)
			{
				// iterate the list of outgoing connections, call handleTrigger on nodes with correct outputSocketIndex
				
				const VfxTriggerData & triggerData = outputSocket.getTriggerData();
				
				for (auto & triggerTarget : triggerTargets)
				{
					if (triggerTarget.dstSocketIndex == outputSocketIndex)
					{
						triggerTarget.srcNode->editorIsTriggered = true;
						
						triggerTarget.srcNode->handleTrigger(triggerTarget.srcSocketIndex, triggerData);
					}
				}
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
