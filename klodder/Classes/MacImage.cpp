#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Benchmark.h"
#include "BlitTransform.h"
#include "Debugging.h"
#include "Exception.h"
#include "ImageResampling.h"
#include "MacImage.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "Util_Mem.h"

#if USE_CXIMAGE
	#include "ximage.h"
#endif

MacImage::MacImage()
{
	mData = nullptr;
	mSx = 0;
	mSy = 0;
	
#ifdef IPHONEOS
	mImage = nullptr;
#endif
}

MacImage::~MacImage()
{
	Size_set(0, 0, false);
}

void MacImage::Clear(const MacRgba & color)
{
	const int count = mSx * mSy;
	
	for (int i = 0; i < count; ++i)
		mData[i] = color;
}

void MacImage::ExtractTo(MacImage * dst, const int _x, const int _y, const int sx, const int sy) const
{
	Assert(_x + sx <= mSx);
	Assert(_y + sy <= mSy);
	Assert(sx <= dst->Sx_get());
	Assert(sy <= dst->Sy_get());

	for (int y = 0; y < sy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(y + _y) + _x;
		      MacRgba * __restrict dstLine = dst->Line_get(y);

		Mem::Copy(srcLine, dstLine, sx * sizeof(MacRgba));
	}
}

void MacImage::Blit(MacImage * dst) const
{
	Assert(dst->Sx_get() == Sx_get());
	Assert(dst->Sy_get() == Sy_get());

	if (Sx_get() != dst->Sx_get() || Sy_get() != dst->Sy_get())
		throw ExceptionVA("size mismatch");

	const int byteCount = Sx_get() * Sy_get() * sizeof(MacRgba);

	const void * __restrict srcData = mData;
	      void * __restrict dstData = dst->mData;
	
	Mem::Copy(srcData, dstData, byteCount);
}

void MacImage::Blit(MacImage * dst, const int dpx, const int dpy) const
{
	Assert(dpx >= 0);
	Assert(dpy >= 0);
	Assert(dpx + Sx_get() <= dst->mSx);
	Assert(dpy + Sy_get() <= dst->mSy);

	const int byteCount = Sx_get() * sizeof(MacRgba);

	for (int y = 0; y < Sy_get(); ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(y);
		      MacRgba * __restrict dstLine = dst->Line_get(dpy + y) + dpx;

		Mem::Copy(srcLine, dstLine, byteCount);
	}
}

