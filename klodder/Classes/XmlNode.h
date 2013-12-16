#pragma once

#include <map>
#include <string>
#include <vector>
#include "libgg_forward.h"

class XmlNode
{
public:
	XmlNode();
	XmlNode(XmlNode* parent, const char* name);
	~XmlNode();
	
	std::string GetAttribute(const char* name, const char* defaultValue);
	int32_t GetAttribute_Int32(const char* name, int defaultValue);
	uint32_t GetAttribute_UInt32(const char* name, uint32_t defaultValue);
	bool GetAttribute_Bytes(const char* name, Stream* stream);
	
	static std::string BytesEncode(uint8_t* bytes, int byteCount);
	static std::vector<uint8_t> BytesDecode(std::string bytes);
	
	XmlNode* mParent;
	std::string mName;
	std::vector<XmlNode*> mChildNodes;
	std::map<std::string, std::string> mAttributes;
};
