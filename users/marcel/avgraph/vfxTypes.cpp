#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include "vfxTypes.h"
#include <algorithm>

using namespace tinyxml2;

VfxTimeline::Key::Key()
	: beat(0.0)
	, id(0)
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
			printer->PushAttribute("id", keys[i].id);
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
			key->id = intAttrib(keyElem, "id", 0);
		}
		else
		{
			// todo : emit warning?
		}
	}

	sortKeys();
}
