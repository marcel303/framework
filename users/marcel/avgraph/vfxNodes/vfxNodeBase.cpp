#include "framework.h"
#include "StringEx.h"
#include "Timer.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include <string.h>
#include <xmmintrin.h>

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

VfxImageCpu::VfxImageCpu()
	: sx(0)
	, sy(0)
	, numChannels(0)
	, alignment(1)
	, isInterleaved(false)
	, isPlanar(false)
	, channel()
{
}

void VfxImageCpu::setDataInterleaved(const uint8_t * data, const int _sx, const int _sy, const int _numChannels, const int _alignment, const int pitch)
{
	sx = _sx;
	sy = _sy;
	
	numChannels = _numChannels;
	alignment = _alignment;
	isInterleaved = true;
	isPlanar = _numChannels == 1;
	
	for (int i = 0; i < 4; ++i)
	{
		channel[i].data = data + (i % numChannels);
		channel[i].stride = numChannels;
		channel[i].pitch = pitch;
	}
}

void VfxImageCpu::setDataR8(const uint8_t * r, const int sx, const int sy, const int alignment, const int _pitch)
{
	const int pitch = _pitch == 0 ? sx * 1 : _pitch;
	
	setDataInterleaved(r, sx, sy, 1, alignment, pitch);
}

void VfxImageCpu::setDataRGBA8(const uint8_t * rgba, const int sx, const int sy, const int alignment, const int _pitch)
{
	const int pitch = _pitch == 0 ? sx * 4 : _pitch;
	
	setDataInterleaved(rgba, sx, sy, 4, alignment, pitch);
}

void VfxImageCpu::reset()
{
	for (int i = 0; i < numChannels; ++i)
	{
		channel[i] = Channel();
	}
	
	sx = 0;
	sy = 0;
	
	numChannels = 0;
	alignment = 1;
	isInterleaved = false;
	isPlanar = false;
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

VfxImageCpuData::VfxImageCpuData()
	: data(nullptr)
	, image()
{
}

VfxImageCpuData::~VfxImageCpuData()
{
	free();	
}

void VfxImageCpuData::alloc(const int sx, const int sy, const int numChannels, const bool interleaved)
{
	free();

	if (sx > 0 && sy > 0 && numChannels > 0)
	{
		data = (uint8_t*)_mm_malloc(sx * sy * numChannels, 16);

		image.setDataInterleaved(data, sx, sy, numChannels, 16, sx * numChannels);
	}
}

void VfxImageCpuData::allocOnSizeChange(const int sx, const int sy, const int numChannels, const bool interleaved)
{
	if (image.sx != sx || image.sy != sy || image.numChannels != numChannels || image.isInterleaved != interleaved)
	{
		alloc(sx, sy, numChannels, interleaved);
	}
}

void VfxImageCpuData::allocOnSizeChange(const VfxImageCpu & reference)
{
	if (image.sx != reference.sx || image.sy != reference.sy || image.numChannels != reference.numChannels)
	{
		alloc(reference.sx, reference.sy, reference.numChannels, image.isInterleaved);
	}
}

void VfxImageCpuData::free()
{
	_mm_free(data);
	data = nullptr;

	image = VfxImageCpu();
}

//

void VfxChannelData::alloc(const int _size)
{
	free();
	
	if (_size > 0)
	{
		data = new float[_size];
		size = _size;
	}
}

void VfxChannelData::free()
{
	delete[] data;
	data = nullptr;
	
	size = 0;
}

//

void VfxChannels::setData(const float * const * data, const bool * continuous, const int _size, const int _numChannels)
{
	Assert(_numChannels <= kMaxVfxChannels);
	
	size = _size;
	numChannels = std::min(_numChannels, kMaxVfxChannels);
	
	for (int i = 0; i < numChannels; ++i)
	{
		channels[i].data = data[i];
		channels[i].continuous = continuous != nullptr ? continuous[i] : false;
	}
}

void VfxChannels::setDataContiguous(const float * data, const bool continuous, const int _size, const int _numChannels)
{
	Assert(_numChannels <= kMaxVfxChannels);
	
	size = _size;
	numChannels = std::min(_numChannels, kMaxVfxChannels);
	
	for (int i = 0; i < numChannels; ++i)
	{
		channels[i].data = data + i * size;
		channels[i].continuous = continuous;
	}
}

void VfxChannels::reset()
{
	for (int i = 0; i < numChannels; ++i)
	{
		channels[i] = VfxChannel();
	}
	
	size = 0;
	numChannels = 0;
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
		
		dst.isReferencedByLink = true;
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

bool VfxPlug::isReferenced() const
{
	if (isReferencedByLink)
		return true;
	
	if (referencedByRealTimeConnectionTick != -1)
	{
		if (referencedByRealTimeConnectionTick >= g_currentVfxGraph->nextTickTraversalId - 60)
			return true;
	}
	
	return false;
}

//

void VfxNodeDescription::add(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	lines.push_back(text);
}

void VfxNodeDescription::add(const VfxImageCpu & image)
{
	add("size: %d x %d", image.sx, image.sy);
	add("numChannels: %d, alignment: %d", image.numChannels, image.alignment);
	add("isInterleaved: %d", image.isInterleaved);
	add("isPlanar: %d", image.isPlanar);
	
	int numBytes = 0;
	
	add("channels:");
	for (int i = 0; i < image.numChannels; ++i)
	{
		const VfxImageCpu::Channel & c = image.channel[i];
		add("[%d] stride: %d, pitch: %06d, data: %p", i, c.stride, c.pitch, c.data);
		
		numBytes += c.pitch * image.sy;
	}
	
	add("MEMORY: %.2f Kb", numBytes / 1024.0);
}

void VfxNodeDescription::add(const VfxChannels & channels)
{
	add("numChannels: %d", channels.numChannels);
	add("size: %d", channels.size);
	add("MEMORY: %.2f Kb", channels.numChannels * channels.size * sizeof(float) / 1024.0);
}

void VfxNodeDescription::addOpenglTexture(const uint32_t id)
{
	add("handle: %d", id);
	
	if (id != 0)
	{
		int sx = 0;
		int sy = 0;
		
		// capture current OpenGL states before we change them

		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
		checkErrorGL();
		
		// restore previous OpenGL states
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
		
		//
		
		add("size: %d x %d", sx, sy);
	}
}

void VfxNodeDescription::newline()
{
	add("");
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
	, tickOrder(0)
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
	
	tickOrder = g_currentVfxGraph->nextTickOrder++;
	
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
