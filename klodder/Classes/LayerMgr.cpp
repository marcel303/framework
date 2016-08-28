#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include <stdlib.h>
#include "Benchmark.h"
#include "Calc.h"
#include "ImageConversion.h"
#include "LayerMgr.h"
#include "Log.h"
#include "Util_Mem.h"

void RenderCheckerBoard(MacImage & __restrict dst, const MacRgba & backColor1, const MacRgba & backColor2, const uint32_t size)
{
	UsingBegin(Benchmark bm("RenderCheckerBoard"))
	{
		const uint32_t blockSize = sizeof(MacRgba) * size;
		
		MacRgba * __restrict c1 = (MacRgba *)alloca(blockSize);
		MacRgba * __restrict c2 = (MacRgba *)alloca(blockSize);
		for (uint32_t i = 0; i < size; ++i)
		{
			c1[i] = backColor1;
			c2[i] = backColor2;
		}
		const MacRgba * __restrict c[2] = { c1, c2 };
		
		const uint32_t sx = dst.Sx_get();
		const uint32_t sy = dst.Sy_get();
		
		const uint32_t blockCount = sx / size;
		const uint32_t remainder = sx - blockCount * size;
		const uint32_t remainderSize = sizeof(MacRgba) * remainder;
		
		uint32_t indexY = 0;
		uint32_t todoY = size;
		
		for (uint32_t y = 0; y < sy; ++y)
		{
			MacRgba * __restrict dstLine = dst.Line_get(y);
			
			todoY--;
			
			if (todoY == 0)
			{
				todoY = size;
				indexY = (indexY + 1) & 1;
			}
			
			uint32_t index = indexY;
			
			for (uint32_t x = blockCount; x != 0; --x)
			{
				memcpy(dstLine, c[index], blockSize);

				index = (index + 1) & 1;
				dstLine += size;
			}
			
			memcpy(dstLine, c[index], remainderSize);
		}
	}
	UsingEnd()
}

LayerMgr::LayerMgr()
{
	// layers
	mLayerCount = 0;
	mActiveDataLayer = 0;
	mBackColor1 = MacRgba_Make(0, 0, 0, 0);
	mBackColor2 = MacRgba_Make(255, 255, 255, 255);
	
	// editing
	mEditingEnabled = false;
	mMode = LayerMode_Undefined;
	mBrushColor = Rgba_Make(1.0f, 1.0f, 1.0f);
	mBrushOpacity = 1.0f;
	mBrushOpacity255i = 255;
	mBrushOpacity255f = 255.0f;
	mEditingDirty.Reset();
}

void LayerMgr::Setup(const int layerCount, const int sx, const int sy, const Rgba & backColor1, const Rgba & backColor2)
{
	Assert(layerCount == MAX_LAYERS);

	UsingBegin(Benchmark bm("LayerMgr::Setup"))
	{
		// destroy
		
		for (int i = 0; i < mLayerCount; ++i)
		{
			mLayerList[i].Size_set(0, 0, false);
		}

		mEditingBuffer.Size_set(0, 0, false);
		mBmpBrush.Size_set(0, 0);
		mCacheBack.Size_set(0, 0, false);
		mCacheFront.Size_set(0, 0, false);
		mMerged.Size_set(0, 0, false);
		
		// create

		mLayerCount = layerCount;
		mActiveDataLayer = 0;

		for (int i = 0; i < layerCount; ++i)
		{
			mLayerList[i].Size_set(sx, sy, true);
			mLayerOrder[i] = i;
			mLayerOpacity[i] = 255;
			mLayerVisibility[i] = true;
		}
		
		mEditingBuffer.Size_set(sx, sy, false);
		mBmpBrush.Size_set(sx, sy);

		mCacheBack.Size_set(sx, sy, false);
		mCacheFront.Size_set(sx, sy, false);
		mMerged.Size_set(sx, sy, false);
		
		mBackColor1 = MacRgba_Make(
			uint8_t(backColor1.rgb[0] * 255.0f),
			uint8_t(backColor1.rgb[1] * 255.0f),
			uint8_t(backColor1.rgb[2] * 255.0f),
			uint8_t(backColor1.rgb[3] * 255.0f));
		mBackColor2 = MacRgba_Make(
			uint8_t(backColor2.rgb[0] * 255.0f),
			uint8_t(backColor2.rgb[1] * 255.0f),
			uint8_t(backColor2.rgb[2] * 255.0f),
			uint8_t(backColor2.rgb[3] * 255.0f));

		mMode = LayerMode_Undefined;

		if (mEditingEnabled)
		{
			RebuildCaches();
		}

		Invalidate(0, 0, sx, sy);
	}
	UsingEnd()
}

