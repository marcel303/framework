#include "Exception.h"
#include "StreamReader.h"
#include "tinyxml.h"
#include "XmlNode.h"
#include "XmlReader.h"

XmlReader::XmlReader()
{
	mRootNode = 0;
}

XmlReader::~XmlReader()
{
	delete mRootNode;
}

static void Load(XmlNode* p, TiXmlElement* e)
{
	p->mName = e->Value();

	// load attributes

	for (TiXmlAttribute* tiAttribute = e->FirstAttribute(); tiAttribute; tiAttribute = tiAttribute->Next())
	{
		p->mAttributes[tiAttribute->Name()] = tiAttribute->Value();
	}

	// load child nodes

	for (TiXmlElement* tiChild = e->FirstChildElement(); tiChild; tiChild = tiChild->NextSiblingElement())
	{
		XmlNode* child = new XmlNode();

		child->mParent = p;

		p->mChildNodes.push_back(child);

		Load(child, tiChild);
	}
}

void XmlReader::Load(Stream* stream)
{
	StreamReader reader(stream, false);

	uint8_t* bytes = reader.ReadAllBytes();

	TiXmlDocument doc;

	doc.Parse((char*)bytes);

	TiXmlElement* e = doc.FirstChildElement();

	mRootNode = new XmlNode();

	::Load(mRootNode, e);

	delete[] bytes;
}

XmlNode* XmlReader::RootNode_get()
{
	return mRootNode;
}
