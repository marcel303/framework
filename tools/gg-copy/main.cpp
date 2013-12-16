#include <string>
#include "Arguments.h"
#include "Exception.h"
#include "FileStreamExtents.h"

enum EqualityCheck
{
	ecFileDate,
	ecAlwaysDifferent
};

class Settings
{
public:
	Settings()
		: mode(ecAlwaysDifferent)
		, recurse(0)
	{
	}
	
	void Validate()
	{
		// todo
	}
	
	std::string src;
	std::string dst;
	EqualityCheck mode;
	int recurse;
};

static bool IsEqual(EqualityCheck ec, std::string fileName1, std::string fileName2)
{
	switch (ec)
	{
		case ecFileDate:
			return true;
		case ecAlwaysDifferent:
			return true;
			
		default:
			return true;
	}
}

static void Process(Settings settings, std::string src, std::string dst, int recurse)
{
	// todo: check if src is a file or directory
	
	bool isFile = FileStream::Exists(src);
	
	if (isFile)
	{
		// todo: if file, copy to dst
		
		FileStream srcStream(src.c_str(), OpenMode_Read);
		FileStream dstStream(dst.c_str(), OpenMode_Write);
		
		StreamExtensions::StreamTo(srcStream, dstStream, 4096);
	}
	else
	{
		if (recurse > 0)
		{
			if (!Directory::Exists(dst))
				Directory::Create(dst);
			
			// todo: enumerate all files and directories
			
			for (int i = 0; i < 0; ++i)
			{
				std::string subSrc = "";
				std::string subDst = dst + "/" + Path::GetFileName(src);
				
				Process(settings, subSrc, subDst, recurse - 1);
			}
		}
		else
		{
			if (settings.recurse == 0)
			{
				throw Exception("source is a directory. must be a file if recursion depth is 0");
			}
			else
			{
				// ignore
			}
		}
	}
}

int main (int argc, char * const argv[])
{
	
	try
	{
		if (argc == 1)
		{
			printf("usage: gg-copy -m <mode> -src <src> -dst <dst> -r <recurse-depth>\n");
			printf("mode: always\n");
			printf("mode: timestamp\n");
			printf("recurse-depth is 0 by default, meaning it will only copy files\n");
			return 0;
		}
		
		Settings settings;
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-m"))
			{
				ARGS_CHECKPARAM(1);
				
				if (!strcmp(argv[i + 1], "always"))
					settings.mode = ecAlwaysDifferent;
				else if (!strcmp([i + 1], "timestamp"))
					settings.mode = ecFileDate;
				else
					throw Exception("unknown mode: %s", argv[i + 1]);
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-src"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.src = argv[i + 1];
			}
			else if (!strcmp(argv[i], "-dst"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.dst = argv[i + 1];
			}
		}
		
		settings.Validate();
		
		Process(settings, settings.src, settings.dst, settings.recurse);
	}
	catch (std::exception & e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
	
    return 0;
}
