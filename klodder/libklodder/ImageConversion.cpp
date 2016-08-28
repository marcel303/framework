#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Benchmark.h"
#include "Bitmap.h"
#include "ImageConversion.h"
#include "MacImage.h"

void BitmapToMacImage(const Bitmap * bitmap, MacImage * image)
{
	Assert(bitmap->Sx_get() == image->Sx_get());
	Assert(bitmap->Sy_get() == image->Sy_get());

	UsingBegin(Benchmark bm("BitmapToMacImage (full)"))
	{
		const uint32_t sx = bitmap->Sx_get();
		const uint32_t sy = bitmap->Sy_get();

		for (uint32_t y = 0; y < sy; ++y)
		{
			const    Rgba * __restrict srcLine = bitmap->Line_get(y);
			      MacRgba * __restrict dstLine = image->Line_get(y);

#ifdef __ARM_NEON__
			// 0.023 sec @ 1024x768 (iPad2)
			
			const uint32_t n = sx >> 1;
			const float32x4_t scale = vdupq_n_f32(255.0f);
			
			for (uint32_t x = n; x != 0; --x)
			{
				__builtin_prefetch(srcLine + 8);
				__builtin_prefetch(srcLine + 16);
				
				const float32x4_t fLo      = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine    )), scale);
				const float32x4_t fHi      = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine + 1)), scale);
				const uint32x4_t  vLo      = vcvtq_u32_f32(fLo);
				const uint32x4_t  vHi      = vcvtq_u32_f32(fHi);
				const uint16x4_t  vLo_16   = vmovn_u32(vLo);
				const uint16x4_t  vHi_16   = vmovn_u32(vHi);
				const uint16x8_t  vLoHi_16 = vcombine_u16(vLo_16, vHi_16);
				const uint8x8_t   vLoHi_08 = vmovn_u16(vLoHi_16);
				
				vst1_u8(reinterpret_cast<uint8_t*>(dstLine), vLoHi_08);
				
				srcLine += 2;
				dstLine += 2;
			}
#else
			// 0.062 sec @ 1024x768 (iPad2)
			
			for (uint32_t x = sx; x != 0; --x)
			{
				for (uint32_t i = 0; i < 4; ++i)
					dstLine->rgba[i] = (uint8_t)(srcLine->rgb[i] * 255.0f);

				srcLine++;
				dstLine++;
			}
#endif
		}
	}
	UsingEnd()
}

