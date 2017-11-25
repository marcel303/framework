#include <stdlib.h>
#include <stdarg.h>
#include "fbx.h"

//

static void fbxLog(int logIndent, const char * fmt, ...);
static bool readFile(const char * filename, std::vector<unsigned char> & bytes);

//

class FbxFileLogger
{
	int m_logIndent;
	
	const FbxReader * m_reader;
	
public:
	FbxFileLogger(const FbxReader & reader)
	{
		m_logIndent = 0;
		
		m_reader = &reader;
	}
	
	enum DumpMode
	{
		DumpNodes      = 0x1,
		DumpProperties = 0x2,
		DumpFirstFewProperties = 0x4 // only dump the first few properties.. not every vertex, index value, etc
	};
	
	// dumpFileContents iterates over the entire file contents and dumps every record and property
	
	void dumpFileContents(bool firstFewProperties)
	{
		dump(DumpNodes | DumpProperties | (firstFewProperties ? DumpFirstFewProperties : 0));
	}
	
	// dumpFileContents iterates over the entire file contents and dumps every record
	
	void dumpHierarchy()
	{
		dump(DumpNodes);
	}
	
	void dump(int dumpMode)
	{
		for (FbxRecord record = m_reader->firstRecord(); record.isValid(); record = record.nextSibling())
		{
			dumpRecord(record, dumpMode);
		}
	}
	
	void dumpRecord(const FbxRecord & record, int dumpMode)
	{
		if (dumpMode & DumpNodes)
		{
			fbxLog(m_logIndent, "node: endOffset=%d, numProperties=%d, name=%s\n",
				(int)record.getEndOffset(),
				(int)record.getNumProperties(),
				record.name.c_str());
		}
		
		m_logIndent++;
		
		std::vector<FbxValue> properties = record.captureProperties<FbxValue>();
		
		if (dumpMode & DumpProperties)
		{
			size_t numProperties = properties.size();
			
			if (dumpMode & DumpFirstFewProperties)
				if (numProperties > 4)
					numProperties = 4;
			
			for (size_t i = 0; i < numProperties; ++i)
			{
				dumpProperty(properties[i]);
			}
		}
		
		for (FbxRecord childRecord = record.firstChild(); childRecord.isValid(); childRecord = childRecord.nextSibling())
		{
			dumpRecord(childRecord, dumpMode);
		}
		
		m_logIndent--;
	}
	
	void dumpProperty(const FbxValue & value)
	{
		switch (value.type)
		{
			case FbxValue::TYPE_BOOL:
				fbxLog(m_logIndent, "bool: %d\n", get<bool>(value));
				break;
			case FbxValue::TYPE_INT:
				fbxLog(m_logIndent, "int: %lld\n", get<int64_t>(value));
				break;
			case FbxValue::TYPE_REAL:
				fbxLog(m_logIndent, "float: %f\n", get<float>(value));
				break;
			case FbxValue::TYPE_STRING:
				fbxLog(m_logIndent, "string: %s\n", value.getString());
				break;
			
			case FbxValue::TYPE_INVALID:
				fbxLog(m_logIndent, "(invalid)\n");
				break;
		}
	}
};

//

static void fbxLog(int logIndent, const char * fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	
	char tabs[128];
	for (int i = 0; i < logIndent; ++i)
		tabs[i] = '\t';
	tabs[logIndent] = 0;
	
	char temp[1024];
	vsprintf(temp, fmt, va);
	va_end(va);
	
	printf("%s%s", tabs, temp);
}

static bool readFile(const char * filename, std::vector<unsigned char> & bytes)
{
	bool result = true;
	
	FILE * file = fopen(filename, "rb");
	
	if (!file)
	{
		result = false;
	}
	else
	{
		const size_t p1 = ftell(file);
		fseek(file, 0, SEEK_END);
		const size_t p2 = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		const size_t numBytes = p2 - p1;
		bytes.resize(numBytes);
		
		if (fread(&bytes[0], numBytes, 1, file) != 1)
		{
			result = false;
		}
		
		fclose(file);
	}
	
	return result;
}

//

int main(int argc, char * argv[])
{
	int logIndent = 0;
	
	// parse command line
	
	bool dumpAll = true;
	bool dumpAllButDoItSilently = false;
	bool dumpHierarchy = true;
	const char * filename = "test.fbx";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-v"))
			dumpAll = true;
		else if (!strcmp(argv[i], "-vs"))
			dumpAllButDoItSilently = true;
		else if (!strcmp(argv[i], "-h"))
			dumpHierarchy = true;
		else
			filename = argv[i];
	}
	
	if (dumpAllButDoItSilently)
		dumpAll = true;
	
	// read file contents
	
	std::vector<unsigned char> bytes;
	
	if (!readFile(filename, bytes))
	{
		fbxLog(logIndent, "failed to open %s\n", filename);
		return -1;
	}
	
	// open FBX file
	
	FbxReader reader;
	
	reader.openFromMemory(&bytes[0], bytes.size());
	
	// dump FBX contents
	
	FbxFileLogger logger(reader);
	
	if (dumpAll)
	{
		logger.dumpFileContents(dumpAllButDoItSilently);
	}
	
	if (dumpHierarchy)
	{
		logger.dumpHierarchy();
	}
	
	return 0;
}
