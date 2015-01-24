#include "csv.h"
#include "Debugging.h"
#include "Parse.h"

void splitString(const std::string & str, std::vector<std::string> & result, char c);

void CsvRow::parse(const std::string & line, char separator, std::vector<std::string> & header)
{
	std::vector<std::string> columns;

	splitString(line, columns, separator);

	Assert(columns.size() == header.size());
	if (columns.size() == header.size())
	{
		for (size_t i = 0; i < columns.size(); ++i)
		{
			m_values[header[i]] = columns[i];
		}
	}
}

int CsvRow::getInt(const char * name, int _default)
{
	if (m_values.count(name) != 0)
		return Parse::Int32(m_values[name]);
	else
		return _default;
}

std::string CsvRow::getString(const char * name, const char * _default)
{
	if (m_values.count(name) != 0)
		return m_values[name];
	else
		return _default;
}

void CsvDocument::parse(const std::vector<std::string> & lines)
{
	m_header.clear();
	m_rows.clear();

	if (lines.size() >= 1)
	{
		splitString(lines[0], m_header, ',');

		for (size_t i = 1; i < lines.size(); ++i)
		{
			CsvRow row;

			row.parse(lines[i], ',', m_header);

			m_rows.push_back(row);
		}
	}
}
