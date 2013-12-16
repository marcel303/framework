#include "FileStream.h"
#include "Log.h"
#include "Parse.h"
#include "Settings_FileSystem.h"
#include "StreamReader.h"
#include "StreamWriter.h"

//#ifdef WIN32
Settings_FileSystem gSettings;
//#endif

std::string Settings_FileSystem::GetString(std::string name, std::string _default)
{
	try
	{
		FileStream stream;
		stream.Open(("cfg_" + name + ".txt").c_str(), OpenMode_Read);
		StreamReader reader(&stream, false);
		return reader.ReadLine();
	}
	catch (std::exception& e)
	{
		LOG_DBG("setting not found: %s", e.what());

		SetString(name, _default);

		return _default;
	}
}

void Settings_FileSystem::SetString(std::string name, std::string value)
{
	FileStream stream;
	stream.Open(("cfg_" + name + ".txt").c_str(), OpenMode_Write);
	StreamWriter writer(&stream, false);
	writer.WriteLine(value);
}
