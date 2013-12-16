#pragma once

#include "Types.h"

typedef struct ScreenshotPixel
{
	union
	{
		struct
		{
			uint8_t rgba[4];
		};
		uint32_t c;
	};
} ScreenshotPixel;

class Screenshot
{
public:
	Screenshot();
	~Screenshot();
	
	void Setup(int sx, int sy);
	
	ScreenshotPixel* mPixels;
	int mSx;
	int mSy;
	
	inline const ScreenshotPixel* GetPixel(int x, int y) const
	{
		return mPixels + y * mSx + x;
	}
	
	inline ScreenshotPixel* GetPixel(int x, int y)
	{
		return mPixels + y * mSx + x;
	}
	
//	Screenshot* FlipX();
	Screenshot* FlipY();
	Screenshot* FlipXY();
	Screenshot* RotateCW90();
	Screenshot* RotateCCW90();
	Screenshot* Copy();
};

namespace ScreenshotUtil
{
	Screenshot* CaptureGL(int sx, int sy);
}
