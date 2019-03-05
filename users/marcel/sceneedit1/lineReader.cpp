#include "lineReader.h"
#include "Log.h"
#include <string.h>

static bool isEmptyLineOrComment(const char * line)
{
	for (int i = 0; line[i] != 0; ++i)
	{
		if (line[i] == '#')
			return true;
		
		if (line[i] != ' ' || line[i] != '\t')
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

LineReader::~LineReader()
{
// todo : add a better way to detect push/pop mismatches and assert 
	//LOG_DBG("indentation level when line reader disposed: %d", indentation_level);
}

const char * LineReader::get_next_line(const bool skipEmptyLines)
{
	// at end? return nullptr
	
	if (line_index == lines.size())
		return nullptr;
	
	const char * line = lines[line_index].c_str();
	
	if (skipEmptyLines && isEmptyLineOrComment(line))
	{
		line_index++;
		return get_next_line(skipEmptyLines);
	}
	else
	{
		// calculate indentation level. less than ours? return nullptr
		
		const size_t next_level = calculateIndentationLevel(line);
		
		if (next_level < indentation_level)
			return nullptr;
		
	// todo : check for indentation level increment greater than one in various place
		
		// return line with appropriate offset to compensate for indentation
		
		line_index++;
		
		return line + indentation_level;
	}
}
