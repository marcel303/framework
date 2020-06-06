#include "particle.h"

#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

#include <algorithm> // sort
#include <stdlib.h> // realloc
#include <string.h> // memcpy

ParticleColorCurve::Key::Key()
	: t(0.f)
	, color()
{
}

bool ParticleColorCurve::Key::operator<(const Key & other) const
{
	return t < other.t;
}

//

ParticleColorCurve::ParticleColorCurve()
	: keys(nullptr)
	, numKeys(0)
	, useLinearColorSpace(true)
{
}

ParticleColorCurve::Key * ParticleColorCurve::allocKey()
{
	keys = (Key*)realloc(keys, (numKeys + 1) * sizeof(Key));
	
	return &keys[numKeys++];
}

void ParticleColorCurve::freeKey(Key *& key)
{
	const int index = key - keys;
	
	for (int i = index + 1; i < numKeys; ++i)
		keys[i - 1] = keys[i];
	
	keys = (Key*)realloc(keys, (numKeys - 1) * sizeof(Key));
	
	numKeys--;
}

void ParticleColorCurve::clearKeys()
{
	free(keys);
	keys = nullptr;
	
	numKeys = 0;
}

static bool compareKeysByTime2(const ParticleColorCurve::Key * k1, const ParticleColorCurve::Key * k2)
{
    return k1->t < k2->t;
}

ParticleColorCurve::Key * ParticleColorCurve::sortKeys(Key * keyToReturn)
{
	Key * result = nullptr;

	if (keyToReturn)
	{
		Key * keyValues = (Key*)alloca(numKeys * sizeof(Key));
		memcpy(keyValues, keys, numKeys * sizeof(Key));
		Key ** keysForSorting = (Key**)alloca(numKeys * sizeof(Key*));
		for (int i = 0; i < numKeys; ++i)
			keysForSorting[i] = &keys[i];
        std::sort(keysForSorting, keysForSorting + numKeys, compareKeysByTime2);
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

void ParticleColorCurve::setLinear(const ParticleColor & v1, const ParticleColor & v2)
{
	clearKeys();

	Key * k1 = allocKey();
	k1->t = 0.f;
	k1->color = v1;

	Key * k2 = allocKey();
	k2->t = 1.f;
	k2->color = v2;

	sortKeys();
}

void ParticleColorCurve::setLinearAlpha(float v1, float v2)
{
	clearKeys();

	Key * k1 = allocKey();
	k1->t = 0.f;
	k1->color.set(1.f, 1.f, 1.f, v1);

	Key * k2 = allocKey();
	k2->t = 1.f;
	k2->color.set(1.f, 1.f, 1.f, v2);

	sortKeys();
}

void ParticleColorCurve::sample(const float t, const bool linearColorSpace, ParticleColor & result) const
{
	if (numKeys == 0)
		result.set(1.f, 1.f, 1.f, 1.f);
	else if (numKeys == 1)
		result = keys[0].color;
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
			result = keys[0].color;
		else if (endKey == numKeys)
			result = keys[numKeys - 1].color;
		else
		{
			const int startKey = endKey - 1;
			const ParticleColor & c1 = keys[startKey].color;
			const ParticleColor & c2 = keys[endKey].color;
			const float t2 = (t - keys[startKey].t) / (keys[endKey].t - keys[startKey].t);
			if (linearColorSpace)
				result.interpolateBetweenLinear(c1, c2, t2);
			else
				result.interpolateBetween(c1, c2, t2);
		}
	}
}

void ParticleColorCurve::save(tinyxml2::XMLPrinter * printer) const
{
	printer->PushAttribute("useLinearColorSpace", useLinearColorSpace);
	
	for (int i = 0; i < numKeys; ++i)
	{
		printer->OpenElement("key");
		{
			printer->PushAttribute("t", keys[i].t);
			keys[i].color.save(printer);
		}
		printer->CloseElement();
	}
}

void ParticleColorCurve::load(const tinyxml2::XMLElement * elem)
{
	clearKeys();
	
	//
	
	useLinearColorSpace = boolAttrib(elem, "useLinearColorSpace", true);
	
	for (auto keyElem = elem->FirstChildElement("key"); keyElem; keyElem = keyElem->NextSiblingElement())
	{
		Key * key = allocKey();
		key->t = floatAttrib(keyElem, "t", 0.f);
		key->color.load(keyElem);
	}

	sortKeys();
}