void LayerMgr::RenderMerged(MacImage & __restrict dst, const MacRgba & backColor1, const MacRgba & backColor2, const int layerBegin, const int layerEnd)
{
	if (dst.Sx_get() != mMerged.Sx_get() || dst.Sy_get() != mMerged.Sy_get())
	{
		dst.Size_set(mMerged.Sx_get(), mMerged.Sy_get(), false);
	}
	
	// render solid background

	RenderCheckerBoard(dst, backColor1, backColor2, 16);
	
	// flatten layers
	
	for (int i = layerBegin; i <= layerEnd; ++i)
	{
		const int index = mLayerOrder[i];
		
		if (!mLayerVisibility[index])
			continue;

		const int opacity = mLayerOpacity[index];

		LOG_DBG("merge to back. index=%d, opacity=%d", index, opacity);

		mLayerList[index].BlitAlpha(&dst, opacity);
	}
}

void LayerMgr::RenderMergedFinal(MacImage & dst)
{
	RenderMerged(dst, mBackColor1, mBackColor2, 0, mLayerCount - 1);
}

void LayerMgr::SetMode_Brush()
{
	LOG_DBG("setMode_Brush", 0);
	
	mMode = LayerMode_Brush;
}

void LayerMgr::SetMode_Eraser()
{
	LOG_DBG("setMode_Eraser", 0);
	
	mMode = LayerMode_Eraser;
}

void LayerMgr::SetMode_Direct()
{
	LOG_DBG("setMode_Direct", 0);
	
	mMode = LayerMode_Direct;
}

void LayerMgr::FlattenBrush(const RectI & rect)
{
	UsingBegin(Benchmark bm("LayerMgr::FlattenBrush"))
	{
		LOG_DBG("flattenBrush", 0);
		
		Assert(mMode == LayerMode_Brush);
		Assert(mEditingBuffer.Sx_get() == mBmpBrush.Sx_get());
		Assert(mEditingBuffer.Sy_get() == mBmpBrush.Sy_get());
		
		const int x1 = rect.m_Position[0];
		const int y1 = rect.m_Position[1];
		const int x2 = rect.m_Position[0] + rect.m_Size[0] - 1;
		const int y2 = rect.m_Position[1] + rect.m_Size[1] - 1;

		Assert(x1 <= x2);
		Assert(y1 <= y2);
		Assert(x1 >= 0 && x1 < mEditingBuffer.Sx_get());
		Assert(y1 >= 0 && y1 < mEditingBuffer.Sy_get());
		Assert(x2 >= 0 && x2 < mEditingBuffer.Sx_get());
		Assert(y2 >= 0 && y2 < mEditingBuffer.Sy_get());

		for (int y = y1; y <= y2; ++y)
		{
			       Rgba * __restrict lineBack = mEditingBuffer.Line_get(y) + x1;
			const float * __restrict lineBrush = mBmpBrush.Line_get(y) + x1;

			for (int x = x1; x <= x2; ++x)
			{
				const float aBrush = (*lineBrush) * mBrushOpacity;
				const float aBack = 1.0f - aBrush;

				for (int i = 0; i < 4; ++i)
				{
					lineBack->rgb[i] = lineBack->rgb[i] * aBack + mBrushColor.rgb[i] * aBrush;
				}

				lineBack++;
				lineBrush++;
			}

			ClearMemory(mBmpBrush.Line_get(y) + x1, rect.m_Size[0] * sizeof(float));
		}
	}
	UsingEnd()
}

