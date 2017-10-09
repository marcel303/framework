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
	Assert((uintptr_t(data) & (_alignment - 1)) == 0);

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

int VfxImageCpu::getMemoryUsage() const
{
	int result = 0;
	
	for (int i = 0; i < numChannels; ++i)
	{
		result += sy * channel[i].pitch / channel[i].stride;
	}
	
	return result;
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
	
	//
	
	Assert(interleaved);

	if (sx > 0 && sy > 0 && numChannels > 0)
	{
		data = (uint8_t*)_mm_malloc(sx * sy * numChannels, 16);
		
		// todo : non interleaved case ?
		
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
	
	sx = _size;
	sy = 1;
	
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
	
	sx = _size;
	sy = 1;
	
	for (int i = 0; i < numChannels; ++i)
	{
		channels[i].data = data + i * size;
		channels[i].continuous = continuous;
	}
}

void VfxChannels::setData2D(const float * const * data, const bool * continuous, const int _sx, const int _sy, const int _numChannels)
{
	Assert(_numChannels <= kMaxVfxChannels);
	
	size = _sx * _sy;
	numChannels = std::min(_numChannels, kMaxVfxChannels);
	
	sx = _sx;
	sy = _sy;
	
	for (int i = 0; i < numChannels; ++i)
	{
		channels[i].data = data[i];
		channels[i].continuous = continuous != nullptr ? continuous[i] : false;
	}
}

void VfxChannels::setData2DContiguous(const float * data, const bool continuous, const int _sx, const int _sy, const int _numChannels)
{
	Assert(_numChannels <= kMaxVfxChannels);
	
	size = _sx * _sy;
	numChannels = std::min(_numChannels, kMaxVfxChannels);
	
	sx = _sx;
	sy = _sy;
	
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
	
	sx = 0;
	sy = 0;
}

//

#if EXTENDED_INPUTS

void VfxFloatArray::update()
{
	const int numElems = elems.size();
	
	if (numElems == 0)
		return;
	if (numElems == 1 && elems[0].hasRange == false)
		return;
	
	//
	
	float s = 0.f;
	
	for (auto & elem : elems)
	{
		float value;
		
		if (elem.hasRange)
		{
			const float in = *elem.value;
			
			float t;
			
			if (elem.inMin == elem.inMax)
				t = 0.f;
			else
				t = (in - elem.inMin) / (elem.inMax - elem.inMin);
			
			const float t1 = t;
			const float t2 = 1.f - t;
			
			value = elem.outMax * t1 + elem.outMin * t2;
		}
		else
		{
			value = *elem.value;
		}
		
		s += value;
	}
	
	sum = s;
}

float * VfxFloatArray::get()
{
	// fixme : g_currentVfxGraph should always be valid here? right now we just validate without checking traversal id when g_currentVfxGraph is nullptr. potetially doing this work twice (or more). on the other hand, if get is called after the update, it may set the traversal id to that of the next frame, without working on the array that will be updated in the future. this would be even worse..
	
	if (g_currentVfxGraph == nullptr || lastUpdateTick != g_currentVfxGraph->nextTickTraversalId)
	{
		if (g_currentVfxGraph != nullptr)
		{
			lastUpdateTick = g_currentVfxGraph->nextTickTraversalId;
		}
		
		update();
	}
	
	const int numElems = elems.size();
	
	if (numElems == 0)
		return immediateValue;
	else if (numElems == 1 && elems[0].hasRange == false)
		return elems[0].value;
	else
		return &sum;
}

#endif

//

void VfxPlug::connectTo(VfxPlug & dst)
{
	if (type == kVfxPlugType_DontCare)
	{
		mem = dst.mem;
		memType = dst.type;
		
		dst.isReferencedByLink = true;
	}
	else if (dst.type != type)
	{
		logError("node connection failed. type mismatch");
	}
#if EXTENDED_INPUTS
	else if (dst.type == kVfxPlugType_Float)
	{
		VfxFloatArray::Elem elem;
		elem.value = (float*)dst.mem;
		floatArray.elems.push_back(elem);
		memType = dst.type;
		
		dst.isReferencedByLink = true;
	}
#endif
	else
	{
		mem = dst.mem;
		memType = dst.type;
		
		dst.isReferencedByLink = true;
	}
}

void VfxPlug::connectTo(void * dstMem, const VfxPlugType dstType, const bool isImmediate)
{
	if (type == kVfxPlugType_DontCare)
	{
		mem = dstMem;
		memType = dstType;
	}
	else if (dstType != type)
	{
		logError("node connection failed. type mismatch");
	}
#if EXTENDED_INPUTS
	else if (dstType == kVfxPlugType_Float)
	{
		if (isImmediate)
		{
			floatArray.immediateValue = (float*)dstMem;
		}
		else
		{
			VfxFloatArray::Elem elem;
			elem.value = (float*)dstMem;
			floatArray.elems.push_back(elem);
		}
		
		memType = dstType;
	}
#endif
	else
	{
		mem = dstMem;
		memType = dstType;
	}
}

void VfxPlug::setMap(const void * dst, const float inMin, const float inMax, const float outMin, const float outMax)
{
#if EXTENDED_INPUTS
	logDebug("map: %.2f, %.2f -> %.2f, %.2f", inMin, inMax, outMin, outMax);
	
	for (auto & elem : floatArray.elems)
	{
		if (elem.value == dst)
		{
			elem.hasRange = true;
			elem.inMin = inMin;
			elem.inMax = inMax;
			elem.outMin = outMin;
			elem.outMax = outMax;
			
			floatArray.lastUpdateTick = -1;
		}
	}
#endif
}

void VfxPlug::clearMap(const void * dst)
{
#if EXTENDED_INPUTS
	logDebug("clearMap");
	
	for (auto & elem : floatArray.elems)
	{
		if (elem.value == dst)
		{
			elem.hasRange = false;
			
			floatArray.lastUpdateTick = -1;
		}
	}
#endif
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

void VfxNodeDescription::add(const char * name, const VfxImageBase & image)
{
	addOpenglTexture(name, image.getTexture());
}

void VfxNodeDescription::add(const char * name, const VfxImageCpu & image)
{
	add("%s. size: %d x %d", name, image.sx, image.sy);
	add("numChannels: %d, alignment: %d", image.numChannels, image.alignment);
	add("isInterleaved: %d, isPlanar: %d", image.isInterleaved, image.isPlanar);
	
	for (int i = 0; i < image.numChannels; ++i)
	{
		const VfxImageCpu::Channel & c = image.channel[i];
		add("[%d] stride: %d, pitch: %06d, data: %p", i, c.stride, c.pitch, c.data);
	}
	
	const int numBytes = image.getMemoryUsage();
	add("MEMORY: %.2f Kb", numBytes / 1024.0);
}

void VfxNodeDescription::add(const VfxChannels & channels)
{
	add("numChannels: %d", channels.numChannels);
	if (channels.sy > 1)
		add("size: %d x %d", channels.sx, channels.sy);
	else
		add("size: %d", channels.size);
	add("MEMORY: %.2f Kb", channels.numChannels * channels.size * sizeof(float) / 1024.0);
}

void VfxNodeDescription::addOpenglTexture(const char * name, const uint32_t id)
{
	if (id == 0)
	{
		add("%s. id: %d", name, id);
	}
	else
	{
		int sx = 0;
		int sy = 0;
		int internalFormat = 0;
		
		// capture current OpenGL states before we change them

		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
		checkErrorGL();
		
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
		checkErrorGL();
		
		// restore previous OpenGL states
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
		
		//
		
		const char * formatString =
			internalFormat == GL_R8 ? "R8, 8 bpp (unorm)" :
			internalFormat == GL_RG8 ? "R8G8, 16 bpp (unorm)" :
			internalFormat == GL_R16F ? "R16F, 16 bpp (half float)" :
			internalFormat == GL_R32F ? "R32F, 32 bpp (float)" :
			internalFormat == GL_RGB8 ? "RGB888, 24 bpp (unorm)" :
			internalFormat == GL_RGBA8 ? "RGBA8888, 32 bpp (unorm)" :
			"n/a";
		
		const int bpp =
			internalFormat == GL_R8 ? 8 :
			internalFormat == GL_RG8 ? 16 :
			internalFormat == GL_R16F ? 16 :
			internalFormat == GL_R32F ? 32 :
			internalFormat == GL_RGB8 ? 24 :
			internalFormat == GL_RGBA8 ? 32 :
			0;
		
		add("%s. size: %d x %d, id: %d", name, sx, sy, id);
		add("format: %s, size: %.2f Mb (estimate)", formatString, (sx * sy * bpp / 8) / 1024.0 / 1024.0);
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
	
	//
	
	for (auto predep : predeps)
	{
		if (predep->lastTickTraversalId != traversalId)
			predep->traverseTick(traversalId, dt);
	}
	
	//
	
	tickOrder = g_currentVfxGraph->nextTickOrder++;
	
	//
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	tick(dt);
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	//
	
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1)) / 100;
}

void VfxNodeBase::traverseDraw(const int traversalId)
{
	Assert(lastDrawTraversalId != traversalId);
	lastDrawTraversalId = traversalId;
	
	//
	
	beforeDraw();
	
	//
	
	for (auto predep : predeps)
	{
		if (predep->lastDrawTraversalId != traversalId)
			predep->traverseDraw(traversalId);
	}
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	draw();
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	//
	
	drawTimeAvg = (drawTimeAvg * 95 + (t2 - t1) * 5) / 100;
	
	//
	
	afterDraw();
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
			
			for (auto & triggerTarget : triggerTargets)
			{
				if (triggerTarget.dstSocketIndex == outputSocketIndex)
				{
					triggerTarget.srcNode->editorIsTriggered = true;
					
					triggerTarget.srcNode->handleTrigger(triggerTarget.srcSocketIndex);
				}
			}
		}
	}
}

