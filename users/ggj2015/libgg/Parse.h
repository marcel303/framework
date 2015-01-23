#pragma once

#include <string>

class Parse
{
public:
	static int Int32(const std::string& text);
	static unsigned int UInt32(const std::string& text);
	static float Float(const std::string& text);
	static bool Bool(const std::string& text);
};
