/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

#if ENABLE_OPENGL

#include "shaderBuilder.h"
#include "StringBuilder.h"
#include "StringEx.h"
#include "TextIO.h"

static bool is_whitespace(const char c)
{
	return isspace(c);
}

// eats an arbitrary word and stores the result in 'word'. has built-in support for quoted strings
static bool eat_word_v2(char *& line, const char *& word)
{
	while (*line != 0 && is_whitespace(*line) == true)
		line++;
	
	if (*line == 0)
		return false;
	
	const bool isQuoted = *line == '"';

	if (isQuoted)
	{
		line++;

		word = line;
		
		while (*line != 0 && *line != '"')
			line++;
	}
	else
	{
		word = line;
		
		while (*line != 0 && *line != ';' && is_whitespace(*line) == false)
			line++;
	}
	
	if (line > word)
	{
		if (*line != 0)
		{
			*line = 0;
			line++;
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

// checks if 'line' begin with 'word' and eats 'word' from 'line' when it does
static bool eat_word(char * & line, const char * word)
{
	while (is_whitespace(*line))
		line++;
	
	int index = 0;
	
	while (word[index] != 0 && word[index] != ';')
	{
		if (line[index] != word[index])
			return false;
		
		index++;
	}

	if (is_whitespace(line[index]) == false && line[index] != 0)
		return false;

	while (is_whitespace(line[index]))
			index++;

	line += index;
	return true;
}

static void report_error(const char * line, const char * format, ...)
{
	char text[1024];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(text, sizeof(text), format, ap);
	va_end(ap);
	
	printf("error: %s\n", text);
}

bool buildOpenglText(const char * text, const char shaderType, const char * outputs, std::string & result)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;

	if (!TextIO::loadText(text, lines, lineEndings))
	{
		return false;
	}
	else
	{
		StringBuilder<32 * 1024> sb; // todo : replace with a more efficient and growing string builder
		
	#if !LEGACY_GL
		if (shaderType == 'p')
		{
			bool hasColor = false;
			bool hasNormal = false;
			
			for (int i = 0; outputs[i] != 0; ++i)
			{
				if (outputs[i] == 'c')
				{
					if (hasColor)
					{
						logError("color output binding appears more than once");
						result = false;
					}
					else
					{
						hasColor = true;
						sb.AppendFormat("layout(location = %d) out vec4 shader_fragColor;\n", i);
					}
				}
				else if (outputs[i] == 'n')
				{
					if (hasNormal)
					{
						logError("normal output binding appears more than once");
						result = false;
					}
					else
					{
						hasNormal = true;
						sb.AppendFormat("layout(location = %d) out vec4 shader_fragNormal;\n", i);
					}
				}
				else
				{
					logError("unknown output binding: %c", outputs[i]);
					result = false;
				}
			}
			
			// use regular variables for unused outputs
			
			if (hasColor == false)
				sb.Append("vec4 shader_fragColor;\n");
			if (hasNormal == false)
				sb.Append("vec4 shader_fragNormal;\n");
		}
	#endif
	
		for (auto & line : lines)
		{
			char * linePtr = (char*)line.c_str();
			
			if (eat_word(linePtr, "shader_attrib"))
			{
				const char * type;
				const char * name;
				const char * index;
				
				if (!eat_word_v2(linePtr, type))
				{
					report_error(line.c_str(), "attribute is missing type");
					return false;
				}
				
				if (!eat_word_v2(linePtr, name))
				{
					report_error(line.c_str(), "attribute is missing name");
					return false;
				}
				
				if (!eat_word_v2(linePtr, index))
				{
					report_error(line.c_str(), "attribute is missing index");
					return false;
				}
				
				sb.AppendFormat("shader_attrib %s %s;\n", type, name);
			}
			else
			{
				sb.AppendFormat("%s\n", line.c_str());
			}
		}
		
		result = sb.ToString();
		
		return true;
	}
}

#endif
