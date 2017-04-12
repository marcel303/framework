#include "Exception.h"
#include "Parse.h"
#include "Stream.h"
#include "StringEx.h"
#include "XmlNode.h"

XmlNode::XmlNode()
{
	mParent = 0;
}

XmlNode::XmlNode(XmlNode* parent, const char* name)
{
	mParent = parent;
	mName = name;
}

XmlNode::~XmlNode()
{
	for (size_t i = 0; i < mChildNodes.size(); ++i)
		delete mChildNodes[i];
	
	mChildNodes.clear();
}

std::string XmlNode::GetAttribute(const char* name, const char* defaultValue)
{
	std::map<std::string, std::string>::iterator i = mAttributes.find(name);
	
	if (i == mAttributes.end())
		return defaultValue;
	else
		return i->second;
}

int32_t XmlNode::GetAttribute_Int32(const char* name, int defaultValue)
{
	std::string value = GetAttribute(name, String::Format("%d", defaultValue).c_str());

	return Parse::Int32(value);
}

uint32_t XmlNode::GetAttribute_UInt32(const char* name, uint32_t defaultValue)
{
	std::string value = GetAttribute(name, String::Format("%lu", defaultValue).c_str());

	return Parse::UInt32(value);
}

bool XmlNode::GetAttribute_Bytes(const char* name, Stream* stream)
{
	std::string value = GetAttribute(name, "");
	
	if (value == "")
		return false;
	
	std::vector<uint8_t> bytes = BytesDecode(value);
	
	stream->Write(&bytes[0], (int)bytes.size());
	
	return true;
}

std::string XmlNode::BytesEncode(uint8_t* bytes, int byteCount)
{
	std::string result;
	
	for (int i = 0; i < byteCount; ++i)
	{
		int v1 = (bytes[i] >> 0) & 0xF;
		int v2 = (bytes[i] >> 4) & 0xF;
		
		result.push_back('a' + v1);
		result.push_back('a' + v2);
	}
	
	return result;
}

std::vector<uint8_t> XmlNode::BytesDecode(std::string bytes)
{
	if (bytes.size() % 2 != 0)
		throw ExceptionVA("invalid XML byte array");
	
	std::vector<uint8_t> result;
	
	for (size_t i = 0; i < bytes.size() / 2; ++i)
	{
		int v1 = (bytes[i * 2 + 0] - 'a') << 0;
		int v2 = (bytes[i * 2 + 1] - 'a') << 4;
		
		result.push_back(v1 | v2);
	}
	
	return result;
}
