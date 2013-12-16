#include "Exception.h"
#include "Parse.h"
#include "ResIndex.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

#ifdef DEBUG
static void CheckResourceName(const std::string& name)
{
	for (size_t i = 0; i < name.size(); ++i)
	{
		char c = name[i];
		
		if (c >= 'A' && c <= 'Z')
			continue;
			
		if (i != 0)
		{
			if (c >= '0' && c <= '9')
				continue;
		}
		
		if (c == '_')
			continue;
		
		throw ExceptionVA("invalid resource name: %s", name.c_str());
	}
}
#endif

ResInfo::ResInfo()
{
	m_DataSetId = 0;
}

ResInfo::ResInfo(int dataSetId, const std::string& platform, const std::string& type, const std::string& name, const std::string& fileName)
{
	m_DataSetId = dataSetId;
	m_Platform = platform;
	m_Type = type;
	m_Name = String::ToUpper(name);
	m_FileName = fileName;
	
#ifdef DEBUG
	CheckResourceName(m_Name);
#endif
}

/*
ResInfo::ResInfo(const std::string& type, const std::string& name, const std::string& fileName, const std::string* arguments)
{
	m_Type = type;
	m_Name = String::ToUpper(name);
	m_FileName = fileName;
	
	if (arguments)
		m_Arguments = arguments;
	
#ifdef DEBUG
	CheckResourceName(m_Name);
#endif
}
*/

void ResIndex::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	while (!reader.EOF_get())
	{
		std::string line = reader.ReadLine();
		
		Parse(line);
	}
}

void ResIndex::Parse(const std::string& line)
{
	if (String::StartsWith(line, "#"))
		return;

	if (String::TrimLeft(line) == String::Empty)
		return;
	
	std::vector<std::string> items = String::Split(line, "\t ");
	
	std::string platform;
	
	if (items.size() >= 1)
	{
		if (String::StartsWith(items[0], "[") && String::EndsWith(items[0], "]"))
		{
			platform = String::SubString(items[0], 1, items[0].size() - 2);
			items.erase(items.begin());
		}
	}
	
	if (items.size() != 4)
		throw ExceptionVA("syntax error: %s", line.c_str());
	
	items[2] = String::ToUpper(items[2]);

	ResInfo res(Parse::Int32(items[0]), platform, items[1], items[2], items[3]);	
	
	Add(res);
}

void ResIndex::LoadBinary(Stream* stream)
{
	StreamReader reader(stream, false);
	
	int32_t resourceCount = reader.ReadInt32();
	
	m_Resources.reserve(resourceCount);
	
	for (int32_t i = 0; i < resourceCount; ++i)
	{
		// read resource info
		
		int dataSetId = reader.ReadInt32();
		std::string type = reader.ReadText_Binary();
		std::string name = reader.ReadText_Binary();
		std::string fileName = reader.ReadText_Binary();
		
		ResInfo res(dataSetId, String::Empty, type, name, fileName);
		
		Add(res);
	}
}

void ResIndex::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	for (size_t i = 0; i < m_Resources.size(); ++i)
	{
		const ResInfo& info = m_Resources[i];
		
		std::string text = String::FormatC(
			"%d %s %s %s %s",
			info.m_DataSetId,
			info.m_Platform.empty() ? "" : String::FormatC("[%s]", info.m_Platform.c_str()).c_str(),
			info.m_Type.c_str(), 
			info.m_Name.c_str(), 
			info.m_FileName.c_str());
		
		writer.WriteLine(text);
	}
}

void ResIndex::SaveBinary(Stream* stream) const
{
	StreamWriter writer(stream, false);

	// write header
	
	int32_t resourceCount = (int32_t)m_Resources.size();
	
	writer.WriteInt32(resourceCount);
	
	// write index
	
	for (size_t i = 0; i < m_Resources.size(); ++i)
	{
		const ResInfo& info = m_Resources[i];
		
		writer.WriteInt32(info.m_DataSetId);
		writer.WriteText_Binary(info.m_Type);
		writer.WriteText_Binary(info.m_Name);
		writer.WriteText_Binary(info.m_FileName);
	}
}

void ResIndex::Add(const ResInfo& res)
{
	m_Resources.push_back(res);
	
	m_ResourcesByName[res.m_Name] = res;
}

static bool ContainsPlatform(const std::string& text, const std::string& platform)
{
	if (text.empty())
		return false;
	
	std::vector<std::string> platformList = String::Split(text, '|');
	
	for (size_t i = 0; i < platformList.size(); ++i)
		if (platformList[i] == platform)
			return true;
			
	return false;
}

void ResIndex::FilterByPlatform(const std::string& platform)
{
	for (std::vector<ResInfo>::iterator i = m_Resources.begin(); i != m_Resources.end();)
	{
		const ResInfo& resInfo = *i;
		
		if (resInfo.m_Platform.empty() || ContainsPlatform(resInfo.m_Platform, platform))
		{
			++i;
			continue;
		}
		
		i = m_Resources.erase(i);
	}
}

const ResInfo& ResIndex::GetResource(int index)
{
#ifndef DEPLOYMENT
	if (index < 0 || index >= (int)m_Resources.size())
		throw ExceptionVA("resource index out of range");
#else
	if (index < 0 || index >= (int)m_Resources.size())
		index = 0;
#endif
	
	return m_Resources[index];
}