void LayerMgr::FlattenEraser(const RectI & rect)
{
	UsingBegin(Benchmark bm("LayerMgr::FlattenEraser"))
	{
		LOG_DBG("flattenEraser", 0);
		
		Assert(mMode == LayerMode_Eraser);
		Assert(mEditingBuffer.Sx_get() == mBmpBrush.Sx_get());
		Assert(mEditingBuffer.Sy_get() == mBmpBrush.Sy_get());
		
		const int x1 = rect.m_Position[0];
		const int y1 = rect.m_Position[1];
		const int x2 = rect.m_Position[0] + rect.m_Size[0] - 1;
		const int y2 = rect.m_Position[1] + rect.m_Size[1] - 1;

		Assert(x1 <= x2);
		Assert(y1 <= y2);
		Assert(x1 >= 0 && x1 < mEditingBuffer.Sx_get());
		Assert(y1 >= 0 && y1 < mEditingBuffer.Sy_get());
		Assert(x2 >= 0 && x2 < mEditingBuffer.Sx_get());
		Assert(y2 >= 0 && y2 < mEditingBuffer.Sy_get());

		for (int y = y1; y <= y2; ++y)
		{
			 Rgba * __restrict lineBack = mEditingBuffer.Line_get(y);
			float * __restrict lineBrush = mBmpBrush.Line_get(y);

			for (int x = x1; x <= x2; ++x)
			{
				Rgba & pixBack = lineBack[x];
				
				const float aBrush = lineBrush[x] * mBrushOpacity;
				const float aBack = 1.0f - aBrush;

				for (int i = 0; i < 4; ++i)
				{
					pixBack.rgb[i] = pixBack.rgb[i] * aBack + 0.0f * aBrush;
				}
			}

			ClearMemory(lineBrush + rect.m_Position[0], rect.m_Size[0] * sizeof(float));
		}
	}
	UsingEnd()
}

void LayerMgr::ClearBrushOverlay(const RectI & rect)
{
	UsingBegin(Benchmark bm("LayerMgr::ClearBrushOverlay"))
	{
		LOG_DBG("clearBrushOverlay", 0);
		
		const int y1 = rect.m_Position[1];
		const int y2 = rect.m_Position[1] + rect.m_Size[1] - 1;

		Assert(y1 <= y2);
		Assert(y1 >= 0 && y1 < mBmpBrush.Sy_get());
		Assert(y2 >= 0 && y2 < mBmpBrush.Sy_get());

		for (int y = y1; y <= y2; ++y)
		{
			float * __restrict line = mBmpBrush.Line_get(y);

			ClearMemory(line + rect.m_Position[0], rect.m_Size[0] * sizeof(float));
		}
	}
	UsingEnd()
}

void LayerMgr::DataLayerAcquire(const int index, const MacImage * image)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerAcquire"))
	{
		MacImage * dst = DataLayer_get(index);
		
		Assert(image->Sx_get() == dst->Sx_get());
		Assert(image->Sy_get() == dst->Sy_get());
		
		bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();
		
		image->Blit(dst);
		
		if (isEditing)
			EditingBegin(index != ActiveDataLayer_get());
	}
	UsingEnd()
}

