#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "Bitmap.h"
#include "Calc.h"
#include "Debugging.h"
#include "Log.h"
#include "Tool_Brush.h"

Tool_Brush::Tool_Brush()
{
	Setup(1, 1.0f, false);
}

void Tool_Brush::Setup(const int diameter, const float hardness, const bool isOriented)
{
	Assert(diameter > 0);
	//Assert(diameter % 2 == 1);
	Assert(hardness >= 0.0f && hardness <= 1.0f);

	if (diameter != mDiameter || hardness != mHardness)
	{
		mDiameter = diameter;
		mHardness = hardness;
		mIsOriented = isOriented;

		CreateFilter();
	}
}

void Tool_Brush::Setup_Pattern(const int diameter, const Filter * filter, const bool isOriented)
{
	Assert(diameter > 0);
	//Assert(diameter % 2 == 1);

	mDiameter = diameter;
	mHardness = 0.0f;
	mIsOriented = isOriented;

//	LOG_DBG("createFilter_Pattern: %d", mDiameter);
	
	mFilter.Size_set(mDiameter, mDiameter);

	filter->Blit_Resampled(&mFilter);
}

void Tool_Brush::ApplyFilter(Filter * bmp, const Filter * filter, const float ___x, const float ___y, const float _dx, const float _dy, AreaI & dirty)
{
	if (mIsOriented)
	{
		const float angle = Vec2F::ToAngle(Vec2F(_dx, _dy));
		ApplyFilter_Rotated(bmp, filter, ___x, ___y, angle, dirty);
		return;
	}
	
	// todo: move code below to sepate method

//	_x -= (mDiameter - 1) / 2;
//	_y -= (mDiameter - 1) / 2;
	const float _x = ___x - (mDiameter - 1) / 2.0f;
	const float _y = ___y - (mDiameter - 1) / 2.0f;

	int __x = (int)floorf(_x);
	int __y = (int)floorf(_y);

	AreaI area;
	area.m_Min[0] = __x;
	area.m_Min[1] = __y;
	area.m_Max[0] = __x + mDiameter;
	area.m_Max[1] = __y + mDiameter;
	
	int x1 = __x;
	int y1 = __y;
	int x2 = __x + mDiameter;
	int y2 = __y + mDiameter;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > bmp->Sx_get() - 1)
		x2 = bmp->Sx_get() - 1;
	if (y2 > bmp->Sy_get() - 1)
		y2 = bmp->Sy_get() - 1;

	float weights[4];
	SampleAA_Prepare(-_x, -_y, weights);
	
	for (int y = y1; y <= y2; ++y)
	{
		float * __restrict line = bmp->Line_get(y);
		
		for (int x = x1; x <= x2; ++x)
		{
			float & __restrict aBmp = line[x];
			const float aFilter = mFilter.SampleAA(x - __x, y - __y, weights);

			aBmp = aFilter + aBmp * (1.0f - aFilter);
		}
	}
	
	if (!area.Clip(bmp->Area_get()))
		return;
	
	dirty.Merge(area);
}

void Tool_Brush::ApplyFilter_Rotated(Filter * bmp, const Filter * filter, const float __x, const float __y, const float angle, AreaI & dirty)
{
//	LOG_DBG("brush: rotated", 0);
	
	const int radius = (mDiameter - 1) / 2;
	const int size = radius;

	const float _x = __x - size;
	const float _y = __y - size;
	
	const int diameter = size * 2 + 1;
	
	int x1 = (int)floorf(_x);
	int y1 = (int)floorf(_y);
	int x2 = (int)ceilf(_x) + diameter;
	int y2 = (int)ceilf(_y) + diameter;

	AreaI area;
	area.m_Min[0] = x1;
	area.m_Min[1] = y1;
	area.m_Max[0] = x2;
	area.m_Max[1] = y2;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > bmp->Sx_get() - 1)
		x2 = bmp->Sx_get() - 1;
	if (y2 > bmp->Sy_get() - 1)
		y2 = bmp->Sy_get() - 1;

	const Vec2F dirX(Vec2F::FromAngle(angle));
	const Vec2F dirY(Vec2F::FromAngle(angle + Calc::mPI2));

	for (int y = y1; y <= y2; ++y)
	{
		float * __restrict line = bmp->Line_get(y);

		const float dy = y - _y - size;
		
		for (int x = x1; x <= x2; ++x)
		{
			const float dx = x - _x - size;

			const float px = dirX[0] * dx + dirX[1] * dy + radius;
			const float py = dirY[0] * dx + dirY[1] * dy + radius;

			float & __restrict aBmp = line[x];
			const float aFilter = filter->SampleAA(px, py);

			aBmp = aFilter + aBmp * (1.0f - aFilter);
		}
	}

	if (!area.Clip(bmp->Area_get()))
		return;
	
	dirty.Merge(area);
}

