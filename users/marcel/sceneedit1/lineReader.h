#pragma once

#include <string>
#include <vector>

class LineReader
{
	const std::vector<std::string> & lines;
	int line_index;
	int indentation_level;
	
public:
	LineReader(
		const std::vector<std::string> & in_lines,
		const int in_line_index,
		const int in_indentation_level);
	~LineReader();
	
	const char * get_next_line(const bool skipEmptyLines);
	int get_current_line_index() const { return line_index; }
	
	void push_indent()
	{
		indentation_level++;
	}
	
	void pop_indent()
	{
		indentation_level--;
	}
};
