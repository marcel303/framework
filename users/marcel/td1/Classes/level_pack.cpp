#include <vector>
#include "level_pack.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

static std::vector<std::string> Split2(std::string line)
{
	int index = String::Find(line, '=');
	
	if (index < 0)
		throw ExceptionVA("separator not found");
	
	std::vector<std::string> result;
	
	result.push_back(String::Trim(String::SubString(line, 0, index)));
	result.push_back(String::Trim(String::SubString(line, index + 1)));
	
	return result;
}

//

LevelPackDescription::LevelPackDescription()
{
}

LevelPackDescription::LevelPackDescription(std::string _path)
{
	path = _path;
}

void LevelPackDescription::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	while (!stream->EOF_get())
	{
		std::string line = reader.ReadLine();
		
		line = String::Trim(line);
		
		if (line.empty())
			continue;
		
		std::vector<std::string> temp = Split2(line);
		
		if (temp[0] == "copyright")
			copyright = temp[1];
		if (temp[0] == "name")
			name = temp[1];
		if (temp[0] == "description")
			description = temp[1];
	}
}

void LevelPackDescription::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteLine(String::Format("copyright=%s", copyright.c_str()));
	writer.WriteLine(String::Format("name=%s", name.c_str()));
	writer.WriteLine(String::Format("description=%s", description.c_str()));
}

//

LevelDescription::LevelDescription()
{
}

LevelDescription::LevelDescription(std::string _path)
{
	path = _path;
}

void LevelDescription::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	while (!stream->EOF_get())
	{
		std::string line = reader.ReadLine();
		
		line = String::Trim(line);
		
		if (line.empty())
			continue;
		
		std::vector<std::string> temp = Split2(line);
		
		if (temp[0] == "author")
			author = temp[1];
		if (temp[0] == "copyright")
			copyright = temp[1];
		if (temp[0] == "name")
			name = temp[1];
	}
}

void LevelDescription::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteLine(String::Format("author=%s", author.c_str()));
	writer.WriteLine(String::Format("copyright=%s", copyright.c_str()));
	writer.WriteLine(String::Format("name=%s", name.c_str()));
}