void LayerMgr::DataLayerAcquireWithTransform(const int index, MacImage * image, const BlitTransform & transform)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerAcquire"))
	{
		MacImage * dst = DataLayer_get(index);
		
		MacImage temp;
		
		temp.Size_set(dst->Sx_get(), dst->Sy_get(), false);
		temp.Clear(MacRgba_Make(0, 0, 0, 0));
		
		image->Blit_Transformed(&temp, transform);
		
		temp.FlipY_InPlace();
		
		DataLayerAcquire(index, &temp);
	}
	UsingEnd()
}

void LayerMgr::DataLayerClear(const int index, const Rgba & color)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerClear"))
	{
		const bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();
		
		MacImage* dst = DataLayer_get(index);
		
		MacRgba color2;
		
		color2.rgba[0] = (uint8_t)(color.rgb[0] * 255.0f);
		color2.rgba[1] = (uint8_t)(color.rgb[1] * 255.0f);
		color2.rgba[2] = (uint8_t)(color.rgb[2] * 255.0f);
		color2.rgba[3] = (uint8_t)(color.rgb[3] * 255.0f);
		
		dst->Clear(color2);

		if (isEditing)
			EditingBegin(index != ActiveDataLayer_get());
	}
	UsingEnd()
}

void LayerMgr::DataLayerMerge(const int index1, const int index2)
{
	UsingBegin(Benchmark bm("LayerMgr::DataLayerMerge"))
	{
		const bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();
		
		MacImage* src = DataLayer_get(index1);
		MacImage* dst = DataLayer_get(index2);
		
//		const int index = mLayerOrder[layer1];

		const int opacity = mLayerOpacity[index1];
		
		src->BlitAlpha(dst, opacity);
		
		src->Clear(MacRgba_Make(0, 0, 0, 0));
		
		if (isEditing)
			EditingBegin(true);
	}
	UsingEnd()
}

void LayerMgr::Invalidate(const int x, const int y, const int sx, const int sy)
{
	if (sx == 0 || sy == 0)
		return;
	
	const AreaI area(Vec2I(x, y), Vec2I(x + sx - 1, y + sy - 1));

	mEditingDirty.Merge(area);
}

AreaI LayerMgr::Validate()
{
	AreaI result = mEditingDirty;

	if (mEditingDirty.IsSet_get())
	{
		ValidateVisible(
			mEditingDirty.m_Min[0],
			mEditingDirty.m_Min[1],
			mEditingDirty.m_Max[0] - mEditingDirty.m_Min[0] + 1,
			mEditingDirty.m_Max[1] - mEditingDirty.m_Min[1] + 1);
		
		mEditingDirty.Reset();
	}

	return result;
}

Vec2I LayerMgr::Size_get() const
{
	return Vec2I(mMerged.Sx_get(), mMerged.Sy_get());
}

MacImage* LayerMgr::DataLayer_get(const int index)
{
	Assert(index >= 0 && index < mLayerCount);
	
	return mLayerList + index;
}

MacImage* LayerMgr::Layer_get(const int layer)
{
	Assert(layer >= 0 && layer < mLayerCount);
	
	const int index = mLayerOrder[layer];
	
	return DataLayer_get(index);
}

int LayerMgr::LayerCount_get() const
{
	return mLayerCount;
}

void LayerMgr::LayerOrder_set(const std::vector<int> & order)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerOrder_set"))
	{
		Assert((int)order.size() == mLayerCount);

		const bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();
		
		for (int i = 0; i < mLayerCount; ++i)
			mLayerOrder[i] = order[i];
		
		if (isEditing)
			EditingBegin(true);
	}
	UsingEnd()
}

std::vector<int> LayerMgr::LayerOrder_get() const
{
	std::vector<int> result;
	
	for (int i = 0; i < mLayerCount; ++i)
		result.push_back(mLayerOrder[i]);
	
	return result;
}

int LayerMgr::LayerOrder_get(const int layer) const
{
	return mLayerOrder[layer];
}

