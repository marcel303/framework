/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "image.h"
#include "MemAlloc.h"
#ifdef WIN32
	#include <Windows.h>
#endif
#include <FreeImage.h>

//

ImageData::ImageData()
{
	memset(this, 0, sizeof(ImageData));
}

ImageData::ImageData(int sx, int sy)
{
	memset(this, 0, sizeof(ImageData));

	this->sx = sx;
	this->sy = sy;

	imageData = (ImageData::Pixel*)MemAlloc(sx * sy * 4, 16);
}

ImageData::~ImageData()
{
	if (imageData)
	{
		MemFree(imageData);
		imageData = 0;
		
		sx = sy = 0;
	}
}

//

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

		data = MemAlloc(sx * sy * 4, 16);
		char * __restrict dest = (char*)data;
		
		for (int y = 0; y < sy; ++y)
		{
			const uint8_t * __restrict source = (uint8_t*)FreeImage_GetScanLine(bmp, sy - 1 - y);
			
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

		data = MemAlloc(sx * sy * 4, 16);
		char * __restrict dest = (char*)data;
		
		for (int y = 0; y < sy; ++y)
		{
			const uint32_t * __restrict source = (uint32_t*)FreeImage_GetScanLine(bmp32, sy - 1 - y);
			
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
	
	ImageData * imageData = new ImageData();
	imageData->sx = sx;
	imageData->sy = sy;
	imageData->imageData = (ImageData::Pixel*)data;
	
	return imageData;
}

ImageData *  imagePremultiplyAlpha(const ImageData * image)
{
	const int numPixels = image->sx * image->sy;
	
	ImageData * result = new ImageData();
	result->sx = image->sx;
	result->sy = image->sy;
	result->imageData = (ImageData::Pixel*)MemAlloc(numPixels * 4, 16);

	for (int i = 0; i < numPixels; ++i)
	{
		result->imageData[i].r = image->imageData[i].r * image->imageData[i].a / 255;
		result->imageData[i].g = image->imageData[i].g * image->imageData[i].a / 255;
		result->imageData[i].b = image->imageData[i].b * image->imageData[i].a / 255;
		result->imageData[i].a = image->imageData[i].a;
	}

	return result;
}

static bool getPixel(const ImageData * image, const int x, const int y, ImageData::Pixel & pixel)
{
	if (x < 0 || y < 0 || x >= image->sx || y >= image->sy)
		return false;
	else
	{
		pixel = image->getLine(y)[x];
		return true;
	}
}

ImageData * imageFixAlphaFilter(const ImageData * image)
{
	ImageData * result = new ImageData();
	result->sx = image->sx;
	result->sy = image->sy;
	result->imageData = (ImageData::Pixel*)MemAlloc(image->sx * image->sy * 4, 16);

	for (int y = 0; y < image->sy; ++y)
	{
		for (int x = 0; x < image->sx; ++x)
		{
			const ImageData::Pixel * __restrict srcLine = image->getLine(y);
				  ImageData::Pixel * __restrict dstLine = result->getLine(y);
			
			if (srcLine[x].a != 0)
			{
				dstLine[x] = srcLine[x];
			}
			else
			{
				int tempR = 0;
				int tempG = 0;
				int tempB = 0;
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
								tempR += pixel.r;
								tempG += pixel.g;
								tempB += pixel.b;

								numAdded++;
							}
						}
					}
				}

				if (numAdded != 0)
				{
					tempR /= numAdded;
					tempG /= numAdded;
					tempB /= numAdded;

					dstLine[x].r = tempR;
					dstLine[x].g = tempG;
					dstLine[x].b = tempB;
					dstLine[x].a = 0;
				}
				else
				{
					dstLine[x] = srcLine[x];
				}
			}
		}
	}

	return result;
}
