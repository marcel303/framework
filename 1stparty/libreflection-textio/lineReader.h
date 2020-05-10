#pragma once

#include <string>
#include <vector>

/**
 * LineReader provides a fast and efficient way to read indented text.
 * When reading lines, its scope is limited by the indentation level that you set.
 * This allows it to read structured data, similar to how scoping works in Python,
 * with the exception LineReader will always use tabs for indentation, for efficiency reasons.
 *
 * LineReader supports skipping empty lines, or commented lines starting with '#'.
 */
class LineReader
{
	const std::vector<std::string> & lines;
	int line_index;
	int indentation_level;
	int initial_indentation_level;
	bool do_dtor_check;
	bool do_jump_check;
	
public:
	LineReader(
		const std::vector<std::string> & in_lines,
		const int in_line_index,
		const int in_indentation_level);
	~LineReader();
	
	const char * get_next_line(const bool skipEmptyLinesAndComments, const bool checkForIndentationJump = true);
	int get_current_line_index() const { return line_index; }
	int get_current_indentation_level() const { return indentation_level; }
	
	void skip_current_section();
	
	void disable_dtor_check() { do_dtor_check = false; }
	void disable_jump_check() { do_jump_check = false; }
	
	void push_indent()
	{
		indentation_level++;
	}
	
	void pop_indent()
	{
		indentation_level--;
	}
};
