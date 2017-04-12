#pragma once

// tinyxml helper functions

namespace tinyxml2
{
	class XMLElement;
}

const char * stringAttrib(const tinyxml2::XMLElement * elem, const char * name, const char * defaultValue);
bool boolAttrib(const tinyxml2::XMLElement * elem, const char * name, const bool defaultValue);
int intAttrib(const tinyxml2::XMLElement * elem, const char * name, const int defaultValue);
float floatAttrib(const tinyxml2::XMLElement * elem, const char * name, const float defaultValue);