void Tool_Brush::ApplyFilter_Cheap(Filter * bmp, const Filter * filter, const float ___x, const float ___y, const float _dx, const float _dy, AreaI & dirty)
{
	const int radius = (mDiameter - 1) / 2;
	
	const float _x = ___x - radius;
	const float _y = ___y - radius;

	int __x = (int)floorf(_x);
	int __y = (int)floorf(_y);

	AreaI area;
	area.m_Min[0] = __x;
	area.m_Min[1] = __y;
	area.m_Max[0] = __x + mDiameter - 1;
	area.m_Max[1] = __y + mDiameter - 1;
	
	int x1 = __x;
	int y1 = __y;
	int x2 = __x + mDiameter - 1;
	int y2 = __y + mDiameter - 1;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > bmp->Sx_get() - 1)
		x2 = bmp->Sx_get() - 1;
	if (y2 > bmp->Sy_get() - 1)
		y2 = bmp->Sy_get() - 1;
	
	for (int y = y1; y <= y2; ++y)
	{
		      float * __restrict line = bmp->Line_get(y);
		const float * __restrict filterLine = filter->Line_get(y - __y) + x1 - __x;
		
		for (int x = x1; x <= x2; ++x, ++filterLine)
		{
			float & __restrict aBmp = line[x];
			const float aFilter = *filterLine;

			aBmp = aFilter + aBmp * (1.0f - aFilter);
		}
	}
	
	if (!area.Clip(bmp->Area_get()))
		return;
	
	dirty.Merge(area);
}

void Tool_Brush::LoadBrush(Stream * stream)
{
	mFilter.Load(stream);
	
	mDiameter = mFilter.Sx_get();
}

int Tool_Brush::Diameter_get() const
{
	return mDiameter;
}

float Tool_Brush::Hardness_get() const
{
	return mHardness;
}

void Tool_Brush::IsOriented_set(const bool isOriented)
{
	mIsOriented = isOriented;
}

const Filter * Tool_Brush::Filter_get() const
{
	return &mFilter;
}

Filter * Tool_Brush::Filter_getRW()
{
	return &mFilter;
}

void Tool_Brush::CreateFilter()
{
	mFilter.MakeSoft(mDiameter, mHardness);
}

// --------------------

Tool_BrushDirect::Tool_BrushDirect()
{
	Setup(1, 1.0f, false);
}

void Tool_BrushDirect::Setup(const int diameter, const float hardness, const bool isOriented)
{
	Assert(diameter > 0);
	//Assert(diameter % 2 == 1);
	Assert(hardness >= 0.0f && hardness <= 1.0f);

	if (diameter != mDiameter || hardness != mHardness)
	{
		mDiameter = diameter;
		mHardness = hardness;
		mIsOriented = isOriented;

		CreateFilter();
	}
}

void Tool_BrushDirect::Setup_Pattern(const int diameter, const Filter * filter, const bool isOriented)
{
	Assert(diameter > 0);
	//Assert(diameter % 2 == 1);

	mDiameter = diameter;
	mHardness = 0.0f;
	mIsOriented = isOriented;

//	LOG_DBG("createFilter_Pattern: %d", mDiameter);
	
	mFilter.Size_set(mDiameter, mDiameter);

	filter->Blit_Resampled(&mFilter);
}

