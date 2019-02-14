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
#include <stdint.h>
#include <stdlib.h>

#ifdef WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif

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

//

void pushAttrib_bytes(tinyxml2::XMLPrinter * printer, const char * name, const void * _bytes, const int numBytes)
{
	if (numBytes == 0)
		printer->PushAttribute(name, "");
	else
	{
		const uint8_t * bytes = (uint8_t*)_bytes;
		
		char * text = (char*)alloca(numBytes * 2 + 1);
		
		for (int i = 0; i < numBytes; ++i)
		{
			const uint8_t v = bytes[i];
			
			const int v1 = (v >> 0) & 0xf;
			const int v2 = (v >> 4) & 0xf;
			
			const int c1 = v1 >= 0 && v1 <= 9 ? '0' + (v1 - 0) : 'a' + (v1 - 10);
			const int c2 = v2 >= 0 && v2 <= 9 ? '0' + (v2 - 0) : 'a' + (v2 - 10);
			
			text[i * 2 + 0] = c1;
			text[i * 2 + 1] = c2;
		}
		
		text[numBytes * 2] = 0;
		
		printer->PushAttribute(name, text);
	}
}

void pushAttrib_array(tinyxml2::XMLPrinter * printer, const char * name, const void * elems, const int elemSize, const int numElems)
{
#if defined(__BIG_ENDIAN__)
	#error "perform endian conversion when needed"
#endif

	// todo : perform endian conversion when needed

	pushAttrib_bytes(printer, name, elems, elemSize * numElems);
}

static int decodeAndConsume(const char *& text)
{
	if (*text == 0)
		return 0;

	const char c = *text;
	
	const int v =
		c >= '0' && c <= '9' ?  0 + c - '0' :
		c >= 'a' && c <= 'f' ? 10 + c - 'a' :
		0;
	
	text++;
	
	return v;
}

void bytesAttrib(tinyxml2::XMLElement * elem, const char * name, void * _bytes, const int numBytes)
{
	if (numBytes == 0)
		return;
	
	uint8_t * bytes = (uint8_t*)_bytes;
	
	memset(bytes, 0, numBytes);
	
	const char * text = stringAttrib(elem, name, nullptr);
	
	if (text == nullptr || text[0] == 0)
		return;
	
	for (int i = 0; i < numBytes; ++i)
	{
		const int v1 = decodeAndConsume(text);
		const int v2 = decodeAndConsume(text);
		
		bytes[i] = v1 | (v2 << 4);
	}
}

void arrayAttrib(tinyxml2::XMLElement * elem, const char * name, void * elems, const int elemSize, const int numElems)
{
	bytesAttrib(elem, name, elems, elemSize * numElems);

#if defined(__BIG_ENDIAN__)
	#error "perform endian conversion when needed"
#endif

	// todo : perform endian conversion when needed
}
