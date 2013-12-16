#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Arguments.h"
#include "Exception.h"
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "ImageLoader_Tga.h"
#include "Parse.h"
#include "StringEx.h"

class Settings
{
public:
	Settings()
	{
		autoType = false;
		dstColorDepth = 16;
	}
	
	std::string src;
	std::string dst;
	bool autoType;
	int dstColorDepth;
};

static Settings g_Settings;

int main(int argc, char* argv[])
{
	try
	{
		// parse arguments

		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.src = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.dst = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-a"))
			{
				ARGS_CHECKPARAM(1);
			
				g_Settings.autoType = Parse::Bool(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-d"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstColorDepth = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}

		// validate settings

		if (g_Settings.src.empty())
			throw ExceptionVA("source not set");
		if (g_Settings.dst.empty())
			throw ExceptionVA("destination not set");
		
		//
		
		ImageLoader_FreeImage loaderSrc;
		
		Image image;
		
		loaderSrc.Load(image, g_Settings.src);
		
		if (g_Settings.autoType)
		{
			ImageLoader_FreeImage loaderDst;
			
			loaderDst.Save(image, g_Settings.dst);
		}
		else
		{
			ImageLoader_Tga loaderDst;
			
			loaderDst.SaveColorDepth = g_Settings.dstColorDepth;
			
			loaderDst.Save(image, g_Settings.dst);
		}
	}
	catch (Exception e)
	{
		printf("%s\n", e.what());

		return -1;
	}

	return 0;
}
