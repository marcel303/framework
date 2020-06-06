#include "particle.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include <algorithm>
#include <cmath>

#include "Debugging.h" // Assert
#include "Log.h"       // LOG_ functions
#include "Path.h"      // GetDirectory
#include "StringEx.h"  // _s functions
#include "ui.h"        // srgb <-> linear

//

#ifdef __SSE2__
	#include <xmmintrin.h>

	#if defined(__GNUC__)
		#define _MM_ACCESS(r, i) r[i]
	#else
		#define _MM_ACCESS(r, i) r.m128_f32[i]
	#endif
#endif

//

ParticleEmitterInfo::ParticleEmitterInfo()
	: duration(1.f)
	, loop(true)
	, prewarm(false)
	, startDelay(0.f)
	, startLifetime(1.f)
	, startSpeed(100.f)
	, startSpeedAngle(90.f)
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

void ParticleEmitterInfo::save(tinyxml2::XMLPrinter * printer) const
{
	printer->PushAttribute("name", name);
	printer->PushAttribute("duration", duration);
	printer->PushAttribute("loop", loop);
	printer->PushAttribute("prewarm", prewarm);
	printer->PushAttribute("startDelay", startDelay);
	printer->PushAttribute("startLifetime", startLifetime);
	printer->PushAttribute("startSpeed", startSpeed);
	printer->PushAttribute("startSpeedAngle", startSpeedAngle);
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

void ParticleEmitterInfo::load(const tinyxml2::XMLElement * elem)
{
	*this = ParticleEmitterInfo();

	strcpy_s(name, sizeof(name), stringAttrib(elem, "name", ""));
	duration = floatAttrib(elem, "duration", duration);
	loop = boolAttrib(elem, "loop", loop);
	prewarm = boolAttrib(elem, "prewarm", prewarm);
	startDelay = floatAttrib(elem, "startDelay", startDelay);
	startLifetime = floatAttrib(elem, "startLifetime", startLifetime);
	startSpeed = floatAttrib(elem, "startSpeed", startSpeed);
	startSpeedAngle = floatAttrib(elem, "startSpeedAngle", startSpeedAngle);
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
	, shape(kShapeType_Circle)
	, randomDirection(false)
	, circleRadius(100.f)
	, boxSizeX(100.f)
	, boxSizeY(100.f)
	, boxSizeZ(100.f)
	, emitFromShell(false)
	// velocity over lifetime
	, velocityOverLifetime(false)
	, velocityOverLifetimeValueX(0.f)
	, velocityOverLifetimeValueY(0.f)
	, velocityOverLifetimeValueZ(0.f)
	// limit velocity over lifetime
	, velocityOverLifetimeLimit(false)
	, velocityOverLifetimeLimitCurve()
	, velocityOverLifetimeLimitDampen(.5f)
	// force over lifetime
	, forceOverLifetime(false)
	, forceOverLifetimeValueX(0.f)
	, forceOverLifetimeValueY(0.f)
	, forceOverLifetimeValueZ(0.f)
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
	// collision
	, collision(false)
	, bounciness(1.f)
	, lifetimeLoss(0.f)
	, minKillSpeed(0.f)
	, collisionRadius(1.f)
	// sub emitters
	, enableSubEmitters(false)
	// drawing
	, sortMode(kSortMode_YoungestFirst)
	, blendMode(kBlendMode_AlphaBlended)
{
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

void ParticleInfo::save(tinyxml2::XMLPrinter * printer) const
{
	// emission
	printer->PushAttribute("rate", rate);
	// shape
	printer->PushAttribute("shape", shape);
	printer->PushAttribute("randomDirection", randomDirection);
	printer->PushAttribute("circleRadius", circleRadius);
	printer->PushAttribute("boxSizeX", boxSizeX);
	printer->PushAttribute("boxSizeY", boxSizeY);
	printer->PushAttribute("boxSizeZ", boxSizeZ);
	printer->PushAttribute("emitFromShell", emitFromShell);
	// velocity over lifetime
	printer->PushAttribute("velocityOverLifetime", velocityOverLifetime);
	printer->PushAttribute("velocityOverLifetimeValueX", velocityOverLifetimeValueX);
	printer->PushAttribute("velocityOverLifetimeValueY", velocityOverLifetimeValueY);
	printer->PushAttribute("velocityOverLifetimeValueZ", velocityOverLifetimeValueZ);
	// limit velocity over lifetime
	printer->PushAttribute("velocityOverLifetimeLimit", velocityOverLifetimeLimit);
	printer->PushAttribute("velocityOverLifetimeLimitDampen", velocityOverLifetimeLimitDampen);
	// force over lifetime
	printer->PushAttribute("forceOverLifetime", forceOverLifetime);
	printer->PushAttribute("forceOverLifetimeValueX", forceOverLifetimeValueX);
	printer->PushAttribute("forceOverLifetimeValueY", forceOverLifetimeValueY);
	printer->PushAttribute("forceOverLifetimeValueZ", forceOverLifetimeValueZ);
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
	// collision
	printer->PushAttribute("collision", collision);
	printer->PushAttribute("bounciness", bounciness);
	printer->PushAttribute("lifetimeLoss", lifetimeLoss);
	printer->PushAttribute("minKillSpeed", minKillSpeed);
	printer->PushAttribute("collisionRadius", collisionRadius);
	// sub emitters
	printer->PushAttribute("subEmitters", enableSubEmitters);
	// drawing
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

void ParticleInfo::load(const tinyxml2::XMLElement * elem)
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
	shape = (ShapeType)intAttrib(elem, "shape", shape);
	randomDirection = boolAttrib(elem, "randomDirection", randomDirection);
	circleRadius = floatAttrib(elem, "circleRadius", circleRadius);
	boxSizeX = floatAttrib(elem, "boxSizeX", boxSizeX);
	boxSizeY = floatAttrib(elem, "boxSizeY", boxSizeY);
	boxSizeZ = floatAttrib(elem, "boxSizeZ", boxSizeZ);
	emitFromShell = boolAttrib(elem, "emitFromShell", emitFromShell);
	// velocity over lifetime
	velocityOverLifetime = boolAttrib(elem, "velocityOverLifetime", velocityOverLifetime);
	velocityOverLifetimeValueX = floatAttrib(elem, "velocityOverLifetimeValueX", velocityOverLifetimeValueX);
	velocityOverLifetimeValueY = floatAttrib(elem, "velocityOverLifetimeValueY", velocityOverLifetimeValueY);
	velocityOverLifetimeValueZ = floatAttrib(elem, "velocityOverLifetimeValueZ", velocityOverLifetimeValueZ);
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
	forceOverLifetimeValueZ = floatAttrib(elem, "forceOverLifetimeValueZ", forceOverLifetimeValueZ);
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
	// drawing
	sortMode = (SortMode)intAttrib(elem, "sortMode", sortMode);
	blendMode = (BlendMode)intAttrib(elem, "blendMode", blendMode);
}

ParticleInfo::SubEmitter::SubEmitter()
	: enabled(false)
	, chance(1.f)
	, count(1)
{
	memset(emitterName, 0, sizeof(emitterName));
}

void ParticleInfo::SubEmitter::save(tinyxml2::XMLPrinter * printer) const
{
	printer->PushAttribute("enabled", enabled);
	printer->PushAttribute("count", count);
	printer->PushAttribute("chance", chance);
	printer->PushAttribute("emitterName", emitterName);
}

void ParticleInfo::SubEmitter::load(const tinyxml2::XMLElement * elem)
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

bool ParticleEmitter::emitParticle(
	const ParticleCallbacks & cbs,
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	ParticlePool & pool,
	const float timeOffset,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	const float positionX,
	const float positionY,
	const float positionZ,
	const float speedX,
	const float speedY,
	const float speedZ,
	Particle *& p)
{
	if (allocParticle(pool, p))
	{
		p->life = 1.f;
		p->lifeRcp = 1.f / pei.startLifetime;

		getParticleSpawnLocation(cbs, pi, p->position[0], p->position[1], p->position[2]);
		p->position[0] += positionX;
		p->position[1] += positionY;
		p->position[2] += positionZ;

		float speedAngle;
		if (pi.randomDirection)
			speedAngle = cbs.randomFloat(cbs.userData, 0.f, float(2.f * M_PI));
		else
			speedAngle = pei.startSpeedAngle * float(M_PI / 180.f);
		p->speed[0] = speedX + cosf(speedAngle) * pei.startSpeed;
		p->speed[1] = speedY + sinf(speedAngle) * pei.startSpeed;
		p->speed[2] = 0.f;
		p->rotation = pei.startRotation;

		/*
		todo
			bool inheritVelocity; // Do the particles start with the same velocity as the particle effect object?
			bool worldSpace; // Should particles be animated in the parent object’s local space (and therefore move with the object) or in world space?
		*/

		tickParticle(
			cbs,
			pei,
			pi,
			timeOffset,
			gravityX,
			gravityY,
			gravityZ,
			*p);

		if (pi.enableSubEmitters && pi.subEmitters[ParticleInfo::kSubEmitterEvent_Birth].enabled)
		{
			handleSubEmitter(
				cbs,
				pi,
				gravityX,
				gravityY,
				gravityZ,
				*p,
				ParticleInfo::kSubEmitterEvent_Birth);
		}

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

ParticlePool::~ParticlePool()
{
	Assert(head == 0);
	Assert(tail == 0);
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

// -- ParticleEffect

ParticleEffect::~ParticleEffect()
{
	emitter.clearParticles(pool);
	Assert(pool.head == 0);
	Assert(pool.tail == 0);
}

bool ParticleEffect::tick(
	const ParticleCallbacks & cbs,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	float dt)
{
	for (Particle * p = pool.head; p; )
	{
		if (!tickParticle(
			cbs,
			info->emitterInfo,
			info->particleInfo,
			dt,
			gravityX,
			gravityY,
			gravityZ,
			*p))
		{
			p = pool.freeParticle(p);
		}
		else
			p = p->next;
	}

	return tickParticleEmitter(
		cbs,
		info->emitterInfo,
		info->particleInfo,
		pool,
		dt,
		gravityX,
		gravityY,
		gravityZ,
		emitter);
}

void ParticleEffect::draw() const
{
	drawParticles(info->emitterInfo, info->particleInfo, pool, info->basePath.c_str());
}

void ParticleEffect::restart()
{
	emitter.restart(pool);
}

// -- ParticleEffectSystem

static int randomInt(void * userData, int min, int max)
{
	return min + (rand() % (max - min + 1));
}

static float randomFloat(void * userData, float min, float max)
{
	return min + (rand() % 4096) / 4095.f * (max - min);
}

static bool getEmitterByName(void * userData, const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe)
{
	ParticleEffectSystem & self = *(ParticleEffectSystem*)userData;
	
	for (auto * effect : self.effects)
	{
		if (!strcmp(name, effect->info->emitterInfo.name))
		{
			pei = &effect->info->emitterInfo;
			pi = &effect->info->particleInfo;
			pool = &effect->pool;
			pe = &effect->emitter;
			return true;
		}
	}

	return false;
}

static bool checkCollision(
	void * userData,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	float & t,
	float & nx, float & ny, float & nz)
{
	return false;
}

ParticleEffectSystem::ParticleEffectSystem()
{
	callbacks.userData = this;
	callbacks.randomInt = randomInt;
	callbacks.randomFloat = randomFloat;
	callbacks.getEmitterByName = getEmitterByName;
	callbacks.checkCollision = checkCollision;
}

ParticleEffectSystem::~ParticleEffectSystem()
{
	Assert(effects.empty());
}

ParticleEffect * ParticleEffectSystem::createEffect(const ParticleEffectInfo * effectInfo)
{
	ParticleEffect * instance = new ParticleEffect();
	instance->info = effectInfo;
	effects.push_back(instance);

	return instance;
}

void ParticleEffectSystem::removeEffect(ParticleEffect * effect)
{
	auto i = std::find(effects.begin(), effects.end(), effect);
	Assert(i != effects.end());

	delete effect;
	effect = nullptr;

	effects.erase(i);
}

void ParticleEffectSystem::tick(
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	float dt)
{
	for (auto * effect : effects)
	{
		effect->tick(
			callbacks,
			gravityX,
			gravityY,
			gravityZ,
			dt);
	}
}

void ParticleEffectSystem::draw() const
{
	for (auto * effect : effects)
	{
		effect->draw();
	}
}

//

bool loadParticleEffectLibrary(
	const char * path,
	std::vector<ParticleEffectInfo> & infos)
{
	tinyxml2::XMLDocument d;

	if (d.LoadFile(path) != tinyxml2::XML_NO_ERROR)
	{
		return false;
	}
	
	// first count the numbers of particle effect infos inside the document

	size_t peiIdx = 0;
	size_t piIdx = 0;
	
	for (tinyxml2::XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
		peiIdx++;
	
	for (tinyxml2::XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
		piIdx++;
	
	const size_t numInfos = std::max(peiIdx, piIdx);
	
	// allocate new particle effect infos
	
	const size_t firstIndex = infos.size();
	infos.resize(infos.size() + numInfos);
	
	// set the base path for each particle effect info
	
	const std::string basePath = Path::GetDirectory(path);
	
	for (size_t i = firstIndex; i < infos.size(); ++i)
		infos[i].basePath = basePath;
	
	// load the emitter and particle infos
	
	peiIdx = firstIndex;
	for (tinyxml2::XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
	{
		infos[peiIdx++].emitterInfo.load(emitterElem);
	}

	piIdx = firstIndex;
	for (tinyxml2::XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
	{
		infos[piIdx++].particleInfo.load(particleElem);
	}
	
	return true;
}
	
//

bool tickParticle(
	const ParticleCallbacks & cbs,
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float _timeStep,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	Particle & p)
{
	Assert(p.life > 0.f);

	// clamp timeStep to available life
	
	const float timeRemaining = p.life / p.lifeRcp;
	const float timeStep = fminf(timeRemaining, _timeStep);
	
	// update life
	
	p.life -= p.lifeRcp * timeStep;
	if (p.life < 0.f)
	{
		p.life = 0.f;
	}
	
	// update movement
	
	p.speed[0] += gravityX * pei.gravityMultiplier * timeStep;
	p.speed[1] += gravityY * pei.gravityMultiplier * timeStep;
	p.speed[2] += gravityZ * pei.gravityMultiplier * timeStep;

	if (pi.forceOverLifetime)
	{
		p.speed[0] += pi.forceOverLifetimeValueX * timeStep;
		p.speed[1] += pi.forceOverLifetimeValueY * timeStep;
		p.speed[2] += pi.forceOverLifetimeValueZ * timeStep;
	}

	const float oldX = p.position[0];
	const float oldY = p.position[1];
	const float oldZ = p.position[2];

	const float newX = oldX + p.speed[0] * timeStep;
	const float newY = oldY + p.speed[1] * timeStep;
	const float newZ = oldZ + p.speed[2] * timeStep;

	if (pi.collision)
	{
		float t;
		float nx;
		float ny;
		float nz;

		if (cbs.checkCollision && cbs.checkCollision(cbs.userData, oldX, oldY, oldZ, newX, newY, newZ, t, nx, ny, nz))
		{
			p.position[0] = oldX + (newX - oldX) * t;
			p.position[1] = oldY + (newY - oldY) * t;
			p.position[2] = oldZ + (newZ - oldZ) * t;

			p.life -= pi.lifetimeLoss;
			if (p.life < 0.f)
				p.life = 0.f;

			const float d = nx * p.speed[0] + ny * p.speed[1] + nz * p.speed[2];

			p.speed[0] -= nx * d * (1.f + pi.bounciness);
			p.speed[1] -= ny * d * (1.f + pi.bounciness);
			p.speed[2] -= nz * d * (1.f + pi.bounciness);
			
		#ifdef __SSE2__
			const float particleSpeed = _MM_ACCESS(_mm_sqrt_ss(_mm_set_ss(p.speed[0] * p.speed[0] + p.speed[1] * p.speed[1] + p.speed[2] * p.speed[2])), 0);
		#else
			const float particleSpeed = sqrtf(p.speed[0] * p.speed[0] + p.speed[1] * p.speed[1] + p.speed[2] * p.speed[2]);
		#endif

			if (particleSpeed < pi.minKillSpeed)
				p.life = 0.f;

			if (pi.enableSubEmitters && pi.subEmitters[ParticleInfo::kSubEmitterEvent_Collision].enabled)
			{
				handleSubEmitter(cbs, pi, gravityX, gravityY, gravityZ, p, ParticleInfo::kSubEmitterEvent_Collision);
			}
		}
		else
		{
			p.position[0] = newX;
			p.position[1] = newY;
			p.position[2] = newZ;
		}
	}
	else
	{
		p.position[0] = newX;
		p.position[1] = newY;
		p.position[2] = newZ;
	}

	const float particleLife = 1.f - p.life;
#ifdef __SSE2__
	const float particleSpeed = _MM_ACCESS(_mm_sqrt_ss(_mm_set_ss(p.speed[0] * p.speed[0] + p.speed[1] * p.speed[1] + p.speed[2] * p.speed[2])), 0);
#else
	const float particleSpeed = sqrtf(p.speed[0] * p.speed[0] + p.speed[1] * p.speed[1] + p.speed[2] * p.speed[2]);
#endif
	
	p.speedScalar = particleSpeed;

	p.rotation = computeParticleRotation(pei, pi, timeStep, particleLife, particleSpeed, p.rotation);

#if 0
	ParticleColor color;
	computeParticleColor(pei, pi, particleLife, particleSpeed, color);

	const float size = computeParticleSize(pei, pi, particleLife, particleSpeed);

	printf("tickParticle: color=%1.2f, %1.2f, %1.2f, %1.2f size=%03.2f, rotation=%+03.2f\n",
		color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3], size, p.rotation);
#endif

	if (p.life <= 0.f)
	{
		if (pi.enableSubEmitters && pi.subEmitters[ParticleInfo::kSubEmitterEvent_Death].enabled)
		{
			handleSubEmitter(cbs, pi, gravityX, gravityY, gravityZ, p, ParticleInfo::kSubEmitterEvent_Death);
		}
	}

	return p.life > 0.f;
}

void handleSubEmitter(
	const ParticleCallbacks & cbs,
	const ParticleInfo & pi,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	const Particle & p,
	const ParticleInfo::SubEmitterEvent e)
{
	if (!pi.subEmitters[e].emitterName[0])
		return;

	for (int i = 0; i < pi.subEmitters[e].count; ++i)
	{
		const float t = cbs.randomFloat(cbs.userData, 0.f, 1.f);
		if (t > pi.subEmitters[e].chance)
			continue;

		const ParticleEmitterInfo * subPei;
		const ParticleInfo * subPi;
		ParticlePool * subPool;
		ParticleEmitter * subPe;
		if (cbs.getEmitterByName && cbs.getEmitterByName(
			cbs.userData,
			pi.subEmitters[e].emitterName,
			subPei,
			subPi,
			subPool,
			subPe))
		{
			// Sub-emitter cannot be the same as the current emitter. Otherwise, we may get into a loop.
			// Note that this doesn't protect against getting into a loop in a roundabout fashion.. be
			// guarding against this is much more complicated than this simple check here.
			Assert(subPi != &pi);

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
					gravityZ,
					p.position[0],
					p.position[1],
					p.position[2],
					subPei->inheritVelocity ? p.speed[0] : 0.f,
					subPei->inheritVelocity ? p.speed[1] : 0.f,
					subPei->inheritVelocity ? p.speed[2] : 0.f,
					subP))
				{
					//
				}
			}
		}
	}
}

void getParticleSpawnLocation(
	const ParticleCallbacks & cbs,
	const ParticleInfo & pi,
	float & x,
	float & y,
	float & z)
{
	switch (pi.shape)
	{
	case ParticleInfo::kShapeType_Edge:
		{
			const float t = cbs.randomFloat(cbs.userData, 0.f, 1.f);
			x = (t - .5f) * 2.f * pi.boxSizeX;
			y = 0.f;
			z = 0.f;
		}
		break;
	case ParticleInfo::kShapeType_Box:
		{
			if (pi.emitFromShell)
			{
				const float s = pi.boxSizeX * 2.f + pi.boxSizeY * 2.f;
				const float t = cbs.randomFloat(cbs.userData, 0.f, s);

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
				const float tx = cbs.randomFloat(cbs.userData, 0.f, 1.f);
				const float ty = cbs.randomFloat(cbs.userData, 0.f, 1.f);
				x = (tx - .5f) * 2.f * pi.boxSizeX;
				y = (ty - .5f) * 2.f * pi.boxSizeY;
			}

			z = 0.f;
		}
		break;
	case ParticleInfo::kShapeType_Circle:
		{
			if (pi.emitFromShell)
			{
				const float a = cbs.randomFloat(cbs.userData, 0.f, float(2.f * M_PI));
				x = cosf(a) * pi.circleRadius;
				y = sinf(a) * pi.circleRadius;
			}
			else
			{
				for (;;)
				{
					const float tx = cbs.randomFloat(cbs.userData, 0.f, 1.f);
					const float ty = cbs.randomFloat(cbs.userData, 0.f, 1.f);
					x = (tx - .5f) * 2.f * pi.circleRadius;
					y = (ty - .5f) * 2.f * pi.circleRadius;
					const float dSquared = x * x + y * y;
					if (dSquared <= pi.circleRadius * pi.circleRadius)
						break;
				}
			}

			z = 0.f;
		}
		break;
	case ParticleInfo::kShapeType_Box3d:
		{
			const float tx = cbs.randomFloat(cbs.userData, 0.f, 1.f);
			const float ty = cbs.randomFloat(cbs.userData, 0.f, 1.f);
			const float tz = cbs.randomFloat(cbs.userData, 0.f, 1.f);
			x = (tx - .5f) * 2.f * pi.boxSizeX;
			y = (ty - .5f) * 2.f * pi.boxSizeY;
			z = (tz - .5f) * 2.f * pi.boxSizeZ;
		}
		break;
	case ParticleInfo::kShapeType_Sphere:
		{
			for (;;)
			{
				const float tx = cbs.randomFloat(cbs.userData, 0.f, 1.f);
				const float ty = cbs.randomFloat(cbs.userData, 0.f, 1.f);
				const float tz = cbs.randomFloat(cbs.userData, 0.f, 1.f);
				x = (tx - .5f) * 2.f * pi.circleRadius;
				y = (ty - .5f) * 2.f * pi.circleRadius;
				z = (tz - .5f) * 2.f * pi.circleRadius;
				const float dSquared = x * x + y * y + z * z;
				if (dSquared <= pi.circleRadius * pi.circleRadius)
					break;
			}
		}
		break;
	default:
		x = 0.f;
		y = 0.f;
		z = 0.f;
		break;
	}
}

void computeParticleColor(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float particleLife,
	const float particleSpeed,
	ParticleColor & result)
{
	result = pei.startColor;

	if (pi.colorOverLifetime)
	{
		ParticleColor temp(true);
		pi.colorOverLifetimeCurve.sample(particleLife, pi.colorOverLifetimeCurve.useLinearColorSpace, temp);
		result.modulateWith(temp);
	}

	if (pi.colorBySpeed)
	{
		const float t = (particleSpeed - pi.colorBySpeedRangeMin) / (pi.colorBySpeedRangeMax - pi.colorBySpeedRangeMin);
		ParticleColor temp(true);
		pi.colorBySpeedCurve.sample(t, pi.colorBySpeedCurve.useLinearColorSpace, temp);
		result.modulateWith(temp);
	}
}

float computeParticleSize(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float particleLife,
	const float particleSpeed)
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

float computeParticleRotation(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const float timeStep,
	const float particleLife,
	const float particleSpeed,
	float particleRotation)
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

bool tickParticleEmitter(
	const ParticleCallbacks & cbs,
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	ParticlePool & pool,
	const float timeStep,
	const float gravityX,
	const float gravityY,
	const float gravityZ,
	ParticleEmitter & pe)
{
	if (pi.rate <= 0.f)
		return false;
	if (!pei.loop && pe.totalTime >= pei.duration)
		return false;

	pe.time += timeStep;
	pe.totalTime += timeStep;

	if (pe.delaying)
	{
		if (pe.time < pei.startDelay)
			return true;
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
		pe.emitParticle(
			cbs,
			pei,
			pi,
			pool,
			timeOffset,
			gravityX,
			gravityY,
			gravityZ,
			0.f, // position
			0.f,
			0.f,
			0.f, // speed
			0.f,
			0.f,
			p);
	}

	return true;
}

//

#include "framework.h"

void drawParticles(
	const ParticleEmitterInfo & pei,
	const ParticleInfo & pi,
	const ParticlePool & pool,
	const char * basePath)
{
	char materialPath[PATH_MAX];
	sprintf_s(materialPath, sizeof(materialPath), "%s/%s", basePath, pei.materialName);
	
	gxSetTexture(Sprite(materialPath).getTexture());
	gxSetTextureSampler(GX_SAMPLE_LINEAR, true);

	if (pi.blendMode == ParticleInfo::kBlendMode_AlphaBlended)
		pushBlend(BLEND_ALPHA);
	else if (pi.blendMode == ParticleInfo::kBlendMode_Additive)
		pushBlend(BLEND_ADD);
	else
	{
		fassert(false);
		pushBlend(BLEND_OPAQUE);
	}

	gxBegin(GX_QUADS);
	{
		for (Particle * p = (pi.sortMode == ParticleInfo::kSortMode_OldestFirst) ? pool.head : pool.tail;
					 p; p = (pi.sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
		{
			const float particleLife = 1.f - p->life;
			const float particleSpeed = p->speedScalar;

			ParticleColor color(true);
			computeParticleColor(pei, pi, particleLife, particleSpeed, color);
			const float size_div_2 = computeParticleSize(pei, pi, particleLife, particleSpeed) / 2.f;

		// todo : determine particle orientation based on the view matrix
			const float s = sinf(-p->rotation * float(M_PI) / 180.f);
			const float c = cosf(-p->rotation * float(M_PI) / 180.f);

			const float * __restrict pos = p->position;
			
			gxColor4fv(color.rgba);
			gxTexCoord2f(0.f, 1.f); gxVertex3f(pos[0] + (- c - s) * size_div_2, pos[1] + (+ s - c) * size_div_2, pos[2]);
			gxTexCoord2f(1.f, 1.f); gxVertex3f(pos[0] + (+ c - s) * size_div_2, pos[1] + (- s - c) * size_div_2, pos[2]);
			gxTexCoord2f(1.f, 0.f); gxVertex3f(pos[0] + (+ c + s) * size_div_2, pos[1] + (- s + c) * size_div_2, pos[2]);
			gxTexCoord2f(0.f, 0.f); gxVertex3f(pos[0] + (- c + s) * size_div_2, pos[1] + (+ s + c) * size_div_2, pos[2]);
		}
	}
	gxEnd();
	
	gxSetTextureSampler(GX_SAMPLE_NEAREST, false);
	gxSetTexture(0);
	
	popBlend();
}
