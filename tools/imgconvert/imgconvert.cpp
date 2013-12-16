#include "Arguments.h"
#include "Exception.h"
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "Parse.h"
#include "StringEx.h"

class AppSettings
{
public:
	AppSettings()
	{
		scaleDown = 1;
	}
	
	std::string src;
	std::string dst;
	int scaleDown;
};

static AppSettings sSettings;

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
				
				sSettings.src = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);
				
				sSettings.dst = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-sd"))
			{
				ARGS_CHECKPARAM(1);
				
				sSettings.scaleDown = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}
		
		// validate arguments
		
		if (sSettings.src == String::Empty)
			throw ExceptionVA("src not set");
		if (sSettings.dst == String::Empty)
			throw ExceptionVA("dst not set");
		if (sSettings.scaleDown < 1)
			throw ExceptionVA("invalid scale down factor. must be >= 1");
			
		// process request
		
		ImageLoader_FreeImage loader;
		
		Image image;
		
		loader.Load(image, sSettings.src);
		
		if (sSettings.scaleDown != 1)
		{
			Image temp;
			
			image.DownscaleTo(temp, sSettings.scaleDown);
			
			image = temp;
		}
		
		loader.Save(image, sSettings.dst);
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}
