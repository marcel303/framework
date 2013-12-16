#pragma once

#include "klodder_forward.h"

class XmlReader
{
public:
	XmlReader();
	~XmlReader();
	void Load(Stream* stream);
	
	XmlNode* RootNode_get();
	
private:
	XmlNode* mRootNode;
};
