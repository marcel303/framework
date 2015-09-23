#include "particle.h"
#include "tinyxml2.h"
#include <algorithm>
#include <assert.h>

using namespace tinyxml2;

//

static const char * stringAttrib(const XMLElement * elem, const char * name, const char * defaultValue)
{
	if (elem->Attribute(name))
		return elem->Attribute(name);
	else
		return defaultValue;
}

static bool boolAttrib(const XMLElement * elem, const char * name, bool defaultValue)
{
	if (elem->Attribute(name))
		return elem->BoolAttribute(name);
	else
		return defaultValue;
}

static int intAttrib(const XMLElement * elem, const char * name, int defaultValue)
{
	if (elem->Attribute(name))
		return elem->IntAttribute(name);
	else
		return defaultValue;
}

static float floatAttrib(const XMLElement * elem, const char * name, float defaultValue)
{
	if (elem->Attribute(name))
		return elem->FloatAttribute(name);
	else
		return defaultValue;
}

//

ParticleColor::ParticleColor()
{
	memset(this, 0, sizeof(ParticleColor));
}

ParticleColor::ParticleColor(float r, float g, float b, float a)
{
	rgba[0] = r;
	rgba[1] = g;
	rgba[2] = b;
	rgba[3] = a;
}

bool ParticleColor::operator==(const ParticleColor & other) const
{
	return memcmp(this, &other, sizeof(ParticleColor)) == 0;
}

bool ParticleColor::operator!=(const ParticleColor & other) const
{
	return !(*this == other);
}

void ParticleColor::set(float r, float g, float b, float a)
{
	rgba[0] = r;
	rgba[1] = g;
	rgba[2] = b;
	rgba[3] = a;
}

void ParticleColor::modulateWith(const ParticleColor & other)
{
	for (int i = 0; i < 4; ++i)
		rgba[i] *= other.rgba[i];
}

void ParticleColor::interpolateBetween(const ParticleColor & v1, const ParticleColor & v2, const float t)
{
	for (int i = 0; i < 4; ++i)
		rgba[i] = v1.rgba[i] * (1.f - t) + v2.rgba[i] * t;
}

void ParticleColor::save(XMLPrinter * printer)
{
	printer->PushAttribute("r", rgba[0]);
	printer->PushAttribute("g", rgba[1]);
	printer->PushAttribute("b", rgba[2]);
	printer->PushAttribute("a", rgba[3]);
}

void ParticleColor::load(XMLElement * elem)
{
	*this = ParticleColor();

	rgba[0] = floatAttrib(elem, "r", rgba[0]);
	rgba[1] = floatAttrib(elem, "g", rgba[1]);
	rgba[2] = floatAttrib(elem, "b", rgba[2]);
	rgba[3] = floatAttrib(elem, "a", rgba[3]);
}

//

ParticleCurve::ParticleCurve()
	: valueMin(0.f)
	, valueMax(1.f)
{
}

void ParticleCurve::setLinear(float v1, float v2)
{
	// todo

	valueMin = v1;
	valueMax = v2;
}

float ParticleCurve::sample(const float t) const
{
	const float t2 = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
	return valueMin * (1.f - t2) + valueMax * t2;
}

void ParticleCurve::save(tinyxml2::XMLPrinter * printer)
{
	printer->PushAttribute("min", valueMin);
	printer->PushAttribute("max", valueMax);
}

void ParticleCurve::load(tinyxml2::XMLElement * elem)
{
	*this = ParticleCurve();

	valueMin = floatAttrib(elem, "min", valueMin);
	valueMax = floatAttrib(elem, "max", valueMax);
}

//

ParticleColorCurve::Key::Key()
	: t(0.f)
{
}

bool ParticleColorCurve::Key::operator<(const Key & other) const
{
	return t < other.t;
}

bool ParticleColorCurve::Key::operator==(const Key & other) const
{
	return memcmp(this, &other, sizeof(Key)) == 0;
}

bool ParticleColorCurve::Key::operator!=(const Key & other) const
{
	return !(*this == other);
}

//

ParticleColorCurve::ParticleColorCurve()
	: numKeys(0)
{
}

bool ParticleColorCurve::operator==(const ParticleColorCurve & other) const
{
	return memcmp(this, &other, sizeof(ParticleColorCurve)) == 0;
}

bool ParticleColorCurve::operator!=(const ParticleColorCurve & other) const
{
	return !(*this == other);
}

