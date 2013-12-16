#include <vector>
#include <string>
#include "Arguments.h"
#include "BrushLibrary.h"
#include "Exception.h"
#include "FileStream.h"
#include "StringEx.h"

class Settings
{
public:
	std::vector<std::string> src;
	std::string dst;
};

static Settings gSettings;

int main(int argc, char* argv[])
{
	try
	{
		// parse arguments
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				for (++i; i < argc && argv[i][0] != '-'; ++i)
				{
					gSettings.src.push_back(argv[i]);
				}
			}
			else if (!strcmp(argv[i], "-o"))
			{
				gSettings.dst = argv[i + 1];
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown arguments: %s", argv[i]);
			}
		}
		
		// validate settings
		
		if (gSettings.src.size() == 0)
			throw ExceptionVA("src not set");
		if (gSettings.dst == String::Empty)
			throw ExceptionVA("dst not set");
		
		// process request
		
		BrushLibrary library;
		
		for (size_t i = 0; i < gSettings.src.size(); ++i)
		{
			Brush_Pattern* pattern = new Brush_Pattern();
			
			FileStream stream;
			
			stream.Open(gSettings.src[i].c_str(), OpenMode_Read);
			
			// todo: version..
			
			pattern->Load(&stream, 1);
			
			library.Append(pattern, true);
		}
		
		FileStream stream;
		
		stream.Open(gSettings.dst.c_str(), OpenMode_Write);
		
		library.Save(&stream);
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s", e.what());
		
		return -1;
	}
}