//

VfxEnumTypeRegistration * g_vfxEnumTypeRegistrationList = nullptr;

VfxEnumTypeRegistration::VfxEnumTypeRegistration()
	: enumName()
	, nextValue(0)
	, elems()
{
	next = g_vfxEnumTypeRegistrationList;
	g_vfxEnumTypeRegistrationList = this;
}
	
void VfxEnumTypeRegistration::elem(const char * name, const int value)
{
	Elem e;
	e.name = name;
	e.value = value == -1 ? nextValue : value;
	
	elems.push_back(e);
	
	nextValue = e.value + 1;
}

// todo : move elsewhere ?

#include "graph.h"

void createVfxEnumTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, VfxEnumTypeRegistration * registrationList)
{
	for (VfxEnumTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions[registration->enumName];
		
		enumDefinition.enumName = registration->enumName;
		
		for (auto & src : registration->elems)
		{
			GraphEdit_EnumDefinition::Elem dst;
			
			dst.name = src.name;
			dst.value = src.value;
			
			enumDefinition.enumElems.push_back(dst);
		}
	}
}

//

VfxNodeTypeRegistration * g_vfxNodeTypeRegistrationList = nullptr;

VfxNodeTypeRegistration::VfxNodeTypeRegistration()
	: next(nullptr)
	, create(nullptr)
	, typeName()
	, inputs()
	, outputs()
	, createResourceEditor(nullptr)
{
	next = g_vfxNodeTypeRegistrationList;
	g_vfxNodeTypeRegistrationList = this;
}