bool ParticleColorCurve::allocKey(Key *& key)
{
	if (numKeys == kMaxKeys)
		return false;
	else
	{
		key = &keys[numKeys++];
		return true;
	}
}

void ParticleColorCurve::freeKey(Key *& key)
{
	const int index = key - keys;
	for (int i = index + 1; i < numKeys; ++i)
		keys[i - 1] = keys[i];
	numKeys--;
}

void ParticleColorCurve::clearKeys()
{
	for (int i = 0; i < numKeys; ++i)
		keys[i] = Key();

	numKeys = 0;
}

ParticleColorCurve::Key * ParticleColorCurve::sortKeys(Key * keyToReturn)
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

void ParticleColorCurve::setLinear(const ParticleColor & v1, const ParticleColor & v2)
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

void ParticleColorCurve::setLinearAlpha(float v1, float v2)
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

void ParticleColorCurve::sample(const float t, ParticleColor & result) const
{
	if (numKeys == 0)
		result.set(1.f, 1.f, 1.f, 1.f);
//	else if (numKeys == 1)
//		result = keys[0].color;
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
			result.interpolateBetween(c1, c2, t2);
		}
	}
}

void ParticleColorCurve::save(XMLPrinter * printer)
{
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

void ParticleColorCurve::load(XMLElement * elem)
{
	clearKeys();

	for (auto keyElem = elem->FirstChildElement("key"); keyElem; keyElem = keyElem->NextSiblingElement())
	{
		Key * key;

		if (allocKey(key))
		{
			key->t = floatAttrib(keyElem, "t", 0.f);
			key->color.load(keyElem);
		}
		else
		{
			// todo : emit warning?
		}
	}

	sortKeys();
}

//

ParticleEmitterInfo::ParticleEmitterInfo()
	: duration(1.f)
	, loop(true)
	, prewarm(false)
	, startDelay(0.f)
	, startLifetime(1.f)
	, startSpeed(100.f)
	, startSize(100.f)
	, startRotation(0.f)
	, startColor(1.f, 1.f, 1.f, 1.f)
	, gravityMultiplier(1.f)
	, inheritVelocity(false)
	, worldSpace(false)
	, maxParticles(100)
{
	memset(name, 0, sizeof(name));
	memset(materialName, 0, sizeof(materialName));
}

bool ParticleEmitterInfo::operator==(const ParticleEmitterInfo & other) const
{
	return memcmp(this, &other, sizeof(ParticleEmitterInfo)) == 0;
}

bool ParticleEmitterInfo::operator!=(const ParticleEmitterInfo & other) const
{
	return !(*this == other);
}

void ParticleEmitterInfo::save(XMLPrinter * printer)
{
	printer->PushAttribute("name", name);
	printer->PushAttribute("duration", duration);
	printer->PushAttribute("loop", loop);
	printer->PushAttribute("prewarm", prewarm);
	printer->PushAttribute("startDelay", startDelay);
	printer->PushAttribute("startLifetime", startLifetime);
	printer->PushAttribute("startSpeed", startSpeed);
	printer->PushAttribute("startSize", startSize);
	printer->PushAttribute("startRotation", startRotation);
	printer->PushAttribute("gravityMultiplier", gravityMultiplier);
	printer->PushAttribute("inheritVelocity", inheritVelocity);
	printer->PushAttribute("worldSpace", worldSpace);
	printer->PushAttribute("maxParticles", maxParticles);
	printer->PushAttribute("materialName", materialName);

	printer->OpenElement("startColor");
	{
		startColor.save(printer);
	}
	printer->CloseElement();
}

void ParticleEmitterInfo::load(XMLElement * elem)
{
	*this = ParticleEmitterInfo();

	strcpy_s(name, sizeof(name), stringAttrib(elem, "name", ""));
	duration = floatAttrib(elem, "duration", duration);
	loop = boolAttrib(elem, "loop", loop);
	prewarm = boolAttrib(elem, "prewarm", prewarm);
	startDelay = floatAttrib(elem, "startDelay", startDelay);
	startLifetime = floatAttrib(elem, "startLifetime", startLifetime);
	startSpeed = floatAttrib(elem, "startSpeed", startSpeed);
	startSize = floatAttrib(elem, "startSize", startSize);
	startRotation = floatAttrib(elem, "startRotation", startRotation);
	auto startColorElem = elem->FirstChildElement("startColor");
	if (startColorElem)
		startColor.load(startColorElem);
	gravityMultiplier = floatAttrib(elem, "gravityMultiplier", gravityMultiplier);
	inheritVelocity = boolAttrib(elem, "inheritVelocity", inheritVelocity);
	worldSpace = boolAttrib(elem, "worldSpace", worldSpace);
	maxParticles = intAttrib(elem, "maxParticles", maxParticles);
	strcpy_s(materialName, sizeof(materialName), stringAttrib(elem, "materialName", ""));
}

//

ParticleInfo::ParticleInfo()
	// emission
	: rate(0.f)
	, numBursts(0)
	// shape
	, shape(kShapeCircle)
	, randomDirection(false)
	, circleRadius(100.f)
	, boxSizeX(100.f)
	, boxSizeY(100.f)
	, emitFromShell(false)
	// velocity over lifetime
	, velocityOverLifetime(false)
	, velocityOverLifetimeValueX(0.f)
	, velocityOverLifetimeValueY(0.f)
	// limit velocity over lifetime
	, velocityOverLifetimeLimit(false)
	, velocityOverLifetimeLimitCurve()
	, velocityOverLifetimeLimitDampen(.5f)
	// force over lifetime
	, forceOverLifetime(false)
	, forceOverLifetimeValueX(0.f)
	, forceOverLifetimeValueY(0.f)
	// color over lifetime
	, colorOverLifetime(false)
	, colorOverLifetimeCurve()
	// color by speed
	, colorBySpeed(false)
	, colorBySpeedCurve()
	, colorBySpeedRangeMin(0.f)
	, colorBySpeedRangeMax(100.f)
	// size over lifetime
	, sizeOverLifetime(false)
	, sizeOverLifetimeCurve()
	// size by speed
	, sizeBySpeed(false)
	, sizeBySpeedCurve()
	, sizeBySpeedRangeMin(0.f)
	, sizeBySpeedRangeMax(100.f)
	// rotation over lifetime
	, rotationOverLifetime(false)
	, rotationOverLifetimeValue(0.f)
	// rotation by speed
	, rotationBySpeed(false)
	, rotationBySpeedCurve()
	, rotationBySpeedRangeMin(0.f)
	, rotationBySpeedRangeMax(100.f)
	// external forces
	//
	// collision
	, collision(false)
	, bounciness(1.f)
	, lifetimeLoss(0.f)
	, minKillSpeed(0.f)
	, collisionRadius(1.f)
	// sub emitters
	, enableSubEmitters(false)
	// texture sheet animation
	//
	// renderer
	, sortMode(kSortMode_YoungestFirst)
	, blendMode(kBlendMode_AlphaBlended)
{
}

bool ParticleInfo::operator==(const ParticleInfo & other) const
{
	return memcmp(this, &other, sizeof(ParticleInfo)) == 0;
}

bool ParticleInfo::operator!=(const ParticleInfo & other) const
{
	return !(*this == other);
}

bool ParticleInfo::allocBurst(Burst *& burst)
{
	if (numBursts == kMaxBursts)
		return false;
	else
	{
		burst = &bursts[numBursts++];
		return true;
	}
}

void ParticleInfo::clearBursts()
{
	for (int i = 0; i < numBursts; ++i)
		bursts[i] = Burst();

	numBursts = 0;
}

void ParticleInfo::save(tinyxml2::XMLPrinter * printer)
{
	// emission
	printer->PushAttribute("rate", rate);
	// shape
	printer->PushAttribute("shape", shape);
	printer->PushAttribute("randomDirection", randomDirection);
	printer->PushAttribute("circleRadius", circleRadius);
	printer->PushAttribute("boxSizeX", boxSizeX);
	printer->PushAttribute("boxSizeY", boxSizeY);
	printer->PushAttribute("emitFromShell", emitFromShell);
	// velocity over lifetime
	printer->PushAttribute("velocityOverLifetime", velocityOverLifetime);
	printer->PushAttribute("velocityOverLifetimeValueX", velocityOverLifetimeValueX);
	printer->PushAttribute("velocityOverLifetimeValueY", velocityOverLifetimeValueY);
	// limit velocity over lifetime
	printer->PushAttribute("velocityOverLifetimeLimit", velocityOverLifetimeLimit);
	printer->PushAttribute("velocityOverLifetimeLimitDampen", velocityOverLifetimeLimitDampen);
	// force over lifetime
	printer->PushAttribute("forceOverLifetime", forceOverLifetime);
	printer->PushAttribute("forceOverLifetimeValueX", forceOverLifetimeValueX);
	printer->PushAttribute("forceOverLifetimeValueY", forceOverLifetimeValueY);
	// color over lifetime
	printer->PushAttribute("colorOverLifetime", colorOverLifetime);
	// color by speed
	printer->PushAttribute("colorBySpeed", colorBySpeed);
	printer->PushAttribute("colorBySpeedRangeMin", colorBySpeedRangeMin);
	printer->PushAttribute("colorBySpeedRangeMax", colorBySpeedRangeMax);
	// size over lifetime
	printer->PushAttribute("sizeOverLifetime", sizeOverLifetime);
	// size by speed
	printer->PushAttribute("sizeBySpeed", sizeBySpeed);
	printer->PushAttribute("sizeBySpeedRangeMin", sizeBySpeedRangeMin);
	printer->PushAttribute("sizeBySpeedRangeMax", sizeBySpeedRangeMax);
	// rotation over lifetime
	printer->PushAttribute("rotationOverLifetime", rotationOverLifetime);
	printer->PushAttribute("rotationOverLifetimeValue", rotationOverLifetimeValue);
	// rotation by speed
	printer->PushAttribute("rotationBySpeed", rotationBySpeed);
	printer->PushAttribute("rotationBySpeedRangeMin", rotationBySpeedRangeMin);
	printer->PushAttribute("rotationBySpeedRangeMax", rotationBySpeedRangeMax);
	// external forces
	//
	// collision
	printer->PushAttribute("collision", collision);
	printer->PushAttribute("bounciness", bounciness);
	printer->PushAttribute("lifetimeLoss", lifetimeLoss);
	printer->PushAttribute("minKillSpeed", minKillSpeed);
	printer->PushAttribute("collisionRadius", collisionRadius);
	// sub emitters
	printer->PushAttribute("subEmitters", enableSubEmitters);
	// texture sheet animation
	// todo
	// renderer
	printer->PushAttribute("sortMode", sortMode);
	printer->PushAttribute("blendMode", blendMode);

	// emission
	for (int i = 0; i < numBursts; ++i)
	{
		printer->OpenElement("burst");
		{
			printer->PushAttribute("time", bursts[i].time);
			printer->PushAttribute("numParticles", bursts[i].numParticles);
		}
		printer->CloseElement();
	}

	// limit velocity over lifetime
	printer->OpenElement("velocityOverLifetimeLimitCurve");
	{
		velocityOverLifetimeLimitCurve.save(printer);
	}
	printer->CloseElement();

	// color over lifetime
	printer->OpenElement("colorOverLifetimeCurve");
	{
		colorOverLifetimeCurve.save(printer);
	}
	printer->CloseElement();

	// color by speed
	printer->OpenElement("colorBySpeedCurve");
	{
		colorBySpeedCurve.save(printer);
	}
	printer->CloseElement();

	// size over lifetime
	printer->OpenElement("sizeOverLifetimeCurve");
	{
		sizeOverLifetimeCurve.save(printer);
	}
	printer->CloseElement();

	// size by speed
	printer->OpenElement("sizeBySpeedCurve");
	{
		sizeBySpeedCurve.save(printer);
	}
	printer->CloseElement();

	// rotation by speed
	printer->OpenElement("rotationBySpeedCurve");
	{
		rotationBySpeedCurve.save(printer);
	}
	printer->CloseElement();

	// sub emitters
	for (int i = 0; i < kSubEmitterEvent_COUNT; ++i)
	{
		printer->OpenElement("subEmitter");
		{
			printer->PushAttribute("event", i);
			subEmitters[i].save(printer);
		}
		printer->CloseElement();
	}
}

void ParticleInfo::load(tinyxml2::XMLElement * elem)
{
	*this = ParticleInfo();

	// emission
	rate = floatAttrib(elem, "rate", rate);
	for (auto burstElem = elem->FirstChildElement("burst"); burstElem; burstElem = burstElem->NextSiblingElement())
	{
		Burst * burst;
		if (allocBurst(burst))
		{
			burst->time = floatAttrib(burstElem, "time", burst->time);
			burst->numParticles = intAttrib(burstElem, "time", burst->numParticles);
		}
	}
	// shape
	shape = (Shape)intAttrib(elem, "shape", shape);
	randomDirection = boolAttrib(elem, "randomDirection", randomDirection);
	circleRadius = floatAttrib(elem, "circleRadius", circleRadius);
	boxSizeX = floatAttrib(elem, "boxSizeX", boxSizeX);
	boxSizeY = floatAttrib(elem, "boxSizeY", boxSizeY);
	emitFromShell = boolAttrib(elem, "emitFromShell", emitFromShell);
	// velocity over lifetime
	velocityOverLifetime = boolAttrib(elem, "velocityOverLifetime", velocityOverLifetime);
	velocityOverLifetimeValueX = floatAttrib(elem, "velocityOverLifetimeValueX", velocityOverLifetimeValueX);
	velocityOverLifetimeValueY = floatAttrib(elem, "velocityOverLifetimeValueY", velocityOverLifetimeValueY);
	// limit velocity over lifetime
	velocityOverLifetimeLimit = boolAttrib(elem, "velocityOverLifetimeLimit", velocityOverLifetimeLimit);
	auto velocityOverLifetimeLimitCurveElem = elem->FirstChildElement("velocityOverLifetimeLimitCurve");
	if (velocityOverLifetimeLimitCurveElem)
		velocityOverLifetimeLimitCurve.load(velocityOverLifetimeLimitCurveElem);
	velocityOverLifetimeLimitDampen = floatAttrib(elem, "velocityOverLifetimeLimitDampen", velocityOverLifetimeLimitDampen);
	// force over lifetime
	forceOverLifetime = boolAttrib(elem, "forceOverLifetime", forceOverLifetime);
	forceOverLifetimeValueX = floatAttrib(elem, "forceOverLifetimeValueX", forceOverLifetimeValueX);
	forceOverLifetimeValueY = floatAttrib(elem, "forceOverLifetimeValueY", forceOverLifetimeValueY);
	// color over lifetime
	colorOverLifetime = boolAttrib(elem, "colorOverLifetime", colorOverLifetime);
	auto colorOverLifetimeCurveElem = elem->FirstChildElement("colorOverLifetimeCurve");
	if (colorOverLifetimeCurveElem)
		colorOverLifetimeCurve.load(colorOverLifetimeCurveElem);
	// color by speed
	colorBySpeed = boolAttrib(elem, "colorBySpeed", colorBySpeed);
	auto colorBySpeedCurveElem = elem->FirstChildElement("colorBySpeedCurve");
	if (colorBySpeedCurveElem)
		colorBySpeedCurve.load(colorBySpeedCurveElem);
	colorBySpeedRangeMin = floatAttrib(elem, "colorBySpeedRangeMin", colorBySpeedRangeMin);
	colorBySpeedRangeMax = floatAttrib(elem, "colorBySpeedRangeMax", colorBySpeedRangeMax);
	// size over lifetime
	sizeOverLifetime = boolAttrib(elem, "sizeOverLifetime", sizeOverLifetime);
	auto sizeOverLifetimeCurveElem = elem->FirstChildElement("sizeOverLifetimeCurve");
	if (sizeOverLifetimeCurveElem)
		sizeOverLifetimeCurve.load(sizeOverLifetimeCurveElem);
	// size by speed
	sizeBySpeed = boolAttrib(elem, "sizeBySpeed", sizeBySpeed);
	auto sizeBySpeedCurveElem = elem->FirstChildElement("sizeBySpeedCurve");
	if (sizeBySpeedCurveElem)
		sizeBySpeedCurve.load(sizeBySpeedCurveElem);
	sizeBySpeedRangeMin = floatAttrib(elem, "sizeBySpeedRangeMin", sizeBySpeedRangeMin);
	sizeBySpeedRangeMax = floatAttrib(elem, "sizeBySpeedRangeMax", sizeBySpeedRangeMax);
	// rotation over lifetime
	rotationOverLifetime = boolAttrib(elem, "rotationOverLifetime", rotationOverLifetime);
	rotationOverLifetimeValue = floatAttrib(elem, "rotationOverLifetimeValue", rotationOverLifetimeValue);
	// rotation by speed
	rotationBySpeed = boolAttrib(elem, "rotationBySpeed", rotationBySpeed);
	auto rotationBySpeedCurveElem = elem->FirstChildElement("rotationBySpeedCurve");
	if (rotationBySpeedCurveElem)
		rotationBySpeedCurve.load(rotationBySpeedCurveElem);
	rotationBySpeedRangeMin = floatAttrib(elem, "rotationBySpeedRangeMin", rotationBySpeedRangeMin);
	rotationBySpeedRangeMax = floatAttrib(elem, "rotationBySpeedRangeMax", rotationBySpeedRangeMax);
	// external forces
	//
	// collision
	collision = boolAttrib(elem, "collision", collision);
	bounciness = floatAttrib(elem, "bounciness", bounciness);
	lifetimeLoss = floatAttrib(elem, "lifetimeLoss", lifetimeLoss);
	minKillSpeed = floatAttrib(elem, "minKillSpeed", minKillSpeed);
	collisionRadius = floatAttrib(elem, "collisionRadius", collisionRadius);
	// sub emitters
	enableSubEmitters = boolAttrib(elem, "subEmitters", enableSubEmitters);
	for (auto subEmitterElem = elem->FirstChildElement("subEmitter"); subEmitterElem; subEmitterElem = subEmitterElem->NextSiblingElement("subEmitter"))
	{
		const int index = intAttrib(subEmitterElem, "event", -1);
		if (index >= 0 && index < kSubEmitterEvent_COUNT)
			subEmitters[index].load(subEmitterElem);
	}
	// texture sheet animation
	// todo
	// renderer
	sortMode = (SortMode)intAttrib(elem, "sortMode", sortMode);
	blendMode = (BlendMode)intAttrib(elem, "blendMode", blendMode);
}

ParticleInfo::SubEmitter::SubEmitter()
	: enabled(false)
	, count(1)
	, chance(1.f)
{
	memset(emitterName, 0, sizeof(emitterName));
}

void ParticleInfo::SubEmitter::save(tinyxml2::XMLPrinter * printer)
{
	printer->PushAttribute("enabled", enabled);
	printer->PushAttribute("count", count);
	printer->PushAttribute("chance", chance);
	printer->PushAttribute("emitterName", emitterName);
}

void ParticleInfo::SubEmitter::load(tinyxml2::XMLElement * elem)
{
	enabled = boolAttrib(elem, "enabled", enabled);
	count = intAttrib(elem, "count", count);
	chance = floatAttrib(elem, "chance", chance);
	strcpy_s(emitterName, sizeof(emitterName), stringAttrib(elem, "emitterName", emitterName));
}

//

ParticleEmitter::ParticleEmitter()
	: active(true)
	, delaying(true)
	, time(0.f)
	, totalTime(0.f)
{
}

bool ParticleEmitter::allocParticle(ParticlePool & pool, Particle *& p)
{
	p = pool.allocParticle();

	return p != 0;
}

void ParticleEmitter::clearParticles(ParticlePool & pool)
{
	while (pool.head)
	{
		pool.freeParticle(pool.head);
	}
}

bool ParticleEmitter::emitParticle(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, ParticlePool & pool, const float timeOffset, const float gravityX, const float gravityY, Particle *& p)
{
	if (allocParticle(pool, p))
	{
		p->life = 1.f;
		p->lifeRcp = 1.f / pei.startLifetime;

		getParticleSpawnLocation(pi, p->position[0], p->position[1]);
		float speedAngle;
		if (pi.randomDirection)
			speedAngle = (rand() % 256) / 255.f * 2.f * M_PI; // fixme : remove all libc random calls
		else
			speedAngle = float(M_PI) / 2.f; // todo : add pei.startAngle;
		p->speed[0] = cosf(speedAngle) * pei.startSpeed;
		p->speed[1] = sinf(speedAngle) * pei.startSpeed;
		p->rotation = pei.startRotation;

		/*
		todo
			bool inheritVelocity; // Do the particles start with the same velocity as the particle system object?
			bool worldSpace; // Should particles be animated in the parent object’s local space (and therefore move with the object) or in world space?
		*/

		tickParticle(cbs, pei, pi, timeOffset, gravityX, gravityY, *p);

		return true;
	}
	else
	{
		return false;
	}
}

void ParticleEmitter::restart(ParticlePool & pool)
{
	clearParticles(pool);

	delaying = true;
	time = 0.f;
	totalTime = 0.f;
}

//

Particle::Particle()
{
	memset(this, 0, sizeof(Particle));
}

//

ParticlePool::ParticlePool()
	: head(0)
	, tail(0)
{
}

Particle * ParticlePool::allocParticle()
{
	Particle * result = new Particle();

	if (!head)
		head = result;
	else
	{
		result->prev = tail;
		tail->next = result;
	}

	tail = result;

	return result;
}

Particle * ParticlePool::freeParticle(Particle * p)
{
	Particle * result = p->next;

	if (p->next)
		p->next->prev = p->prev;
	if (p->prev)
		p->prev->next = p->next;

	if (p == head)
		head = p->next;
	if (p == tail)
		tail = p->prev;

	delete p;

	return result;
}

//

bool tickParticle(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float timeStep, const float gravityX, const float gravityY, Particle & p)
{
	assert(p.life > 0.f);

	// todo : clamp timeStep to available life

	p.life -= p.lifeRcp * timeStep;
	if (p.life < 0.f)
		p.life = 0.f;

	p.speed[0] += gravityX * pei.gravityMultiplier * timeStep;
	p.speed[1] += gravityY * pei.gravityMultiplier * timeStep;

	if (pi.forceOverLifetime)
	{
		p.speed[0] += pi.forceOverLifetimeValueX * timeStep;
		p.speed[1] += pi.forceOverLifetimeValueY * timeStep;
	}

	p.position[0] += p.speed[0] * timeStep;
	p.position[1] += p.speed[1] * timeStep;

	const float particleLife = 1.f - p.life;
	const float particleSpeed = sqrtf(p.speed[0] * p.speed[0] + p.speed[1] * p.speed[1]);

	ParticleColor color;
	computeParticleColor(pei, pi, particleLife, particleSpeed, color);

	const float size = computeParticleSize(pei, pi, particleLife, particleSpeed);

	p.rotation = computeParticleRotation(pei, pi, timeStep, particleLife, particleSpeed, p.rotation);

#if 0
	printf("tickParticle: color=%1.2f, %1.2f, %1.2f, %1.2f size=%03.2f, rotation=%+03.2f\n",
		color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3], size, p.rotation);
#endif

	if (p.life <= 0.f)
	{
		if (pi.enableSubEmitters && pi.subEmitters[ParticleInfo::kSubEmitterEvent_Death].enabled)
		{
			handleSubEmitter(cbs, pi, gravityX, gravityY, p, ParticleInfo::kSubEmitterEvent_Death);
		}
	}

	return p.life > 0.f;
}

