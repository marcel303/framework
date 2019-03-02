#include "lineReader.h"

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

const char * LineReader::get_next_line()
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
	
	return line + indentation_level;
}
