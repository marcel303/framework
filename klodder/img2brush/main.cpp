#include "Arguments.h"
#include "FileStream.h"
#include "Filter.h"
#include "ImageLoader_FreeImage.h"
#include "Parse.h"

class Settings
{
public:
	Settings()
	{
		scale = 1;
	}
	
	std::string src;
	std::string dst;
	int scale;
};

static Settings settings;

int main(int argc, const char* argv[])
{
	try
	{
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.src = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.dst = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.scale = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown switch: %s", argv[i]);
			}
		}
		
		// validate settings

		if (settings.src.empty())
			throw ExceptionVA("source not set");
		if (settings.dst.empty())
			throw ExceptionVA("destination not set");
		if (settings.scale < 1)
			throw ExceptionVA("scale must be >= 1");
		
		ImageLoader_FreeImage loader;
		Image image;
		
		loader.Load(image, settings.src);
		
		// check if size constraints are OK
		
		if (image.m_Sx != image.m_Sy)
		{
			throw ExceptionVA("brush must be square");
		}
		
		int size = image.m_Sx;
		
		if (size <= 0)
		{
			throw ExceptionVA("brush size must be at least 1x1");
		}
		
		if ((size % 2) != 1)
		{
			throw ExceptionVA("brush size must be odd");
		}
		
		// convert to brush
		
		Filter filter1;
		
		filter1.Size_set(image.m_Sx, image.m_Sy);
		
		for (int y = 0; y < image.m_Sy; ++y)
		{
			const ImagePixel* src = image.GetLine(y);
			float* dst = filter1.Line_get(y);
			
			for (int x = 0; x < image.m_Sx; ++x)
			{
				const float v = (src[x].r + src[x].g + src[x].b) / 3.0f / 255.0f;
				
				dst[x] = v;
			}
		}
		
		// todo: scale brush
		
		int scaledSize = (size - 1) / settings.scale + 1;
		
		Filter filter2;
		
		filter2.Size_set(scaledSize, scaledSize);
		
		for (int y = 0; y < scaledSize; ++y)
		{
			float* dst = filter2.Line_get(y);
			
			for (int x = 0; x < scaledSize; ++x)
			{
				float sx = 0.0f;
				float sy = 0.0f;
				
				if (scaledSize > 1)
				{
					sx = x / (scaledSize - 1.0f) * (filter1.Sy_get() - 1.0f);
					sy = y / (scaledSize - 1.0f) * (filter1.Sy_get() - 1.0f);
				}
				
				float v = filter1.SampleAA(sx, sy);
				
				dst[x] = v;
				
//				dst[x] = 1.0f;
				
//				dst[x] = filter1.Line_get(y)[x];
			}
		}
		
		// todo: create MIP maps (?)

		
		// todo: save brush
		
		FileStream dstStream;
		
		dstStream.Open(settings.dst.c_str(), OpenMode_Write);
		
		filter2.Save(&dstStream);
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
}