void MacImageToBitmap(const MacImage * __restrict image, Bitmap * __restrict bitmap)
{
	Assert(bitmap->Sx_get() == image->Sx_get());
	Assert(bitmap->Sy_get() == image->Sy_get());
	
	UsingBegin(Benchmark bm("MacImageToBitmap (full)"))
	{
		const uint32_t sx = bitmap->Sx_get();
		const uint32_t sy = bitmap->Sy_get();

#ifdef __ARM_NEON__
		const uint32_t n = sx >> 2;
		//const uint32_t n = sx >> 1;
		const float32x4_t scale = vdupq_n_f32(1.0f / 255.0f);
		
		for (uint32_t y = 0; y < sy; ++y)
		{
			const MacRgba * __restrict srcLine = image->Line_get(y);
			Rgba * __restrict dstLine = bitmap->Line_get(y);
			
			for (uint32_t x = n; x != 0; --x)
			{
				__builtin_prefetch(srcLine + 8);
				__builtin_prefetch(srcLine + 16);
	
#if 1
				// 0.034 sec @ 1024x768 (iPad2)
				const uint8x16_t v8 = vld1q_u8(reinterpret_cast<const uint8_t*>(srcLine));
				const uint8x8_t v8Lo = vget_low_u8(v8);
				const uint8x8_t v8Hi = vget_high_u8(v8);
				
				{
					const uint16x8_t v16 = vmovl_u8(v8Lo);
					const uint16x4_t v16Lo = vget_low_u16(v16);
					const uint16x4_t v16Hi = vget_high_u16(v16);
					const uint32x4_t v32Lo = vmovl_u16(v16Lo);
					const uint32x4_t v32Hi = vmovl_u16(v16Hi);
					const float32x4_t fLo = vmulq_f32(vcvtq_f32_u32(v32Lo), scale);
					const float32x4_t fHi = vmulq_f32(vcvtq_f32_u32(v32Hi), scale);
					vst1q_f32(reinterpret_cast<float32_t*>(dstLine    ), fLo);
					vst1q_f32(reinterpret_cast<float32_t*>(dstLine + 1), fHi);
				}
				
				{
					const uint16x8_t v16 = vmovl_u8(v8Hi);
					const uint16x4_t v16Lo = vget_low_u16(v16);
					const uint16x4_t v16Hi = vget_high_u16(v16);
					const uint32x4_t v32Lo = vmovl_u16(v16Lo);
					const uint32x4_t v32Hi = vmovl_u16(v16Hi);
					const float32x4_t fLo = vmulq_f32(vcvtq_f32_u32(v32Lo), scale);
					const float32x4_t fHi = vmulq_f32(vcvtq_f32_u32(v32Hi), scale);
					vst1q_f32(reinterpret_cast<float32_t*>(dstLine + 2), fLo);
					vst1q_f32(reinterpret_cast<float32_t*>(dstLine + 3), fHi);
				}
				
				srcLine += 4;
				dstLine += 4;
#elif 1
				// 0.038 sec @ 1024x768 (iPad2)
				const uint8x8_t v8 = vld1_u8(reinterpret_cast<const uint8_t*>(srcLine));
				const uint16x8_t v16 = vmovl_u8(v8);
				const uint16x4_t v16Lo = vget_low_u16(v16);
				const uint16x4_t v16Hi = vget_high_u16(v16);
				const uint32x4_t v32Lo = vmovl_u16(v16Lo);
				const uint32x4_t v32Hi = vmovl_u16(v16Hi);
				const float32x4_t fLo = vmulq_f32(vcvtq_f32_u32(v32Lo), scale);
				const float32x4_t fHi = vmulq_f32(vcvtq_f32_u32(v32Hi), scale);
				vst1q_f32(reinterpret_cast<float32_t*>(dstLine    ), fLo);
				vst1q_f32(reinterpret_cast<float32_t*>(dstLine + 1), fHi);
				
				srcLine += 2;
				dstLine += 2;
#else
				// 0.044 sec @ 1024x768 (iPad2)
				uint32_t srcLo[4] = { srcLine[0].rgba[0], srcLine[0].rgba[1], srcLine[0].rgba[2], srcLine[0].rgba[3] };
				uint32_t srcHi[4] = { srcLine[1].rgba[0], srcLine[1].rgba[1], srcLine[1].rgba[2], srcLine[1].rgba[3] };
				uint32x4_t vLo = vld1q_u32(srcLo);
				uint32x4_t vHi = vld1q_u32(srcHi);
				float32x4_t fLo = vmulq_f32(vcvtq_f32_u32(vLo), scale);
				float32x4_t fHi = vmulq_f32(vcvtq_f32_u32(vHi), scale);
				vst1q_f32(reinterpret_cast<float32_t*>(dstLine    ), fLo);
				vst1q_f32(reinterpret_cast<float32_t*>(dstLine + 1), fHi);
				
				srcLine += 2;
				dstLine += 2;
#endif
			}
		}
#else
		// 0.090 sec @ 1024x768 (iPad2)
		
		const float scale = 1.0f / 255.0f;
		
		for (uint32_t y = 0; y < sy; ++y)
		{
			const MacRgba * __restrict srcLine = image->Line_get(y);
			         Rgba * __restrict dstLine = bitmap->Line_get(y);

			for (uint32_t x = sx; x != 0; --x)
			{
				for (int i = 0; i < 4; ++i)
					dstLine->rgb[i] = srcLine->rgba[i] * scale;

				srcLine++;
				dstLine++;
			}
		}
#endif
	}
	UsingEnd()
}

void BitmapToMacImage(const Bitmap * src, MacImage * dst, const int _x, const int _y, const int sx, const int sy)
{
	Assert(sx > 0);
	Assert(sy > 0);
	
	UsingBegin(Benchmark bm("BitmapToMacImage (extract)"))
	{
		dst->Size_set(sx, sy, false);
		
		const int x1 = _x;
		const int x2 = _x + sx - 1;
		const int y1 = _y;
		const int y2 = _y + sy - 1;
		
		for (int y = y1; y <= y2; ++y)
		{
			const    Rgba * __restrict srcLine = src->Line_get(y) + x1;
			      MacRgba * __restrict dstLine = dst->Line_get(y - y1);
			
			for (int x = x1; x <= x2; ++x)
			{
				dstLine->rgba[0] = (uint8_t)(srcLine->rgb[0] * 255.0f);
				dstLine->rgba[1] = (uint8_t)(srcLine->rgb[1] * 255.0f);
				dstLine->rgba[2] = (uint8_t)(srcLine->rgb[2] * 255.0f);
				dstLine->rgba[3] = (uint8_t)(srcLine->rgb[3] * 255.0f);
				
				srcLine++;
				dstLine++;
			}
		}
	}
	UsingEnd()
}

void MacImageToBitmap(const MacImage * src, Bitmap * dst, const int _x, const int _y)
{
	UsingBegin(Benchmark bm("MacImageToBitmap (offsetted)"))
	{
		const int sx = src->Sx_get();
		const int sy = src->Sy_get();
		
		const int x1 = _x;
		const int x2 = _x + sx - 1;
		const int y1 = _y;
		const int y2 = _y + sy - 1;
		
		for (int y = y1; y <= y2; ++y)
		{
			const MacRgba * __restrict srcLine = src->Line_get(y - y1);
			         Rgba * __restrict dstLine = dst->Line_get(y) + x1;
			
			for (int x = x1; x <= x2; ++x)
			{
				dstLine->rgb[0] = srcLine->rgba[0] / 255.0f;
				dstLine->rgb[1] = srcLine->rgba[1] / 255.0f;
				dstLine->rgb[2] = srcLine->rgba[2] / 255.0f;
				dstLine->rgb[3] = srcLine->rgba[3] / 255.0f;
				
				srcLine++;
				dstLine++;
			}
		}
	}
	UsingEnd()
}
