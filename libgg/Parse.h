#pragma once

#include <stdint.h>
#include <string>

class Parse
{
public:
	// conversion with error checking
	
	static bool Int32(const char * text, int32_t & result);
	static bool UInt32(const char * text, uint32_t & result);
	static bool Float(const char * text, float & result);
	static bool Double(const char * text, double & result);
	static bool Bool(const char * text, bool & result);
	
	// conversion without error checking. return 0/false on failure
	
	static int32_t Int32(const char * text);
	static uint32_t UInt32(const char * text);
	static float Float(const char * text);
	static double Double(const char * text);
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
	
	static double Double(const std::string & text)
	{
		return Double(text.c_str());
	}
	
	static bool Bool(const std::string & text)
	{
		return Bool(text.c_str());
	}
	
	//
	
	static bool Int32(const std::string & text, int32_t & result)
	{
		return Int32(text.c_str(), result);
	}
	
	static bool UInt32(const std::string & text, uint32_t & result)
	{
		return UInt32(text.c_str(), result);
	}
	
	static bool Float(const std::string & text, float & result)
	{
		return Float(text.c_str(), result);
	}
	
	static bool Double(const std::string & text, double & result)
	{
		return Double(text.c_str(), result);
	}
	
	static bool Bool(const std::string & text, bool & result)
	{
		return Bool(text.c_str(), result);
	}
};
