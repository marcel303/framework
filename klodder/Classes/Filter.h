#pragma once

#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Hash.h"
#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

static inline void SampleAA_Prepare(float x, float y, float* out_w)
{
	const float xf = floorf(x);
	const float yf = floorf(y);
	
	const float wx2 = x - xf;
	const float wy2 = y - yf;
	const float wx1 = 1.0f - wx2;
	const float wy1 = 1.0f - wy2;
	
	out_w[0] = wx1 * wy1;
	out_w[1] = wx2 * wy1;
	out_w[2] = wx1 * wy2;
	out_w[3] = wx2 * wy2;
}

class Filter
{
public:
	Filter();
	~Filter();
	
	void Load(Stream* stream);
	void Save(Stream* stream) const;

	void Multiply(float value);
	void MakeSoft(int diameter, float hardness);
#ifndef FILTER_STANDALONE
	void ToMacImage(MacImage& image, const Rgba& color) const;
#endif
	void Blit(Filter* filter) const;
	void Blit_Resampled(Filter* filter) const;
	
	inline float Sample(int x, int y) const;
	inline int Sample_Safe(int x, int y, float & __restrict out_value) const __restrict;
	inline float Sample_Clamped(int x, int y) const;
	inline float Sample_Border(int x, int y) const;
	int SampleAA(float x, float y, float * __restrict out_value, float * __restrict out_w) const __restrict;
	float SampleAA(float x, float y) const;
	inline float SampleAA(int x, int y, const float * __restrict weights) const __restrict;

	void Size_set(int sx, int sy);
	inline float* Line_get(int y);
	inline const float* Line_get(int y) const;
	inline int Sx_get() const;
	inline int Sy_get() const;
	AreaI Area_get() const;
	Hash Hash_get() const;
	
private:
	float* mData;
	int mSx;
	int mSy;
};

inline float Filter::Sample(int x, int y) const
{
	Assert(x >= 0 && x < mSx);
	Assert(y >= 0 && y < mSy);

	return mData[x + y * mSx];
}

inline int Filter::Sample_Safe(int x, int y, float & __restrict out_value) const __restrict
{
	if (x < 0 || y < 0 || x >= mSx || y >= mSy)
	{
		out_value = 0.0f;
		return 0;
	}
	else
	{
		out_value = Sample(x, y);
		return 1;
	}
}

inline float Filter::Sample_Clamped(int x, int y) const
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= mSx)
		x = mSx - 1;
	if (y >= mSy)
		y = mSy - 1;

	return Sample(x, y);
}

inline float Filter::Sample_Border(int x, int y) const
{
	if (x < 0 || y < 0 || x >= mSx || y >= mSy)
		return 0.0f;
	else
		return Sample(x, y);
}

inline float Filter::SampleAA(int x, int y, const float * __restrict weights) const __restrict
{
	const int correct = x < 0 || y < 0 || x + 1 >= mSx || y + 1 >= mSy;
	
	if (!correct)
	{
		const float * __restrict base = Line_get(y) + x;
		
		return
			weights[0] * *(base) +
			weights[1] * *(base + 1) +
			weights[2] * *(base + mSx) +
			weights[3] * *(base + mSx + 1);
	}
	else
	{
		return
			weights[0] * Sample_Border(x,     y    ) +
			weights[1] * Sample_Border(x + 1, y    ) +
			weights[2] * Sample_Border(x,     y + 1) +
			weights[3] * Sample_Border(x + 1, y + 1);
	}
}

inline float* Filter::Line_get(int y)
{
	return mData + mSx * y;
}

inline const float* Filter::Line_get(int y) const
{
	return mData + mSx * y;
}

inline int Filter::Sx_get() const
{
	return mSx;
}

inline int Filter::Sy_get() const
{
	return mSy;
}
