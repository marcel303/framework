#include "cube.h"
#include "script.h"
#include <stdlib.h>

//

#include "GameOfLife.h"

static const float kParticleDirInterval = 1.f;
static const float kParticlePosInterval = .1f;

class MyEffect : public Effect
{
	const static int kNumParticles = 10;

	EffectCtx & m_ctx;

	float m_time;
	Transform m_testMatrix;
	Transform m_pointMatrix1;
	Transform m_pointMatrix2;
	Transform m_particleMatrix;
	Particle m_particles[kNumParticles];

	enum Test
	{
		kTest_None,
		kTest_Calibrate,
		kTest_MinParticleDistance,
		kTest_CircleDistance,
		kTest_SphereDistance,
		kTest_CubeDistance,
		kTest_ToroidDistance,
		kTest_CircleHullDistance,
		kTest_PointDistance,
		kTest_LineDistance,
		kTest_LineSegmentDistance,
		kTest_PerlineNoise,
		kTest_Torus82,
		kTest_Torus88,
		kTest_COUNT
	};

	enum TransformMode
	{
		kTransformMode_Identity,
		kTransformMode_Scale,
		kTransformMode_Rotate,
		kTransformMode_Translate,
		kTransformMode_COUNT
	};

	Test m_test;
	TransformMode m_transform;

	int m_bucketIndex;

	GameOfLife<SX, SY, 1> m_gameOfLife;
	float m_gameOfLifeUpdateTimer;

	CubeBuffer m_particleBuffer;
	struct ShootingStar
	{
		enum Randomize
		{
			kRandomize_Position = 1 << 0,
			kRandomize_Direction = 1 << 1
		};

		enum CollisionResponse
		{
			kCollisionResponse_Wrap,
			kCollisionResponse_Bounce,
			kCollisionResponse_Respawn
		};

		int m_randomize;
		CollisionResponse m_collisionResponse;

		int m_particleDir[3];
		float m_particleDirTimer;
		int m_particlePos[3];
		float m_particlePosTimer;

		ShootingStar()
			: m_randomize((kRandomize_Direction * 1) | (kRandomize_Position * 0))
			, m_collisionResponse(kCollisionResponse_Wrap)
			//, m_collisionResponse(kCollisionResponse_Respawn)
			//, m_collisionResponse(kCollisionResponse_Bounce)
			, m_particleDirTimer(kParticleDirInterval)
			, m_particlePosTimer(kParticlePosInterval)
		{
			randomizePosition();
			randomizeDirection();
		}

		void getNextPosition(int * xyz) const
		{
			for (int i = 0; i < 3; ++i)
				xyz[i] = m_particlePos[i] + m_particleDir[i];
		}

		void randomizePosition()
		{
			m_particlePos[0] = rand() % SX;
			m_particlePos[1] = rand() % SY;
			m_particlePos[2] = rand() % SZ;
		}

		void randomizeDirection()
		{
			m_particleDir[0] = 0;
			m_particleDir[1] = 0;
			m_particleDir[2] = 0;
			m_particleDir[rand() % 3] = (rand() % 2) ? -1 : +1;
		}

		void tick(float dt, CubeBuffer & buffer)
		{
			m_particleDirTimer += dt;
			while (m_particleDirTimer >= kParticleDirInterval)
			{
				m_particleDirTimer -= kParticleDirInterval;
				if (m_randomize & kRandomize_Position)
					randomizePosition();
				if (m_randomize & kRandomize_Direction)
					randomizeDirection();
			}

			m_particlePosTimer += dt;
			while (m_particlePosTimer >= kParticlePosInterval)
			{
				m_particlePosTimer -= kParticlePosInterval;
				buffer.m_value[m_particlePos[0]][m_particlePos[1]][m_particlePos[2]] = 1.f;

				const int wrapSizes[3] = { SX, SY, SZ };

				for (int i = 0; i < 3; ++i)
				{
					if (m_collisionResponse == kCollisionResponse_Wrap)
						m_particlePos[i] = (m_particlePos[i] + m_particleDir[i] + wrapSizes[i]) % wrapSizes[i];
					else if (m_collisionResponse == kCollisionResponse_Bounce)
					{
						int newPos = m_particlePos[i] + m_particleDir[i];
						if (newPos < 0 || newPos >= wrapSizes[i])
						{
							m_particleDir[i] *= -1;
							newPos = m_particlePos[i] + m_particleDir[i];
						}
						m_particlePos[i] = newPos;
					}
					else if (m_collisionResponse == kCollisionResponse_Respawn)
					{
						const int newPos = m_particlePos[i] + m_particleDir[i];
						if (newPos < 0 || newPos >= wrapSizes[i])
						{
							randomizePosition();
							randomizeDirection();
							break;
						}
						else
							m_particlePos[i] = newPos;
					}
				}
			}
		}
	};

