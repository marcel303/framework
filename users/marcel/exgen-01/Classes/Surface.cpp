#include "Surface.h"

Surface::Surface()
{
	mSx = 0;
	mSy = 0;
	mPixels = 0;
}

Surface::~Surface()
{
	Setup(0, 0);
}

void Surface::Setup(int sx, int sy)
{
	delete[] mPixels;
	mPixels = 0;
	mSx = 0;
	mSy = 0;
	
	if (sx * sy > 0)
	{
		mPixels = new Pixel[sx * sy];
		mSx = sx;
		mSy = sy;
	}
}

void Surface::Pixel_set(int x, int y, Pixel pixel)
{
	int index = x + y * mSx;
	
	mPixels[index] = pixel;
}
