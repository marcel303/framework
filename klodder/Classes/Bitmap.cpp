#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Bitmap.h"
#include "Calc.h"
#include "MacImage.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "Util_Mem.h"

#define IO_MAX_SIZE 1024

Bitmap::Bitmap()
{
	mData = nullptr;
	mSx = 0;
	mSy = 0;
}

Bitmap::~Bitmap()
{
	Size_set(0, 0, false);
}

void Bitmap::Load(Stream * stream)
{
	StreamReader reader(stream, false);

	const uint32_t version = reader.ReadUInt32();

	switch (version)
	{
	case 1:
		{
			const uint32_t sx = reader.ReadUInt32();
			const uint32_t sy = reader.ReadUInt32();

			if (sx > IO_MAX_SIZE || sy > IO_MAX_SIZE)
				throw ExceptionVA("bitmap size too large: %dx%d", (int)sx, (int)sy);

			Size_set(sx, sy, false);

			stream->Read(mData, sx * sy * sizeof(Rgba));

			break;
		}

	default:
		throw ExceptionVA("unknown version: %d", (int)version);
	}
}

void Bitmap::Save(Stream * stream) const
{
	StreamWriter writer(stream, false);

	writer.WriteUInt32(1); // version
	writer.WriteUInt32(mSx);
	writer.WriteUInt32(mSy);
	stream->Write(mData, mSx * mSy * sizeof(Rgba));
}

void Bitmap::ToMacImage(MacImage & image) const
{
	image.Size_set(mSx, mSy, false);

#ifdef __ARM_NEON__
	float32x4_t scale = vdupq_n_f32(255.0f);
#endif
	
	for (int y = 0; y < mSy; ++y)
	{
		const    Rgba * src = Line_get(y);
		      MacRgba * dst = image.Line_get(y);
		
		for (int x = 0; x < mSx; ++x)
		{
#ifdef __ARM_NEON__
			float32x4_t v = vld1q_f32(reinterpret_cast<const float32_t*>(src[x].rgb));
			v = vmulq_f32(v, scale);
			uint32x4_t vi = vcvtq_u32_f32(v);
			dst[x].rgba[0] = vgetq_lane_s32(vi, 0);
			dst[x].rgba[1] = vgetq_lane_s32(vi, 1);
			dst[x].rgba[2] = vgetq_lane_s32(vi, 2);
			dst[x].rgba[3] = vgetq_lane_s32(vi, 3);
#else
			dst[x].rgba[0] = (uint8_t)(src[x].rgb[0] * 255.0f);
			dst[x].rgba[1] = (uint8_t)(src[x].rgb[1] * 255.0f);
			dst[x].rgba[2] = (uint8_t)(src[x].rgb[2] * 255.0f);
			dst[x].rgba[3] = (uint8_t)(src[x].rgb[3] * 255.0f);
#endif
		}
	}
}

void Bitmap::ToMacImage_GrayScale(MacImage & image) const
{
	image.Size_set(mSx, mSy, false);

	const float scale = 1.0f / 3.0f;

	for (int y = 0; y < mSy; ++y)
	{
		const    Rgba * src = Line_get(y);
		      MacRgba * dst = image.Line_get(y);
		
		for (int x = 0; x < mSx; ++x)
		{
			const float v = src[x].rgb[0] + src[x].rgb[1] + src[x].rgb[2];

			const uint8_t c = (uint8_t)(v * scale * 255.0f);

			dst[x].rgba[0] = c;
			dst[x].rgba[1] = c;
			dst[x].rgba[2] = c;
			dst[x].rgba[3] = 255;
		}
	}
}

void Bitmap::ToMacImage_GrayAlpha(MacImage & image) const
{
	image.Size_set(mSx, mSy, false);

	const float scale = 1.0f / 3.0f * 255.0f;

	for (int y = 0; y < mSy; ++y)
	{
		const    Rgba * src = Line_get(y);
		      MacRgba * dst = image.Line_get(y);
		
		for (int x = 0; x < mSx; ++x)
		{
			const float v = src[x].rgb[0] + src[x].rgb[1] + src[x].rgb[2];

			const uint8_t c = (uint8_t)(v * scale);

			dst[x].rgba[0] = 255;
			dst[x].rgba[1] = 255;
			dst[x].rgba[2] = 255;
			dst[x].rgba[3] = c;
		}
	}
}

void Bitmap::ExtractTo(Bitmap * bmp, const int _x, const int _y, const int sx, const int sy) const
{
	Assert(_x + sx <= mSx);
	Assert(_y + sy <= mSy);
	Assert(sx <= bmp->Sx_get());
	Assert(sy <= bmp->Sy_get());

	for (int y = 0; y < sy; ++y)
	{
		const Rgba * __restrict src = Line_get(y + _y) + _x;
		      Rgba * __restrict dst = bmp->Line_get(y);

		Mem::Copy(src, dst, sx * sizeof(Rgba));
	}
}

