#include "Precompiled.h"

#include <string>
#include <string.h>
#include <vector>
#include "Arguments.h"
#include "Directory.h"
#include "Exception.h"
#include "FileStream.h"
#include "PackageCompiler.h"

#include "StreamReader.h"
#include "StreamWriter.h"

enum RequestType
{
	RequestType_Compile,
	RequestType_TestSubStream,
	RequestType_List
};

class Settings
{
public:
	Settings()
	{
		requestType = RequestType_Compile;
	}
	
	void Validate()
	{
		if (requestType == RequestType_Compile)
		{
//			if (src.empty())
//				throw ExceptionVA("source file list not set");
			if (dst.empty())
				throw ExceptionVA("destination not set");
		}
		else if (requestType == RequestType_TestSubStream)
		{
//			if (src.empty())
//				throw ExceptionVA("source file list not set");
		}
		else if (requestType == RequestType_List)
		{
			if (src.empty())
				throw ExceptionVA("source file list not set");
		}
		else
		{
			throw ExceptionVA("unknown request type: %d", (int)requestType);
		}
	}

	RequestType requestType;
	std::vector<std::string> src;
	std::string dst;
};

static Settings g_Settings;

static void HandleCompile();
static void HandleTestSubStream();
static void HandleList();

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 1)
		{
			printf("usage: pkgcompiler -c <source_list> -o <destination>\n");
			printf("usage: pkgcompiler -m test-substream -c <source_list>\n");
			printf("usage: pkgcompiler -m list -c <source>\n");
			return -1;
		}
		
		// parse arguments

		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				for (++i; i < argc && argv[i][0] != '-'; ++i)
				{
					g_Settings.src.push_back(argv[i]);
				}
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.dst = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-m"))
			{
				ARGS_CHECKPARAM(1);
				
				if (!strcmp(argv[i + 1], "compile"))
					g_Settings.requestType = RequestType_Compile;
				else if (!strcmp(argv[i + 1], "test-substream"))
					g_Settings.requestType = RequestType_TestSubStream;
				else if (!strcmp(argv[i + 1], "list"))
					g_Settings.requestType = RequestType_List;
				else
					throw ExceptionVA("unknown request type: %s", argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}

		// validate settings

		g_Settings.Validate();
		
		//
		
		if (g_Settings.requestType == RequestType_Compile)
			HandleCompile();
		else if (g_Settings.requestType == RequestType_TestSubStream)
			HandleTestSubStream();
		else if (g_Settings.requestType == RequestType_List)
			HandleList();
		else
			throw ExceptionVA("unknown request type: %d", (int)g_Settings.requestType);
	}
	catch (Exception e)
	{
		printf("%s\n", e.what());

		return -1;
	}

	return 0;
}

static void HandleCompile()
{
	CompiledPackage package;

	PackageCompiler::Compile(g_Settings.src, g_Settings.dst, package);

	FileStream dstStream;

	dstStream.Open(g_Settings.dst.c_str(), OpenMode_Write);

	package.Save(&dstStream);

	dstStream.Close();
}

static void DumpStream(Stream* stream, int length)
{
	StreamReader reader(stream, false);
	
	if (length >= 0)
	{
		for (int i = 0; i < length; ++i)
		{
			char c = reader.ReadInt8();
			
			printf("%c", c);
		}

	}
	else
	{
		while (!reader.EOF_get())
		{
			char c = reader.ReadInt8();
			
			printf("%c", c);
		}
	}
	
	printf("\n");
}

static void HandleTestSubStream()
{
	CompiledPackage package;

	PackageCompiler::Compile(g_Settings.src, g_Settings.dst, package);
	
	MemoryStream* pkgStream = new MemoryStream();

	package.Save(pkgStream);

	pkgStream->Seek(0, SeekMode_Begin);

	package.Load(pkgStream, true);

	printf("index item count: %d\n", package.m_Index.m_ItemCount);
	
	for (int i = 0; i < package.m_Index.m_ItemCount; ++i)
	{
		printf("opening substream: %s\n", package.m_Index.m_Items[i].m_FileName.c_str());
		
		SubStream dataStream = package.Open(package.m_Index.m_Items[i].m_FileName.c_str());

		int position = dataStream.Position_get();
		int length = dataStream.Length_get();
		
		printf("%s: stream_position=%d, stream_length=%d\n", package.m_Index.m_Items[i].m_FileName.c_str(), position, length);
		
		StreamReader dataReader(&dataStream, false);
		
		while (!dataStream.EOF_get())
		{
			char v = dataReader.ReadInt8();
			if (v >= 'a' && v <= 'Z')
				printf("%c", (char)v);
			else
				printf("%02x", v);
		}
		
		printf("\n");
		
		int position2 = dataStream.Position_get();

		printf("%s: stream_position(EOF)=%d\n", package.m_Index.m_Items[i].m_FileName.c_str(), position2);
	}
	
	//
	
	MemoryStream stream;
	
	StreamWriter writer(&stream, false);
	
	writer.WriteLine("hello world");
	
	int offset = 5;
	
	SubStream subStream(&stream, false, offset, stream.Length_get() - offset);
	
	printf("length=%d, position=%d\n", subStream.Length_get(), subStream.Position_get());
	
	subStream.Seek(0, SeekMode_Begin);
	
	DumpStream(&subStream, -1);
	
	subStream.Seek(0, SeekMode_Begin);
	
	DumpStream(&subStream, -1);
	
	subStream.Seek(2, SeekMode_Begin);
	
	DumpStream(&subStream, -1);
}

static void HandleList()
{
	for (size_t i = 0; i < g_Settings.src.size(); ++i)
	{
		FileStream* stream = new FileStream(g_Settings.src[i].c_str(), OpenMode_Read);

		CompiledPackage package;

		package.Load(stream, true);

		std::map<std::string, PackageItem*> m_ItemsByFileName;

		for (int i = 0; i < package.m_Index.m_ItemCount; ++i)
		{
			const PackageItem& item = package.m_Index.m_Items[i];

			printf("[%08x] [%08d @ %08d] %s\n",
				item.m_Hash,
				item.m_Length,
				item.m_Offset,
				item.m_FileName.c_str());
		}
	}
}
