#pragma once

#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Debugging.h"
#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

typedef struct Rgba
{
	float rgb[4];
} Rgba;

static inline Rgba Rgba_Make(float r, float g, float b)
{
	Assert(r >= 0.0f && r <= 1.0f);
	Assert(g >= 0.0f && g <= 1.0f);
	Assert(b >= 0.0f && b <= 1.0f);

	const Rgba result = { { r, g, b, 1.0f } };

	return result;
}

static inline Rgba Rgba_Make(float r, float g, float b, float a)
{
	Assert(r >= 0.0f && r <= 1.0f);
	Assert(g >= 0.0f && g <= 1.0f);
	Assert(b >= 0.0f && b <= 1.0f);
	Assert(a >= 0.0f && a <= 1.0f);

	const Rgba result = { { r, g, b, a } };

	return result;
}

static inline bool Rgba_Equals(const Rgba& color1, const Rgba& color2)
{
	for (int i = 0; i < 4; ++i)
		if (color1.rgb[i] != color2.rgb[i])
			return false;
	
	return true;
}

class Bitmap
{
public:
	Bitmap();
	~Bitmap();
	
	void Load(Stream* stream);
	void Save(Stream* stream) const;

	void ToMacImage(MacImage& image) const;
	void ToMacImage_GrayScale(MacImage& image) const;
	void ToMacImage_GrayAlpha(MacImage& image) const;
	
	void ExtractTo(Bitmap* bmp, int x, int y, int sx, int sy) const;
	void Blit(Bitmap* bmp, int spx, int spy, int sx, int sy, int dpx, int dpy) const;
	void Blit(Bitmap* bmp) const;

	inline void Sample(int x, int y, Rgba & __restrict out_value) const __restrict;
	inline const Rgba* Sample_Ref(int x, int y) const;
	inline int Sample_Safe(int x, int y, Rgba & __restrict out_value) const __restrict;
	inline void Sample_Clamped(int x, int y, Rgba & __restrict out_value) const __restrict;
	inline const Rgba* Sample_Clamped_Ref(int x, int y) const;
	inline void Sample_Border(int x, int y, Rgba & __restrict out_value) const __restrict;
	int SampleAA(float x, float y, const Rgba * __restrict * __restrict out_values, float * __restrict out_w) const __restrict;
	void SampleAA(float x, float y, Rgba & __restrict out_value) const __restrict;
#ifdef __ARM_NEON__
	inline void SampleAA(float32x2_t s, Rgba & __restrict out_value) const __restrict;
#endif
	void Clear(Rgba color);

	void Size_set(int sx, int sy, bool clear);
	inline Rgba* Line_get(int y);
	inline const Rgba* Line_get(int y) const;
	inline int Sx_get() const;
	inline int Sy_get() const;
	AreaI Area_get() const;
	
private:
	Rgba* mData;
	int mSx;
	int mSy;
};

inline void Bitmap::Sample(int x, int y, Rgba& out_value) const
{
	Assert(x >= 0 && x < mSx);
	Assert(y >= 0 && y < mSy);
	
	out_value = mData[x + y * mSx];
}

inline const Rgba* Bitmap::Sample_Ref(int x, int y) const
{
	Assert(x >= 0 && x < mSx);
	Assert(y >= 0 && y < mSy);
	
	return mData + x + y * mSx;
}

inline int Bitmap::Sample_Safe(int x, int y, Rgba& out_value) const
{
	if (x < 0 || y < 0 || x >= mSx || y >= mSy)
	{
		out_value = Rgba_Make(0.0f, 0.0f, 0.0f);
		return 0;
	}
	else
	{
		Sample(x, y, out_value);
		return 1;
	}
}

inline void Bitmap::Sample_Clamped(int x, int y, Rgba& out_value) const
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= mSx)
		x = mSx - 1;
	if (y >= mSy)
		y = mSy - 1;

	Sample(x, y, out_value);
}

inline const Rgba* Bitmap::Sample_Clamped_Ref(int x, int y) const
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= mSx)
		x = mSx - 1;
	if (y >= mSy)
		y = mSy - 1;

	return Sample_Ref(x, y);
}