void Bitmap::Blit(Bitmap * bmp, const int spx, const int spy, const int sx, const int sy, const int dpx, const int dpy) const
{
	Assert(spx + sx <= mSx);
	Assert(spy + sy <= mSy);
	Assert(dpx + sx <= bmp->Sx_get());
	Assert(dpy + sy <= bmp->Sy_get());

	for (int y = 0; y < sy; ++y)
	{
		const Rgba * __restrict src = Line_get(spy + y) + spx;
		      Rgba * __restrict dst = bmp->Line_get(dpy + y) + dpx;
		
		Mem::Copy(src, dst, sx * sizeof(Rgba));
	}
}

void Bitmap::Blit(Bitmap * bmp) const
{
	Blit(bmp, 0, 0, mSx, mSy, 0, 0);
}

int Bitmap::SampleAA(const float x, const float y, const Rgba *__restrict * __restrict out_values, float * __restrict out_w) const
{
	const int ix1 = (int)floorf(x);
	const int iy1 = (int)floorf(y);
	const int ix2 = ix1 + 1;
	const int iy2 = iy1 + 1;

	const int correct = ix1 < 0 || iy1 < 0 || ix2 >= mSx || iy2 >= mSy;

	if (!correct)
	{
		out_values[0] = Sample_Ref(ix1, iy1);
		out_values[1] = Sample_Ref(ix2, iy1);
		out_values[2] = Sample_Ref(ix1, iy2);
		out_values[3] = Sample_Ref(ix2, iy2);
	}
	else
	{
		out_values[0] = Sample_Clamped_Ref(ix1, iy1);
		out_values[1] = Sample_Clamped_Ref(ix2, iy1);
		out_values[2] = Sample_Clamped_Ref(ix1, iy2);
		out_values[3] = Sample_Clamped_Ref(ix2, iy2);
	}

	const float wx2 = x - (float)ix1;
	const float wy2 = y - (float)iy1;
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

/*Rgba Bitmap::SampleAA(float x, float y) const
{
	const Rgba* values[4];
	float w[4];

	SampleAA(x, y, values, w);

	Rgba result;

	for (int i = 0; i < 4; ++i)
	{
		result.rgb[i] =
			values[0]->rgb[i] * w[0] +
			values[1]->rgb[i] * w[1] +
			values[2]->rgb[i] * w[2] +
			values[3]->rgb[i] * w[3];
	}

	return result;
}*/

#ifdef __ARM_NEON__

void Bitmap::SampleAA(const float x, const float y, Rgba & out_value) const
{
	float32_t p[2] = { x, y };
	float32x2_t vp = vld1_f32(p);
	return SampleAA(vp, out_value);
}

#else

void Bitmap::SampleAA(const float x, const float y, Rgba & out_value) const
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

	const Rgba * out_values[4];
	
	if (!correct)
	{
		const Rgba * base = Line_get(iy1) + ix1;
		
		out_values[0] = base;
		out_values[1] = base + 1;
		out_values[2] = base + mSx;
		out_values[3] = base + mSx + 1;
	}
	else
	{
		out_values[0] = Sample_Clamped_Ref(ix1, iy1);
		out_values[1] = Sample_Clamped_Ref(ix2, iy1);
		out_values[2] = Sample_Clamped_Ref(ix1, iy2);
		out_values[3] = Sample_Clamped_Ref(ix2, iy2);
	}
	
	for (uint32_t i = 0; i < 4; ++i)
	{
		out_value.rgb[i] =
			out_values[0]->rgb[i] * out_w[0] +
			out_values[1]->rgb[i] * out_w[1] +
			out_values[2]->rgb[i] * out_w[2] +
			out_values[3]->rgb[i] * out_w[3];
	}
}

#endif

void Bitmap::Clear(const Rgba & color)
{
	const int count = mSx * mSy;
	
#if 1
	for (int i = 0; i < count; ++i)
		mData[i] = color;
#else
	Rgba * ptr = mData;

	for (int i = count; i != 0; --i)
		*ptr++ = color;
#endif
}

void Bitmap::Size_set(const int sx, const int sy, const bool clear)
{
	delete [] mData;
	mData = nullptr;
	mSx = 0;
	mSy = 0;
	
	if (sx * sy > 0)
	{
		mData = new Rgba[sx * sy];
		mSx = sx;
		mSy = sy;

		if (clear)
		{
			Clear(Rgba_Make(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

AreaI Bitmap::Area_get() const
{
	return AreaI(Vec2I(0, 0), Vec2I(mSx - 1, mSy - 1));
}
