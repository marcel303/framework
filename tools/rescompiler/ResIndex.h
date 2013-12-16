#pragma once

#include <map>
#include <string>
#include <vector>
#include "libgg_forward.h"

class ResInfo
{
public:
	ResInfo();
	ResInfo(int dataSetId, const std::string& platform, const std::string& type, const std::string& name, const std::string& fileName);
	
	int m_DataSetId;
	std::string m_Platform;
	std::string m_Type;
	std::string m_Name;
	std::string m_FileName;
};

typedef struct CompiledResInfo
{
	int dataSetId;
	const char* type;
	const char* file;
	const char* name;
} CompiledResInfo;

class ResIndex
{
public:
	void Load(Stream* stream);
	void LoadBinary(Stream* stream);
	void Save(Stream* stream);
	void SaveBinary(Stream* stream) const;
	void Parse(const std::string& line);	
	void Add(const ResInfo& res);
	void FilterByPlatform(const std::string& platform);
	
	const ResInfo& GetResource(int index);
	
	std::vector<ResInfo> m_Resources;
	std::map<std::string, ResInfo> m_ResourcesByName;
};