	static const int kNumShootingStars = 10;
	ShootingStar m_shootingStars[kNumShootingStars];

public:
	MyEffect(EffectCtx & ctx)
		: m_ctx(ctx)
		, m_time(0.f)
		, m_test(kTest_None)
		, m_transform(kTransformMode_Identity)
		, m_bucketIndex(0)
		, m_gameOfLifeUpdateTimer(0.f)
	{
		for (int i = 0; i < kNumParticles; ++i)
		{
			m_particles[i].active = true;
			m_particles[i].x = rand(-1.f, +1.f);
			m_particles[i].y = rand(-1.f, +1.f);
			m_particles[i].z = rand(-1.f, +1.f);
		}

		m_gameOfLife.randomize();
	}

	virtual void tick(const float dt) override
	{
		if (m_ctx.keyWentDown(SDLK_LEFT))
			m_bucketIndex = max(0, m_bucketIndex - 1);
		if (m_ctx.keyWentDown(SDLK_RIGHT))
			m_bucketIndex = min(m_bucketIndex + 1, m_ctx.fftBucketCount() - 1);

		if (m_ctx.keyWentDown(SDLK_t))
			m_test = (Test)((m_test + (m_ctx.keyIsDown(SDLK_LSHIFT) ? -1 : 1) + kTest_COUNT) % kTest_COUNT);
		if (m_ctx.keyWentDown(SDLK_x))
			m_transform = (TransformMode)((m_transform + (m_ctx.keyIsDown(SDLK_LSHIFT) ? -1 : 1) + kTransformMode_COUNT) % kTransformMode_COUNT);

		static int timeScale = 0;
		if (m_ctx.keyWentDown(SDLK_s))
		{
			timeScale = (timeScale + 1) % 5;
		}

		m_time += dt / (timeScale + 1.f);

		//

		m_testMatrix.reset();
		m_testMatrix.scale(.5f, .5f, .5f);

		switch (m_transform)
		{
		case kTransformMode_Identity:
			break;

		case kTransformMode_Scale:
			{
				const float s = std::sinf(m_time);

				m_testMatrix.scale(s, s, s);
			}
			break;

		case kTransformMode_Rotate:
			{
				m_testMatrix.rotateX(m_time / 1.111f);
				m_testMatrix.rotateY(m_time / 1.333f);
				m_testMatrix.rotateZ(m_time / 1.777f);
			}
			break;

		case kTransformMode_Translate:
			{
				const float dx = std::sinf(m_time / 1.111f);
				const float dy = std::sinf(m_time / 1.333f);
				const float dz = std::sinf(m_time / 1.777f);

				m_testMatrix.translate(dx, dy, dz);
			}
			break;
		}

		//

		m_pointMatrix1.reset();
		m_pointMatrix1.scale(.2f, .2f, .2f);
		m_pointMatrix1.rotateY(m_time);
		m_pointMatrix1.rotateZ(m_time);

		m_pointMatrix2.reset();
		m_pointMatrix2.scale(.2f, .2f, .2f);
		m_pointMatrix2.rotateZ(m_time);
		m_pointMatrix2.rotateY(m_time);

		m_particleMatrix.reset();
		m_particleMatrix.rotateX(m_time);

		//

		const float kGameOfLifeUpdateInterval = .2f;
		m_gameOfLifeUpdateTimer += dt;
		while (m_gameOfLifeUpdateTimer >= kGameOfLifeUpdateInterval)
		{
			m_gameOfLifeUpdateTimer -= kGameOfLifeUpdateInterval;
			m_gameOfLife.evolve();
		}

		//

		m_particleBuffer.fadeLinear(.25f, dt);

		for (int i = 0; i < kNumShootingStars; ++i)
			m_shootingStars[i].tick(dt, m_particleBuffer);
	}

	virtual void evaluateRaw(int x, int y, int z, float & value) override
	{
		value = max(value, m_particleBuffer.m_value[x][y][z]);

	#if 0
		//if (z == SZ / 2)
			if (m_gameOfLife.sample(x, y, z))
				value = max(value, 1.f);
	#endif
	}

