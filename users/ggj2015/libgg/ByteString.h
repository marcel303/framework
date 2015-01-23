#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class ByteString
{
public:
	ByteString();
	ByteString(int length);
	static ByteString FromString(const std::string& text);
	
	int Length_get() const;
	void Length_set(int length);
	
	ByteString Extract(int offset, int length);
	std::string ToString() const;
	
	bool Equals(const ByteString& other);
	
	std::vector<uint8_t> m_Bytes;
};
