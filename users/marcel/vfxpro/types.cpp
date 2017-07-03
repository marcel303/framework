#include "Calc.h"
#include "Debugging.h"
#include "framework.h"
#include "types.h"

ParticleSystem::ParticleSystem(const int numElements)
	: numParticles(0)
	, numFree(0)
{
	resize(numElements);
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::resize(const int numElements)
{
	numParticles = numElements;

	freeList.resize(numElements, true);
	for (int i = 0; i < numElements; ++i)
		freeList[i] = i;
	numFree = numElements;

	alive.resize(numElements, true);
	autoKill.resize(numElements, true);

	x.resize(numElements, true);
	y.resize(numElements, true);
	vx.resize(numElements, true);
	vy.resize(numElements, true);
	sx.resize(numElements, true);
	sy.resize(numElements, true);
	angle.resize(numElements, true);
	vangle.resize(numElements, true);
	life.resize(numElements, true);
	lifeRcp.resize(numElements, true);
	hasLife.resize(numElements, true);
}

bool ParticleSystem::alloc(const bool _autoKill, float _life, int & id)
{
	if (numFree == 0)
	{
		id = -1;

		return false;
	}
	else
	{
		id = freeList[--numFree];

		fassert(!alive[id]);

		alive[id] = true;
		autoKill[id] = _autoKill;

		x[id] = 0.f;
		y[id] = 0.f;
		vx[id] = 0.f;
		vy[id] = 0.f;
		sx[id] = 1.f;
		sy[id] = 1.f;

		angle[id] = 0.f;
		vangle[id] = 0.f;

		if (_life == 0.f)
		{
			life[id] = 1.f;
			lifeRcp[id] = 1.f;
			hasLife[id] = false;
		}
		else
		{
			life[id] = _life;
			lifeRcp[id] = 1.f / _life;
			hasLife[id] = true;
		}

		return true;
	}
}

void ParticleSystem::free(const int id)
{
	if (isValidIndex(id))
	{
		fassert(alive[id]);

		alive[id] = false;

		freeList[numFree++] = id;
	}
}

void ParticleSystem::tick(const float dt)
{
	for (int i = 0; i < numParticles; ++i)
	{
		if (alive[i])
		{
			if (hasLife[i])
			{
				life[i] = life[i] - dt;
			}

			if (life[i] < 0.f)
			{
				life[i] = 0.f;

				if (autoKill[i])
				{
					free(i);

					continue;
				}
			}

			x[i] += vx[i] * dt;
			y[i] += vy[i] * dt;

			angle[i] += vangle[i] * dt;
		}
	}
}

void ParticleSystem::draw(const float alpha)
{
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numParticles; ++i)
		{
			if (alive[i])
			{
				const float value = life[i] * lifeRcp[i];

				gxColor4f(1.f, 1.f, 1.f, alpha * value);

				const float s = std::sinf(angle[i]);
				const float c = std::cosf(angle[i]);

				const float sx_2 = sx[i] * .5f;
				const float sy_2 = sy[i] * .5f;

				const float s_sx_2 = s * sx_2;
				const float s_sy_2 = s * sy_2;
				const float c_sx_2 = c * sx_2;
				const float c_sy_2 = c * sy_2;

				gxTexCoord2f(0.f, 0.f); gxVertex2f(x[i] + (-c_sx_2 - s_sy_2), y[i] + (+s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x[i] + (+c_sx_2 - s_sy_2), y[i] + (-s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x[i] + (+c_sx_2 + s_sy_2), y[i] + (-s_sx_2 + c_sy_2));
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x[i] + (-c_sx_2 + s_sy_2), y[i] + (+s_sx_2 + c_sy_2));
			}
		}
	}
	gxEnd();
}

//

BlendMode parseBlendMode(const std::string & blend)
{
	if (blend == "add")
		return kBlendMode_Add;
	else if (blend == "subtract")
		return kBlendMode_Subtract;
	else if (blend == "alpha")
		return kBlendMode_Alpha;
	else if (blend == "premultiplied_alpha" || blend == "pre_alpha")
		return kBlendMode_PremultipliedAlpha;
	else if (blend == "opaque")
		return kBlendMode_Opaque;
	else if (blend == "alpha_test")
		return kBlendMode_AlphaTest;
	else if (blend == "multiply")
		return kBlendMode_Multiply;
	else
	{
		logWarning("unknown blend type: %s", blend.c_str());
		return kBlendMode_Add;
	}
}

//

ColorCurve::Key::Key()
	: t(0.f)
{
}

bool ColorCurve::Key::operator<(const Key & other) const
{
	return t < other.t;
}

bool ColorCurve::Key::operator==(const Key & other) const
{
	return memcmp(this, &other, sizeof(Key)) == 0;
}

bool ColorCurve::Key::operator!=(const Key & other) const
{
	return !(*this == other);
}

//

ColorCurve::ColorCurve()
	: numKeys(0)
{
}

bool ColorCurve::allocKey(Key *& key)
{
	if (numKeys == kMaxKeys)
		return false;
	else
	{
		key = &keys[numKeys++];
		return true;
	}
}

void ColorCurve::freeKey(Key *& key)
{
	const int index = key - keys;
	for (int i = index + 1; i < numKeys; ++i)
		keys[i - 1] = keys[i];
	numKeys--;
}

void ColorCurve::clearKeys()
{
	for (int i = 0; i < numKeys; ++i)
		keys[i] = Key();

	numKeys = 0;
}

ColorCurve::Key * ColorCurve::sortKeys(Key * keyToReturn)
{
	Key * result = 0;

	if (keyToReturn)
	{
		Key keyValues[kMaxKeys];
		memcpy(keyValues, keys, sizeof(Key) * numKeys);
		Key * keysForSorting[kMaxKeys];
		for (int i = 0; i < numKeys; ++i)
			keysForSorting[i] = &keys[i];
		std::sort(keysForSorting, keysForSorting + numKeys, [](Key * k1, Key * k2) { return k1->t < k2->t; });
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

void ColorCurve::setLinear(const Color & v1, const Color & v2)
{
	clearKeys();

	Key * k1;
	if (allocKey(k1))
	{
		k1->t = 0.f;
		k1->color = v1;
	}

	Key * k2;
	if (allocKey(k2))
	{
		k2->t = 1.f;
		k2->color = v2;
	}

	sortKeys();
}

void ColorCurve::setLinearAlpha(float v1, float v2)
{
	clearKeys();

	Key * k1;
	if (allocKey(k1))
	{
		k1->t = 0.f;
		k1->color.set(1.f, 1.f, 1.f, v1);
	}

	Key * k2;
	if (allocKey(k2))
	{
		k2->t = 1.f;
		k2->color.set(1.f, 1.f, 1.f, v2);
	}

	sortKeys();
}

void ColorCurve::sample(const float t, Color & result) const
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
			const Color & c1 = keys[startKey].color;
			const Color & c2 = keys[endKey].color;
			const float t2 = (t - keys[startKey].t) / (keys[endKey].t - keys[startKey].t);
			result = c1.interp(c2, t2);
		}
	}
}
