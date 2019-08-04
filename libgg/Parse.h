#pragma once

#include <string>

class Parse
{
public:
	static int Int32(const char * text);
	static unsigned int UInt32(const char * text);
	static float Float(const char * text);
	static bool Bool(const char * text);
	
	// convenience functions for std::string
	
	static int Int32(const std::string & text)
	{
		return Int32(text.c_str());
	}
	
	static unsigned int UInt32(const std::string & text)
	{
		return UInt32(text.c_str());
	}
	
	static float Float(const std::string & text)
	{
		return Float(text.c_str());
	}
	
	static bool Bool(const std::string & text)
	{
		return Bool(text.c_str());
	}
};
