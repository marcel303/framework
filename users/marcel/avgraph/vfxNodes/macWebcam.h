#pragma once

#include <stdint.h>

struct MacWebcamImage
{
	static const int kMaxSx = 1920;
	static const int kMaxSy = 1080;
	static const int kMaxPixels = kMaxSx * kMaxSy;
	
	int sx;
	int sy;
	int pitch;
	
	int index;
	
	uint32_t pixels[kMaxPixels];
	
	MacWebcamImage()
		: sx(0)
		, sy(0)
		, pitch(0)
		, index(-1)
	{
	}
};

struct MacWebcam
{
	void * webcamImpl;
	
	MacWebcamImage image;
	bool gotImage;

	MacWebcam();

	bool init();
	void shut();
};
