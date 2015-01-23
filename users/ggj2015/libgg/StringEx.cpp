#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "Exception.h"
#include "StringEx.h"

std::string String::Empty;

std::string String::Format(std::string text, ...)
{
	va_list va;
	char temp[1024];
	va_start(va, text);
	vsprintf(temp, text.c_str(), va);
	va_end(va);
	return std::string(temp);
}

std::string String::FormatC(const char* text, ...)
{
	va_list va;
	char temp[1024];
	va_start(va, text);
	vsprintf(temp, text, va);
	va_end(va);
	return std::string(temp);
}

std::string String::TrimLeft(const std::string& text)
{
	size_t start = 0;

	for (; start < text.length(); ++start)
	{
		if (text[start] != '\t' && text[start] != ' ' && text[start] != '\0')
			break;
	}

	return text.substr(start);
}

std::string String::TrimRight(const std::string& text)
{
	int end = (int)text.length() - 1;

	for (; end >= 0; --end)
	{
		if (text[end] != '\t' && text[end] != ' ' && text[end] != '\0')
			break;
	}

	return text.substr(0, end + 1);
}

std::string String::Trim(const std::string& text)
{
	return TrimRight(TrimLeft(text));
}

std::vector<std::string> String::Split(const std::string& text, char separator)
{
	char temp[] = { separator, 0 };

	return Split(text, temp);
}

std::vector<std::string> String::Split(const std::string& text, const std::string& separators)
{
	std::vector<std::string> result;

	size_t index1 = 0;
	size_t index2 = 0;

	for (size_t i = 0; i < text.length(); ++i)
	{
		if (Contains(separators, text[i]))
		{
			if (!Contains(separators, text[index1]))
			{
				std::string temp = text.substr(index1, index2 - index1 + 1);

//				for (size_t j = index1; j <= index2; ++j)
//					temp.push_back(text[j]);

				result.push_back(temp);
			}

			index1 = i + 1;
			index2 = i + 1;
		}
		else
		{
			index2 = i;
		}
	}

	if (index1 != text.length())
	{
		if (!Contains(separators, text[index1]))
		{
			std::string temp = text.substr(index1, index2 - index1 + 1);

//			for (size_t j = index1; j <= index2; ++j)
//				temp.push_back(text[j]);

			result.push_back(temp);
		}
	}

	return result;
}

int String::Find(const std::string& text, char value)
{
	for (size_t i = 0; i < text.length(); ++i)
	{
		if (text[i] == value)
			return (int)i;
	}
	
	return -1;
}

bool String::Contains(const std::string& text, char c)
{
	for (size_t i = 0; i < text.length(); ++i)
		if (text[i] == c)
			return true;
	
	return false;
}

bool String::StartsWith(const std::string& text, const std::string& substring)
{
	size_t length1 = text.length();
	size_t length2 = substring.length();

	if (length1 < length2)
		return false;

	for (size_t i = 0; i < length2; ++i)
		if (text[i] != substring[i])
			return false;

	return true;
}

bool String::EndsWith(const std::string& text, const std::string& substring)
{
	size_t length1 = text.length();
	size_t length2 = substring.length();

	if (length1 < length2)
		return false;

	size_t start = length1 - length2;

	for (size_t i = 0; i < length2; ++i)
		if (text[start + i] != substring[i])
			return false;

	return true;
}

std::string String::Replace(const std::string& text, char c1, char c2)
{
	std::string temp;
	
	temp.resize(text.length());
	
	for (size_t i = 0; i < text.length(); ++i)
		if (text[i] == c1)
			temp[i] = c2;
		else
			temp[i] = text[i];
	
	return temp;
}

static inline char ToUpper(char c)
{
	if (c >= 'a' && c <= 'z')
		c = c - 'a' + 'A';
	
	return c;
}

static inline char ToLower(char c)
{
	if (c >= 'A' && c <= 'Z')
		c = c - 'A' + 'a';
	
	return c;
}

std::string String::ToUpper(const std::string& text)
{
	const size_t length = text.length();
	
	std::string temp;
	
	temp.resize(length);
	
	for (size_t  i = 0; i < length; ++i)
		temp[i] = ::ToUpper(text[i]);
	
	return temp;
}

std::string String::ToLower(const std::string& text)
{
	const size_t length = text.length();
	
	std::string temp;
	
	temp.resize(length);
	
	for (size_t  i = 0; i < length; ++i)
		temp[i] = ::ToLower(text[i]);
	
	return temp;
}

std::string String::ToString(int value)
{
#if 1
	return String::Format("%d", value);
#else
	std::stringstream stream;
	
	stream << value;
	
	return stream.str();
#endif
}

std::string String::SubString(const std::string& text, size_t offset)
{
	if (offset > text.length())
		throw ExceptionVA("index out of bounds");
	
#if 0
	std::string result;
	
	result.reserve(text.length() - offset);
	
	for (size_t i = offset; i < text.length(); ++i)
		result.push_back(text[i]);
	
	return result;
#else
	return text.substr(offset, text.length() - offset);
#endif
}

std::string String::SubString(const std::string& text, size_t offset, size_t length)
{
	if (offset + length > text.length())
		throw ExceptionVA("index out of bounds");
	
#if 0
	std::string result;
	
	result.reserve(length);
	
	for (size_t i = offset; i < offset + length; ++i)
		result.push_back(text[i]);
	
	return result;
#else
	return text.substr(offset, length);
#endif
}

bool String::Equals(const char* text1, const char* text2)
{
	return strcmp(text1, text2) == 0;
}

std::string String::Join(const std::vector<std::string>& strings)
{
	std::string result;
	
	size_t length = 0;
	
	for (size_t i = 0; i < strings.size(); ++i)
		length += strings[i].length();
	
	result.reserve(length);
	
	for (size_t i = 0; i < strings.size(); ++i)
		result.append(strings[i]);
	
	return result;
}

std::string String::Join(const std::vector<std::string>& strings, const std::string& separator)
{
	std::string result;
	
	size_t length = 0;
	
	for (size_t i = 0; i < strings.size(); ++i)
		length += strings[i].length();
	
	if (strings.size() > 1)
		length += separator.length() * (strings.size() - 1);
	
	result.reserve(length);
	
	for (size_t i = 0; i < strings.size(); ++i)
	{
		result.append(strings[i]);
		
		if (i + 1 < strings.size())
			result.append(separator);
	}
	
	return result;
}