void Tool_BrushDirect::ApplyFilter(Bitmap * bmp, const Filter * filter, const float __x, const float __y, const float _dx, const float _dy, const Rgba & _color, AreaI & dirty)
{
	Rgba color;
	for (int i = 0; i < 4; ++i)
		color.rgb[i] = _color.rgb[i] * 0.1f;
	
	if (mIsOriented)
	{
		const float angle = Vec2F::ToAngle(Vec2F(_dx, _dy));
		ApplyFilter_Rotated(bmp, filter, __x, __y, angle, color, dirty);
		return;
	}
	
	// todo: move code below to sepate method

//	_x -= (mDiameter - 1) / 2;
//	_y -= (mDiameter - 1) / 2;
	const float _x = __x - (mDiameter - 1) / 2.0f;
	const float _y = __y - (mDiameter - 1) / 2.0f;

	int xFloor = (int)floorf(_x);
	int yFloor = (int)floorf(_y);
	
	AreaI area;
	area.m_Min[0] = xFloor;
	area.m_Min[1] = yFloor;
	area.m_Max[0] = xFloor + mDiameter;
	area.m_Max[1] = yFloor + mDiameter;
	
	int x1 = xFloor;
	int y1 = yFloor;
	int x2 = xFloor + mDiameter;
	int y2 = yFloor + mDiameter;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > bmp->Sx_get() - 1)
		x2 = bmp->Sx_get() - 1;
	if (y2 > bmp->Sy_get() - 1)
		y2 = bmp->Sy_get() - 1;

	float weights[4];
	SampleAA_Prepare(-_x, -_y, weights);
	int xCeil = (int)ceilf(_x);
	int yCeil = (int)ceilf(_y);
	
	for (int y = y1; y <= y2; ++y)
	{
		Rgba * __restrict line = bmp->Line_get(y);
		
		for (int x = x1; x <= x2; ++x)
		{
			Rgba & __restrict cBmp = line[x];
			const float aFilter = mFilter.SampleAA(x - xCeil, y - yCeil,  weights);
			const float aBmp = 1.0f - aFilter * color.rgb[3];

#ifdef __ARM_NEON__
			float32x4_t a1 = vld1q_dup_f32((float32_t*)&aFilter);
			float32x4_t v1 = vld1q_f32((float32_t*)color.rgb);
			float32x4_t a2 = vld1q_dup_f32((float32_t*)&aBmp);
			float32x4_t v2 = vld1q_f32((float32_t*)cBmp.rgb);
			
			float32x4_t c = vmulq_f32(v1, a1);
			c = vmlaq_f32(c, v2, a2);
			
			vst1q_f32((float32_t*)cBmp.rgb, c);
#else
			for (int i = 0; i < 4; ++i)
			{
				cBmp.rgb[i] = aFilter * color.rgb[i] + aBmp * cBmp.rgb[i];
				//Assert(cBmp.rgb[i] >= 0.0f && cBmp.rgb[i] <= 1.0f);
			}
#endif
		}
	}
	
	if (!area.Clip(bmp->Area_get()))
		return;
	
	dirty.Merge(area);
}

void Tool_BrushDirect::ApplyFilter_Rotated(Bitmap * bmp, const Filter * filter, const float __x, const float __y, const float angle, const Rgba & color, AreaI & dirty)
{
//	LOG_DBG("brush: rotated", 0);
	
	const int radius = (mDiameter - 1) / 2;

//	_x -= radius;
//	_y -= radius;

//	const int size = (int)ceilf(radius * 1.5f);
	const int size = radius;

	const float _x = __x - size;
	const float _y = __y - size;
	
	const int diameter = size * 2 + 1;
	
	int x1 = (int)floorf(_x);
	int y1 = (int)floorf(_y);
	int x2 = (int)ceilf(_x) + diameter;
	int y2 = (int)ceilf(_y) + diameter;

	AreaI area;
	area.m_Min[0] = x1;
	area.m_Min[1] = y1;
	area.m_Max[0] = x2;
	area.m_Max[1] = y2;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > bmp->Sx_get() - 1)
		x2 = bmp->Sx_get() - 1;
	if (y2 > bmp->Sy_get() - 1)
		y2 = bmp->Sy_get() - 1;

	const Vec2F dirX(Vec2F::FromAngle(angle));
	const Vec2F dirY(Vec2F::FromAngle(angle + Calc::mPI2));

	for (int y = y1; y <= y2; ++y)
	{
		Rgba * __restrict line = bmp->Line_get(y);

		const float dy = y - _y - size;
		
		for (int x = x1; x <= x2; ++x)
		{
			const float dx = x - _x - size;

			const float px = dirX[0] * dx + dirX[1] * dy + radius;
			const float py = dirY[0] * dx + dirY[1] * dy + radius;

			Rgba & __restrict aBmp = line[x];
			
#if 0
#warning
			float weights[4];
			SampleAA_Prepare(px, py, weights);
			const float aFilter = filter->SampleAA(floorf(px), floorf(py), weights);
#else
			const float aFilter = filter->SampleAA(px, py);
#endif

			for (int i = 0; i < 4; ++i)
			{
				aBmp.rgb[i] = aFilter * color.rgb[i] + aBmp.rgb[i] * (1.0f - aFilter);
			}
		}
	}

	if (!area.Clip(bmp->Area_get()))
		return;
	
	dirty.Merge(area);
}

int Tool_BrushDirect::Diameter_get() const
{
	return mDiameter;
}

float Tool_BrushDirect::Hardness_get() const
{
	return mHardness;
}

void Tool_BrushDirect::IsOriented_set(const bool isOriented)
{
	mIsOriented = isOriented;
}

const Filter * Tool_BrushDirect::Filter_get() const
{
	return &mFilter;
}

Filter * Tool_BrushDirect::Filter_getRW()
{
	return &mFilter;
}

void Tool_BrushDirect::CreateFilter()
{
	mFilter.MakeSoft(mDiameter, mHardness);
}