void MacImage::Blit(MacImage * dst, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const
{
	Assert(spx >= 0);
	Assert(spy >= 0);
	Assert(spx + sx <= mSx);
	Assert(spy + sy <= mSy);
	Assert(dpx >= 0);
	Assert(dpy >= 0);
	Assert(dpx + sx <= dst->mSx);
	Assert(dpy + sy <= dst->mSy);
	
	const int byteCount = sx * sizeof(MacRgba);

	for (int y = 0; y < sy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(spy + y) + spx;
		      MacRgba * __restrict dstLine = dst->Line_get(dpy + y) + dpx;

		Mem::Copy(srcLine, dstLine, byteCount);
	}
}

void MacImage::BlitAlpha(MacImage * dst, const int opacity) const
{
	Assert(Sx_get() == dst->Sx_get());
	Assert(Sy_get() == dst->Sy_get());
	
	Benchmark bm("MacImage::BlitAlpha");
	
	for (int y = 0; y < mSy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(y);
		      MacRgba * __restrict dstLine = dst->Line_get(y);
		
#if defined(__ARM_NEON__) && 1
		const uint16x4_t srcOpacity_x4 = vdup_n_u16(opacity);
		const uint16x8_t srcOpacity_x8 = vdupq_n_u16(opacity);
		const uint32_t n = mSx >> 1;
		for (uint32_t x = n; x != 0; --x)
		{
#if 0
			// 0.020 sec @ 1024x768 (iPad2)
			
			// load 2 pixels from src
			const uint8x8_t  srcLoHi_08 = vld1_u8(reinterpret_cast<const uint8_t*>(srcLine));
			const uint16x8_t srcLoHi_16 = vmovl_u8(srcLoHi_08);
			const uint16x4_t srcLo = vget_low_u16(srcLoHi_16);
			const uint16x4_t srcHi = vget_high_u16(srcLoHi_16);
			
			// load 2 pixels from dst
			const uint8x8_t  dstLoHi_08 = vld1_u8(reinterpret_cast<const uint8_t*>(dstLine));
			const uint16x8_t dstLoHi_16 = vmovl_u8(dstLoHi_08);
			
			// dst opacity = 255 - srcAlpha x srcOpacity
			
			const uint16x4_t srcAlphaLo = vdup_lane_u16(srcLo, 3);
			const uint16x4_t srcAlphaHi = vdup_lane_u16(srcHi, 3);
			const uint16x8_t srcAlpha = vcombine_u16(srcAlphaLo, srcAlphaHi);
			
			const uint16x8_t one = vdupq_n_u16(65535);
			const uint16x8_t dstOpacity = vshrq_n_u16(vmlsq_u16(one, srcAlpha, srcOpacity_x8), 8);
			
			// final = dst x dstOpacity +
			//         src x srcOpacity
			
			uint16x8_t src_x_srcOpacity = vmulq_u16(srcLoHi_16, srcOpacity_x8);
			
			uint16x8_t sum = vmlaq_u16(src_x_srcOpacity, /* dst_x_dstOpacity */ dstLoHi_16, dstOpacity);
			
			sum = vshrq_n_u16(sum, 8);
			uint8x8_t sum_8 = vmovn_u16(sum);
			
			// store
			
			vst1_u8(reinterpret_cast<uint8_t*>(dstLine), sum_8);
#else
			// 0.019 sec @ 1024x768 (iPad2)
			
			// load 2 pixels from src
			const uint8x8_t  srcLoHi_08 = vld1_u8(reinterpret_cast<const uint8_t*>(srcLine));
			const uint16x8_t srcLoHi_16 = vmovl_u8(srcLoHi_08);
			const uint16x4_t srcLo = vget_low_u16(srcLoHi_16);
			const uint16x4_t srcHi = vget_high_u16(srcLoHi_16);
			
			// load 2 pixels from dst
			const uint8x8_t dstLoHi_8 = vld1_u8(reinterpret_cast<const uint8_t*>(dstLine));
			const uint16x8_t dstLoHi_16 = vmovl_u8(dstLoHi_8);
			const uint16x4_t dstLo = vget_low_u16(dstLoHi_16);
			const uint16x4_t dstHi = vget_high_u16(dstLoHi_16);

			// dst opacity = 255 - srcAlpha x srcOpacity

			const uint16x4_t srcAlphaLo = vdup_lane_u16(srcLo, 3);
			const uint16x4_t srcAlphaHi = vdup_lane_u16(srcHi, 3);
			
			const uint16x4_t one = vdup_n_u16(65535);
			const uint16x4_t dstOpacityLo = vshr_n_u16(vmls_u16(one, srcAlphaLo, srcOpacity_x4), 8);
			const uint16x4_t dstOpacityHi = vshr_n_u16(vmls_u16(one, srcAlphaHi, srcOpacity_x4), 8);

			// final = dst x dstOpacity +
			//         src x srcOpacity
			
			uint16x4_t srcLo_x_srcOpacity = vmul_u16(srcLo, srcOpacity_x4);
			uint16x4_t srcHi_x_srcOpacity = vmul_u16(srcHi, srcOpacity_x4);
			
			uint16x4_t sumLo = vmla_u16(srcLo_x_srcOpacity, /* dstLo_x_dstOpacityLo */ dstLo, dstOpacityLo);
			uint16x4_t sumHi = vmla_u16(srcHi_x_srcOpacity, /* dstHi_x_dstOpacityHi */ dstHi, dstOpacityHi);
			
			sumLo = vshr_n_u16(sumLo, 8);
			sumHi = vshr_n_u16(sumHi, 8);
			
			uint16x8_t sum_16 = vcombine_u16(sumLo, sumHi);
			uint8x8_t sum_8 = vmovn_u16(sum_16);
			
			// store
			
			vst1_u8(reinterpret_cast<uint8_t*>(dstLine), sum_8);
#endif
			
			srcLine += 2;
			dstLine += 2;
		}
#else
		// 0.029 sec @ 1024x768 (iPad2)
		
		for (int x = 0; x < mSx; ++x)
		{
			const int opacity2 = 255 - ((srcLine->rgba[3] * opacity) >> 8);

			for (int i = 0; i < 4; ++i)
			{
				dstLine->rgba[i] =
					(dstLine->rgba[i] * opacity2 +
					srcLine->rgba[i] * opacity) >> 8;
			}
			
			srcLine++;
			dstLine++;
		}
#endif
	}
}

void MacImage::BlitAlpha(MacImage * dst, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const
{
	Assert(spx >= 0);
	Assert(spy >= 0);
	Assert(spx + sx <= mSx);
	Assert(spy + sy <= mSy);
	Assert(dpx >= 0);
	Assert(dpy >= 0);
	Assert(dpx + sx <= dst->mSx);
	Assert(dpy + sy <= dst->mSy);
	
	for (int y = 0; y < sy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(spy + y) + spx;
		      MacRgba * __restrict dstLine = dst->Line_get(dpy + y) + dpx;
		
		for (int x = 0; x < sx; ++x)
		{
			const int opacity2 = 255 - srcLine->rgba[3];

			for (int i = 0; i < 4; ++i)
			{
				dstLine->rgba[i] =
					((dstLine->rgba[i] * opacity2) >> 8) +
					srcLine->rgba[i];
			}
			
			srcLine++;
			dstLine++;
		}
	}
}

void MacImage::BlitAlpha(MacImage * dst, const int opacity, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const
{
	Assert(spx >= 0);
	Assert(spy >= 0);
	Assert(spx + sx <= mSx);
	Assert(spy + sy <= mSy);
	Assert(dpx >= 0);
	Assert(dpy >= 0);
	Assert(dpx + sx <= dst->mSx);
	Assert(dpy + sy <= dst->mSy);
	
	for (int y = 0; y < sy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(spy + y) + spx;
		      MacRgba * __restrict dstLine = dst->Line_get(dpy + y) + dpx;
		
		for (int x = 0; x < sx; ++x)
		{
			const int opacity2 = 255 - ((srcLine->rgba[3] * opacity) >> 8);

			for (int i = 0; i < 4; ++i)
			{
				dstLine->rgba[i] =
					(dstLine->rgba[i] * opacity2 +
					srcLine->rgba[i] * opacity) >> 8;
			}
			
			srcLine++;
			dstLine++;
		}
	}
}

void MacImage::Downsample(MacImage * dst, const int scale) const
{
	Assert(scale >= 1);
	
	const int sx = Sx_get() / scale;
	const int sy = Sy_get() / scale;
	
	dst->Size_set(sx, sy, false);
	
	for (int y = 0; y < sy; ++y)
	{
		MacRgba * __restrict dstLine = dst->Line_get(y);
		
		for (int x = 0; x < sx; ++x)
		{
			const int x1 = (x + 0) * scale;
			const int y1 = (y + 0) * scale;
			int x2 = (x + 1) * scale - 1;
			int y2 = (y + 1) * scale - 1;
			
			if (x2 >= Sx_get())
				x2 = Sx_get() - 1;
			if (y2 >= Sy_get())
				y2 = Sy_get() - 1;
			
			int rgba[4] = { 0, 0, 0, 0 };
			
			for (int py = y1; py <= y2; ++py)
			{
				const MacRgba * __restrict srcLine = Line_get(py);
				
				for (int px = x1; px <= x2; ++px)
				{
					for (int i = 0; i < 4; ++i)
						rgba[i] += srcLine[px].rgba[i];
				}
			}
			
			const int area = (x2 - x1 + 1) * (y2 - y1 + 1);
			
			for (int i = 0; i < 4; ++i)
				dstLine[x].rgba[i] = rgba[i] / area;
		}
	}
}

#ifdef USE_CXIMAGE

void MacImage::Blit_Resampled(MacImage * dst, const bool includeAlpha) const
{
	if (includeAlpha)
	{
		for (int i = 0; i < 4; ++i)
		{
			CxImage image(mSx, mSy, 24);
			
			for (int y = 0; y < mSy; ++y)
			{
				const MacRgba * __restrict srcLine = Line_get(y);
				         BYTE * __restrict dstLine = image.GetBits(y);
				
				for (int x = 0; x < mSx; ++x)
				{
					dstLine[0] = srcLine[x].rgba[i];
					dstLine += 3;
				}
			}
			
			image.Resample2(dst->Sx_get(), dst->Sy_get());
			
			for (int y = 0; y < dst->mSy; ++y)
			{
				const    BYTE * __restrict srcLine = image.GetBits(y);
				      MacRgba * __restrict dstLine = dst->Line_get(y);
				
				for (int x = 0; x < dst->mSx; ++x)
				{
					dstLine[x].rgba[i] = srcLine[0];
					srcLine += 3;
				}
			}
		}
	}
	else
	{
#define N 3
		
	CxImage image(mSx, mSy, 8 * N);
	
	for (int y = 0; y < mSy; ++y)
	{
		const MacRgba * __restrict srcLine = Line_get(y);
		         BYTE * __restrict dstLine = image.GetBits(y);

#if 1
		for (int x = 0; x < mSx; ++x)
		{
			for (int i = 0; i < N; ++i)
				dstLine[i] = srcLine[x].rgba[i];

			dstLine += N;
		}
#else
		Mem::Copy(srcLine, dstLine, mSx * sizeof(MacRgba));
#endif
	}

	image.Resample2(dst->Sx_get(), dst->Sy_get());

	for (int y = 0; y < dst->Sy_get(); ++y)
	{
		const    BYTE * __restrict srcLine = image.GetBits(y);
		      MacRgba * __restrict dstLine = dst->Line_get(y);

#if 1
		for (int x = 0; x < dst->Sx_get(); ++x)
		{
			for (int i = 0; i < N; ++i)
				dstLine[x].rgba[i] = srcLine[i];

#if 1
			dstLine[x].rgba[3] = 255;
#endif
			
			srcLine += N;
		}
#else
		Mem::Copy(srcLine, dstLine, dst->Sx_get() * sizeof(MacRgba));
#endif
	}
	}
}

#endif

#if USE_AGG
void MacImage::Blit_Transformed(MacImage * dst, const BlitTransform & transform)
{
	ImageResampling::Blit_Transformed(*this, *dst, transform);
}
#endif

MacImage * MacImage::FlipY() const
{
	MacImage * result = new MacImage();
	
	result->Size_set(Sx_get(), Sy_get(), false);
	
	for (int y = 0; y < Sy_get(); ++y)
	{
		const MacRgba * __restrict src = Line_get(y);
		      MacRgba * __restrict dst = result->Line_get(Sy_get() - 1 - y);
		
		Mem::Copy(src, dst, Sx_get() * sizeof(MacRgba));
	}
	
	return result;
}

void MacImage::FlipY_InPlace()
{
	MacImage * temp = FlipY();
	temp->Blit(this);
	delete temp;
	temp = nullptr;
}

void MacImage::Save(Stream * stream) const
{
	StreamWriter writer(stream, false);
	
	const uint8_t version = 1;
	
	writer.WriteUInt8(version);
	
	switch (version)
	{
		case 1:
		{
			writer.WriteInt32(mSx);
			writer.WriteInt32(mSy);
			
			const int byteCount = mSx * mSy * 4;
			
			stream->Write(mData, byteCount);
			
			break;
		}
			
		default:
			throw ExceptionVA("unknown version");
	}
}

void MacImage::Load(Stream * stream)
{
	StreamReader reader(stream, false);
	
	const uint8_t version = reader.ReadUInt8();
	
	switch (version)
	{
		case 1:
		{
			const int sx = reader.ReadInt32();
			const int sy = reader.ReadInt32();
			
			Assert(sx >= 0);
			Assert(sy >= 0);
			
			Size_set(sx, sy, false);
			
			const int byteCount = sx * sy * 4;
			
			stream->Read(mData, byteCount);
			
			break;
		}
			
		default:
			throw ExceptionVA("unknown image version");
	}
}

void MacImage::Size_set(const int sx, const int sy, const bool clear)
{
	delete[] mData;
	mData = nullptr;
	mSx = 0;
	mSy = 0;
	
#ifdef IPHONEOS
	if (mImage)
	{
		CGImageRelease(mImage);
		mImage = 0;
	}
#endif
	
	const int area = sx * sy;
	
	if (area > 0)
	{
		mData = new MacRgba[area];
		mSx = sx;
		mSy = sy;
		
		if (clear)
		{
			ClearMemory(mData, area * sizeof(MacRgba));
		}
		
#ifdef IPHONEOS
		mImage = CreateImage();
#endif
	}
}

Vec2I MacImage::Size_get() const
{
	return Vec2I(mSx, mSy);
}

#ifdef IPHONEOS
CGImageRef MacImage::Image_get()
{
	if (!mImage)
		return 0;
	
#if 0
	for (int i = 0; i < 1000; ++i)
	{
		int x = rand() % mSx;
		int y = rand() % mSy;
		Line_get(y)[x].rgba[0] = rand() % 256;
		Line_get(y)[x].rgba[3] = 255;
	}
#endif
	
	CGImageRetain(mImage);
	
	return mImage;
}

CGImageRef MacImage::ImageWithAlpha_get() const
{
	Assert(mData);
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	CGImageRef image = CGImageCreate(
		mSx,
		mSy, 8, 32, mSx * 4,
		colorSpace,
		kCGImageAlphaLast,
		CGDataProviderCreateWithData(0, mData, mSx * mSy * 4, 0),
		0, FALSE,
		kCGRenderingIntentDefault);
	CGColorSpaceRelease(colorSpace);
	
	return image;
}
#endif

#ifdef IPHONEOS
CGImageRef MacImage::CreateImage()
{
	Assert(mData);
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	CGImageRef image = CGImageCreate(
		mSx,
		mSy, 8, 32, mSx * 4,
		colorSpace,
		0,
		CGDataProviderCreateWithData(0, mData, mSx * mSy * 4, 0),
		0, FALSE,
		kCGRenderingIntentDefault);
	CGColorSpaceRelease(colorSpace);
	
	return image;
}
#endif
