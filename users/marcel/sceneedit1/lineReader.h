#pragma once

#include <string>
#include <vector>

struct LineReader
{
	const std::vector<std::string> & lines;
	int line_index;
	int indentation_level;
	
	LineReader(
		const std::vector<std::string> & in_lines,
		const int in_line_index,
		const int in_indentation_level);
	
	const char * get_next_line(const bool skipEmptyLines);
	
	void push_indent()
	{
		indentation_level++;
	}
	
	void pop_indent()
	{
		indentation_level--;
	}
};
