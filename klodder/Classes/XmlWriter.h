#pragma once

#include "libgg_forward.h"
#include "XmlNode.h"

class XmlWriter
{
public:
	XmlWriter();
	XmlWriter(Stream* stream, bool own);	
	~XmlWriter();	
	void Open(Stream* stream, bool own);
	void Close();
	
private:
	void Flush();
	void WriteNode(XmlNode* node);	
	
public:
	void BeginNode(const char* name);
	void EndNode();
	void WriteAttribute(const char* name, const char* value);
	void WriteAttribute_Int32(const char* name, int value);
	void WriteAttribute_UInt32(const char* name, uint32_t value);
	void WriteAttribute_Bytes(const char* name, uint8_t* bytes, int byteCount);
	
private:
	Stream* mStream;
	bool mOwned;
	
	XmlNode mRootNode;
	XmlNode* mCurrentNode;
};
