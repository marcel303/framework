
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
	
	const int sx = FreeImage_GetWidth(bmp);
	const int sy = FreeImage_GetHeight(bmp);
	
	void * data = nullptr;

	BITMAPINFO * info = FreeImage_GetInfo(bmp);
	
	if (info->bmiHeader.biBitCount == 24)
	{
		const int sx = FreeImage_GetWidth(bmp);
		const int sy = FreeImage_GetHeight(bmp);

		data = malloc(sx * sy * 4);
		char * dest = (char*)data;
		
		for (int y = 0; y < sy; ++y)
		{
			uint8_t * source = (uint8_t*)FreeImage_GetScanLine(bmp, y);
			
			for (int x = 0; x < sx; ++x)
			{
				const uint8_t r = source[2];
				const uint8_t g = source[1];
				const uint8_t b = source[0];
				const uint8_t a = 255;

				*dest++ = r;
				*dest++ = g;
				*dest++ = b;
				*dest++ = a;
				
				source += 3;
			}
		}
	}
	else
	{
		FIBITMAP * bmp32 = FreeImage_ConvertTo32Bits(bmp);
		
		if (!bmp32)
		{
			logError("failed to convert image to 32 bpp: %s", filename);
			return 0;
		}

		data = malloc(sx * sy * 4);
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
	}
	
	FreeImage_Unload(bmp);
	
	ImageData * imageData = new ImageData;
	imageData->sx = sx;
	imageData->sy = sy;
	imageData->imageData = (ImageData::Pixel*)data;
	
	return imageData;
}

ImageData *  imagePremultiplyAlpha(ImageData * image)
{
	const int numPixels = image->sx * image->sy;
	ImageData * result = new ImageData();
	result->sx = image->sx;
	result->sy = image->sy;
	result->imageData = new ImageData::Pixel[numPixels];

	for (int i = 0; i < numPixels; ++i)
	{
		result->imageData[i].r = image->imageData[i].r * image->imageData[i].a / 255;
		result->imageData[i].g = image->imageData[i].g * image->imageData[i].a / 255;
		result->imageData[i].b = image->imageData[i].b * image->imageData[i].a / 255;
		result->imageData[i].a = image->imageData[i].a;
	}

	return result;
}

static bool getPixel(ImageData * image, int x, int y, ImageData::Pixel & pixel)
{
	if (x < 0 || y < 0 || x >= image->sx || y >= image->sy)
		return false;
	else
	{
		pixel = image->getLine(y)[x];
		return true;
	}
}

ImageData * imageFixAlphaFilter(ImageData * image)
{
	ImageData * result = new ImageData();
	result->sx = image->sx;
	result->sy = image->sy;
	result->imageData = new ImageData::Pixel[image->sx * image->sy];

	for (int x = 0; x < image->sx; ++x)
	{
		for (int y = 0; y < image->sy; ++y)
		{
			if (image->getLine(y)[x].a != 0)
			{
				result->getLine(y)[x] = image->getLine(y)[x];
			}
			else
			{
				ImageData::Pixel temp;
				int numAdded = 0;

				for (int dx = -1; dx <= +1; ++dx)
				{
					for (int dy = -1; dy <= +1; ++dy)
					{
						if (dx == 0 && dy == 0)
							continue;

						ImageData::Pixel pixel;

						if (getPixel(image, x + dx, y + dy, pixel))
						{
							if (pixel.a != 0)
							{
								if (numAdded == 0)
								{
									temp = pixel;
								}
								else
								{
									temp.r += pixel.r;
									temp.g += pixel.g;
									temp.b += pixel.b;
									temp.a += pixel.a;
								}

								numAdded++;
							}
						}
					}
				}

				if (numAdded != 0)
				{
					temp.r /= numAdded;
					temp.g /= numAdded;
					temp.b /= numAdded;
					temp.a = 0;

					result->getLine(y)[x] = temp;
				}
				else
				{
					result->getLine(y)[x] = image->getLine(y)[x];
				}
			}
		}
	}

	return result;
}
