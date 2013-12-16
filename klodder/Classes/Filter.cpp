#include "Bitmap.h"
#include "Calc.h"
#include "Debugging.h"
#include "Filter.h"
#include "MacImage.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "Util_Mem.h"
#include "ximage.h"

#define IO_MAX_SIZE 512

Filter::Filter()
{
	mData = 0;
	mSx = 0;
	mSy = 0;
}

Filter::~Filter()
{
	Size_set(0, 0);
}

void Filter::Load(Stream* stream)
{
	StreamReader reader(stream, false);

	uint32_t version = reader.ReadUInt32();

	switch (version)
	{
	case 1:
		{
			uint32_t sx = reader.ReadUInt32();
			uint32_t sy = reader.ReadUInt32();

			if (sx > IO_MAX_SIZE || sy > IO_MAX_SIZE)
				throw ExceptionVA("brush size too large: %dx%d", (int)sx, (int)sy);

			Size_set(sx, sy);

			stream->Read(mData, sx * sy * sizeof(float));

			break;
		}

	default:
		throw ExceptionVA("unknown version: %d", (int)version);
	}
}

void Filter::Save(Stream* stream) const
{
	StreamWriter writer(stream, false);

	writer.WriteUInt32(1); // version
	writer.WriteUInt32(mSx);
	writer.WriteUInt32(mSy);
	stream->Write(mData, mSx * mSy * sizeof(float));
}


void Filter::Multiply(float value)
{
	int area = mSx * mSy;
	
	for (int i = 0; i < area; ++i)
		mData[i] *= value;
}

void Filter::MakeSoft(int diameter, float hardness)
{
//	LOG_DBG("createFilter: %d, %f", diameter, 3);
	
	Size_set(diameter, diameter);

	//

	const float midX = (diameter - 1.0f) / 2.0f;
	const float midY = (diameter - 1.0f) / 2.0f;

	for (int y = 0; y < diameter; ++y)
	{
		float* line = Line_get(y);
		
		for (int x = 0; x < diameter; ++x)
		{
			//const int deltaX = x - midX;
			//const int deltaY = y - midY;
			const float deltaX = x - midX;
			const float deltaY = y - midY;

			const float delta = sqrtf((float)(deltaX * deltaX + deltaY * deltaY)) / (diameter / 2.0f);

			if (delta >= 1.0f)
				line[x] = 0.0f;
			else
			{
				float value = 1.0f - (delta - hardness) / (1.0f - hardness + 0.001f);
				
				if (value < 0.0f)
					value = 0.0f;
				else if (value > 1.0f)
					value = 1.0f;
				
				line[x] = value;
			}
		}
	}

#if 0
	for (int y = - diameter / 6; y <= +diameter / 6; ++y)
		for (int x = 0; x < diameter; ++x)
			mFilter.Line_get((diameter - 1) / 2 + y)[x] = 1.0f;
#endif
}

#ifndef FILTER_STANDALONE

void Filter::ToMacImage(MacImage& image, const Rgba& color) const
{
	Assert(image.Sx_get() == mSx);
	Assert(image.Sy_get() == mSy);
	
	MacRgba macColor;
	macColor.rgba[0] = (uint8_t)(color.rgb[0] * 255.0f);
	macColor.rgba[1] = (uint8_t)(color.rgb[1] * 255.0f);
	macColor.rgba[2] = (uint8_t)(color.rgb[2] * 255.0f);
	macColor.rgba[3] = 255;
	
	image.Clear(macColor);
	
	for (int y = 0; y < mSy; ++y)
	{
		const float* srcLine = Line_get(y);
		MacRgba* dstLine = image.Line_get(y);
		
		for (int x = 0; x < mSx; ++x)
		{
			dstLine->rgba[3] = (uint8_t)(srcLine[x] * 255.0f);
			
#if 0
			dstLine[x].rgba[0] = dstLine[x].rgba[3];
			dstLine[x].rgba[1] = dstLine[x].rgba[3];
			dstLine[x].rgba[2] = dstLine[x].rgba[3];
#endif
			
			dstLine++;
		}
	}
}

#endif

void Filter::Blit(Filter* filter) const
{
	Assert(filter->Sx_get() == mSx);
	Assert(filter->Sy_get() == mSy);

	for (int y = 0; y < mSy; ++y)
	{
		const float * __restrict srcLine = Line_get(y);
		float * __restrict dstLine = filter->Line_get(y);

		Mem::Copy(srcLine, dstLine, mSx * sizeof(float));

		//for (int x = 0; x < mSx; ++x)
		//	dstLine[x] = srcLine[x];
	}
}

void Filter::Blit_Resampled(Filter* filter) const
{
	CxImage image(mSx, mSy, 24);

#if defined(__ARM_NEON__) && 0
	// 0.123
	uint32_t sx = mSx;
	uint32_t sy = mSy;
	uint32_t n = sx >> 3;
	float32x4_t vscale = vdupq_n_f32(255.0f);
	for (uint32_t y = 0; y < sy; ++y)
	{
		const float * __restrict srcLine = Line_get(y);
		BYTE * __restrict dstLine = image.GetBits(y);
		
		for (uint32_t x = n; x != 0; --x)
		{
			float32x4_t fLo = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine    )), vscale);
			float32x4_t fHi = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine + 4)), vscale);
			uint32x4_t vLo = vcvtq_u32_f32(fLo);
			uint32x4_t vHi = vcvtq_u32_f32(fHi);
			uint16x4_t vLo16 = vmovn_u32(vLo);
			uint16x4_t vHi16 = vmovn_u32(vHi);
			uint16x8_t vLoHi16 = vcombine_u16(vLo16, vHi16);
			uint8x8_t vLoHi8 = vmovn_u16(vLoHi16);
			uint8_t temp[8];
			vst1_u8(temp, vLoHi8);
			dstLine[ 0] = temp[0];
			dstLine[ 3] = temp[1];
			dstLine[ 6] = temp[2];
			dstLine[ 9] = temp[3];
			dstLine[12] = temp[4];
			dstLine[15] = temp[5];
			dstLine[18] = temp[6];
			dstLine[21] = temp[7];
			srcLine += 8;
			dstLine += 24;
		}
		for (uint32_t x = n << 3; x < sx; ++x)
		{
			dstLine[0] = (BYTE)(srcLine[x] * 255.0f);
			dstLine += 3;
		}
	}
