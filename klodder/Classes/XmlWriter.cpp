#include "Stream.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "XmlWriter.h"

XmlWriter::XmlWriter()
{
	mStream = 0;
	mOwned = false;
	mCurrentNode = 0;
}

XmlWriter::XmlWriter(Stream* stream, bool own)
{
	mStream = 0;
	mOwned = false;
	mCurrentNode = 0;
	
	Open(stream, own);
}

XmlWriter::~XmlWriter()
{
	if (mStream)
	{
		Close();
	}
}

void XmlWriter::Open(Stream* stream, bool own)
{
	mStream = stream;
	mOwned = own;
	mCurrentNode = &mRootNode;
}

void XmlWriter::Close()
{
	Flush();
	
	if (mOwned)
	{
		delete mStream;
		mStream = 0;
	}
	
	mStream = 0;
	mOwned = false;
	mCurrentNode = 0;
}

void XmlWriter::Flush()
{
	WriteNode(&mRootNode);
}

void XmlWriter::WriteNode(XmlNode* node)
{
	StreamWriter writer(mStream, false);
	
	if (!node->mName.empty())
	{
		writer.WriteText(String::Format("<%s ", node->mName.c_str()));
		for (std::map<std::string, std::string>::iterator i = node->mAttributes.begin(); i != node->mAttributes.end(); ++i)
			writer.WriteText(String::Format("%s=\"%s\" ", i->first.c_str(), i->second.c_str()));
		writer.WriteText(">");
	}
	
	for (size_t i = 0; i < node->mChildNodes.size(); ++i)
		WriteNode(node->mChildNodes[i]);
	
	if (!node->mName.empty())
	{
		writer.WriteText(String::Format("</%s>", node->mName.c_str()));
	}
}

void XmlWriter::BeginNode(const char* name)
{
	XmlNode* node = new XmlNode(mCurrentNode, name);
	
	mCurrentNode->mChildNodes.push_back(node);
	
	mCurrentNode = node;
}

void XmlWriter::EndNode()
{
	mCurrentNode = mCurrentNode->mParent;
}

void XmlWriter::WriteAttribute(const char* name, const char* value)
{
	mCurrentNode->mAttributes[name] = value;
}

void XmlWriter::WriteAttribute_Int32(const char* name, int value)
{
	WriteAttribute(name, String::Format("%d", value).c_str());
}

void XmlWriter::WriteAttribute_UInt32(const char* name, uint32_t value)
{
	WriteAttribute(name, String::Format("%d", value).c_str());
}

void XmlWriter::WriteAttribute_Bytes(const char* name, uint8_t* bytes, int byteCount)
{
	WriteAttribute(name, XmlNode::BytesEncode(bytes, byteCount).c_str());
}