void handleSubEmitter(const ParticleCallbacks & cbs, const ParticleInfo & pi, const float gravityX, const float gravityY, const Particle & p, const ParticleInfo::SubEmitterEvent e)
{
	if (!pi.subEmitters[e].emitterName[0])
		return;

	for (int i = 0; i < pi.subEmitters[e].count; ++i)
	{
		const float t = cbs.randomFloat(0.f, 1.f);
		if (t > pi.subEmitters[e].chance)
			continue;

		const ParticleEmitterInfo * subPei;
		const ParticleInfo * subPi;
		ParticlePool * subPool;
		ParticleEmitter * subPe;
		if (cbs.getEmitterByName(
			pi.subEmitters[e].emitterName,
			subPei,
			subPi,
			subPool,
			subPe))
		{
			assert(subPi != &pi); // todo : warn about this?

			if (subPi != &pi)
			{
				Particle * subP;
				if (subPe->emitParticle(
					cbs,
					*subPei,
					*subPi,
					*subPool,
					0.f,
					gravityX,
					gravityY,
					subP))
				{
					subP->position[0] += p.position[0];
					subP->position[1] += p.position[1];

					if (subPei->inheritVelocity)
					{
						subP->speed[0] += p.speed[0];
						subP->speed[1] += p.speed[1];
					}
				}
			}
		}
	}
}

