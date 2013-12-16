#pragma once

#include <string>
#include "ByteString.h"

// Base64 conversion functions

class Base64
{
public:
	static std::string Encode(const ByteString& data);
	static ByteString Decode(const std::string& text);
	
	static std::string HttpFix(const std::string& text);
	static std::string HttpUnFix(const std::string& text);
	
	static void DBG_SelfTest();
};
