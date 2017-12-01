#include "Csv.h"
#include "Debugging.h"
#include "FileStream.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StringEx.h"

bool CsvRow::parse(const std::string & line, const char separator, const std::vector<std::string> & header)
{
	const std::vector<std::string> columns = String::Split(line, separator);

	Assert(columns.size() == header.size());
	if (columns.size() != header.size())
	{
		return false;
	}
	else
	{
		for (size_t i = 0; i < columns.size(); ++i)
		{
			m_values[header[i]] = columns[i];
		}
		
		return true;
	}
}

const char * CsvRow::getString(const char * name, const char * _default) const
{
	auto i = m_values.find(name);

	if (i != m_values.end())
		return i->second.c_str();
	else
		return _default;
}

int CsvRow::getInt(const char * name, const int _default) const
{
	auto i = m_values.find(name);

	if (i != m_values.end())
		return Parse::Int32(i->second);
	else
		return _default;
}

float CsvRow::getFloat(const char * name, const float _default) const
{
	auto i = m_values.find(name);

	if (i != m_values.end())
		return Parse::Float(i->second);
	else
		return _default;
}

//

void CsvDocument::clear()
{
	m_header.clear();
	m_rows.clear();
}

bool CsvDocument::load(const char * filename, const bool hasHeader, const char separator)
{
	try
	{
		FileStream stream(filename, OpenMode_Read);
		StreamReader reader(&stream, false);
		auto lines = reader.ReadAllLines();
		
		if (parse(lines, hasHeader, separator) == false)
		{
			clear();
			
			return false;
		}
		else
		{
			return true;
		}
	}
	catch (std::exception &)
	{
		clear();
		
		return false;
	}
}

bool CsvDocument::parse(const std::vector<std::string> & lines, const bool hasHeader, const char separator)
{
	clear();
	
	//
	
	bool result = true;
	
	m_rows.reserve(lines.size());

	if (lines.size() >= 1)
	{
		if (hasHeader)
		{
			m_header = String::Split(lines[0], separator);
		}
		else
		{
			std::vector<std::string> columns = String::Split(lines[0], separator);
			
			for (size_t i = 0; i < columns.size(); ++i)
			{
				m_header.push_back(String::FormatC("%d", i));
			}
		}

		for (size_t i = hasHeader ? 1 : 0; i < lines.size(); ++i)
		{
			CsvRow row;

			result &= row.parse(lines[i], separator, m_header);

			m_rows.push_back(row);
		}
	}
	
	return result;
}