void getParticleSpawnLocation(const ParticleInfo & pi, float & x, float & y)
{
	switch (pi.shape)
	{
	case ParticleInfo::kShapeEdge:
		{
			const float t = (rand() % 256) / 255.f; // fixme : random
			x = (t - .5f) * 2.f * pi.boxSizeX;
			y = 0.f;
		}
		break;
	case ParticleInfo::kShapeBox:
		{
			if (pi.emitFromShell)
			{
				const float s = pi.boxSizeX * 2.f + pi.boxSizeY * 2.f;
				const float t = (rand() % 1024) / 1023.f * s; // fixme : random

				const float s1 = pi.boxSizeX;
				const float s2 = s1 + pi.boxSizeX;
				const float s3 = s2 + pi.boxSizeY;

				if (t < s1)
				{
					x = t;
					y = 0.f;
				}
				else if (t < s2)
				{
					x = t - s1;
					y = pi.boxSizeY;
				}
				else if (t < s3)
				{
					x = 0.f;
					y = t - s2;
				}
				else
				{
					x = pi.boxSizeX;
					y = t - s3;
				}

				x = x * 2.f - pi.boxSizeX;
				y = y * 2.f - pi.boxSizeY;
			}
			else
			{
				const float tx = (rand() % 256) / 255.f; // fixme : random
				const float ty = (rand() % 256) / 255.f; // fixme : random
				x = (tx - .5f) * 2.f * pi.boxSizeX;
				y = (ty - .5f) * 2.f * pi.boxSizeY;
			}
		}
		break;
	case ParticleInfo::kShapeCircle:
		{
			if (pi.emitFromShell)
			{
				const float a = (rand() % 1024) / 1023.f * 2.f * float(M_PI); // fixme : random
				x = std::cosf(a) * pi.circleRadius;
				y = std::sinf(a) * pi.circleRadius;
			}
			else
			{
				for (;;)
				{
					const float tx = (rand() % 256) / 255.f; // fixme : random
					const float ty = (rand() % 256) / 255.f; // fixme : random
					x = (tx - .5f) * 2.f * pi.circleRadius;
					y = (ty - .5f) * 2.f * pi.circleRadius;
					const float dSquared = x * x + y * y;
					if (dSquared <= pi.circleRadius * pi.circleRadius)
						break;
				}
			}
		}
		break;
	default:
		x = 0.f;
		y = 0.f;
		break;
	}
}