void LayerMgr::DataLayerOpacity_set(const int index, const float _opacity)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerOpacity_set"))
	{
		const bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();

		const float opacity = Calc::Mid(_opacity, 0.0f, 1.0f);
		
		const int opacity255 = Calc::Mid((int)Calc::RoundUp(opacity * 255.0f), 0, 255);
		
		mLayerOpacity[index] = opacity255;
		
		if (isEditing)
			EditingBegin(true);
	}
	UsingEnd()
}

float LayerMgr::DataLayerOpacity_get(const int index) const
{
	return mLayerOpacity[index] / 255.0f;
}

void LayerMgr::DataLayerVisibility_set(const int index, const bool visibility)
{
	UsingBegin(Benchmark bm("LayerMgr::LayerVisibility_set"))
	{
		const bool isEditing = mEditingEnabled;
		
		if (isEditing)
			EditingEnd();
		
		mLayerVisibility[index] = visibility;
		
		if (isEditing)
			EditingBegin(true);
	}
	UsingEnd()
}

bool LayerMgr::DataLayerVisibility_get(const int index) const
{
	return mLayerVisibility[index];
}

/*MacImage* LayerMgr::Back_get()
{
	return &mCacheBack;
}*/

MacImage * LayerMgr::Merged_get()
{
	return &mMerged;
}

Bitmap * LayerMgr::EditingBuffer_get()
{
	return &mEditingBuffer;
}

const Bitmap * LayerMgr::EditingBuffer_get() const
{
	return &mEditingBuffer;
}

Filter * LayerMgr::EditingBrush_get()
{
	return &mBmpBrush;
}

const Rgba & LayerMgr::BrushColor_get() const
{
	return mBrushColor;
}

void LayerMgr::BrushColor_set(const Rgba & color)
{
	mBrushColor = color;
	mBrushColor.rgb[3] = 1.0f;
}

float LayerMgr::BrushOpacity_get() const
{
	return mBrushOpacity;
}

void LayerMgr::BrushOpacity_set(const float _opacity)
{
	const float opacity = Calc::Mid(_opacity, 0.0f, 1.0f);
	const int opacity255 = Calc::Mid((int)Calc::RoundUp(opacity * 255.0f), 0, 255);
	
	mBrushOpacity = opacity;
	mBrushOpacity255i = opacity255;
	mBrushOpacity255f = opacity * 255.0f;
}

MacRgba LayerMgr::BackColor1_get() const
{
	return mBackColor1;
}

MacRgba LayerMgr::BackColor2_get() const
{
	return mBackColor2;
}

