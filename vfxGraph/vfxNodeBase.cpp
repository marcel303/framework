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
#include "MemAlloc.h"
#include "StringEx.h"
#include "Timer.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include <string.h>

//

#if __SSE2__
	// for interleave and deinterleave methods of VfxImageCpu
	#include <xmmintrin.h>

	// for _mm_shuffle_epi8 in deinterleave4
	// todo : _mm_shuffle_epi8 requires SSSE3. check for it
	#include <tmmintrin.h>
#endif

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
		// todo : use glGetTextureLevelParameteriv. upgrade GLEW ?
		
		/*
		if (glGetTextureLevelParameteriv)
		{
			glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &result);
			checkErrorGL();
		}
		else
		*/
		{
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &result);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
		}
	}
	
	return result;
}

int VfxImage_Texture::getSy() const
{
	int result = 0;
	
	if (texture != 0)
	{
		// todo : use glGetTextureLevelParameteriv. upgrade GLEW ?
		
		/*
		if (glGetTextureLevelParameteriv)
		{
			glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &result);
			checkErrorGL();
		}
		else
		*/
		{
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &result);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
		}
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
	, channel()
{
}

void VfxImageCpu::setDataContiguous(const uint8_t * data, const int _sx, const int _sy, const int _numChannels, const int _alignment, const int pitch)
{
	Assert((uintptr_t(data) & (_alignment - 1)) == 0);

	sx = _sx;
	sy = _sy;
	
	numChannels = _numChannels;
	alignment = _alignment;
	
	for (int i = 0; i < numChannels; ++i)
	{
		channel[i].data = data + i * sy * pitch;
		channel[i].pitch = pitch;
	}
	
	for (int i = numChannels; i < 4; ++i)
		channel[i] = channel[0];
}

void VfxImageCpu::setData(const uint8_t ** datas, const int _sx, const int _sy, const int _numChannels, const int _alignment, const int * pitches)
{
	sx = _sx;
	sy = _sy;
	
	numChannels = _numChannels;
	alignment = _alignment;
	
	for (int i = 0; i < numChannels; ++i)
	{
		const uint8_t * data = datas[i];
		const int pitch = pitches[i];
		
		Assert((uintptr_t(data) & (_alignment - 1)) == 0);
		
		channel[i].data = data;
		channel[i].pitch = pitch;
	}
	
	for (int i = numChannels; i < 4; ++i)
		channel[i] = channel[0];
}

void VfxImageCpu::setDataR8(const uint8_t * r, const int sx, const int sy, const int alignment, const int _pitch)
{
	const int pitch = _pitch == 0 ? sx * 1 : _pitch;
	
	setData(&r, sx, sy, 1, alignment, &pitch);
}

void VfxImageCpu::setDataRGB8(const uint8_t * r, const uint8_t * g, const uint8_t * b, const int sx, const int sy, const int alignment, const int _pitch)
{
	const int pitch = _pitch == 0 ? sx * 3 : _pitch;
	
	const uint8_t * datas[3] = { r, g, b };
	const int pitches[3] = { pitch, pitch, pitch };
	
	setData(datas, sx, sy, 3, alignment, pitches);
}

void VfxImageCpu::setDataRGBA8(const uint8_t * r, const uint8_t * g, const uint8_t * b, const uint8_t * a, const int sx, const int sy, const int alignment, const int _pitch)
{
	const int pitch = _pitch == 0 ? sx * 3 : _pitch;
	
	const uint8_t * datas[4] = { r, g, b, a };
	const int pitches[4] = { pitch, pitch, pitch, pitch };
	
	setData(datas, sx, sy, 4, alignment, pitches);
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
}

int VfxImageCpu::getMemoryUsage() const
{
	int result = 0;
	
	for (int i = 0; i < numChannels; ++i)
	{
		result += sy * channel[i].pitch;
	}
	
	return result;
}

void VfxImageCpu::interleave1(const Channel & channel1, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
{
	const int dstPitch = _dstPitch == 0 ? sx * 1 : _dstPitch;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict src1 = channel1.data + y * channel1.pitch;
			  uint8_t * __restrict dst  = _dst + y * dstPitch;
		
		memcpy(dst, src1, sx);
	}
}

void VfxImageCpu::interleave3(const Channel & channel1, const Channel & channel2, const Channel & channel3, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
{
	const int dstPitch = _dstPitch == 0 ? sx * 3 : _dstPitch;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict src1 = channel1.data + y * channel1.pitch;
		const uint8_t * __restrict src2 = channel2.data + y * channel2.pitch;
		const uint8_t * __restrict src3 = channel3.data + y * channel3.pitch;
			  uint8_t * __restrict dst  = _dst + y * dstPitch;
		
		for (int x = 0; x < sx; ++x)
		{
			*dst++ = *src1++;
			*dst++ = *src2++;
			*dst++ = *src3++;
		}
	}
}

void VfxImageCpu::interleave4(const Channel & channel1, const Channel & channel2, const Channel & channel3, const Channel & channel4, uint8_t * _dst, const int _dstPitch, const int sx, const int sy)
{
	const int dstPitch = _dstPitch == 0 ? sx * 4 : _dstPitch;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict src1 = channel1.data + y * channel1.pitch;
		const uint8_t * __restrict src2 = channel2.data + y * channel2.pitch;
		const uint8_t * __restrict src3 = channel3.data + y * channel3.pitch;
		const uint8_t * __restrict src4 = channel4.data + y * channel4.pitch;
			  uint8_t * __restrict dst  = _dst + y * dstPitch;
		
		int begin = 0;
		
	#if __SSE2__
		/*
		// without SSE
			[II] Benchmark: interleave4: 0.000067 sec
			[II] Benchmark: interleave4: 0.000069 sec
			[II] Benchmark: interleave4: 0.000069 sec
			[II] Benchmark: interleave4: 0.000069 sec

		// with SSE
			[II] Benchmark: interleave4: 0.000040 sec
			[II] Benchmark: interleave4: 0.000044 sec
			[II] Benchmark: interleave4: 0.000034 sec
			[II] Benchmark: interleave4: 0.000034 sec
		*/
		const int sx_16 = sx / 16;
		
		const __m128i * __restrict src1_16 = (const __m128i*)src1;
		const __m128i * __restrict src2_16 = (const __m128i*)src2;
		const __m128i * __restrict src3_16 = (const __m128i*)src3;
		const __m128i * __restrict src4_16 = (const __m128i*)src4;
		__m128i * __restrict dst_4 = (__m128i*)dst;

		for (int x = 0; x < sx_16; ++x)
		{
			const __m128i r_16 = _mm_loadu_si128(&src1_16[x]);
			const __m128i g_16 = _mm_loadu_si128(&src2_16[x]);
			const __m128i b_16 = _mm_loadu_si128(&src3_16[x]);
			const __m128i a_16 = _mm_loadu_si128(&src4_16[x]);
			
			const __m128i rg1_8 = _mm_unpacklo_epi8(r_16, g_16);
			const __m128i rg2_8 = _mm_unpackhi_epi8(r_16, g_16);
			const __m128i ba1_8 = _mm_unpacklo_epi8(b_16, a_16);
			const __m128i ba2_8 = _mm_unpackhi_epi8(b_16, a_16);
			
			const __m128i rgba1_16 = _mm_unpacklo_epi16(rg1_8, ba1_8);
			const __m128i rgba2_16 = _mm_unpackhi_epi16(rg1_8, ba1_8);
			const __m128i rgba3_16 = _mm_unpacklo_epi16(rg2_8, ba2_8);
			const __m128i rgba4_16 = _mm_unpackhi_epi16(rg2_8, ba2_8);
			
			_mm_storeu_si128(&dst_4[x * 4 + 0], rgba1_16);
			_mm_storeu_si128(&dst_4[x * 4 + 1], rgba2_16);
			_mm_storeu_si128(&dst_4[x * 4 + 2], rgba3_16);
			_mm_storeu_si128(&dst_4[x * 4 + 3], rgba4_16);
		}
		
		begin = sx_16 * 16;
	#endif
	
		for (int x = begin; x < sx; ++x)
		{
			dst[x * 4 + 0] = src1[x];
			dst[x * 4 + 1] = src2[x];
			dst[x * 4 + 2] = src3[x];
			dst[x * 4 + 3] = src4[x];
		}
	}
}

