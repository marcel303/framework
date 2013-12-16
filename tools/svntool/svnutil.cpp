#include <string>
#include <string.h>
#include "Arguments.h"
#include "FileStream.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

class Settings
{
public:
	Settings()
	{
		writeVersion = false;
	}
	
	std::string path;
	bool writeVersion;
	std::string writeVersionDstSource;
	std::string writeVersionDstHeader;
};

static Settings gSettings;

class SvnHelper
{
public:
	SvnHelper(const char* path)
	{
		FileStream stream;
		
		std::string entries = std::string(path) + ".svn/entries";
		
		stream.Open(entries.c_str(), OpenMode_Read);
		
		StreamReader reader(&stream, false);
		
		for (int i = 0; i < 3; ++i)
			reader.ReadLine();
		
		revision = Parse::Int32(reader.ReadLine());
	}

	int revision;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc < 2)
		{
			printf("usage:\n");
			printf("svntool -wt <repository> <dst_source> <dsp_header>\n");
			
			return 0;
		}
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-wv"))
			{
				ARGS_CHECKPARAM(3);
				
				gSettings.writeVersion = true;
				gSettings.path = argv[i + 1];
				gSettings.writeVersionDstSource = argv[i + 2];
				gSettings.writeVersionDstHeader = argv[i + 3];
				i += 4;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}
		
		if (gSettings.writeVersion && gSettings.path == String::Empty)
			throw ExceptionVA("path not set");
			
		if (gSettings.writeVersion)
		{
			SvnHelper svn(gSettings.path.c_str());
			
			FileStream stream;
			
			stream.Open(gSettings.writeVersionDstHeader.c_str(), OpenMode_Write);
			
			StreamWriter writer(&stream, false);
			
			writer.WriteLine("#pragma once");
			writer.WriteLine("namespace Svn");
			writer.WriteLine("{");
			writer.WriteLine("\textern int Revision;");
			writer.WriteLine("}");
			
			stream.Close();
			
			stream.Open(gSettings.writeVersionDstSource.c_str(), OpenMode_Write);
			
			writer.WriteLine("namespace Svn");
			writer.WriteLine("{");
			writer.WriteLine(String::Format("\tint Revision = %d;", svn.revision).c_str());
			writer.WriteLine("}");
		}
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}
