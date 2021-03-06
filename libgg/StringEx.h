#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <stddef.h>
#include <stdio.h> // snprintf
#include <stdint.h>

class String
{
public:
	static std::string Empty;
	
	static std::string Format(std::string text, ...);
	static std::string FormatC(const char* text, ...);
	static std::string TrimLeft(const std::string& text);
	static std::string TrimRight(const std::string& text);
	static std::string Trim(const std::string& text);
	static std::vector<std::string> Split(const std::string& text, char separator);
	static std::vector<std::string> Split(const std::string& text, const std::string& separators);
	static int Find(const std::string& text, char value);
	static bool Contains(const std::string& text, char value);
	static bool StartsWith(const std::string& text, const std::string& substring);
	static bool StartsWithC(const char* text, const char* substring);
	static bool EndsWith(const std::string& text, const std::string& substring);
	static std::string Replace(const std::string& text, char c1, char c2);
	static std::string ToUpper(const std::string& text);
	static std::string ToLower(const std::string& text);
	static std::string ToString(int value);
	static std::string ToString(double value);
	static std::string SubString(const std::string& text, size_t offset);
	static std::string SubString(const std::string& text, size_t offset, size_t length);
	static bool Equals(const char* text1, const char* text2);
	static bool MatchesWildcard(const char * text, const char * wildcard);
	static std::string Join(const std::vector<std::string>& strings);
	static std::string Join(const std::vector<std::string>& strings, const std::string& separator);
	
};

#if defined(PSP) || defined(__GNUC__)
    #define sprintf_s(s, ss, f, ...) snprintf(s, ss, f, ##__VA_ARGS__)
    #define vsprintf_s(s, ss, f, a) vsnprintf(s, ss, f, a)
    #define strcpy_s __libgg_strcpy_s
    #define sscanf_s sscanf

    int __libgg_strcpy_s(char * dst, size_t dst_size, const char * src);
#endif

#if defined(WINDOWS)
	#define strcasecmp _stricmp
	extern char * strcasestr(const char * haystack, const char * needle);
#endif