void VfxImageCpu::deinterleave1(
	const uint8_t * src, const int sx, const int sy, const int alignment, const int _pitch,
	Channel & channel1)
{
	const int pitch = _pitch == 0 ? sx * 4 : _pitch;
	
	if (pitch == channel1.pitch)
	{
		memcpy((uint8_t*)channel1.data, src, sy * pitch);;
	}
	else
	{
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict srcPtr = src + y * pitch + 0;
			
			uint8_t * __restrict dst = (uint8_t*)channel1.data + y * channel1.pitch;
			
			memcpy(dst, srcPtr, sx);
		}
	}
}

void VfxImageCpu::deinterleave3(
	const uint8_t * src, const int sx, const int sy, const int alignment, const int _pitch,
	Channel & channel1, Channel & channel2, Channel & channel3)
{
	const int pitch = _pitch == 0 ? sx * 4 : _pitch;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcPtr = src + y * pitch;
		
		uint8_t * __restrict dst1 = (uint8_t*)channel1.data + y * channel1.pitch;
		uint8_t * __restrict dst2 = (uint8_t*)channel2.data + y * channel2.pitch;
		uint8_t * __restrict dst3 = (uint8_t*)channel3.data + y * channel3.pitch;
		
		for (int x = 0; x < sx; ++x)
		{
			dst1[x] = srcPtr[0];
			dst2[x] = srcPtr[1];
			dst3[x] = srcPtr[2];
			
			srcPtr += 3;
		}
	}
}

