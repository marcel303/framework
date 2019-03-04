#pragma once

#include <stdarg.h>
#include <string>

#include <vector> // todo : remove. use LineReader to read individual lines

struct LineWriter
{
	static const int kMaxFormattedTextSize = 1 << 12;
	
	std::string text;
	
	LineWriter()
	{
		text.reserve(1 << 16);
	}
	
	void Append(const char c)
	{
		text.push_back(c);
	}
	
	void Append(const char * text)
	{
		this->text.append(text);
	}
	
	void AppendFormat(const char * format, ...)
	{
		va_list va;
		va_start(va, format);
		char text[kMaxFormattedTextSize];
		vsprintf(text, format, va);
		va_end(va);

		Append(text);
	}
	
	std::vector<std::string> ToLines() const // todo : remove
	{
		// todo : remove !
		
		std::vector<std::string> result;
		
		size_t begin = 0;
		size_t end = 0;
		
		const char * str = text.c_str();
		
		while (str[begin] != 0)
		{
			while (str[end] != 0 && str[end] != '\n')
				end++;
			
			result.emplace_back(text.substr(begin, end - begin));
			
			begin = end + 1;
			end = begin;
		}
		
		return result;
	}
};