void LayerMgr::ValidateVisible(const int _x, const int _y, const int sx, const int sy)
{
	UsingBegin(Benchmark bm("LayerMgr::ValidateVisible"))

	LOG_DBG("validateVisible: %d, %d - %d, %d", _x, _y, sx, sy);
	
	const int x1 = _x;
	const int y1 = _y;
	const int x2 = _x + sx - 1;
	const int y2 = _y + sy - 1;
	
	Assert(x1 >= 0 && x1 < mEditingBuffer.Sx_get());
	Assert(y1 >= 0 && y1 < mEditingBuffer.Sy_get());
	Assert(x2 >= 0 && x2 < mEditingBuffer.Sx_get());
	Assert(y2 >= 0 && y2 < mEditingBuffer.Sy_get());

	Assert(mEditingBuffer.Sx_get() == mBmpBrush.Sx_get());
	Assert(mEditingBuffer.Sy_get() == mBmpBrush.Sy_get());
	Assert(mEditingBuffer.Sx_get() == mMerged.Sx_get());
	Assert(mEditingBuffer.Sy_get() == mMerged.Sy_get());

	switch (mMode)
	{
	case LayerMode_Undefined:
	case LayerMode_Brush:
		{
//			LOG_DBG("-> brush", 0);
			
			mCacheBack.Blit(&mMerged, x1, y1, x1, y1, sx, sy);
			
			MacRgba color;
			for (int i = 0; i < 4; ++i)
				color.rgba[i] = (uint8_t)(mBrushColor.rgb[i] * 255.0f);
			
			const MacImage* srcEdit = DataLayer_get(mActiveDataLayer);
			
			const int alphaLayer = mLayerOpacity[mActiveDataLayer];
			
			for (int y = y1; y <= y2; ++y)
			{
				const MacRgba * __restrict srcEditLine = srcEdit->Line_get(y) + x1;
				const   float * __restrict srcBrushLine = mBmpBrush.Line_get(y) + x1;
				MacRgba* dstLine = mMerged.Line_get(y) + x1;

				for (int xi = sx; xi != 0; --xi)
				{
					const int alphaBrush = int((*srcBrushLine) * mBrushOpacity255f);
					const int alphaBrushInv = 255 - alphaBrush;
					
					uint8_t * __restrict dst = dstLine->rgba;
					
					int temp[4];
					
					for (int i = 0; i < 4; ++i)
					{
						temp[i] = ((srcEditLine->rgba[i] * alphaBrushInv + color.rgba[i] * alphaBrush) * alphaLayer) >> 16;
					}
					
					const int alphaTempInv = 255 - temp[3];
					
					for (int i = 0; i < 4; ++i)
					{
						dst[i] = ((dst[i] * alphaTempInv) >> 8) + temp[i];
					}
					
					srcEditLine++;
					srcBrushLine++;
					dstLine++;
				}
			}
			break;
		}

	case LayerMode_Eraser:
		{
//			LOG_DBG("-> eraser", 0);
			
			const int alphaLayer = mLayerOpacity[ActiveDataLayer_get()];
			
			const MacImage * srcEdit = DataLayer_get(mActiveDataLayer);
			
			mCacheBack.Blit(&mMerged, x1, y1, x1, y1, sx, sy);
			
			for (int y = y1; y <= y2; ++y)
			{
				const MacRgba * __restrict srcEditLine2 = srcEdit->Line_get(y) + x1;
				const   float * __restrict srcBrushLine = mBmpBrush.Line_get(y) + x1;
				      MacRgba * __restrict dstLine = mMerged.Line_get(y) + x1;

				for (int x = x1; x <= x2; ++x)
				{
					const int alphaBrush = int((*srcBrushLine) * mBrushOpacity255f);
					const int alphaBrushInv = 255 - alphaBrush;
					
					uint8_t * __restrict dst = dstLine->rgba;
					
					int temp[4];
					
					for (int i = 0; i < 4; ++i)
					{
						temp[i] = (srcEditLine2->rgba[i] * alphaBrushInv * alphaLayer) >> 16;
					}
					
					const int alphaTempInv = 255 - temp[3];
					
					for (int i = 0; i < 4; ++i)
					{
						dst[i] = ((dst[i] * alphaTempInv) >> 8) + temp[i];
					}

					srcEditLine2++;
					srcBrushLine++;
					dstLine++;
				}
			}
			break;
		}

	case LayerMode_Direct:
		{
//			LOG_DBG("-> direct", 0);

			const float alphaLayer = (float)mLayerOpacity[ActiveDataLayer_get()];
			
			for (int y = y1; y <= y2; ++y)
			{
				const MacRgba * __restrict srcBackLine = mCacheBack.Line_get(y) + x1;
				const    Rgba * __restrict srcEditLine = mEditingBuffer.Line_get(y) + x1;
				      MacRgba * __restrict dstLine = mMerged.Line_get(y) + x1;

				for (int x = x1; x <= x2; ++x)
				{
					const uint8_t * __restrict srcBack = srcBackLine->rgba;
					const   float * __restrict srcEdit = srcEditLine->rgb;
					
					const int alphaEditInv = 255 - int(srcEdit[3] * alphaLayer);
					
					//uint8_t* dst = dstLine->rgba;

#if defined(DEBUG) && 0
					for (int i = 0; i < 3; ++i)
					{
						if (srcEdit[i] > srcEdit[3])
							printf("[%d] %g > %g\n", i, srcEdit[i], srcEdit[3]);
						Assert(srcEdit[i] <= srcEdit[3]);
					}
#endif
					
					const int r = ((srcBack[0] * alphaEditInv) >> 8) + (int)(srcEdit[0] * alphaLayer);
					const int g = ((srcBack[1] * alphaEditInv) >> 8) + (int)(srcEdit[1] * alphaLayer);
					const int b = ((srcBack[2] * alphaEditInv) >> 8) + (int)(srcEdit[2] * alphaLayer);
					const int a = ((srcBack[3] * alphaEditInv) >> 8) + (int)(srcEdit[3] * alphaLayer);
					
					*dstLine = MacRgba_Make(r, g, b, a);
					
					srcBackLine++;
					srcEditLine++;
					dstLine++;
				}
			}
			break;
		}

	default:
		break;
	}
	
	mCacheFront.BlitAlpha(&mMerged, x1, y1, x1, y1, sx, sy);
	
	UsingEnd()
}

