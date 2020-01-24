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

//

/*
Highly optimized CSV document reader. It only does a single allocation to load the file contents and
split the file contents into lines and cells. It does some more allocations to store the cells and
header information, but it's a relatively small amount compared to CsvDocument.
*/
class ReadOnlyCsvDocument
{
public:
	typedef std::vector<const char*> CellArray;
	
protected:
	char * m_text = nullptr;
	
	CellArray m_cells;
	int m_numColumns = 0;
	
	std::vector<const char*> m_header;
	
public:
	~ReadOnlyCsvDocument();
	
	void clear();
	
	bool load(const char * filename, const bool hasHeader, const char separator);
	bool loadText(const char * text, const bool hasHeader, const char separator);
	bool parseLine(const char * begin, const char * end, const char separator);
	bool finalize(const bool hasHeader);
	
	int getColumnIndex(const char * name) const;
	
	CellArray::const_iterator firstRow() { return m_cells.begin() + m_header.size(); }
	CellArray::const_iterator lastRow() { return m_cells.end(); }
	CellArray::const_iterator nextRow(CellArray::const_iterator i) { return i + m_numColumns; }
};
