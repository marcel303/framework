#include "Debugging.h"
#include "lineReader.h"
#include <string.h>

static int isEmptyLineOrComment(const char * line)
{
	for (int i = 0; line[i] != 0; ++i)
	{
		if (line[i] == '#')
			return true;
		
		if (line[i] != ' ' && line[i] != '\t')
			return false;
	}
	
	return true;
}

static int calculateIndentationLevel(const char * line)
{
	int result = 0;
	
	while (line[result] == '\t')
		result++;
	
	return result;
}

//

LineReader::LineReader(
	const std::vector<std::string> * in_lines,
	const int in_line_index,
	const int in_indentation_level)
	: lines(*in_lines)
	, line_index(in_line_index)
	, indentation_level(in_indentation_level)
	, initial_indentation_level(in_indentation_level)
	, do_dtor_check(true)
	, do_jump_check(true)
{
	Assert(in_lines != nullptr);
}

LineReader::~LineReader()
{
	if (do_dtor_check)
	{
		AssertMsg(indentation_level == initial_indentation_level, "indentation level when line reader disposed: %d", indentation_level);
	}
}

const char * LineReader::get_next_line(const bool skipEmptyLinesAndComments, const bool checkForIndentationJump)
{
	// at end? return nullptr
	
	if (line_index == lines.size())
		return nullptr;
	
	const char * line = lines[line_index].c_str();
	
	if (skipEmptyLinesAndComments && isEmptyLineOrComment(line))
	{
		line_index++;
		return get_next_line(skipEmptyLinesAndComments);
	}
	else
	{
		// calculate indentation level. less than ours? we reached the end of a block: return nullptr
		
		const size_t next_level = calculateIndentationLevel(line);
		
		if (next_level < indentation_level)
			return nullptr;
		
		// check we aren't skipping more than one indentation level
		
		Assert(!checkForIndentationJump || !do_jump_check || next_level <= indentation_level + 1);
		
		if (checkForIndentationJump && next_level > indentation_level + 1)
			return nullptr;
		
		// return line with appropriate offset to compensate for indentation
		
		line_index++;
		
		return line + indentation_level;
	}
}

void LineReader::skip_current_section()
{
	for (;;)
	{
		const char * line = get_next_line(true, false);
		
		if (line == nullptr)
			break;
	}
}
