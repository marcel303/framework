#pragma once

#include <map>
#include <string>
#include <vector>

class CsvRow
{
	std::map<std::string, std::string> m_values;

public:
	void parse(const std::string & line, char separator, std::vector<std::string> & header);

	int getInt(const char * name, int _default);
	std::string getString(const char * name, const char * _default);
};

class CsvDocument
{
public:
	std::vector<std::string> m_header;
	std::vector<CsvRow> m_rows;

	void parse(const std::vector<std::string> & lines);
};
