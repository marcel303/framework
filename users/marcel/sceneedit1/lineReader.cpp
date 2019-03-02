#include "lineReader.h"
#include <string.h>

static bool isEmptyLineOrComment(const char * line)
{
	for (int i = 0; line[i] != 0; ++i)
	{
		if (line[i] == '#')
			return true;
		
		if (!isspace(line[i]))
			return false;
	}
	
	return true;
}

static int calculateIndentationLevel(const char * line) // todo : add helper functions for dealing with lines
{
	int result = 0;
	
	while (line[result] == '\t')
		result++;
	
	return result;
}

LineReader::LineReader(
	const std::vector<std::string> & in_lines,
	const int in_line_index,
	const int in_indentation_level)
	: lines(in_lines)
	, line_index(in_line_index)
	, indentation_level(in_indentation_level)
{
}

const char * LineReader::get_next_line(const bool skipEmptyLines)
{
	// at end? return nullptr
	
	if (line_index == lines.size())
		return nullptr;
	
	const char * line = lines[line_index].c_str();
	
	// calculate indentation level. less than ours? return nullptr
	
	const size_t next_level = calculateIndentationLevel(line);
	
	if (next_level < indentation_level)
		return nullptr;
	
	// return line with appropriate offset to compensate for indentation
	
	line_index++;
	
	const char * result = line + indentation_level;
	
	if (skipEmptyLines && isEmptyLineOrComment(result))
		return get_next_line(skipEmptyLines);
	else
		return result;
}
