#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
#include "Arguments.h"
#include "CompositionCompiler.h"
#include "CompositionIO.h"
#include "Exception.h"
#include "FileStream.h"
#include "Path.h"
#include "StringEx.h"
#include "VecComposition.h"

class Settings
{
public:
	std::string src;
	std::string srcLib;
	std::string dst;
};

static Settings settings;

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
					settings.src = argv[i];
				}
			}
			if (!strcmp(argv[i], "-c-slib"))
			{
				for (++i; i < argc && argv[i][0] != '-'; ++i)
				{
					settings.srcLib = argv[i];
				}
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				settings.dst = argv[i + 1];

				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}

		// sanitize arguments
		
		if (settings.srcLib == String::Empty)
			settings.srcLib = Path::ReplaceExtension(settings.src, "slib");
			
		// validate arguments
		
		if (settings.src == String::Empty)
			throw ExceptionVA("src not set");
		if (settings.srcLib == String::Empty)
			throw ExceptionVA("src library not set");
		if (settings.dst == String::Empty)
			throw ExceptionVA("dst not set");
		
		//
		
		std::string cwd;
		
		char cwdTemp[1000];
		
		if (getcwd(cwdTemp, 1000) == 0)
			assert(false);
		
		cwd = cwdTemp;
		
		std::string directory = Path::GetDirectory(settings.src);
		
		if (directory != String::Empty)
		{
			if (chdir(directory.c_str()) < 0)
				assert(false);
		}
		
		settings.src = Path::GetFileName(settings.src);
		settings.srcLib = Path::GetFileName(settings.srcLib);
		
		// open source streams
		
		FileStream srcLibStream;
		FileStream srcStream;
		
		srcLibStream.Open(settings.srcLib.c_str(), OpenMode_Read);
		srcStream.Open(settings.src.c_str(), OpenMode_Read);
		
		// read shape library & composition
		
		Composition composition;
		
		CompositionIO::Load(&srcLibStream, &srcStream, composition);
		
		// open destination stream
		
		if (chdir(cwd.c_str()) < 0)
			assert(false);
		
		FileStream dstStream;
		dstStream.Open(settings.dst.c_str(), OpenMode_Write);
		
		// compile composition
		
		VRCC::CompiledComposition cc;
		
		CompositionCompiler::Compile(composition, cc);
		
		// save compiled composition
		
		cc.Save(&dstStream);
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}
