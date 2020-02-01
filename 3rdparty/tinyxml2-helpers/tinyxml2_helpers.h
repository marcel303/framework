/*
	Copyright (C) 2020 Marcel Smit
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

// higher level functions for dealing with arrays

void pushAttrib_bytes(tinyxml2::XMLPrinter * printer, const char * name, const void * bytes, const int numBytes);
void pushAttrib_array(tinyxml2::XMLPrinter * printer, const char * name, const void * elems, const int elemSize, const int numElems);

void bytesAttrib(tinyxml2::XMLElement * elem, const char * name, void * bytes, const int numBytes);
void arrayAttrib(tinyxml2::XMLElement * elem, const char * name, void * elems, const int elemSize, const int numElems);
