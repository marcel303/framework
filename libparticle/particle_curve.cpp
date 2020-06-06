#include "particle.h"

#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

#include <algorithm> // sort
#include <stdlib.h> // realloc
#include <string.h> // memcpy

ParticleCurve::Key::Key()
	: t(0.f)
	, value(0.f)
{
}

bool ParticleCurve::Key::operator<(const Key & other) const
{
	return t < other.t;
}

ParticleCurve::ParticleCurve()
	: keys(nullptr)
	, numKeys(0)
{
	setLinear(0.f, 1.f);
}

void ParticleCurve::setLinear(float v1, float v2)
{
	clearKeys();
	
	Key * k1 = allocKey();
	k1->t = 0.f;
	k1->value = v1;
	
	Key * k2 = allocKey();
	k2->t = 1.f;
	k2->value = v2;
}

ParticleCurve::Key * ParticleCurve::allocKey()
{
	keys = (Key*)realloc(keys, (numKeys + 1) * sizeof(Key));
	
	return &keys[numKeys++];
}

void ParticleCurve::freeKey(Key *& key)
{
	const int index = key - keys;
	
	for (int i = index + 1; i < numKeys; ++i)
		keys[i - 1] = keys[i];
	
	keys = (Key*)realloc(keys, (numKeys - 1) * sizeof(Key));
	
	numKeys--;
}

void ParticleCurve::clearKeys()
{
	free(keys);
	keys = nullptr;
	
	numKeys = 0;
}

static bool compareKeysByTime(const ParticleCurve::Key * k1, const ParticleCurve::Key * k2)
{
    return k1->t < k2->t;
}

ParticleCurve::Key * ParticleCurve::sortKeys(Key * keyToReturn)
{
	Key * result = 0;

	if (keyToReturn)
	{
		Key * keyValues = (Key*)alloca(numKeys * sizeof(Key));
		memcpy(keyValues, keys, sizeof(Key) * numKeys);
		Key ** keysForSorting = (Key**)alloca(numKeys * sizeof(Key*));
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

float ParticleCurve::sample(const float _t) const
{
	const float t = _t < 0.f ? 0.f : _t > 1.f ? 1.f : _t;

	if (numKeys == 0)
		return 1.f;
	else if (numKeys == 1)
		return keys[0].value;
	else
	{
		int endKey = 0;

		while (endKey < numKeys)
		{
			if (t < keys[endKey].t)
				break;
			else
				++endKey;
		}

		if (endKey == 0)
			return keys[0].value;
		else if (endKey == numKeys)
			return keys[numKeys - 1].value;
		else
		{
			const int startKey = endKey - 1;
			const float c1 = keys[startKey].value;
			const float c2 = keys[endKey].value;
			const float t2 = (t - keys[startKey].t) / (keys[endKey].t - keys[startKey].t);
			return c1 * (1.f - t2) + c2 * t2;
		}
	}
}

void ParticleCurve::save(tinyxml2::XMLPrinter * printer) const
{
	printer->PushAttribute("numKeys", numKeys);
	for (int i = 0; i < numKeys; ++i)
	{
		printer->OpenElement("key");
		{
			printer->PushAttribute("t", keys[i].t);
			printer->PushAttribute("value", keys[i].value);
		}
		printer->CloseElement();
	}
}

void ParticleCurve::load(const tinyxml2::XMLElement * elem)
{
	clearKeys();
	
	//
	
	for (const tinyxml2::XMLElement * keyElem = elem->FirstChildElement("key"); keyElem; keyElem = keyElem->NextSiblingElement())
	{
		Key * key = allocKey();

		key->t = floatAttrib(keyElem, "t", 0.f);
		key->value = floatAttrib(keyElem, "value", 0.f);
	}

	if (numKeys == 0)
	{
		setLinear(0.f, 1.f);
	}
}