	virtual float evaluate(const Coord & c) override
	{
		const float controlX = m_ctx.mouseX() / float(800.f);
		const float controlY = m_ctx.mouseY() / float(800.f);

		const float power = m_ctx.keyIsDown(SDLK_p) ? 1.f : m_ctx.fftBucketValue(m_bucketIndex);

		bool doTestPostEffects = true;

		const Coord testCoord = m_testMatrix.apply(c);

		float d;

		switch (m_test)
		{
		case kTest_None:
			{
				doTestPostEffects = false;

				const int mode = int(m_time / 4.f) % 5;
			
				if (mode == 0)
				{
					const float s = 10.f;
					const float t1 = (sinf(m_time * s / 1.111f) + 1.f) * .25f;
					const float t2 = (sinf(m_time * s / 2.333f) + 1.f) * .25f;
					d = computePlaneDistance(twistZ(twistY(c, t1), t2)) * 2.f;
					d = (1.f - pow(d, power));
				}
				else if (mode == 1)
				{
					const float r = .4f + .3f * sinf(m_time);
					d = computeCircleHullDistance(repeat(c, 0.f, 0.f, r)) * 3.f;
					d = 1.f - pow(d, power);
				}
				else if (mode == 2)
				{
					const Coord c1 = c;
					const Coord c2 = c + Coord(5.2f, 1.3f, 0.f);
					const Coord c3(
						computePerlinNoise(c1, m_time / 1.111f),
						computePerlinNoise(c2, m_time / 1.333f),
						0.f);

					d = computePerlinNoise(c + c3, m_time / 5.777f) * 3.f;
				}
				else if (mode == 3)
				{
					d = computePerlinNoise(c, m_time) * 3.f;
				}
				else
				{
					auto c1 = m_pointMatrix1.apply(c);
					auto c2 = m_pointMatrix2.apply(c);
					//const float d = 1.f - computePointDistance(x, y, z);
					//const float d = 1.f - pow(computeLineDistance(x, y, z), 4.f);
					const float d1 = computePlaneDistance(twistY(c1, .05f + controlX / 10.f));
					const float d2 = computePlaneDistance(twistZ(c2, .03f + controlY / 10.f));
					const float d3 = computePerlinNoise(testCoord, m_time) * 2.f + 1.f;
					const float d4 = computeMinParticleDistance(m_particleMatrix.apply(c), m_particles, kNumParticles) * 4.f;
					d = min(d1, d2, d3, d4);
					//d = min(d4);
					d = (1.f - pow(d, power));
				}
			}
			break;

		case kTest_Calibrate:
			{
				doTestPostEffects = false;

				Transform topMatrix;
				topMatrix.translate(0.f, -1.f, 0.f);
				topMatrix.rotateZ((float)M_PI/2.f);
				d = computePlaneDistance(topMatrix.apply(c));

				if (m_ctx.keyIsDown(SDLK_LSHIFT))
				{
					Transform leftMatrix;
					leftMatrix.translate(-1.f, 0.f, 0.f);
					//d = csgUnion(d, computePlaneDistance(leftMatrix.apply(c)));
					d = csgSoftUnion(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
					//d = csgSoftIntersection(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
				}

				d = 1.f - d * 4.f;
			}
			break;

		case kTest_MinParticleDistance:
			d = computeMinParticleDistance(testCoord, m_particles, kNumParticles) * 2.f;
			break;

		case kTest_CircleDistance:
			d = computeCircleDistance(testCoord);
			break;

		case kTest_SphereDistance:
			d = computeSphereDistance(testCoord);
			break;

		case kTest_CubeDistance:
			d = computeCubeDistance(testCoord, 1.f, 1.f, 1.f);
			break;

		case kTest_ToroidDistance:
			d = computeToroidDistance(testCoord, .2f, .8f);
			break;

		case kTest_CircleHullDistance:
			d = computeCircleHullDistance(testCoord);
			break;

		case kTest_PointDistance:
			d = computePointDistance(testCoord);
			break;

		case kTest_LineDistance:
			d = computeLineDistance(testCoord);
			break;

		case kTest_LineSegmentDistance:
			d = computeLineSegmentDistance(testCoord, -1.f, 0.f, .25f);
			break;

		case kTest_PerlineNoise:
			d = computePerlinNoise(testCoord, m_time) + 1.f;
			break;

		case kTest_Torus82:
			d = computeTorus82(testCoord, 1.f, .25f);
			break;

		case kTest_Torus88:
			d = computeTorus88(testCoord, 1.f, .25f);
			break;
		}

		if (d < 0.f)
			d = 0.f;

		if (doTestPostEffects)
			d = 1.f - pow(d, power);

		return d;
	}

	virtual void debugDraw() override
	{
		m_ctx.setColor(255, 255, 255);
		m_ctx.drawText(m_ctx.mouseX(), m_ctx.mouseY() + 25, 12, 0, 1, "power:%.2f", m_ctx.fftBucketValue(m_bucketIndex));
	}
};

//

#if defined(WIN32)

extern "C"
{
	__declspec(dllexport) Effect * __cdecl create(EffectCtx & ctx)
	{
		return new MyEffect(ctx);
	}

	__declspec(dllexport) void __cdecl destroy(Effect * effect)
	{
		delete effect;
	}
}

#endif
