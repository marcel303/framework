#include "jpegCompression.h"
#include "Log.h"

#if defined(LINUX)
	#include <turbojpeg.h>
#else
	#include <turbojpeg/turbojpeg.h>
#endif

bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void *& dstBuffer, int & dstBufferSize)
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
		const TJPF pixelFormat = TJPF_RGBX;
		const TJSAMP subsamp = TJSAMP_422;
		const int quality = 85;
		
		const int xPitch = srcSx * tjPixelSize[pixelFormat];
		
		unsigned long dstBufferSize2 = dstBufferSize;
		
		if (tjCompress2(h, (unsigned char *)srcBuffer, srcSx, xPitch, srcSy, TJPF_RGBX, (unsigned char**)&dstBuffer, &dstBufferSize2, subsamp, quality, 0) < 0)
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

bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data)
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
			
			const TJPF pixelFormat = TJPF_RGBX;
			
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
			
			if (tjDecompress2(h, (unsigned char*)buffer, (unsigned long)bufferSize, (unsigned char*)data.buffer, sx, pitch, data.sy, pixelFormat, flags) != 0)
			{
				LOG_ERR("turbojpeg: %s", tjGetErrorStr());
				
				result = false;
			}
			else
			{
				//LOG_DBG("decoded jpeg!");
			}
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}
