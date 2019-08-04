#include "Csv.h"
#include "Debugging.h"
#include "FileStream.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StringEx.h"
#include <string.h>

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

//

#include "TextIO.h"

ReadOnlyCsvDocument::~ReadOnlyCsvDocument()
{
	clear();
}

void ReadOnlyCsvDocument::clear()
{
	m_numColumns = 0;
	
	m_cells.clear();
	
	delete [] m_text;
	m_text = nullptr;
}

bool ReadOnlyCsvDocument::load(const char * filename, const bool hasHeader, const char separator)
{
	clear();
	
	//
	
	Assert(m_text == nullptr);
	Assert(m_cells.empty());
	
	bool result = true;
	
	TextIO::LineEndings lineEndings;
	
	struct UserData
	{
		ReadOnlyCsvDocument * self;
		char separator;
		bool result;
	};
	
	auto lineCallback = [](void * in_userData, const char * begin, const char * end)
	{
		UserData * userData = (UserData*)in_userData;
		
		userData->self->parseLine(begin, end, userData->separator);
	};
	
	UserData userData;
	userData.self = this;
	userData.separator = separator;
	userData.result = true;
	
	if (result == true)
	{
		m_cells.reserve(100);
		
		if (TextIO::loadWithCallback(filename, m_text, lineEndings, lineCallback, &userData) == false)
			result = false;
		else if (userData.result == false)
			result = false;
	}
	
	if (result)
	{
		if (finalize(hasHeader) == false)
			result = false;
	}
	
	if (result == false)
	{
		clear();
	}
	else
	{
		Assert(m_numColumns > 0);
		Assert(m_cells.empty() == false);
	}
	
	return result;
}

bool ReadOnlyCsvDocument::parseLine(const char * begin, const char * end, const char separator)
{
	bool result = true;

	// add the first string
	
	m_cells.push_back(begin);
	
	while (begin < end)
	{
		if (*begin == separator)
		{
			// terminate the current string
			
			*const_cast<char*>(begin) = 0;
			
			// add the next string
			
			m_cells.push_back(begin + 1);
		}
		
		begin++;
	}
	
	// terminate the last string
	
	*const_cast<char*>(begin) = 0;
	
	if (m_numColumns == 0)
		m_numColumns = m_cells.size();
	
	return result;
}

bool ReadOnlyCsvDocument::finalize(const bool hasHeader)
{
	bool result = true;
	
	if (m_numColumns == 0)
		result = false;
	else if ((m_cells.size() % m_numColumns) != 0)
		result = false;

	if (result == true)
	{
		if (hasHeader)
		{
			for (size_t i = 0; i < m_numColumns; ++i)
				m_header.push_back(m_cells[i]);
		}
	}
	
	return result;
}

int ReadOnlyCsvDocument::getColumnIndex(const char * name) const
{
	for (size_t i = 0; i < m_header.size(); ++i)
		if (strcmp(m_header[i], name) == 0)
			return i;
	
	return -1;
}