void VfxImageCpu::deinterleave4(
	const uint8_t * src, const int sx, const int sy, const int alignment, const int _pitch,
	Channel & channel1, Channel & channel2, Channel & channel3, Channel & channel4)
{
	const int pitch = _pitch == 0 ? sx * 4 : _pitch;
	
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcPtr = src + y * pitch;
		
		uint8_t * __restrict dst1 = (uint8_t*)channel1.data + y * channel1.pitch;
		uint8_t * __restrict dst2 = (uint8_t*)channel2.data + y * channel2.pitch;
		uint8_t * __restrict dst3 = (uint8_t*)channel3.data + y * channel3.pitch;
		uint8_t * __restrict dst4 = (uint8_t*)channel4.data + y * channel4.pitch;
		
		int begin = 0;
		
	#if __SSE2__
		/*
		without SSE:
			[II] Benchmark: deinterleave4: 0.000278 sec
			[II] Benchmark: deinterleave4: 0.000313 sec
			[II] Benchmark: deinterleave4: 0.000359 sec
		 
		with SSE:
			[II] Benchmark: deinterleave4: 0.000061 sec
			[II] Benchmark: deinterleave4: 0.000069 sec
			[II] Benchmark: deinterleave4: 0.000076 sec
		*/
		
		const __m128i * __restrict src_4 = (__m128i*)srcPtr;
		__m64 * __restrict dst1_8 = (__m64*)dst1;
		__m64 * __restrict dst2_8 = (__m64*)dst2;
		__m64 * __restrict dst3_8 = (__m64*)dst3;
		__m64 * __restrict dst4_8 = (__m64*)dst4;
		
		const int sx_8 = sx / 8;
		
		for (int x = 0; x < sx_8; ++x)
		{
			__m128i rgba1 = _mm_loadu_si128(&src_4[x * 2 + 0]);
			__m128i rgba2 = _mm_loadu_si128(&src_4[x * 2 + 1]);
			__m128i shuffleIndices = _mm_set_epi8(
				15, 11, 7, 3,
				14, 10, 6, 2,
				13, 9,  5, 1,
				12, 8,  4, 0);
			
			rgba1 = _mm_shuffle_epi8(rgba1, shuffleIndices);
			rgba2 = _mm_shuffle_epi8(rgba2, shuffleIndices);
			
			__m128i rg = _mm_unpacklo_epi32(rgba1, rgba2);
			__m128i ba = _mm_unpackhi_epi32(rgba1, rgba2);
			
      		_mm_storel_pi(&dst1_8[x], _mm_castsi128_ps(rg));
      		_mm_storeh_pi(&dst2_8[x], _mm_castsi128_ps(rg));
      		_mm_storel_pi(&dst3_8[x], _mm_castsi128_ps(ba));
      		_mm_storeh_pi(&dst4_8[x], _mm_castsi128_ps(ba));
		}
		
		begin = sx_8 * 8;
	#endif
	
		for (int x = begin; x < sx; ++x)
		{
			dst1[x] = srcPtr[x * 4 + 0];
			dst2[x] = srcPtr[x * 4 + 1];
			dst3[x] = srcPtr[x * 4 + 2];
			dst4[x] = srcPtr[x * 4 + 3];
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

void VfxImageCpuData::alloc(const int sx, const int sy, const int numChannels)
{
	free();
	
	//

	if (sx > 0 && sy > 0 && numChannels > 0)
	{
		const int paddedSx = (sx + 15) & (~15);
		const int pitch = paddedSx;
		
		data = (uint8_t*)MemAlloc(pitch * sy * numChannels, 16);
		
		image.setDataContiguous(data, sx, sy, numChannels, 16, pitch);
	}
}

void VfxImageCpuData::allocOnSizeChange(const int sx, const int sy, const int numChannels)
{
	if (image.sx != sx || image.sy != sy || image.numChannels != numChannels)
	{
		alloc(sx, sy, numChannels);
	}
}

void VfxImageCpuData::allocOnSizeChange(const VfxImageCpu & reference)
{
	if (image.sx != reference.sx || image.sy != reference.sy || image.numChannels != reference.numChannels)
	{
		alloc(reference.sx, reference.sy, reference.numChannels);
	}
}

void VfxImageCpuData::free()
{
	MemFree(data);
	data = nullptr;

	image.reset();
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

void VfxChannelData::allocOnSizeChange(const int _size)
{
	if (_size != size)
	{
		alloc(_size);
	}
}

void VfxChannelData::free()
{
	delete[] data;
	data = nullptr;
	
	size = 0;
}

#include "Parse.h" // todo : move

// todo : remove framework dependency

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

void VfxChannelData::parse(const char * text, float *& data, int & dataSize)
{
	std::vector<std::string> parts;
	splitString(text, parts, ' ');
	
	if (parts.empty())
	{
		data = nullptr;
		dataSize = 0;
	}
	else
	{
		data = new float[parts.size()];
		dataSize = parts.size();
		
		for (size_t i = 0; i < parts.size(); ++i)
		{
			const float value = Parse::Float(parts[i]);
			
			data[i] = value;
		}
	}
}

//

void VfxChannel::setData(const float * _data, const bool _continuous, const int _size)
{
	data = _data;
	continuous = _continuous;
	
	size = _size;
	
	sx = _size;
	sy = 1;
}

void VfxChannel::setData2D(const float * _data, const bool _continuous, const int _sx, const int _sy)
{
	data = _data;
	continuous = _continuous;
	
	size = _sx * _sy;
	
	sx = _sx;
	sy = _sy;
}

void VfxChannel::reset()
{
	data = nullptr;
	continuous = false;
	
	size = 0;
	
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
	Assert(g_currentVfxGraph != nullptr);
	if (lastUpdateTick != g_currentVfxGraph->currentTickTraversalId)
	{
		lastUpdateTick = g_currentVfxGraph->currentTickTraversalId;
		
		update();
	}
	
	Assert(!elems.empty());
	
	const int numElems = elems.size();
	
	if (numElems == 1 && elems[0].hasRange == false)
		return elems[0].value;
	else
		return &sum;
}

#endif

//

void VfxPlug::connectTo(VfxPlug & dst)
{
	if (dst.type != type)
	{
		logError("node connection failed. type mismatch");
	}
#if EXTENDED_INPUTS
	else if (dst.type == kVfxPlugType_Float)
	{
		VfxFloatArray::Elem elem;
		elem.value = (float*)dst.mem;
		floatArray.elems.push_back(elem);
		
		dst.isReferencedByLink = true;
	}
#endif
	else
	{
		mem = dst.mem;
		
		dst.isReferencedByLink = true;
	}
}

void VfxPlug::connectToImmediate(void * dstMem, const VfxPlugType dstType)
{
	if (dstType != type)
	{
		logError("node connection failed. type mismatch");
	}
	else
	{
		immediateMem = dstMem;
	
		if (mem == nullptr)
		{
			mem = dstMem;
		}
	}
}

void VfxPlug::setMap(const void * dst, const float inMin, const float inMax, const float outMin, const float outMax)
{
#if EXTENDED_INPUTS
	logDebug("setMap: %.2f, %.2f -> %.2f, %.2f", inMin, inMax, outMin, outMax);
	
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

void VfxPlug::disconnect()
{
	mem = immediateMem;
}

void VfxPlug::disconnect(const void * dstMem)
{
	Assert(dstMem != nullptr && dstMem != immediateMem);
	
	if (type == kVfxPlugType_Float)
	{
		bool removed = false;
		
		for (auto elemItr = floatArray.elems.begin(); elemItr != floatArray.elems.end(); )
		{
			if (elemItr->value == dstMem)
			{
				elemItr = floatArray.elems.erase(elemItr);
				removed = true;
				break;
			}
			
			++elemItr;
		}
		
		Assert(removed);
		
		Assert(mem == immediateMem);
	}
	else
	{
		Assert(mem == dstMem);
		
		disconnect();
	}
}

bool VfxPlug::isConnected() const
{
	if (mem != nullptr)
		return true;
	
#if EXTENDED_INPUTS
	if (floatArray.elems.empty() == false)
		return true;
#endif

	Assert(immediateMem == nullptr); // mem should be equal to immediateMem when we get here
	
	return false;
}

bool VfxPlug::isReferenced() const
{
	if (isReferencedByLink)
		return true;
	
	if (referencedByRealTimeConnectionTick != -1)
	{
		if (referencedByRealTimeConnectionTick >= g_currentVfxGraph->currentTickTraversalId - 60)
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
	
	for (int i = 0; i < image.numChannels; ++i)
	{
		const VfxImageCpu::Channel & c = image.channel[i];
		add("[%d] pitch: %06d, data: %p", i, c.pitch, c.data);
	}
	
	const int numBytes = image.getMemoryUsage();
	add("MEMORY: %.2f Kb", numBytes / 1024.0);
}

void VfxNodeDescription::add(const VfxChannel & channel)
{
	if (channel.sy > 1)
		add("size: %d x %d", channel.sx, channel.sy);
	else
		add("size: %d", channel.size);
	add("MEMORY: %.2f Kb", channel.size * sizeof(float) / 1024.0);
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
	: id(kGraphNodeIdInvalid)
	, inputs()
	, outputs()
	, predeps()
	, triggerTargets()
	, flags(0)
	, lastTickTraversalId(-1)
	, lastDrawTraversalId(-1)
	, editorIsTriggered(false)
	, editorIssue()
	, isPassthrough(false)
#if ENABLE_VFXGRAPH_CPU_TIMING
	, tickTimeAvg(0)
	, drawTimeAvg(0)
#endif
#if ENABLE_VFXGRAPH_GPU_TIMING
	, gpuTimeAvg(0)
	, gpuTimer()
#endif
{
}

void VfxNodeBase::traverseTick(const int traversalId, const float dt)
{
	Assert(lastTickTraversalId != traversalId);
	lastTickTraversalId = traversalId;
	
	//
	
	if (flags & kFlag_CustomTraverseTick)
	{
		customTraverseTick(traversalId, dt);
	}
	else
	{
		for (auto predep : predeps)
		{
			if (predep->lastTickTraversalId != traversalId)
				predep->traverseTick(traversalId, dt);
		}
	}
	
	//
	
	for (int i = 0; i < inputs.size(); ++i)
	{
		if (inputs[i].isTriggered)
		{
			inputs[i].isTriggered = false;
			
			handleTrigger(i);
		}
	}
	
	//
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	const uint64_t t1 = g_TimerRT.TimeUS_get();
#endif
	
	tick(dt);
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	const uint64_t t2 = g_TimerRT.TimeUS_get();
#endif
	
	//
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	tickTimeAvg = (tickTimeAvg * 99 + (t2 - t1)) / 100;
#endif
}

void VfxNodeBase::traverseDraw(const int traversalId)
{
	Assert(lastDrawTraversalId != traversalId);
	lastDrawTraversalId = traversalId;
	
	//
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	uint64_t t = 0;
	
	t -= g_TimerRT.TimeUS_get();
#endif
	
	beforeDraw();
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	t += g_TimerRT.TimeUS_get();
#endif
	
	//
	
	if (flags & kFlag_CustomTraverseDraw)
	{
		customTraverseDraw(traversalId);
	}
	else
	{
		// first traverse the draw inputs
		
		for (auto & input : inputs)
		{
			if (input.type == kVfxPlugType_Draw && input.isConnected())
			{
				VfxNodeBase * vfxNode = reinterpret_cast<VfxNodeBase*>(input.mem);
				
				if (vfxNode->lastDrawTraversalId != traversalId)
				{
					vfxNode->traverseDraw(traversalId);
				}
			}
		}
		
		// some nodes output an image during draw so we need to traverse the other nodes too
		// but since we traverse each node only once we need to traverse the draw node first
		// as drawing works as an 'immediate mode' operation, whereas all the other outputs
		// after traversal will point to a cached value calculated during traversal
		
		for (auto predep : predeps)
		{
			if (predep->lastDrawTraversalId != traversalId)
				predep->traverseDraw(traversalId);
		}
	}
	
	//
	
#if ENABLE_VFXGRAPH_GPU_TIMING
	gpuTimer.begin();
#endif
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	t -= g_TimerRT.TimeUS_get();
#endif
	
	draw();
	
#if ENABLE_VFXGRAPH_GPU_TIMING
	gpuTimer.end();
#endif
	
	//
	
	afterDraw();
	
	//
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	t += g_TimerRT.TimeUS_get();
	
	drawTimeAvg = (drawTimeAvg * 95 + t * 5) / 100;
#endif
	
	//

#if ENABLE_VFXGRAPH_GPU_TIMING
	gpuTimer.poll();
	
	gpuTimeAvg = (gpuTimeAvg * 95 + gpuTimer.elapsed * 5) / 100;
#endif
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
			outputSocket.editorIsTriggered = true;
			
			// iterate the list of outgoing connections, call queueTrigger on nodes with correct outputSocketIndex
			
			for (auto & triggerTarget : triggerTargets)
			{
				if (triggerTarget.dstSocketIndex == outputSocketIndex)
				{
					triggerTarget.srcNode->editorIsTriggered = true;
					
					triggerTarget.srcNode->queueTrigger(triggerTarget.srcSocketIndex);
				}
			}
		}
	}
}

void VfxNodeBase::reconnectDynamicInputs(const int dstNodeId)
{
	const int numStaticInputs = inputs.size() - dynamicInputs.size();
	
	for (int i = numStaticInputs; i < inputs.size(); ++i)
	{
		// note : we can safely reset vfx plugs like this. predeps only depend on the links and dynamic links established, not on actually hooking up memory. memory for literals has already been added to the free list, and we don't need to bother with memory allocations here
		
		const VfxPlugType type = inputs[i].type;
		
		inputs[i] = VfxPlug();
		inputs[i].type = type;
	}
	
	// hook up with the dynamic input socket values
	
	for (auto & inputSocketValue : g_currentVfxGraph->dynamicData->inputSocketValues)
	{
		VfxPlug * input = nullptr;
	
		for (int i = 0; i < dynamicInputs.size(); ++i)
		{
			if (inputSocketValue.socketName == dynamicInputs[i].name)
				input = &inputs[numStaticInputs + i];
		}
		
		if (input != nullptr)
		{
			Assert(input->isConnected() == false);
			
			g_currentVfxGraph->connectToInputLiteral(*input, inputSocketValue.value);
		}
	}
	
	// hook up the dynamic links
	
	for (auto & link : g_currentVfxGraph->dynamicData->links)
	{
		if (link.srcNodeId != id)
			continue;
		if (dstNodeId != -1 && link.dstNodeId != dstNodeId)
			continue;
		
		if (link.srcSocketIndex == -1)
		{
			VfxPlug * input = nullptr;
			
			for (int i = 0; i < dynamicInputs.size(); ++i)
			{
				if (link.srcSocketName == dynamicInputs[i].name)
					input = &inputs[numStaticInputs + i];
			}
			
			if (input == nullptr)
			{
				if (link.srcSocketIndex != -1)
				{
					logError("failed to find input node socket. nodeId=%d, socketName=%s", link.srcNodeId, link.srcSocketName.c_str());
				}
				
				continue;
			}
			
			auto dstNodeItr = g_currentVfxGraph->nodes.find(link.dstNodeId);
			
			if (dstNodeItr == g_currentVfxGraph->nodes.end())
			{
				logError("failed to find output node. nodeId=%d", link.dstNodeId);
				continue;
			}
			
			auto dstNode = dstNodeItr->second;
			
			VfxPlug * output = nullptr;
			
			if (link.dstSocketIndex == -1)
			{
				const int numStaticOutputs = dstNode->outputs.size() - dstNode->dynamicOutputs.size();
				
				for (int i = 0; i < dstNode->dynamicOutputs.size(); ++i)
				{
					if (link.dstSocketName == dstNode->dynamicOutputs[i].name)
						output = &dstNode->outputs[numStaticOutputs + i];
				}
			}
			else
			{
				output = dstNode->tryGetOutput(link.dstSocketIndex);
			}
			
			if (output == nullptr)
			{
				if (link.dstSocketIndex != -1)
				{
					logError("failed to find output node socket. nodeId=%d, socketIndex=%d, socketName=%s", link.dstNodeId, link.dstSocketIndex, link.dstSocketName.c_str());
				}
				
				continue;
			}
			
			connectVfxSockets(this, link.srcSocketIndex, input, dstNode, link.dstSocketIndex, output, link.params, false);
			
			// make sure the output is up to date, as it may have been skipped during traversal as it was possibly ot connected
			
			if (dstNode->lastTickTraversalId != g_currentVfxGraph->currentTickTraversalId)
			{
				dstNode->traverseTick(g_currentVfxGraph->currentTickTraversalId, 0.f);
			}
		}
	}
}

void VfxNodeBase::setDynamicInputs(const DynamicInput * newInputs, const int numInputs)
{
	const int numStaticInputs = inputs.size() - dynamicInputs.size();
	
	// copy the list of dynamic inputs. we will need the definitions to reconnect inputs at a future time
	
	dynamicInputs.resize(numInputs);
	
	for (int i = 0; i < numInputs; ++i)
	{
		dynamicInputs[i] = newInputs[i];
	}
	
	// allocate plugs for the dynamic inputs
	
	inputs.resize(numStaticInputs + numInputs);
	
	for (int i = 0; i < numInputs; ++i)
	{
		inputs[numStaticInputs + i].type = dynamicInputs[i].type;
	}
	
	// re-establish socket connections
	
	reconnectDynamicInputs();
}

void VfxNodeBase::setDynamicOutputs(const DynamicOutput * newOutputs, const int numOutputs)
{
	const int numStaticOutputs = outputs.size() - dynamicOutputs.size();
	
	// remove connections to src nodes
	
	for (auto & link : g_currentVfxGraph->dynamicData->links)
	{
		if (link.dstNodeId != id)
			continue;
		
		if (link.dstSocketIndex != -1)
			continue;
		
		VfxPlug * dstSocket = nullptr;
		
		for (int i = 0; i < dynamicOutputs.size(); ++i)
			if (link.dstSocketName == dynamicOutputs[i].name)
				dstSocket = &outputs[i + numStaticOutputs];
		
		if (dstSocket == nullptr)
			continue; // can happen when the output for the link doesn't exist (yet)
		
		auto srcNodeItr = g_currentVfxGraph->nodes.find(link.srcNodeId);
		
		if (srcNodeItr == g_currentVfxGraph->nodes.end())
		{
			logError("failed to find input node");
			continue;
		}
		
		auto srcNode = srcNodeItr->second;
		
		auto srcSocket = srcNode->tryGetInput(link.srcSocketIndex);
		
		if (srcSocket == nullptr)
			continue; // can happen when the input for the link doesn't exist (yet)
		
		srcSocket->disconnect(dstSocket->mem);
	}
	
	//
	
	dynamicOutputs.resize(numOutputs);
	
	for (int i = 0; i < numOutputs; ++i)
	{
		dynamicOutputs[i] = newOutputs[i];
	}
	
	// allocate plugs for the dynamic outputs
	
	outputs.resize(numStaticOutputs + numOutputs);
	
	for (int i = 0; i < numOutputs; ++i)
	{
		outputs[numStaticOutputs + i] = VfxPlug();
		outputs[numStaticOutputs + i].type = dynamicOutputs[i].type;
		outputs[numStaticOutputs + i].mem = dynamicOutputs[i].mem;
	}
	
	// reconnect inputs to our outputs
	
	for (auto & link : g_currentVfxGraph->dynamicData->links)
	{
		if (link.dstNodeId != id)
			continue;
		
		if (link.dstSocketIndex != -1)
			continue;
		
		VfxPlug * dstSocket = nullptr;
		
		for (int i = 0; i < dynamicOutputs.size(); ++i)
			if (link.dstSocketName == dynamicOutputs[i].name)
				dstSocket = &outputs[i + numStaticOutputs];
		
		if (dstSocket == nullptr)
			continue; // can happen when the output for the link doesn't exist (yet)
		
		auto srcNodeItr = g_currentVfxGraph->nodes.find(link.srcNodeId);
		
		if (srcNodeItr == g_currentVfxGraph->nodes.end())
		{
			logError("failed to find input node");
			continue;
		}
		
		auto srcNode = srcNodeItr->second;
		
		auto srcSocket = srcNode->tryGetInput(link.srcSocketIndex);
		
		if (srcSocket == nullptr)
			continue; // can happen when the input for the link doesn't exist (yet)
		
		connectVfxSockets(srcNode, link.srcSocketIndex, srcSocket, this, link.dstSocketIndex, dstSocket, link.params, false);
	}
}

// todo : move elsewhere ?

#include "graph.h"
#include "tinyxml2.h"

void createVfxValueTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	auto addSimple = [&](const char * typeName, const char * editor, const char * visualizer = nullptr) -> GraphEdit_ValueTypeDefinition &
	{
		auto & td = typeDefinitionLibrary.valueTypeDefinitions[typeName];
		
		td.typeName = typeName;
		td.editor = editor;
		if (visualizer)
			td.visualizer = visualizer;
		
		return td;
	};
	
	addSimple("bool", "checkbox");
	addSimple("int", "textbox_int", "valueplotter");
	addSimple("float", "textbox_float", "valueplotter").multipleInputs = true;
	addSimple("string", "textbox");
	addSimple("color", "colorpicker");
	addSimple("trigger", "button").multipleInputs = true;
	addSimple("image", "none", "opengl-texture");
	addSimple("image_cpu", "none");
	addSimple("channel", "textbox", "channels");
	addSimple("draw", "none");
	
	{
		std::pair<std::string, std::string> key("float", "float");
		
		GraphEdit_LinkTypeDefinition & floatLink = typeDefinitionLibrary.linkTypeDefinitions[key];
		
		floatLink.srcTypeName = "float";
		floatLink.dstTypeName = "float";
		
		floatLink.params.resize(4);
		
		floatLink.params[0].typeName = "float";
		floatLink.params[0].name = "in.min";
		floatLink.params[0].defaultValue = "0";
		
		floatLink.params[1].typeName = "float";
		floatLink.params[1].name = "in.max";
		floatLink.params[1].defaultValue = "1";
		
		floatLink.params[2].typeName = "float";
		floatLink.params[2].name = "out.min";
		floatLink.params[2].defaultValue = "0";
		
		floatLink.params[3].typeName = "float";
		floatLink.params[3].name = "out.max";
		floatLink.params[3].defaultValue = "1";
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
	const int enumValue = value == -1 ? nextValue : value;
	
	Elem e;
	e.name = name;
	e.valueText = String::FormatC("%d", enumValue);
	
	elems.push_back(e);
	
	nextValue = enumValue + 1;
}

void VfxEnumTypeRegistration::elem(const char * name, const char * valueText)
{
	Elem e;
	e.name = name;
	e.valueText = valueText;
	
	elems.push_back(e);
	
	nextValue = nextValue + 1;
}

#include "graph.h"

extern void getFsfxShaderList(std::vector<std::string> & shaderList);

void createVfxEnumTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxEnumTypeRegistration * registrationList)
{
	for (const VfxEnumTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions[registration->enumName];
		
		enumDefinition.enumName = registration->enumName;
		
		for (auto & src : registration->elems)
		{
			GraphEdit_EnumDefinition::Elem dst;
			
			dst.name = src.name;
			dst.valueText = src.valueText;
			
			enumDefinition.enumElems.push_back(dst);
		}
	}
	
	{
		auto & enumDefinition = typeDefinitionLibrary.enumDefinitions["fsfxShader"];
		
		std::vector<std::string> shaderList;
		getFsfxShaderList(shaderList);
		
		for (auto & shader : shaderList)
		{
			GraphEdit_EnumDefinition::Elem elem;
			elem.name = shader;
			elem.valueText = shader;
			
			enumDefinition.enumElems.push_back(elem);
		}
	}
}

//

VfxNodeTypeRegistration * g_vfxNodeTypeRegistrationList = nullptr;

VfxNodeTypeRegistration::VfxNodeTypeRegistration()
	: next(nullptr)
	, create(nullptr)
	, createResourceEditor(nullptr)
	, typeName()
	, displayName()
	, resourceTypeName()
	, inputs()
	, outputs()
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

void createVfxNodeTypeDefinitions(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxNodeTypeRegistration * registrationList)
{
	for (const VfxNodeTypeRegistration * registration = registrationList; registration != nullptr; registration = registration->next)
	{
		GraphEdit_TypeDefinition typeDefinition;
		
		typeDefinition.typeName = registration->typeName;
		
		if (registration->displayName.empty())
		{
			auto pos = registration->typeName.rfind('.');
			
			if (pos == std::string::npos)
				typeDefinition.displayName = registration->typeName;
			else
				typeDefinition.displayName = registration->typeName.substr(pos + 1);
		}
		else
		{
			typeDefinition.displayName = registration->displayName;
		}
		
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
			inputSocket.hasDefaultValue = true;
			inputSocket.displayName = src.displayName;
			
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
			outputSocket.displayName = src.displayName;
			
			typeDefinition.outputSockets.push_back(outputSocket);
		}
		
		typeDefinition.resourceEditor.create = registration->createResourceEditor;
		
		typeDefinition.createUi();
		
		typeDefinitionLibrary.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

//

void createVfxTypeDefinitionLibrary(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const VfxEnumTypeRegistration * enumRegistrationList, const VfxNodeTypeRegistration * nodeRegistrationList)
{
	createVfxValueTypeDefinitions(typeDefinitionLibrary);
	createVfxEnumTypeDefinitions(typeDefinitionLibrary, enumRegistrationList);
	createVfxNodeTypeDefinitions(typeDefinitionLibrary, nodeRegistrationList);
}
