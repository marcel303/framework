#pragma once

#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Hash.h"
#include "libgg_forward.h"
#include "libklodder_forward.h"
#include "Types.h"

static inline void SampleAA_Prepare(const float x, const float y, float * out_w)
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
	
	void Load(Stream * stream);
	void Save(Stream * stream) const;

	void Multiply(const float value);
	void MakeSoft(const int diameter, const float hardness);
#ifndef FILTER_STANDALONE
	void ToMacImage(MacImage & image, const Rgba & color) const;
#endif
	void Blit(Filter * filter) const;
	void Blit_Resampled(Filter * filter) const;
	
	inline float Sample(const int x, const int y) const;
	inline int Sample_Safe(const int x, const int y, float & __restrict out_value) const __restrict;
	inline float Sample_Clamped(const int x, const int y) const;
	inline float Sample_Border(const int x, const int y) const;
	int SampleAA(const float x, const float y, float * __restrict out_value, float * __restrict out_w) const __restrict;
	float SampleAA(const float x, const float y) const;
	inline float SampleAA(const int x, const int y, const float * __restrict weights) const __restrict;

	void Size_set(const int sx, const int sy);
	inline float * Line_get(const int y);
	inline const float * Line_get(const int y) const;
	inline int Sx_get() const;
	inline int Sy_get() const;
	AreaI Area_get() const;
	Hash Hash_get() const;
	
private:
	float * mData;
	int mSx;
	int mSy;
};

inline float Filter::Sample(const int x, const int y) const
{
	Assert(x >= 0 && x < mSx);
	Assert(y >= 0 && y < mSy);

	return mData[x + y * mSx];
}

inline int Filter::Sample_Safe(const int x, const int y, float & __restrict out_value) const __restrict
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

inline float Filter::Sample_Clamped(const int _x, const int _y) const
{
	const int x = _x < 0 ? 0 : _x >= mSx ? mSx - 1 : _x;
	const int y = _y < 0 ? 0 : _y >= mSy ? mSy - 1 : _y;
	
	return Sample(x, y);
}

inline float Filter::Sample_Border(const int x, const int y) const
{
	if (x < 0 || y < 0 || x >= mSx || y >= mSy)
		return 0.0f;
	else
		return Sample(x, y);
}

inline float Filter::SampleAA(const int x, const int y, const float * __restrict weights) const __restrict
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

inline float * Filter::Line_get(const int y)
{
	return mData + mSx * y;
}

inline const float * Filter::Line_get(const int y) const
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