inline void Bitmap::Sample_Border(int x, int y, Rgba& out_value) const
{
	if (x < 0 || y < 0 || x >= mSx || y >= mSy)
		out_value = Rgba_Make(0.0f, 0.0f, 0.0f);
	else
		Sample(x, y, out_value);
}

#ifdef __ARM_NEON__
inline void Bitmap::SampleAA(float32x2_t v, Rgba& out_value) const __restrict
{
	// calculate w = v - floor(v), assuming v >= 0
	
	const int32x2_t   v2i    = vcvt_s32_f32(v);   // convert to int (rounds towards zero)
	const float32x2_t i2f    = vcvt_f32_s32(v2i); // convert back to float (= floor iff v >= 0)
	const float32x2_t w_x2y2 = vsub_f32(v, i2f);  // w = v - floor(v)
	
	// check if sample positions are within bounds. if not, we'll need to sample the border..
	
	const int x1 = vget_lane_s32(v2i, 0);
	const int y1 = vget_lane_s32(v2i, 1);
	const int x2 = x1 + 1;
	const int y2 = y1 + 1;
	const int correct = x1 < 0 || y1 < 0 || x2 >= mSx || y2 >= mSy;
	
	// calculate 1 - w
	
	const float32x2_t one = vdup_n_f32(1.0f);
	const float32x2_t w_x1y1 = vsub_f32(one, w_x2y2);
	
	// calculate bilinear weights
	
	float32x2_t w_y1x1 = vrev64_f32(w_x1y1);
	float32x2_t w_y2x2 = vrev64_f32(w_x2y2);
	
	float32x2_t w_x1y1_y1x1 = vmul_f32(w_x1y1, w_y1x1);
	float32x2_t w_x2y2_y1x1 = vmul_f32(w_x2y2, w_y1x1);
	float32x2_t w_x2y2_y2x2 = vmul_f32(w_x2y2, w_y2x2);
	
	float32x4_t w1 = vdupq_lane_f32(w_x1y1_y1x1, 0); // x1 * y1
	float32x4_t w2 = vdupq_lane_f32(w_x2y2_y1x1, 0); // x2 * y1
	float32x4_t w3 = vdupq_lane_f32(w_x2y2_y1x1, 1); // x1 * y2
	float32x4_t w4 = vdupq_lane_f32(w_x2y2_y2x2, 0); // x2 * y2
	
	float32x4_t v1;
	float32x4_t v2;
	float32x4_t v3;
	float32x4_t v4;
	
	if (!correct)
	{
		// sample 4 color values
		
		const Rgba* base = Line_get(y1) + x1;
		
		v1 = vld1q_f32(reinterpret_cast<const float32_t*>(base          ));
		v2 = vld1q_f32(reinterpret_cast<const float32_t*>(base + 1      ));
		v3 = vld1q_f32(reinterpret_cast<const float32_t*>(base     + mSx));
		v4 = vld1q_f32(reinterpret_cast<const float32_t*>(base + 1 + mSx));
	}
	else
	{
		// sample 4 color values, possibly at border
		
		v1 = vld1q_f32(reinterpret_cast<const float32_t*>(Sample_Clamped_Ref(x1, y1)));
		v2 = vld1q_f32(reinterpret_cast<const float32_t*>(Sample_Clamped_Ref(x2, y1)));
		v3 = vld1q_f32(reinterpret_cast<const float32_t*>(Sample_Clamped_Ref(x1, y2)));
		v4 = vld1q_f32(reinterpret_cast<const float32_t*>(Sample_Clamped_Ref(x2, y2)));
	}
	
	// bilinear interpolation
	
	float32x4_t c;
	c = vmulq_f32(v1, w1);
	c = vmlaq_f32(c, v2, w2);
	c = vmlaq_f32(c, v3, w3);
	c = vmlaq_f32(c, v4, w4);
	
	vst1q_f32(reinterpret_cast<float32_t*>(out_value.rgb), c);
}
#endif

inline Rgba* Bitmap::Line_get(int y)
{
	return mData + y * mSx;
}

inline const Rgba* Bitmap::Line_get(int y) const
{
	return mData + y * mSx;
}

inline int Bitmap::Sx_get() const
{
	return mSx;
}

inline int Bitmap::Sy_get() const
{
	return mSy;
}
