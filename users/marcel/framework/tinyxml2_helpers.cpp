#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

using namespace tinyxml2;

const char * stringAttrib(const XMLElement * elem, const char * name, const char * defaultValue)
{
	if (elem->Attribute(name))
		return elem->Attribute(name);
	else
		return defaultValue;
}

bool boolAttrib(const XMLElement * elem, const char * name, const bool defaultValue)
{
	if (elem->Attribute(name))
		return elem->BoolAttribute(name);
	else
		return defaultValue;
}

int intAttrib(const XMLElement * elem, const char * name, const int defaultValue)
{
	if (elem->Attribute(name))
		return elem->IntAttribute(name);
	else
		return defaultValue;
}

float floatAttrib(const XMLElement * elem, const char * name, const float defaultValue)
{
	if (elem->Attribute(name))
		return elem->FloatAttribute(name);
	else
		return defaultValue;
}