void LayerMgr::ValidateLayer(const int _x, const int _y, const int sx, const int sy)
{
	UsingBegin(Benchmark bm("LayerMgr::ValidateLayer"))

	LOG_DBG("validateLayer: %d, %d - %d, %d", _x, _y, sx, sy);
	
	const int x1 = _x;
	const int y1 = _y;
	const int x2 = _x + sx - 1;
	const int y2 = _y + sy - 1;
	
	Assert(x1 >= 0 && x1 < mEditingBuffer.Sx_get());
	Assert(y1 >= 0 && y1 < mEditingBuffer.Sy_get());
	Assert(x2 >= 0 && x2 < mEditingBuffer.Sx_get());
	Assert(y2 >= 0 && y2 < mEditingBuffer.Sy_get());
	
	MacImage * dst = DataLayer_get(mActiveDataLayer);
	
	for (int y = y1; y <= y2; ++y)
	{
		const    Rgba * __restrict srcLine = mEditingBuffer.Line_get(y);
		      MacRgba * __restrict dstLine = dst->Line_get(y);
		
#if defined(__ARM_NEON__) && 1
		// 0.036 sec @ 1024x768 (iPad2)
		
		const uint32_t x1a = x1 >> 1;
		const uint32_t x2a = x2 >> 1;
		srcLine += (x1a << 1);
		dstLine += (x1a << 1);
		const float32x4_t scale = vdupq_n_f32(255.0f);
		for (uint32_t x = x1a; x <= x2a; ++x)
		{
			float32x4_t f1 = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine    )), scale);
			float32x4_t f2 = vmulq_f32(vld1q_f32(reinterpret_cast<const float32_t*>(srcLine + 1)), scale);
			uint32x4_t v1 = vcvtq_u32_f32(f1);
			uint32x4_t v2 = vcvtq_u32_f32(f2);
			uint16x4_t v1_16 = vmovn_u32(v1);
			uint16x4_t v2_16 = vmovn_u32(v2);
			uint16x8_t v_16 = vcombine_u16(v1_16, v2_16);
			uint8x8_t v_8 = vmovn_u16(v_16);
			vst1_u8(reinterpret_cast<uint8_t*>(dstLine), v_8);
			srcLine += 2;
			dstLine += 2;
		}
#else
		// 0.058 sec @ 1024x768 (iPad2)
		
		srcLine += x1;
		dstLine += x1;
		
		for (int x = x1; x <= x2; ++x)
		{
			for (int i = 0; i < 4; ++i)
			{
				dstLine->rgba[i] = (uint8_t)(srcLine->rgb[i] * 255.0f);
			}
			
			srcLine++;
			dstLine++;
		}
#endif
	}
	
	UsingEnd()
}