#else
	for (int y = 0; y < mSy; ++y)
	{
		const float * __restrict srcLine = Line_get(y);
		BYTE * __restrict dstLine = image.GetBits(y);

		for (int x = 0; x < mSx; ++x)
		{
			dstLine[0] = (BYTE)(srcLine[x] * 255.0f);
			dstLine += 3;
		}
	}
#endif
	
	image.Resample2(filter->Sx_get(), filter->Sy_get());

	for (int y = 0; y < filter->Sy_get(); ++y)
	{
		const BYTE* srcLine = image.GetBits(y);
		float* dstLine = filter->Line_get(y);

		const float scale = 1.0f / 255.0f;

		for (int x = 0; x < filter->Sx_get(); ++x)
		{
			dstLine[x] = srcLine[0] * scale;
			srcLine += 3;
		}
	}
}

//#define floorf(v) (int)v;
#define floorf2i(v) (int)floorf(v)

int Filter::SampleAA(float x, float y, float* __restrict out_value, float* __restrict out_w) const __restrict
{
	const float ix1f = floorf(x);
	const float iy1f = floorf(y);
	const int ix1 = (int)ix1f;
	const int iy1 = (int)iy1f;
	const int ix2 = ix1 + 1;
	const int iy2 = iy1 + 1;
	
	const int correct = ix1 < 0 || iy1 < 0 || ix2 >= mSx || iy2 >= mSy;

	if (!correct)
	{
		const float* base = Line_get(iy1) + ix1;
		
		out_value[0] = *(base);
		out_value[1] = *(base + 1);
		out_value[2] = *(base + mSy);
		out_value[3] = *(base + mSy + 1);
	}
	else
	{
		out_value[0] = Sample_Border(ix1, iy1);
		out_value[1] = Sample_Border(ix2, iy1);
		out_value[2] = Sample_Border(ix1, iy2);
		out_value[3] = Sample_Border(ix2, iy2);
	}

	const float wx2 = x - ix1f;
	const float wy2 = y - iy1f;
	const float wx1 = 1.0f - wx2;
	const float wy1 = 1.0f - wy2;

	out_w[0] = wx1 * wy1;
	out_w[1] = wx2 * wy1;
	out_w[2] = wx1 * wy2;
	out_w[3] = wx2 * wy2;

	Assert(out_w[0] >= 0.0f && out_w[0] <= 1.0f);
	Assert(out_w[1] >= 0.0f && out_w[1] <= 1.0f);
	Assert(out_w[2] >= 0.0f && out_w[2] <= 1.0f);
	Assert(out_w[3] >= 0.0f && out_w[3] <= 1.0f);

	return correct;
}

/*float Filter::SampleAA(float x, float y) const
{
	float value[4];
	float w[4];
	
	SampleAA(x, y, value, w);

	return
		value[0] * w[0] +
		value[1] * w[1] +
		value[2] * w[2] +
		value[3] * w[3];
}*/

float Filter::SampleAA(float x, float y) const
{
	const float ix1f = floorf(x);
	const float iy1f = floorf(y);
	const int ix1 = (int)ix1f;
	const int iy1 = (int)iy1f;
	const int ix2 = ix1 + 1;
	const int iy2 = iy1 + 1;
	
	const float wx2 = x - ix1f;
	const float wy2 = y - iy1f;
	const float wx1 = 1.0f - wx2;
	const float wy1 = 1.0f - wy2;
	
	float out_w[4];
	
	out_w[0] = wx1 * wy1;
	out_w[1] = wx2 * wy1;
	out_w[2] = wx1 * wy2;
	out_w[3] = wx2 * wy2;
	
	const int correct = ix1 < 0 || iy1 < 0 || ix2 >= mSx || iy2 >= mSy;

	if (!correct)
	{
		const float* base = Line_get(iy1) + ix1;
		
		return
			out_w[0] * *(base) +
			out_w[1] * *(base + 1) +
			out_w[2] * *(base + mSx) +
			out_w[3] * *(base + mSx + 1);
	}
	else
	{
		return
			out_w[0] * Sample_Border(ix1, iy1) +
			out_w[1] * Sample_Border(ix2, iy1) +
			out_w[2] * Sample_Border(ix1, iy2) +
			out_w[3] * Sample_Border(ix2, iy2);
	}
}

void Filter::Size_set(int sx, int sy)
{
	if (sx == mSx && sy == mSy)
	{
		if (sx * sy > 0)
		{
			ClearMemory(mData, sx * sy * sizeof(float));
		}
	}
	else
	{
		delete[] mData;
		mData = 0;
		mSx = 0;
		mSy = 0;
		
		if (sx * sy > 0)
		{
			mData = new float[sx * sy];
			mSx = sx;
			mSy = sy;
			ClearMemory(mData, sx * sy * sizeof(float));
		}
	}
}

AreaI Filter::Area_get() const
{
	return AreaI(Vec2I(0, 0), Vec2I(mSx - 1, mSy - 1));
}

Hash Filter::Hash_get() const
{
	return HashFunc::Hash_FNV1(mData, mSx * mSy * sizeof(float));
}
