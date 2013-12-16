#include <map>
#include <string>
#include <string.h>
#include <vector>
#include "Arguments.h"
#include "FileStream.h"
#include "FileStreamExtends.h"
#include "MemoryStream.h"
#include "Path.h"
#include "ResIndex.h"
#include "ResIndexCompiler.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "Types.h"

/*

resource index file format:

<type>	<name>	<filename>

on application startup:

	- load resource index
	
	- load all resources of type (eg, vector shapes)
	
	- defer loading of other resources to engine
	
	- use resource index to get Stream object to file

*/

class Settings
{
public:
	Settings()
	{
		list = false;
		listStripExtension = true;
	}
	
	std::string src;
	std::string name;
	std::string dstHeader;
	std::string dstSource;
	std::string dstIndex;
	bool list;
	bool listStripExtension;
	std::string type;
	std::string platform;
};

static Settings g_Settings;

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 1)
		{
			printf("usage: rescompiler -c <list-file> -name <class-name> -p <platform>\n");
			printf("\t-o-index <index-file> -o-header <header-file> -o-source <source-file> -o-path <dst-path>\n");
			printf("\tcompile resource list\n");
			printf("\n");
			printf("usage: rescompiler -c <list-file> -l -t <type> [-e]\n");
			printf("\tlist contents of resource list\n");
			printf("\n");
			printf("\tlist-file: text file containing the list of resources\n");
			printf("\tclass-name: name of the class generated in the source/header files\n");
			printf("\tindex-file: name of the index file to create\n");
			printf("\theader-file: name of the header file to create\n");
			printf("\tsource-file: name of the source file to create\n");
			printf("\ttype: type of resource to list\n");
			printf("\t-e: list extensions of resources\n");
			printf("\t-p: specify platform\n");
//			printf("\tdst-path: the destination path where resources will be copied\n");
			
			return 0;
		}
		
		// parse arguments
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.src = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-name"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.name = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o-index"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstIndex = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o-header"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstHeader = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o-source"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstSource = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-l"))
			{
				g_Settings.list = true;
				
				i += 1;
			}
			else if (!strcmp(argv[i], "-e"))
			{
				g_Settings.listStripExtension = false;

				i += 1;
			}
			else if (!strcmp(argv[i], "-t"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.type = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.platform = argv[i + 1];
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}
		
		// validate arguments
		
		if (g_Settings.src == String::Empty)
			throw ExceptionVA("src not set");
			
		if (!g_Settings.list)
		{
			if (g_Settings.name == String::Empty)
				throw ExceptionVA("name not set");
			if (g_Settings.dstIndex == String::Empty)
				throw ExceptionVA("dst index not set");
			if (g_Settings.dstHeader == String::Empty)
				throw ExceptionVA("dst header not set");
			if (g_Settings.dstSource == String::Empty)
				throw ExceptionVA("dst source not set");
		}
		
		if (g_Settings.list)
		{
			ResIndex index;
			
			FileStream srcStream;
			
			srcStream.Open(g_Settings.src.c_str(), OpenMode_Read);
			
			index.Load(&srcStream);
			
			for (size_t i = 0; i < index.m_Resources.size(); ++i)
			{
				if (g_Settings.type != String::Empty && g_Settings.type != index.m_Resources[i].m_Type)
					continue;
				
				std::string fileName = index.m_Resources[i].m_FileName;

				if (g_Settings.listStripExtension)
					fileName = Path::StripExtension(fileName);

				printf("%s ", fileName.c_str());
			}
		}
		else
		{
			ResIndex index;
			
			FileStream srcStream;
			
			srcStream.Open(g_Settings.src.c_str(), OpenMode_Read);
			
			index.Load(&srcStream);
			
			FileStream indexStream;
			MemoryStream headerStream;
			MemoryStream sourceStream;
			
			indexStream.Open(g_Settings.dstIndex.c_str(), OpenMode_Write);
			
			ResIndexCompiler::Compile(index, g_Settings.name, g_Settings.dstHeader, g_Settings.platform, &indexStream, &headerStream, &sourceStream);
			
			FileStreamExtents::OverwriteIfChanged(&headerStream, g_Settings.dstHeader);
			FileStreamExtents::OverwriteIfChanged(&sourceStream, g_Settings.dstSource);
		}

		return 0;
	}
	catch (Exception e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}
