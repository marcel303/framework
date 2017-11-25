#include "framework.h"
#include "particle.h"
#include "particle_editor.h"
#include "tinyxml2.h"

#include "StringEx.h" // _s functions
#include "ui.h"

using namespace tinyxml2;

#define DO_PARTICLELIB_TEST 0

#if DO_PARTICLELIB_TEST

static void testParticleLib()
{
	ParticleColor c1;
	ParticleColor c2;
	ParticleColorCurve cc1;
	ParticleColorCurve cc2;
	ParticleEmitterInfo pei1;
	ParticleEmitterInfo pei2;
	ParticleInfo pi1;
	ParticleInfo pi2;
	c1.rgba[0] = (rand() % 256) / 255.f;
	c1.rgba[1] = (rand() % 256) / 255.f;
	c1.rgba[2] = (rand() % 256) / 255.f;
	c1.rgba[3] = (rand() % 256) / 255.f;
	for (int i = 0; i < 4; ++i)
	{
		ParticleColorCurve::Key * key;
		if (cc1.allocKey(key))
		{
			key->t = (rand() % 256) / 255.f;
			key->color.rgba[0] = (rand() % 256) / 255.f;
			key->color.rgba[1] = (rand() % 256) / 255.f;
			key->color.rgba[2] = (rand() % 256) / 255.f;
			key->color.rgba[3] = (rand() % 256) / 255.f;
		}
	}
	cc1.sortKeys();
	pei1.duration = 0.f;
	pei1.gravityMultiplier = .05f;
	pei1.inheritVelocity = false;
	pei1.loop = true;
	pei1.maxParticles = 100;
	pei1.prewarm = false;
	pei1.startColor.set(1.f, 1.f, 1.f, 1.f);
	pei1.startDelay = .1f;
	pei1.startLifetime = 2.f;
	pei1.startRotation = 0.f;
	pei1.startSize = 200.f;
	pei1.startSpeed = 100.f;
	pei1.worldSpace = true;
	strcpy_s(pei1.materialName, sizeof(pei1.materialName), "texture.png");
	pi1.blendMode = ParticleInfo::kBlendMode_AlphaBlended;
	pi1.bounciness = .8f;
	pi1.boxSizeX = 100.f;
	pi1.boxSizeY = 10.f;
	pi1.circleRadius = 50.f;
	pi1.collision = false;
	pi1.collisionRadius = 5.f;
	pi1.colorBySpeed = true;
	pi1.colorBySpeedCurve.setLinearAlpha(0.f, 1.f);
	pi1.colorBySpeedRangeMin = 0.f;
	pi1.colorBySpeedRangeMax = 100.f;
	pi1.colorOverLifetime = true;
	pi1.colorOverLifetimeCurve.setLinear(ParticleColor(1.f, 1.f, 1.f, 1.f), ParticleColor(1.f, .5f, .25f, 0.f));
	pi1.emitFromShell = false;
	pi1.forceOverLifetime = false;
	pi1.forceOverLifetimeValueX = 100.f;
	pi1.forceOverLifetimeValueY = 0.f;
	pi1.lifetimeLoss = 0.f;
	pi1.minKillSpeed = 0.f;
	pi1.numBursts = 0;
	pi1.randomDirection = true;
	pi1.rate = 13.f;
	pi1.rotationBySpeed = true;
	pi1.rotationBySpeedCurve.setLinear(0.f, 200.f);
	pi1.rotationBySpeedRangeMin = 0.f;
	pi1.rotationBySpeedRangeMax = 200.f;
	pi1.rotationOverLifetime = true;
	pi1.rotationOverLifetimeValue = 200.f;
	pi1.shape = ParticleInfo::kShapeBox;
	pi1.sizeBySpeed = true;
	pi1.sizeBySpeedCurve.setLinear(1.f, .5f);
	pi1.sizeBySpeedRangeMin = 0.f;
	pi1.sizeBySpeedRangeMax = 100.f;
	pi1.sizeOverLifetime = true;
	pi1.sizeOverLifetimeCurve.setLinear(0.f, 1.f);
	pi1.sortMode = ParticleInfo::kSortMode_YoungestFirst;
	pi1.enableSubEmitters = false;
	pi1.velocityOverLifetime = false;
	//pi1.velocityOverLifetimeValue;
	pi1.velocityOverLifetimeLimit = false;
	//pi1.velocityOverLifetimeLimitCurve;
	//pi1.velocityOverLifetimeLimitDampen;

#if 0
	c2 = c1;
	cc2 = cc1;
	pei2 = pei1;
	pi2 = pi1;
#else
	XMLPrinter p;

	p.OpenElement("c");
	{
		c1.save(&p);
	}
	p.CloseElement();

	p.OpenElement("cc");
	{
		cc1.save(&p);
	}
	p.CloseElement();

	p.OpenElement("pei");
	{
		pei1.save(&p);
	}
	p.CloseElement();

	p.OpenElement("pi");
	{
		pi1.save(&p);
	}
	p.CloseElement();

	XMLDocument d;
	auto e = d.Parse(p.CStr());

	auto cElem = d.FirstChildElement("c");
	if (cElem)
		c2.load(cElem);
	auto ccElem = d.FirstChildElement("cc");
	if (ccElem)
		cc2.load(ccElem);
	auto peiElem = d.FirstChildElement("pei");
	if (peiElem)
		pei2.load(peiElem);
	auto piElem = d.FirstChildElement("pi");
	if (piElem)
		pi2.load(piElem);

	fassert(c1 == c2);
	fassert(cc1 == cc2);
	fassert(pei1 == pei2);
	fassert(pi1 == pi2);
#endif

#if 0
	Particle part;
	part.life = 1.f;
	part.lifeRcp = 1.f;
	part.position[0] = 0.f;
	part.position[1] = 0.f;
	part.rotation = 0.f;
	part.speed[0] = 0.f;
	part.speed[1] = -100.f;

	for (int i = 0; i < 100; ++i)
	{
		const float timeStep = 1.f / 100.f;
		const float gravityX = 0.f;
		const float gravityY = 100.f;
		tickParticle(pei2, pi2, timeStep, gravityX, gravityY, part);
	}
#endif

	ParticlePool pp;

#if 0
	std::vector<Particle*> particles;
	for (int i = 0; i < 100; ++i)
	{
		if ((rand() % 2) && !particles.empty())
		{
			pp.freeParticle(particles.back());
			particles.pop_back();
		}
		else
			particles.push_back(pp.allocParticle());
		size_t count = 0;
		for (auto p = pp.head; p; p = p->next)
			count++;
		fassert(count == particles.size());
	}
	while (!particles.empty())
	{
		pp.freeParticle(particles.back());
		particles.pop_back();
	}
	fassert(!pp.head && !pp.tail);
#endif

	ParticleEmitter pe;

#if 0
	for (int i = 0; i < 100; ++i)
	{
		const float timeStep = 1.f / 100.f;
		const float gravityX = 0.f;
		const float gravityY = 100.f;
		for (Particle * p = pp.head; p; p = p->next)
			tickParticle(pei2, pi2, timeStep, gravityX, gravityY, *p);
		tickParticleEmitter(pei2, pi2, pp, timeStep, gravityX, gravityY, pe);
	}
#endif

	if (framework.init(0, nullptr, 1400, 900))
	{
		while (!framework.quitRequested)
		{
			framework.process();
			
			const float timeStep = std::min(framework.timeStep, 1.f / 15.f);
			
		#if 1
			const float gravityX = 0.f;
			const float gravityY = 100.f;
			ParticleCallbacks cbs;
			cbs.randomInt = [](void* userData, int min, int max) { return random(min, max); };
			cbs.randomFloat = [](void* userData, float min, float max) { return random(min, max); };
			for (Particle * p = pp.head; p; )
				if (!tickParticle(cbs, pei2, pi2, timeStep, gravityX, gravityY, *p))
					p = pp.freeParticle(p);
				else
					p = p->next;
			tickParticleEmitter(cbs, pei2, pi2, pp, timeStep, gravityX, gravityY, pe);
		#endif
		
			framework.beginDraw(0, 0, 0, 0);
			{
			#if 1
				gxSetTexture(Sprite(pei2.materialName).getTexture());
				for (Particle * p = pp.head; p; p = p->next)
				{
					const float particleLife = 1.f - p->life;
					const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);

					ParticleColor color;
					computeParticleColor(pei2, pi2, particleLife, particleSpeed, color);
					const float size = computeParticleSize(pei2, pi2, particleLife, particleSpeed);
					gxPushMatrix();
					const float offs[2] = { 320, 240 };
					gxTranslatef(
						offs[0] + p->position[0],
						offs[1] + p->position[1],
						0.f);
					gxRotatef(p->rotation, 0.f, 0.f, 1.f);
					setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
					drawRect(
						- size / 2.f,
						- size / 2.f,
						+ size / 2.f,
						+ size / 2.f);
					gxPopMatrix();
				}
				gxSetTexture(0);
			#endif

			#if 1
				gxBegin(GL_QUADS);
				{
					const int sx = 5;
					const int sy = 100;
					for (int i = 0; i < 100; ++i)
					{
						ParticleColor c;
						cc2.sample(i / 99.f, std::fmod(framework.time, 2.f) < 1.f, c);
						gxColor4f(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
						gxVertex2f((i + 0) * sx, 0);
						gxVertex2f((i + 1) * sx, 0);
						gxVertex2f((i + 1) * sx, sy);
						gxVertex2f((i + 0) * sx, sy);
						//printf("sampled color: %01.2f, %01.2f, %01.2f, %01.2f\n", c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
					}
				}
				gxEnd();

			#if 0 // work around for weird (driver?) issue where next draw call retains the color of the previous one
				gxBegin(GL_TRIANGLES);
				gxColor4f(0.f, 0.f, 0.f, 0.f);
				gxEnd();
			#endif
			#endif
			}
			framework.endDraw();
		}

		framework.shutdown();
	}
}

#endif

int main(int argc, char * argv[])
{
#ifdef WIN32
	_CrtSetDebugFillThreshold(0);
#endif

#if DO_PARTICLELIB_TEST
	testParticleLib();
#endif
	
	framework.fullscreen = false;
	
	framework.enableRealTimeEditing = true;

	const int windowSx = 1400;
	const int windowSy = 900;

	if (framework.init(argc, (const char**)argv, windowSx, windowSy))
	{
		initUi();
		
		ParticleEditor * particleEditor = new ParticleEditor();
		
		bool menuActive = true;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			const float timeStep = std::min(framework.timeStep, 1.f / 15.f);

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			if (keyboard.wentDown(SDLK_F1))
			{
				framework.fullscreen = !framework.fullscreen;
				framework.setFullscreen(framework.fullscreen);
			}

			if (keyboard.wentDown(SDLK_TAB))
				menuActive = !menuActive;
			
			particleEditor->tick(menuActive, windowSx, windowSy, timeStep);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				particleEditor->draw(menuActive, windowSx, windowSy);
			}
			framework.endDraw();
		}
		
		delete particleEditor;
		particleEditor = nullptr;
		
		shutUi();

		framework.shutdown();
	}

	return 0;
}
