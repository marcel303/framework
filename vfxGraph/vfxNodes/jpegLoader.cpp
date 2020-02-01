/*
	Copyright (C) 2020 Marcel Smit
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

#if ENABLE_TURBOJPEG

#include "Debugging.h"
#include "jpegLoader.h"
#include "Log.h"
#include <stdint.h>
#include <stdio.h>

#if defined(LINUX)
	#include <turbojpeg.h>
#else
	#include <turbojpeg/turbojpeg.h>
#endif

JpegLoadData::JpegLoadData()
	: flipY(false)
	, buffer(nullptr)
	, bufferSize(0)
	, sx(0)
	, sy(0)
{
}

JpegLoadData::~JpegLoadData()
{
	free();
}

void JpegLoadData::disownBuffer()
{
	buffer = 0;
	bufferSize = 0;
}

void JpegLoadData::free()
{
	delete [] buffer;
	buffer = nullptr;
	bufferSize = 0;

	sx = 0;
	sy = 0;
}

//

static bool loadFileContents(const char * filename, void * bytes, int & numBytes)
{
	bool result = true;
	
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		// load source from file
		
		fseek(file, 0, SEEK_END);
		const int fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		if (numBytes < fileSize)
		{
			result = false;
		}
		else
		{
			numBytes = fileSize;
			
			if (fread(bytes, numBytes, 1, file) != (size_t)1)
			{
				result = false;
			}
		}
		
		fclose(file);
	}
	
	if (!result)
	{
		numBytes = 0;
	}
	
	return result;
}

//

bool loadImage_turbojpeg(const void * buffer, const int bufferSize, const void * dstBuffer, const int dstBufferSize, const bool decodeGrayscale)
{
	bool result = true;
	
	tjhandle h = tjInitDecompress();
	
	if (h == nullptr)
	{
		LOG_ERR("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		int sx = 0;
		int sy = 0;
		
		int jpegSubsamp = 0;
		int jpegColorspace = 0;
		
		if (tjDecompressHeader3(h, (unsigned char*)buffer, (unsigned long)bufferSize, &sx, &sy, &jpegSubsamp, &jpegColorspace) != 0)
		{
			LOG_ERR("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			const TJPF pixelFormat = decodeGrayscale ? TJPF_GRAY : TJPF_RGBX;
			
			const int pitch = sx * tjPixelSize[pixelFormat];
			
			const int flags = 0;
			
			const int requiredBufferSize = pitch * sy;
			
			Assert(requiredBufferSize <= dstBufferSize);
			(void)requiredBufferSize;
			
			if (tjDecompress2(h, (unsigned char*)buffer, (unsigned long)bufferSize, (unsigned char*)dstBuffer, sx, pitch, sy, pixelFormat, flags) != 0)
			{
				LOG_ERR("turbojpeg: %s", tjGetErrorStr());
				
				result = false;
			}
			else
			{
				//logDebug("decoded jpeg!");
			}
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data, const bool decodeGrayscale)
{
	bool result = true;
	
	tjhandle h = tjInitDecompress();
	
	if (h == nullptr)
	{
		LOG_ERR("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		int sx = 0;
		int sy = 0;
		
		int jpegSubsamp = 0;
		int jpegColorspace = 0;
		
		if (tjDecompressHeader3(h, (unsigned char*)buffer, (unsigned long)bufferSize, &sx, &sy, &jpegSubsamp, &jpegColorspace) != 0)
		{
			LOG_ERR("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			data.sx = sx;
			data.sy = sy;
			
			const TJPF pixelFormat = decodeGrayscale ? TJPF_GRAY : TJPF_RGBX;
			
			const int pitch = sx * tjPixelSize[pixelFormat];
			
			const int flags = TJFLAG_BOTTOMUP * (data.flipY ? 1 : 0);
			
			const int requiredBufferSize = pitch * sy;
			
			if (data.buffer == nullptr || data.bufferSize != requiredBufferSize)
			{
				delete [] data.buffer;
				data.buffer = nullptr;
				data.bufferSize = 0;
				
				//
				
				data.buffer = new unsigned char[requiredBufferSize];
				data.bufferSize = requiredBufferSize;
			}
			
			if (tjDecompress2(h, (unsigned char*)buffer, (unsigned long)bufferSize, (unsigned char*)data.buffer, sx, pitch, sy, pixelFormat, flags) != 0)
			{
				LOG_ERR("turbojpeg: %s", tjGetErrorStr());
				
				result = false;
			}
			else
			{
				//logDebug("decoded jpeg!");
			}
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, void * fileBuffer, int fileBufferSize, const bool decodeGrayscale)
{
	bool result = true;
	
	if (loadFileContents(filename, fileBuffer, fileBufferSize) == false)
	{
		LOG_DBG("turbojpeg: %s", "failed to load file contents");
		
		result = false;
	}
	else
	{
		result = loadImage_turbojpeg(fileBuffer, fileBufferSize, data, decodeGrayscale);
	}
	
	return result;
}

bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, const bool decodeGrayscale)
{
	bool result = true;

	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		// load source from file
		
		fseek(file, 0, SEEK_END);
		const int fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		uint8_t * fileBuffer = new uint8_t[fileSize];
		int fileBufferSize = fileSize;
	
		if (loadFileContents(filename, fileBuffer, fileBufferSize) == false)
		{
			LOG_DBG("turbojpeg: %s", "failed to load file contents");
			
			result = false;
		}
		else
		{
			result = loadImage_turbojpeg(fileBuffer, fileBufferSize, data, decodeGrayscale);
		}

		delete[] fileBuffer;
		fileBuffer = nullptr;
	}
	
	return result;
}

static bool saveImage_turbojpegInternal(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, void *& dstBuffer, int & dstBufferSize, const bool allowAllocation, const bool flipImage)
{
	bool result = true;
	
	tjhandle h = tjInitCompress();
	
	if (h == nullptr)
	{
		LOG_ERR("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		const TJPF pixelFormat = srcIsColor ? TJPF_RGBX : TJPF_GRAY;
		const TJSAMP subsamp = srcIsColor ? TJSAMP_422 : TJSAMP_GRAY;
		const int quality = dstQualityLevel;
		const int flags = (TJFLAG_NOREALLOC * (allowAllocation ? 0 : 1)) | (TJFLAG_BOTTOMUP * (flipImage ? 1 : 0));
		
		const int xPitch = srcSx * tjPixelSize[pixelFormat];
		
		unsigned long dstBufferSize2 = dstBufferSize;
		
		if (tjCompress2(h, (unsigned char *)srcBuffer, srcSx, xPitch, srcSy, pixelFormat, (unsigned char**)&dstBuffer, &dstBufferSize2, subsamp, quality, flags) < 0)
		{
			LOG_ERR("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			dstBufferSize = dstBufferSize2;
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, void *& dstBuffer, int & dstBufferSize)
{
	return saveImage_turbojpegInternal(srcBuffer, srcBufferSize, srcSx, srcSy, srcIsColor, dstQualityLevel, dstBuffer, dstBufferSize, false, false);
}

bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, void * _saveBuffer, int _saveBufferSize)
{
	bool result = true;
	
	void * saveBuffer = _saveBuffer;
	int saveBufferSize = _saveBufferSize;
	
	if (saveImage_turbojpegInternal(srcBuffer, srcBufferSize, srcSx, srcSy, srcIsColor, dstQualityLevel, saveBuffer, saveBufferSize, false, false) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	Assert(saveBuffer == _saveBuffer);
	Assert(saveBufferSize <= _saveBufferSize);
	
	return result;
}

bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, const bool flipImage)
{
	bool result = true;
	
	void * saveBuffer = nullptr;
	int saveBufferSize = 0;
	
	if (saveImage_turbojpegInternal(srcBuffer, srcBufferSize, srcSx, srcSy, srcIsColor, dstQualityLevel, saveBuffer, saveBufferSize, true, flipImage) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	tjFree((unsigned char*)saveBuffer);
	saveBuffer = nullptr;
	saveBufferSize = 0;
	
	return result;
}

#endif
