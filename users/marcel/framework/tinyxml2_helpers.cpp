/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

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