void VfxNodeTypeRegistration::in(const char * name, const char * typeName, const char * defaultValue, const char * displayName)
{
	Input i;
	i.name = name;
	i.displayName = displayName;
	i.typeName = typeName;
	i.defaultValue = defaultValue;
	
	inputs.push_back(i);
}

void VfxNodeTypeRegistration::inEnum(const char * name, const char * enumName, const char * defaultValue, const char * displayName)
{
	Input i;
	i.name = name;
	i.displayName = displayName;
	i.typeName = "int";
	i.enumName = enumName;
	i.defaultValue = defaultValue;
	
	inputs.push_back(i);
}

void VfxNodeTypeRegistration::out(const char * name, const char * typeName, const char * displayName)
{
	Output o;
	o.name = name;
	o.typeName = typeName;
	o.displayName = displayName;
	
	outputs.push_back(o);
}

void VfxNodeTypeRegistration::outEditable(const char * name)
{
	for (auto & o : outputs)
		if (o.name == name)
			o.isEditable = true;
}

//

// todo : move elsewhere ?

#include "graph.h"

void createVfxNodeTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, VfxNodeTypeRegistration * registrationList)
{
	for (VfxNodeTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		GraphEdit_TypeDefinition typeDefinition;
		
		typeDefinition.typeName = registration->typeName;
		typeDefinition.displayName = registration->displayName;
		
		typeDefinition.resourceTypeName = registration->resourceTypeName;
		
		for (int i = 0; i < registration->inputs.size(); ++i)
		{
			auto & src = registration->inputs[i];
			
			GraphEdit_TypeDefinition::InputSocket inputSocket;
			inputSocket.typeName = src.typeName;
			inputSocket.name = src.name;
			inputSocket.index = i;
			inputSocket.enumName = src.enumName;
			inputSocket.defaultValue = src.defaultValue;
			
			typeDefinition.inputSockets.push_back(inputSocket);
		}
		
		for (int i = 0; i < registration->outputs.size(); ++i)
		{
			auto & src = registration->outputs[i];
			
			GraphEdit_TypeDefinition::OutputSocket outputSocket;
			outputSocket.typeName = src.typeName;
			outputSocket.name = src.name;
			outputSocket.isEditable = src.isEditable;
			outputSocket.index = i;
			
			typeDefinition.outputSockets.push_back(outputSocket);
		}
		
		typeDefinition.resourceEditor.create = registration->createResourceEditor;
		
		typeDefinition.createUi();
		
		typeDefinitionLibrary.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}
