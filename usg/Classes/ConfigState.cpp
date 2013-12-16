#include <map>
#include "ConfigState.h"
#include "Exception.h"
#include "FileStream.h"
#include "GameSettings.h"
#include "MemoryStream.h"
#include "Parse.h"
#if defined(PSP)
#include "PspSaveData.h"
#endif
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "System.h"

#define CONFIG_FILENAME g_System.GetDocumentPath("config.txt").c_str()

static void ConfigLoadEx(Stream* stream);
static void ConfigSaveEx(Stream* stream);

typedef FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigKey;
typedef FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigValue;

typedef std::map<ConfigKey, ConfigValue> KeyValueList;
typedef KeyValueList::iterator KeyValueListItr;

static KeyValueList sKeyValueList;
static bool sKeyValueListIsDirty = false;

static bool ConfigExists(const char* name)
{
	return sKeyValueList.count(name) != 0;
}

void ConfigSetInt(const char* name, int value)
{
	char temp[32];
	sprintf(temp, "%d", value);
	ConfigSetString(name, temp);
}

void ConfigSetString(const char* name, const char* value)
{
	ConfigKey tempName = name;
	ConfigValue tempValue = value;

	tempName.Replace('\n', '_');
	tempName.Replace('\r', '_');
	tempValue.Replace('\n', '_');
	tempValue.Replace('\r', '_');

	sKeyValueList[tempName] = tempValue;
	sKeyValueListIsDirty = true;
}

int ConfigGetInt(const char* name)
{
	FixedSizeString<MAX_CONFIG_STRING_SIZE> temp = ConfigGetString(name);	
	return Parse::Int32(temp.c_str());
}

int ConfigGetIntEx(const char* name, int _default)
{
	if (!ConfigExists(name))
		return _default;
	
	return ConfigGetInt(name);
}

FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetString(const char* name)
{
	KeyValueListItr i = sKeyValueList.find(name);

	if (i == sKeyValueList.end())
		return FixedSizeString<MAX_CONFIG_STRING_SIZE>();
	
	return i->second.c_str();
}

FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetStringEx(const char* name, const char* _default)
{
	if (!ConfigExists(name))
		return _default;
	
	return ConfigGetString(name);
}

void ConfigLoad()
{
	Assert(sKeyValueListIsDirty == false);

	try
	{
#if defined(WIN32) || defined(MACOS) || defined(LINUX) || defined(IPHONEOS) || defined(BBOS)
		FileStream stream;
		stream.Open(CONFIG_FILENAME, OpenMode_Read);
#elif defined(PSP)
		uint8_t bytes[65536];
		int byteCount = sizeof(bytes);
		PspSaveData_Load(PSPSAVE_APPNAME, PSPSAVE_SETTINGS, bytes, byteCount, byteCount);
		MemoryStream stream(bytes, byteCount);
#else
#error
#endif

		ConfigLoadEx(&stream);
	}
	catch (std::exception& e)
	{
		LOG_DBG("failed to load config: %s", e.what());
	}
}

void ConfigSave(bool force)
{
	if (sKeyValueListIsDirty == false)
		return;

	MemoryStream stream;

	ConfigSaveEx(&stream);

	// write stream contents

#if defined(WIN32) || defined(MACOS) || defined(LINUX) || defined(IPHONEOS) || defined(BBOS)
	FileStream fileStream(CONFIG_FILENAME, OpenMode_Write);
	StreamExtensions::WriteTo(&stream, &fileStream);
	fileStream.Close();
#elif defined(PSP)
	PspSaveData_Save(PSPSAVE_APPNAME, PSPSAVE_SETTINGS, PSPSAVE_DESC, PSPSAVE_DESC_LONG, stream.Bytes_get(), stream.Length_get(), true);
#else
	#error
#endif

	sKeyValueListIsDirty = false;
}

static void ConfigLoadEx(Stream* stream)
{
	StreamReader reader(stream, false);

	KeyValueList keyValueList;
	ConfigKey key;
	ConfigValue value;

	int count = 0;

	while (!reader.EOF_get())
	{
		const std::string line = reader.ReadLine();

		if (count == 0)
			key = line.c_str();
		if (count == 1)
			value = line.c_str();

		count = (count + 1) % 2;

		if (count == 0)
		{
			keyValueList[key] = value;
		}
	}

	Assert(count == 0);

	sKeyValueList = keyValueList;
}

static void ConfigSaveEx(Stream* stream)
{
	StreamWriter writer(stream, false);

	for (KeyValueListItr i = sKeyValueList.begin(); i != sKeyValueList.end(); ++i)
	{
		const char* key = i->first.c_str();
		const char* value = i->second.c_str();

		writer.WriteLine(key);
		writer.WriteLine(value);
	}
}
