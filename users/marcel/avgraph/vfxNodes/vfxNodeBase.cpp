#include "framework.h"
#include "Timer.h"
#include "vfxNodeBase.h"
#include <string.h>

//

VfxTransform::VfxTransform()
{
	matrix.MakeIdentity();
}

//

VfxTriggerData::VfxTriggerData()
	: type(kVfxTriggerDataType_None)
{
	memset(mem, 0, sizeof(mem));
}

//

VfxImage_Texture::VfxImage_Texture()
	: VfxImageBase()
	, texture(0)
{
}

int VfxImage_Texture::getSx() const
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

int VfxImage_Texture::getSy() const
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

uint32_t VfxImage_Texture::getTexture() const
{
	return texture;
}

//

VfxImageCpu::Channel::Channel()
	: data(nullptr)
	, stride(0)
	, pitch(0)
{
}

VfxImageCpu::VfxImageCpu()
	: sx(0)
	, sy(0)
	, interleaved()
	, channel()
{
}

void VfxImageCpu::setDataR8(const uint8_t * r, const int _sx, const int _sy, const int _pitch)
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

void VfxImageCpu::setDataRGBA8(const uint8_t * rgba, const int _sx, const int _sy, const int _pitch)
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

void VfxImageCpu::reset()
{
	sx = 0;
	sy = 0;
	
	interleaved = Channel();
	
	for (int i = 0; i < 4; ++i)
	{
		channel[i] = Channel();
	}
}

void VfxImageCpu::interleave1(const Channel * channel1, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
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

void VfxImageCpu::interleave3(const Channel * channel1, const Channel * channel2, const Channel * channel3, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
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

void VfxImageCpu::interleave4(const Channel * channel1, const Channel * channel2, const Channel * channel3, const Channel * channel4, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
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

//

void VfxPlug::connectTo(VfxPlug & dst)
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

void VfxPlug::connectTo(void * dstMem, const VfxPlugType dstType)
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

//

VfxNodeBase::TriggerTarget::TriggerTarget()
	: srcNode(nullptr)
	, srcSocketIndex(-1)
	, dstSocketIndex(-1)
{
}

VfxNodeBase::VfxNodeBase()
	: inputs()
	, outputs()
	, predeps()
	, triggerTargets()
	, lastTickTraversalId(-1)
	, lastDrawTraversalId(-1)
	, editorIsTriggered(false)
	, isPassthrough(false)
	, tickTimeAvg(0)
	, drawTimeAvg(0)
{
}

void VfxNodeBase::traverseTick(const int traversalId, const float dt)
{
	Assert(lastTickTraversalId != traversalId);
	lastTickTraversalId = traversalId;
	
	for (auto predep : predeps)
	{
		if (predep->lastTickTraversalId != traversalId)
			predep->traverseTick(traversalId, dt);
	}
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	tick(dt);
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
		
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1)) / 100;
}

void VfxNodeBase::traverseDraw(const int traversalId)
{
	Assert(lastDrawTraversalId != traversalId);
	lastDrawTraversalId = traversalId;
	
	for (auto predep : predeps)
	{
		if (predep->lastDrawTraversalId != traversalId)
			predep->traverseDraw(traversalId);
	}
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	draw();
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
		
	drawTimeAvg = (drawTimeAvg * 99 + (t2 - t1)) / 100;
}

void VfxNodeBase::trigger(const int outputSocketIndex)
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
