#pragma once

#include <map>
#include <string>
#include <vector>

class CsvRow
{
	std::map<std::string, std::string> m_values;

public:
	bool parse(const std::string & line, const char separator, const std::vector<std::string> & header);
	
	const char * getString(const char * name, const char * _default) const;
	int getInt(const char * name, const int _default) const;
	float getFloat(const char * name, const float _default) const;
};

class CsvDocument
{
public:
	std::vector<std::string> m_header;
	std::vector<CsvRow> m_rows;
	
	void clear();
	
	bool load(const char * filename, const bool hasHeader, const char separator);
	bool parse(const std::vector<std::string> & lines, const bool hasHeader, const char separator = ',');
};