void LayerMgr::CopyLayerToEditingBuffer(const int x, const int y, const int sx, const int sy)
{
	MacImageToBitmap(DataLayer_get(mActiveDataLayer), &mEditingBuffer);
}

void LayerMgr::CopyEditingBufferToLayer(const int x, const int y, const int sx, const int sy)
{
	BitmapToMacImage(&mEditingBuffer, DataLayer_get(mActiveDataLayer), x, y, sx, sy);
}

int LayerMgr::ActiveDataLayer_get() const
{
	return mActiveDataLayer;
}

void LayerMgr::ActiveDataLayer_set(const int index)
{
	const bool isEditing = mEditingEnabled;
	
	if (isEditing)
		EditingEnd();
	
	mActiveDataLayer = index;
	
	if (isEditing)
		EditingBegin(true);
}

/*void LayerMgr::SwapLayerOrder(int layer1, int layer2)
{
	Assert(layer1 >= 0 && layer1 < mLayerCount);
	Assert(layer2 >= 0 && layer2 < mLayerCount);

	std::vector<int> order = LayerOrder_get();
	std::swap(order[layer1], order[layer2]);

	LayerOrder_set(order);
}*/

void LayerMgr::RebuildCaches()
{
	RebuildBack();
	RebuildFront();
}

void LayerMgr::RebuildBack()
{
	UsingBegin(Benchmark bm("LayerMgr::RebuildBack"))
	{
		const int layer1 = 0;
		const int layer2 = IndexToLayer(mActiveDataLayer) - 1;
		
		RenderMerged(mCacheBack, mBackColor1, mBackColor2, layer1, layer2);
	}
	UsingEnd()
}

void LayerMgr::RebuildFront()
{
	UsingBegin(Benchmark bm("LayerMgr::RebuildFront"))
	{
		const MacRgba color = MacRgba_Make(0, 0, 0, 0);
		
		const int layer1 = IndexToLayer(mActiveDataLayer) + 1;
		const int layer2 = mLayerCount - 1;
		
		RenderMerged(mCacheFront, color, color, layer1, layer2);
	}
	UsingEnd()
}

void LayerMgr::EditingBegin(const bool rebuildCaches)
{
	Assert(!mEditingEnabled);
	
#if defined(DEBUG) && defined(IPHONEOS)
	sleep(1);
#endif
	
	UsingBegin(Benchmark bm("LayerMgr::EditingBegin"))
	{
		mEditingEnabled = true;

		if (mLayerCount == 0)
			return;
		
		// copy active layer to FP buffer
		
		MacImageToBitmap(&mLayerList[mActiveDataLayer], &mEditingBuffer);
		
		if (rebuildCaches)
		{
			// rebuild backdrop and overlay images

			RebuildCaches();
		}

		// update merged layer

		Invalidate(0, 0, mMerged.Sx_get(), mMerged.Sy_get());
//		ValidateVisible(0, 0, mMerged.Sx_get(), mMerged.Sy_get());
	}
	UsingEnd()
}

void LayerMgr::EditingEnd()
{
	Assert(mEditingEnabled);
	
	UsingBegin(Benchmark bm("LayerMgr::EditingEnd"))
	{
		mEditingEnabled = false;

		// copy FP buffer to active layer

		BitmapToMacImage(&mEditingBuffer, &mLayerList[mActiveDataLayer]);
	}
	UsingEnd()
}

bool LayerMgr::EditingIsEnabled_get() const
{
	return mEditingEnabled;
}

/*AreaI& LayerMgr::EditingDirty_get()
{
	return mEditingDirty;
}*/

int LayerMgr::IndexToLayer(const int index) const
{
	Assert(index >= 0 && index < mLayerCount);
	
	for (int i = 0; i < mLayerCount; ++i)
		if (mLayerOrder[i] == index)
			return i;
	
	throw ExceptionNA();
}

int LayerMgr::LayerToIndex(const int layer) const
{
	return mLayerOrder[layer];
}