void computeParticleColor(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float particleLife, const float particleSpeed, ParticleColor & result)
{
	result = pei.startColor;

	if (pi.colorOverLifetime)
	{
		ParticleColor temp;
		pi.colorOverLifetimeCurve.sample(particleLife, temp);
		result.modulateWith(temp);
	}

	if (pi.colorBySpeed)
	{
		const float t = (particleSpeed - pi.colorBySpeedRangeMin) / (pi.colorBySpeedRangeMax - pi.colorBySpeedRangeMin);
		ParticleColor temp;
		pi.colorBySpeedCurve.sample(t, temp);
		result.modulateWith(temp);
	}
}

float computeParticleSize(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float particleLife, const float particleSpeed)
{
	float result = pei.startSize;

	if (pi.sizeOverLifetime)
	{
		const float t = particleLife;
		result *= pi.sizeOverLifetimeCurve.sample(t);
	}

	if (pi.sizeBySpeed)
	{
		const float t = (particleSpeed - pi.sizeBySpeedRangeMin) / (pi.sizeBySpeedRangeMax - pi.sizeBySpeedRangeMin);
		result *= pi.sizeBySpeedCurve.sample(t);
	}

	return result;
}

float computeParticleRotation(const ParticleEmitterInfo & pei, const ParticleInfo & pi, const float timeStep, const float particleLife, const float particleSpeed, float particleRotation)
{
	float result = particleRotation;

	if (pi.rotationOverLifetime)
	{
		result += pi.rotationOverLifetimeValue * timeStep;
	}

	if (pi.rotationBySpeed)
	{
		const float t = (particleSpeed - pi.rotationBySpeedRangeMin) / (pi.rotationBySpeedRangeMax - pi.rotationBySpeedRangeMin);
		result += pi.rotationBySpeedCurve.sample(t) * timeStep;
	}

	return result;
}

void tickParticleEmitter(const ParticleCallbacks & cbs, const ParticleEmitterInfo & pei, const ParticleInfo & pi, ParticlePool & pool, const float timeStep, const float gravityX, const float gravityY, ParticleEmitter & pe)
{
	if (pi.rate <= 0.f)
		return;

	pe.time += timeStep;
	pe.totalTime += timeStep;

	if (pe.delaying)
	{
		if (pe.time < pei.startDelay)
			return;
		else
		{
			pe.time -= pei.startDelay;
			pe.delaying = false;
		}
	}

	const float rateTime = 1.f / pi.rate;

	while (pe.time >= rateTime && (pe.totalTime < pei.duration || pei.loop))
	{
		pe.time -= rateTime;

		const float timeOffset = fmodf(pe.time, rateTime);

		Particle * p;
		pe.emitParticle(cbs, pei, pi, pool, timeOffset, gravityX, gravityY, p);
	}

	// todo : int maxParticles; // The maximum number of particles in the system at once. Older particles will be removed when the limit is reached.
}
