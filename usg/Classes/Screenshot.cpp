#include <string.h>
#include "OpenGLCompat.h"
#include "Screenshot.h"

Screenshot::Screenshot()
{
	mPixels = 0;
	mSx = 0;
	mSy = 0;
}

Screenshot::~Screenshot()
{
	Setup(0, 0);
}

void Screenshot::Setup(int sx, int sy)
{
	delete[] mPixels;
	mSx = 0;
	mSy = 0;
	
	int area = sx * sy;
	
	if (area > 0)
	{
		mPixels = new ScreenshotPixel[area];
		mSx = sx;
		mSy = sy;
	}
}

Screenshot* Screenshot::FlipY()
{
	Screenshot* ss = new Screenshot();
	ss->Setup(mSx, mSy);
	
	for (int y = 0; y < mSy; ++y)
	{
		ScreenshotPixel* src = GetPixel(0, y);
		ScreenshotPixel* dst = ss->GetPixel(0, mSy - y - 1);
		
		memcpy(dst, src, sizeof(ScreenshotPixel) * mSx);
	}
	
	return ss;
}

Screenshot* Screenshot::FlipXY()
{
	Screenshot* ss = new Screenshot();
	ss->Setup(mSy, mSx);
	
	for (int x = 0; x < mSx; ++x)
	{
		for (int y = 0; y < mSy; ++y)
		{
			ScreenshotPixel* src = GetPixel(x, y);
			ScreenshotPixel* dst = ss->GetPixel(y, x);
			
			dst->c = src->c;
		}
	}
	
	return ss;
}

Screenshot* Screenshot::RotateCW90()
{
	Screenshot* ss = new Screenshot();
	ss->Setup(mSy, mSx);
	
	const ScreenshotPixel* src = GetPixel(0, 0);
	
	for (int y = 0; y < mSy; ++y)
	{
		for (int x = 0; x < mSx; ++x)
		{
			ScreenshotPixel* dst = ss->GetPixel(y, x);
			
			dst->c = src->c;
			
			src++;
		}
	}
	
	return ss;
}

Screenshot* Screenshot::RotateCCW90()
{
	Screenshot* ss = new Screenshot();
	ss->Setup(mSy, mSx);
	
	const ScreenshotPixel* src = GetPixel(0, 0);
	
	for (int y = 0; y < mSy; ++y)
	{
		for (int x = 0; x < mSx; ++x)
		{
			ScreenshotPixel* dst = ss->GetPixel(mSy-1-y, mSx-1-x);
			
			dst->c = src->c;
			
			src++;
		}
	}
	
	return ss;
}

Screenshot* Screenshot::Copy()
{
	Screenshot* ss = new Screenshot();
	ss->Setup(mSx, mSy);
	
	memcpy(ss->mPixels, mPixels, mSx * mSy * sizeof(ScreenshotPixel));
	
	return ss;
}

namespace ScreenshotUtil
{
	Screenshot* CaptureGL(int sx, int sy)
	{
		Screenshot* ss = new Screenshot();
		ss->Setup(sx, sy);
		
		glReadPixels(0, 0, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, ss->mPixels);
		
		return ss;
	}
}
