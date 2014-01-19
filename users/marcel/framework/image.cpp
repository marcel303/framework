
// Copyright (C) 2013 Grannies Games - All rights reserved

#ifdef WIN32
	#include <Windows.h>
#endif
#include <FreeImage.h>
#include "framework.h"
#include "image.h"

ImageData * loadImage(const char * filename)
{
	FIBITMAP * bmp = FreeImage_Load(FreeImage_GetFileType(filename), filename);
	
	if (!bmp)
	{
		//logError("failed to load image: %s", filename);
		return 0;
	}
	
	FIBITMAP * bmp32 = FreeImage_ConvertTo32Bits(bmp);
	
	if (!bmp32)
	{
		logError("failed to convert image to 32 bpp: %s", filename);
		return 0;
	}

	const int sx = FreeImage_GetWidth(bmp32);
	const int sy = FreeImage_GetHeight(bmp32);

	void * data = malloc(sx * sy * 4);
	char * dest = (char*)data;
	
	for (int y = 0; y < sy; ++y)
	{
		uint32_t * source = (uint32_t*)FreeImage_GetScanLine(bmp32, y);
		
		for (int x = 0; x < sx; ++x)
		{
			const int r = ((*source) >> FI_RGBA_RED_SHIFT) & 0xff;
			const int g = ((*source) >> FI_RGBA_GREEN_SHIFT) & 0xff;
			const int b = ((*source) >> FI_RGBA_BLUE_SHIFT) & 0xff;
			const int a = ((*source) >> FI_RGBA_ALPHA_SHIFT) & 0xff;

			*dest++ = r;
			*dest++ = g;
			*dest++ = b;
			*dest++ = a;
			
			source++;
		}
	}
	
	FreeImage_Unload(bmp32);
	FreeImage_Unload(bmp);
	
	ImageData * imageData = new ImageData;
	imageData->sx = sx;
	imageData->sy = sy;
	imageData->imageData = data;
	
	return imageData;
}
