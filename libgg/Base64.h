#pragma once

#include <string>
#include "ByteString.h"

// Base64 conversion functions

class Base64
{
public:
	static std::string Encode(const ByteString& data);
	static ByteString Decode(const char* text);
	
	static std::string HttpFix(const char* text);
	static std::string HttpUnFix(const char* text);
	
	static void DBG_SelfTest();
};
