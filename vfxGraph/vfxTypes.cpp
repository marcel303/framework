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

#include "Log.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include "vfxTypes.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace tinyxml2;

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

VfxTimeline::Key::Key()
	: beat(0.f)
	, value(0.f)
{
}

bool VfxTimeline::Key::operator<(const Key & other) const
{
	return beat < other.beat;
}

bool VfxTimeline::Key::operator==(const Key & other) const
{
	return memcmp(this, &other, sizeof(Key)) == 0;
}

bool VfxTimeline::Key::operator!=(const Key & other) const
{
	return !(*this == other);
}

//

VfxTimeline::VfxTimeline()
	: length(60.f)
	, bpm(60.f)
	, keys()
	, numKeys(0)
{
}

bool VfxTimeline::allocKey(Key *& key)
{
	if (numKeys == kMaxKeys)
		return false;
	else
	{
		key = &keys[numKeys++];
		return true;
	}
}

void VfxTimeline::freeKey(Key *& key)
{
	const int index = key - keys;
	for (int i = index + 1; i < numKeys; ++i)
		keys[i - 1] = keys[i];
	numKeys--;
}

void VfxTimeline::clearKeys()
{
	for (int i = 0; i < numKeys; ++i)
		keys[i] = Key();

	numKeys = 0;
}

static bool compareKeysByTime(const VfxTimeline::Key * k1, const VfxTimeline::Key * k2)
{
    return k1->beat < k2->beat;
}

VfxTimeline::Key * VfxTimeline::sortKeys(Key * keyToReturn)
{
	Key * result = 0;

	if (keyToReturn)
	{
		Key keyValues[kMaxKeys];
		memcpy(keyValues, keys, sizeof(Key) * numKeys);
		Key * keysForSorting[kMaxKeys];
		for (int i = 0; i < numKeys; ++i)
			keysForSorting[i] = &keys[i];
        std::sort(keysForSorting, keysForSorting + numKeys, compareKeysByTime);
		for (int i = 0; i < numKeys; ++i)
		{
			if (keysForSorting[i] == keyToReturn)
				result = &keys[i];
			const int index = keysForSorting[i] - keys;
			keys[i] = keyValues[index];
		}
	}
	else
	{
		std::sort(keys, keys + numKeys);
	}

	return result;
}

void VfxTimeline::save(XMLPrinter * printer)
{
	printer->PushAttribute("length", length);
	printer->PushAttribute("bpm", bpm);
	
	for (int i = 0; i < numKeys; ++i)
	{
		printer->OpenElement("key");
		{
			printer->PushAttribute("beat", keys[i].beat);
			printer->PushAttribute("value", keys[i].value);
		}
		printer->CloseElement();
	}
}

void VfxTimeline::load(XMLElement * elem)
{
	length = 60.f;
	bpm = 60.f;
	
	clearKeys();
	
	//
	
	length = floatAttrib(elem, "length", length);
	bpm = floatAttrib(elem, "bpm", bpm);
	
	for (auto keyElem = elem->FirstChildElement("key"); keyElem; keyElem = keyElem->NextSiblingElement())
	{
		Key * key;

		if (allocKey(key))
		{
			key->beat = floatAttrib(keyElem, "beat", 0.f);
			key->value = floatAttrib(keyElem, "value", 0.f);
		}
		else
		{
			LOG_WRN("timeline key allocation failed", 0);
		}
	}

	sortKeys();
}

//

VfxSwizzle::VfxSwizzle()
	: elems()
	, numElems(0)
{

}

void VfxSwizzle::reset()
{
	numElems = 0;
}

bool VfxSwizzle::parse(const char * text)
{
	bool result = true;

	std::vector<std::string> elemTexts;
	splitString(text, elemTexts, ' ');

	if (elemTexts.size() > kMaxElems)
	{
		result = false;
	}
	else
	{
		for (auto & elemText : elemTexts)
		{
			Elem & elem = elems[numElems++];

			elem.channelIndex = Parse::Int32(elemText);
		}
	}

	if (result == false)
	{
		reset();
	}

	return result;
}

//

void VfxOscPath::save(tinyxml2::XMLPrinter * printer)
{
	printer->PushAttribute("path", path.c_str());
}

void VfxOscPath::load(tinyxml2::XMLElement * elem)
{
	path = stringAttrib(elem, "path", "");
}

//

void VfxOscPathList::save(tinyxml2::XMLPrinter * printer)
{
	for (auto & elem : elems)
	{
		printer->OpenElement("elem");
		{
			printer->PushAttribute("path", elem.path.c_str());
			printer->PushAttribute("name", elem.name.c_str());
		}
		printer->CloseElement();
	}
}

void VfxOscPathList::load(tinyxml2::XMLElement * elem)
{
	elems.clear();
	
	for (auto xmlElem = elem->FirstChildElement("elem"); xmlElem != nullptr; xmlElem = xmlElem->NextSiblingElement("elem"))
	{
		Elem elem;
		
		elem.path = stringAttrib(xmlElem, "path", "");
		elem.name = stringAttrib(xmlElem, "name", "");
		
		elems.push_back(elem);
	}
}
