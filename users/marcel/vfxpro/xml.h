#pragma once

namespace tinyxml2
{
	class XMLElement;
}

// tinyxml helper functions

const char * stringAttrib(const tinyxml2::XMLElement * elem, const char * name, const char * defaultValue);
bool boolAttrib(const tinyxml2::XMLElement * elem, const char * name, bool defaultValue);
int intAttrib(const tinyxml2::XMLElement * elem, const char * name, int defaultValue);
float floatAttrib(const tinyxml2::XMLElement * elem, const char * name, float defaultValue);
