#include "AudioFFT.h"
#include "Calc.h"
#include "framework.h"
#include "GameOfLife.h"
#include "simplexnoise.h"

#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"

#include <Windows.h>

#define USE_AUDIO_INPUT 1

#define ArraySize(x) (sizeof(x) / sizeof(x[0]))

#if 0
	#define SX 30
	#define SY 30
	#define SZ 30
#else
	#define SX 20
	#define SY 20
	#define SZ 20
#endif

static float min(float x) { return x; }
static float min(float x, float y) { return x < y ? x : y; }
static float min(float x, float y, float z) { return min(x, min(y, z)); }
static float min(float x, float y, float z, float w) { return min(x, min(y, min(z, w))); }
static float min(float x, float y, float z, float w, float s) { return min(x, min(y, min(z, min(w, s)))); }

static float max(float x, float y)
{
	return x > y ? x : y;
}

static float max(float x, float y, float z)
{
	return max(x, max(y, z));
}

static float rand(float min, float max)
{
	return min + (max - min) * (rand() % 1024) / 1023.f;
}

struct Coord
{
	float x, y, z;

	Coord()
	{
	}

	Coord(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Coord operator+(const Coord & other) const
	{
		Coord result;
		result.x = x + other.x;
		result.y = y + other.y;
		result.z = z + other.z;
		return result;
	}
};

class Effect
{
public:
	virtual void evaluateRaw(int x, int y, int z, float & value) = 0;
	virtual float evaluate(const Coord & c) = 0;
};

struct CubeBuffer
{
	float m_value[SX][SY][SZ];

	CubeBuffer()
	{
		reset();
	}

	void reset()
	{
		memset(m_value, 0, sizeof(m_value));
	}

	void copyFrom(const CubeBuffer & other)
	{
		memcpy(m_value, other.m_value, sizeof(m_value));
	}

	void fadeLinear(float speed, float dt)
	{
		for (int x = 0; x < SX; ++x)
		{
			for (int y = 0; y < SY; ++y)
			{
				for (int z = 0; z < SZ; ++z)
				{
					m_value[x][y][z] = std::max(0.f, m_value[x][y][z] - speed * dt);
				}
			}
		}
	}

	void fadeIncremental(float lossPerSecond, float dt)
	{
		const float loss = std::powf(lossPerSecond, dt);

		for (int x = 0; x < SX; ++x)
		{
			for (int y = 0; y < SY; ++y)
			{
				for (int z = 0; z < SZ; ++z)
				{
					m_value[x][y][z] *= loss;
				}
			}
		}
	}
};

struct Cube : CubeBuffer
{
	Cube()
		: CubeBuffer()
	{
	}
};

class BiMatrix
{
	Mat4x4 m_matrix;
	Mat4x4 m_matrixInverse;
	bool m_dirty;

public:
	BiMatrix()
		: m_dirty(false)
	{
		m_matrix.MakeIdentity();
		m_matrixInverse.MakeIdentity();
	}

	void reset()
	{
		matrix().MakeIdentity();
	}

	void scale(float x, float y, float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeScaling(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void translate(float x, float y, float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeTranslation(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void rotateX(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationX(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateY(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationY(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateZ(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationZ(r);
		matrix() = matrix() * tempMatrix;
	}

	Coord apply(const Coord & c)
	{
		const Vec3 v = matrixInverse().Mul4(Vec3(c.x, c.y, c.z));
		Coord r;
		r.x = v[0];
		r.y = v[1];
		r.z = v[2];
		return r;
	}

	Mat4x4 & matrix()
	{
		m_dirty = true;
		return m_matrix;
	}

	const Mat4x4 & matrixInverse()
	{
		if (m_dirty)
		{
			m_dirty = false;
			m_matrixInverse = m_matrix.CalcInv();
		}

		return m_matrixInverse;
	}
};

struct Particle
{
	bool active;
	float x, y, z;

	Particle()
	{
		memset(this, 0, sizeof(*this));
	}
};

static float computeMinParticleDistance(const Coord & c, const Particle * particles, int numParticles);
static float computeCircleDistance(const Coord & c);
static float computeSphereDistance(const Coord & c);
static float computeCubeDistance(const Coord & c);
static float computeToroidDistance(const Coord & c, float thickness, float outer);
static float computeCircleHullDistance(const Coord & c);
static float computePointDistance(const Coord & c);
static float computeLineDistance(const Coord & c);
static float computeLineSegmentDistance(const Coord & c, const float x1, const float x2, float r);
static float computePerlinNoise(const Coord & c, float w);

static float length1(float x)
{
	return sqrtf(x * x);
}

static float length2(float x, float y)
{
	return sqrtf(x * x + y * y);
}

static float length3(float x, float y, float z)
{
	return sqrtf(x * x + y * y + z * z);
}

static float dot3(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return
		x1 * x2 +
		y1 * y2 +
		z1 * z2;
}

static Coord repeat(const Coord & coord, float repeatX, float repeatY, float repeatZ)
{
	Coord result;
	result.x = repeatX == 0.f ? coord.x : fmodf(coord.x, repeatX);
	result.y = repeatY == 0.f ? coord.y : fmodf(coord.y, repeatY);
	result.z = repeatZ == 0.f ? coord.z : fmodf(coord.z, repeatZ);
	return result;
}

static float computeMinParticleDistance(const Coord & c, const Particle * particles, int numParticles)
{
	float minDistance = std::numeric_limits<float>::max();

	for (int i = 0; i < numParticles; ++i)
	{
		if (!particles[i].active)
			return false;

		const float dx = c.x - particles[i].x;
		const float dy = c.y - particles[i].y;
		const float dz = c.z - particles[i].z;
		const float d = length3(dx, dy, dz);

		if (d < minDistance)
			minDistance = d;
	}

	return minDistance;
}

static float computeCircleDistance(const Coord & c)
{
	const float dx = c.x;
	const float dy = c.y;
	const float dc = length2(dx, dy);
	const float d =
		dc <= 1.f
		? length1(c.z)
		: length1(c.z) + dc - 1.f;
	return d;
}

static float computeSphereDistance(const Coord & c)
{
	return length3(c.x, c.y, c.z) - 1.f;
}

static float computeCubeDistance(const Coord & c, float sx, float sy, float sz)
{
	// actually a round box

	return length3(
		max(0.f, abs(c.x) - sx),
		max(0.f, abs(c.y) - sy),
		max(0.f, abs(c.z) - sz));
}

static float computeToroidDistance(const Coord & c, float thickness, float outer)
{
	const float s = length2(c.x, c.z);
	const float dx = s - outer;
	const float dy = c.y;
	const float d = length2(dx, dy);
	return d - thickness;
}

static float computeCircleHullDistance(const Coord & c)
{
	const float dx = c.x;
	const float dy = c.y;
	const float dc = length2(dx, dy);
	if (dc == 0.f)
		return 1.f;
	else
	{
		const float x = dx / dc;
		const float y = dy / dc;
		const float z = 0.f;
		const float dx2 = x - c.x;
		const float dy2 = y - c.y;
		const float dz2 = z - c.z;
		const float d = length3(dx2, dy2, dz2);
		return d;
	}
}

static float computePointDistance(const Coord & c)
{
	const float dx = c.x;
	const float dy = c.y;
	const float dz = c.z;
	const float d = length3(dx, dy, dz);
	return d;
}

static float computeLineDistance(const Coord & c)
{
	const float dx = 0.f;
	const float dy = c.y;
	const float dz = c.z;
	const float d = length3(dx, dy, dz);
	return d;
}

static float computePlaneDistance(const Coord & c)
{
	return std::abs(c.x);
}

static float computeLineSegmentDistance(const Coord & c, const float x1, const float x2, float r)
{
	const float px = c.x - x1;
	const float py = c.y - 0.f;
	const float pz = c.z - 0.f;
	const float bx = x2 - x1;
	const float by = 0.f;
	const float bz = 0.f;
	const float h = max(0.f, min(1.f, dot3(px, py, pz, bx, by, bz) / dot3(bx, by, bz, bx, by, bz)));
	const float d = length3(
		px - bx * h,
		py - by * h,
		pz - bz * h);
	return d - r;
}

static float computePerlinNoise(const Coord & c, float w)
{
	const float value = octave_noise_4d(4.f, .5f, .5f, c.x, c.y, c.z, w);

	return value;
}

float torus82(const Coord & c, float a, float b)
{
	const float dx = (c.x * c.x + c.z * c.z) - a;
	const float dy = c.y;
	const float d = dx * dx * dx * dx + dy * dy * dy * dy;
	return d - b;
}

float torus88(const Coord & c, float a, float b)
{
	const float dx = (c.x * c.x * c.x * c.x + c.z * c.z * c.z * c.z) - a;
	const float dy = c.y;
	const float d = dx * dx * dx * dx + dy * dy * dy * dy;
	return d - b;
}

//

static Coord twistX(const Coord & c, const float scale)
{
	Coord result;
	Mat4x4 m;
	m.MakeRotationX(-2.f * M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
	const Vec3 t = m * Vec3(c.x, c.y, c.z);
	result.x = t[0];
	result.y = t[1];
	result.z = t[2];
	return result;
}

static Coord twistY(const Coord & c, const float scale)
{
	Coord result;
	Mat4x4 m;
	m.MakeRotationY(-2.f * M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
	const Vec3 t = m * Vec3(c.x, c.y, c.z);
	result.x = t[0];
	result.y = t[1];
	result.z = t[2];
	return result;
}

static Coord twistZ(const Coord & c, const float scale)
{
	Coord result;
	Mat4x4 m;
	m.MakeRotationZ(-2.f * M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
	const Vec3 t = m * Vec3(c.x, c.y, c.z);
	result.x = t[0];
	result.y = t[1];
	result.z = t[2];
	return result;
}

//

static float csgUnion(float d1, float d2)
{
	return min(d1, d2);
}

static float csgSubtraction(float d1, float d2)
{
	return max(-d1, d2);
}

static float csgIntersection(float d1, float d2)
{
	return max(d1, d2);
}

float csgSoftUnion(float a, float b, float k)
{
	const float h = clamp(.5f + .5f * (b - a) / k, 0.f, 1.f);
	return lerp(b, a, h) - k * h * (1.f - h);
}

float csgSoftIntersection(float a, float b, float k)
{
	const float h = clamp(.5f + .5f * (b - a) / k, 0.f, 1.f);
	return lerp(a, b, h) + k * h * (1.f - h);
}

//

static const int kFFTBufferSize = 4096;
static const int kFFTSize = 1024;
static const int kFFTComplexSize = 513; // n/2+1
static const int kFFTBucketCount = 32;
static audiofft::AudioFFT s_fft;

float s_fftInputBuffer[4096];
float s_fftInput[kFFTSize] = { };
float s_fftReal[kFFTComplexSize] = { };
float s_fftImaginary[kFFTComplexSize] = { };
float s_fftBuckets[kFFTBucketCount] = { };

static float s_fftProvideTime = 0.f;

static void fftInit()
{
	s_fft.init(kFFTSize);
}

static float fftPowerValue(int i)
{
	float p = s_fftReal[i] * s_fftReal[i] + s_fftImaginary[i] * s_fftImaginary[i];
#if 1
	p = sqrtf(p);
#else
	p = 10.f * std::log10f(p);
#endif
	return p;
}

static void fftProcess()
{
	const float dt = framework.time - s_fftProvideTime;
	int sampleStart = dt * 44100.f; // fixme
	if (sampleStart + kFFTSize > kFFTBufferSize)
		sampleStart = kFFTBufferSize - kFFTSize;

	//sampleStart = 0;

	s_fft.fft(s_fftInputBuffer + sampleStart, s_fftReal, s_fftImaginary);

	for (int i = 0; i < kFFTBucketCount; ++i)
	{
		const int numSamples = kFFTComplexSize / kFFTBucketCount;
		const int j1 = (i + 0) * numSamples;
		const int j2 = (i + 1) * numSamples;
		Assert(j2 <= kFFTSize);

		float result = 0.f;

		for (int j = j1; j < j2; ++j)
			result += fftPowerValue(j);

		s_fftBuckets[i] = result / numSamples;
	}
}

//

static const float kParticleDirInterval = 1.f;
static const float kParticlePosInterval = .1f;

class MyEffect : public Effect
{
	const static int kNumParticles = 10;

	float m_time;
	BiMatrix m_testMatrix;
	BiMatrix m_pointMatrix1;
	BiMatrix m_pointMatrix2;
	BiMatrix m_particleMatrix;
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

	enum Transform
	{
		kTransform_Identity,
		kTransform_Scale,
		kTransform_Rotate,
		kTransform_Translate,
		kTransform_COUNT
	};

	Test m_test;
	Transform m_transform;

	int m_bucketIndex;

	GameOfLife<SX, SY, 1> m_gameOfLife;
	float m_gameOfLifeUpdateTimer;

	CubeBuffer m_particleBuffer;
	struct ShootingStar
	{
		int m_particleDir[3];
		float m_particleDirTimer;
		int m_particlePos[3];
		float m_particlePosTimer;

		ShootingStar()
			: m_particleDirTimer(kParticleDirInterval)
			, m_particlePosTimer(kParticlePosInterval)
		{
		}

		void tick(float dt, CubeBuffer & buffer)
		{
			m_particleDirTimer += dt;
			while (m_particleDirTimer >= kParticleDirInterval)
			{
				m_particleDirTimer -= kParticleDirInterval;
				m_particlePos[0] = rand() % SX;
				m_particlePos[1] = rand() % SY;
				m_particlePos[2] = rand() % SZ;
				m_particleDir[0] = 0;
				m_particleDir[1] = 0;
				m_particleDir[2] = 0;
				m_particleDir[rand() % 3] = (rand() % 2) ? -1 : +1;
			}

			m_particlePosTimer += dt;
			while (m_particlePosTimer >= kParticlePosInterval)
			{
				m_particlePosTimer -= kParticlePosInterval;
				buffer.m_value[m_particlePos[0]][m_particlePos[1]][m_particlePos[2]] = 1.f;
				const int wrapSizes[3] = { SX, SY, SZ };
				for (int i = 0; i < 3; ++i)
					m_particlePos[i] = (m_particlePos[i] + m_particleDir[i] + wrapSizes[i]) % wrapSizes[i];
			}
		}
	};

	static const int kNumShootingStars = 10;
	ShootingStar m_shootingStars[kNumShootingStars];

public:
	MyEffect()
		: m_time(0.f)
		, m_test(kTest_None)
		, m_transform(kTransform_Identity)
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

	void tick(float dt)
	{
		if (keyboard.wentDown(SDLK_LEFT))
			m_bucketIndex = Calc::Max(0, m_bucketIndex - 1);
		if (keyboard.wentDown(SDLK_RIGHT))
			m_bucketIndex = Calc::Min(m_bucketIndex + 1, kFFTBucketCount - 1);

		if (keyboard.wentDown(SDLK_t))
			m_test = (Test)((m_test + (keyboard.isDown(SDLK_LSHIFT) ? -1 : 1) + kTest_COUNT) % kTest_COUNT);
		if (keyboard.wentDown(SDLK_x))
			m_transform = (Transform)((m_transform + (keyboard.isDown(SDLK_LSHIFT) ? -1 : 1) + kTransform_COUNT) % kTransform_COUNT);

		m_time += dt;

		//

		m_testMatrix.reset();
		m_testMatrix.scale(.5f, .5f, .5f);

		switch (m_transform)
		{
		case kTransform_Identity:
			break;

		case kTransform_Scale:
			{
				const float s = std::sinf(framework.time);

				m_testMatrix.scale(s, s, s);
			}
			break;

		case kTransform_Rotate:
			{
				m_testMatrix.rotateX(framework.time / 1.111f);
				m_testMatrix.rotateY(framework.time / 1.333f);
				m_testMatrix.rotateZ(framework.time / 1.777f);
			}
			break;

		case kTransform_Translate:
			{
				const float dx = std::sinf(framework.time / 1.111f);
				const float dy = std::sinf(framework.time / 1.333f);
				const float dz = std::sinf(framework.time / 1.777f);

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

	virtual void evaluateRaw(int x, int y, int z, float & value)
	{
		value = max(value, m_particleBuffer.m_value[x][y][z]);

	#if 0
		//if (z == SZ / 2)
			if (m_gameOfLife.sample(x, y, z))
				value = max(value, 1.f);
	#endif
	}

	virtual float evaluate(const Coord & c)
	{
		const float controlX = mouse.x / float(800.f);
		const float controlY = mouse.y / float(800.f);

		const float power = keyboard.isDown(SDLK_p) ? 1.f : s_fftBuckets[m_bucketIndex];

		bool doTestPostEffects = true;

		const Coord testCoord = m_testMatrix.apply(c);

		float d;

		switch (m_test)
		{
		case kTest_None:
			{
				doTestPostEffects = false;

			#if 0
				const float s = 10.f;
				const float t1 = (sinf(framework.time * s / 1.111f) + 1.f) * .25f;
				const float t2 = (sinf(framework.time * s / 2.333f) + 1.f) * .25f;
				d = computePlaneDistance(twistZ(twistY(c, t1), t2)) * 2.f;
				d = (1.f - pow(d, power));
			#elif 0
				const float r = .4f + .3f * sinf(framework.time);
				d = computeCircleHullDistance(repeat(c, 0.f, 0.f, r)) * 3.f;
				d = 1.f - pow(d, power);
			#elif 0
				const Coord c1 = c;
				const Coord c2 = c + Coord(5.2f, 1.3f, 0.f);
				const Coord c3(
					computePerlinNoise(c1, framework.time / 1.111f),
					computePerlinNoise(c2, framework.time / 1.333f),
					0.f);

				d = computePerlinNoise(c + c3, framework.time / 5.777f) * 3.f;
			#elif 0
				d = computePerlinNoise(c, framework.time) * 3.f;
			#else
				auto c1 = m_pointMatrix1.apply(c);
				auto c2 = m_pointMatrix2.apply(c);
				//const float d = 1.f - computePointDistance(x, y, z);
				//const float d = 1.f - pow(computeLineDistance(x, y, z), 4.f);
				const float d1 = computePlaneDistance(twistY(c1, .05f));
				const float d2 = computePlaneDistance(twistZ(c2, .03f));
				const float d3 = computePerlinNoise(testCoord, framework.time) * 2.f + 1.f;
				const float d4 = computeMinParticleDistance(m_particleMatrix.apply(c), m_particles, kNumParticles) * 4.f;
				d = min(d1, d2, d3, d4);
				//d = min(d4);
				d = (1.f - pow(d, power));
			#endif
			}
			break;

		case kTest_Calibrate:
			{
				doTestPostEffects = false;

				BiMatrix topMatrix;
				topMatrix.translate(0.f, -1.f, 0.f);
				topMatrix.rotateZ(M_PI/2.f);
				d = computePlaneDistance(topMatrix.apply(c));

				if (keyboard.isDown(SDLK_LSHIFT))
				{
					BiMatrix leftMatrix;
					leftMatrix.translate(-1.f, 0.f, 0.f);
					//d = csgUnion(d, computePlaneDistance(leftMatrix.apply(c)));
					//d = csgSoftUnion(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
					d = csgSoftIntersection(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
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
			d = computePerlinNoise(testCoord, framework.time) + 1.f;
			break;

		case kTest_Torus82:
			d = torus82(testCoord, 1.f, .25f);
			break;

		case kTest_Torus88:
			d = torus88(testCoord, 1.f, .25f);
			break;
		}

		if (d < 0.f)
			d = 0.f;

		if (doTestPostEffects)
			d = 1.f - pow(d, power);

		return d;
	}

	void debugDraw()
	{
		setColor(colorWhite);
		drawText(mouse.x, mouse.y + 25, 12, 0, 1, "power:%.2f", s_fftBuckets[m_bucketIndex]);
	}
};

static void evalCube(Cube & cube, Effect * effect)
{
	for (int x = 0; x < SX; ++x)
	{
		for (int y = 0; y < SY; ++y)
		{
			for (int z = 0; z < SZ; ++z)
			{
				Coord c;

				c.x = (x / (SX - 1.f) - .5f) * 2.f;
				c.y = (y / (SY - 1.f) - .5f) * 2.f;
				c.z = (z / (SZ - 1.f) - .5f) * 2.f;

				cube.m_value[x][y][z] = effect->evaluate(c);

				effect->evaluateRaw(x, y, z, cube.m_value[x][y][z]);
			}
		}
	}
}

static void drawCube(const Cube & cube)
{
	gxPushMatrix();
	{
		gxScalef(
			1.f / SX,
			1.f / SY,
			1.f / SZ);

		gxTranslatef(
			-(SX - 1) / 2.f,
			-(SY - 1) / 2.f,
			-(SZ - 1) / 2.f);

	#if 1
		gxBegin(GL_LINES);
		{
			gxColor4f(1.f, 1.f, 1.f, .25f);
			for (int x1 = 0; x1 <= 1; ++x1)
				for (int y1 = 0; y1 <= 1; ++y1)
					for (int z1 = 0; z1 <= 1; ++z1)
						for (int x2 = 0; x2 <= 1; ++x2)
							for (int y2 = 0; y2 <= 1; ++y2)
								for (int z2 = 0; z2 <= 1; ++z2)
									if (std::abs(x1-x2) + std::abs(y1-y2) + std::abs(z2-z1) == 1)
									{
										gxVertex3f(x1 * SX, y1 * SY, z1 * SZ);
										gxVertex3f(x2 * SX, y2 * SY, z2 * SZ);
									}
		}
		gxEnd();
	#endif

		glPointSize(2.f);
		setBlend(BLEND_ADD);

		gxBegin(GL_POINTS);
		{
			for (int x = 0; x < SX; ++x)
			{
				for (int y = 0; y < SY; ++y)
				{
					for (int z = 0; z < SZ; ++z)
					{
						const float value = cube.m_value[x][y][z];

						gxColor4f(value, value, value, 1.f);
						gxVertex3f(x, y, z);
					}
				}
			}
		}
		gxEnd();

		setBlend(BLEND_ALPHA);
	}
	gxPopMatrix();
}

class AudioStream_Capture : public AudioStream
{
public:
	AudioStream_Capture()
		: mSource(0)
	{
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		if (mSource)
		{
			const int result = mSource->Provide(numSamples, buffer);
			const int copySize = Calc::Min(result, kFFTBufferSize);
			const float scale = 2.f / 65536.f;

			for (int i = 0; i < copySize; ++i)
				s_fftInputBuffer[i] = buffer[i].channel[0] * scale;
			for (int i = copySize; i < kFFTBufferSize; ++i)
				s_fftInputBuffer[i] = 0.f;

#if 0
			for (int i = 0; i < kFFTBufferSize / kFFTSize; ++i)
			{
				s_fft.fft(s_fftInputBuffer + kFFTSize * i, s_fftReal, s_fftImaginary);
				const int kMoveSize = 4;
				memmove(s_fftReal + kMoveSize, s_fftReal, sizeof(s_fftReal) - sizeof(float) * kMoveSize);
				memmove(s_fftImaginary + kMoveSize, s_fftImaginary, sizeof(s_fftImaginary) - sizeof(float) * kMoveSize);
				for (int j = 0; j < kMoveSize; ++j)
				{
					s_fftReal[j] = 0.f;
					s_fftImaginary[j] = 0.f;
				}
				s_fft.ifft(s_fftInputBuffer + kFFTSize * i, s_fftReal, s_fftImaginary);

				for (int j = 0; j < copySize; ++j)
					buffer[j].channel[0] = buffer[j].channel[1] = s_fftInputBuffer[j + kFFTSize * i] / scale;
			}
#endif

			s_fftProvideTime = framework.time;

			return result;
		}
		else
		{
			return 0;
		}
	}

	AudioStream * mSource;
};

class AudioIn
{
	HWAVEIN m_waveIn;
	WAVEFORMATEX m_waveFormat;
	WAVEHDR m_waveHeader;
	short * m_buffer;

public:
	AudioIn()
		: m_waveIn(nullptr)
		, m_buffer(nullptr)
	{
		memset(&m_waveFormat, 0, sizeof(m_waveFormat));
		memset(&m_waveHeader, 0, sizeof(m_waveHeader));
	}

	~AudioIn()
	{
		shutdown();
	}

	bool init(int channelCount, int sampleRate, int bufferSampleCount)
	{
		Assert((bufferSampleCount % channelCount) == 0);
		bufferSampleCount /= channelCount;

		//const int numDevices = waveInGetNumDevs();

		MMRESULT mmResult = MMSYSERR_NOERROR;

		memset(&m_waveFormat, 0, sizeof(m_waveFormat));
		m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		m_waveFormat.nSamplesPerSec = sampleRate;
		m_waveFormat.nChannels = channelCount;
		m_waveFormat.wBitsPerSample = 16;
		m_waveFormat.nBlockAlign = m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8;
		m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
		m_waveFormat.cbSize = 0;

		mmResult = waveInOpen(&m_waveIn, WAVE_MAPPER, &m_waveFormat, 0, 0, CALLBACK_NULL);
		Assert(mmResult == MMSYSERR_NOERROR);

		if (mmResult == MMSYSERR_NOERROR)
		{
			m_buffer = new short[bufferSampleCount * channelCount];

			memset(&m_waveHeader, 0, sizeof(m_waveHeader));
			m_waveHeader.lpData = (LPSTR)m_buffer;
			m_waveHeader.dwBufferLength = m_waveFormat.nBlockAlign * bufferSampleCount;
			m_waveHeader.dwBytesRecorded = 0;
			m_waveHeader.dwUser = 0;
			m_waveHeader.dwFlags = 0;
			m_waveHeader.dwLoops = 0;
			mmResult = waveInPrepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			if (mmResult == MMSYSERR_NOERROR)
			{
				mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
				Assert(mmResult == MMSYSERR_NOERROR);

				if (mmResult == MMSYSERR_NOERROR)
				{
					mmResult = waveInStart(m_waveIn);
					Assert(mmResult == MMSYSERR_NOERROR);

					if (mmResult == MMSYSERR_NOERROR)
					{
						return true;
					}
				}
			}
		}

		shutdown();

		return false;
	}

	void shutdown()
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

		if (m_waveHeader.dwFlags & WHDR_PREPARED)
		{
			mmResult = waveInReset(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			mmResult = waveInUnprepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);
		}

		if (m_waveIn)
		{
			mmResult = waveInClose(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			m_waveIn = nullptr;
		}

		delete [] m_buffer;
		m_buffer = nullptr;
	}

	bool provide(short * buffer, int & sampleCount)
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

	#if 0
		printf("wave header flags: %08x (prepared:%d, inqueue:%d, done:%d)\n",
			m_waveHeader.dwFlags,
			(m_waveHeader.dwFlags & WHDR_PREPARED) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_INQUEUE) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_DONE) ? 1 : 0);
	#endif

		if (m_waveHeader.dwFlags & WHDR_DONE)
		{
			memcpy(buffer, m_buffer, m_waveHeader.dwBytesRecorded);
			sampleCount = m_waveHeader.dwBytesRecorded / sizeof(short);

			mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			return true;
		}
		else
		{
			return false;
		}
	}
};

int main(int argc, char * argv[])
{
#if USE_AUDIO_INPUT
	AudioIn audioIn;

	audioIn.init(2, 44100, 4096 * 2);
#endif

#if 0
	for (;;)
	{
		short buffer[1024];
		int sampleCount = 0;

		if (audioIn.provide(buffer, sampleCount))
		{
			int sum = 0;
			for (int i = 0; i < sampleCount; ++i)
				sum += Calc::Abs(buffer[i]);
			printf("wave volume: %d\n", sum / sampleCount);
		}

		Sleep(10);
	}
#endif

	if (!framework.init(0, 0, 800, 800))
	{
		showErrorMessage("Startup Error", "Failed to initialise framework.");
	}
	else
	{
		fftInit();

	#if !USE_AUDIO_INPUT
		AudioStream_Vorbis audioStreamOGG;
		audioStreamOGG.Open("music1.ogg", true);

		AudioStream_Capture audioStream;
		audioStream.mSource = &audioStreamOGG;

		AudioOutput_OpenAL audioOutput;
		audioOutput.Initialize(2, audioStreamOGG.mSampleRate, 1 << 14);
		audioOutput.Play();
	#endif

		Cube cube;

		MyEffect effect;

		float rotation[3] = { };

		bool stop = false;

		while (!stop)
		{
			framework.process();

			// process input

			if (mouse.isDown(BUTTON_LEFT))
			{
				rotation[0] += mouse.dy / 100.f;
				rotation[1] -= mouse.dx / 100.f;
			}

			// process audio

		#if USE_AUDIO_INPUT
			short buffer[4096 * 2];
			int sampleCount = 0;
			if (audioIn.provide(buffer, sampleCount))
			{
				const float scale = 2.f / 65536.f;
				for (int i = 0; i < 4096; ++i)
					s_fftInputBuffer[i] = buffer[i * 2] * scale;
				s_fftProvideTime = framework.time;
			}
		#else
			audioOutput.Update(&audioStream);
		#endif

			// generate FFT

			fftProcess();

			// evaluate cube

			effect.tick(framework.timeStep);

			evalCube(cube, &effect);

			// todo : send output towards hardware

			framework.beginDraw(0, 0, 0, 0);
			{
				// draw debug visualisation

				setFont("calibri.ttf");

				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				{
					Mat4x4 t;
					t.MakePerspectiveGL(M_PI/2.f, 1.f, .1f, 10.f);
					gxLoadMatrixf(t.m_v);
					glScalef(1.f, -1.f, 1.f);

					gxMatrixMode(GL_MODELVIEW);
					gxPushMatrix();
					{
						const float scale = 1.f;

						gxTranslatef(0.f, 0.f, 1.5f);
						gxRotatef(rotation[0]*180.f/M_PI, 1.f, 0.f, 0.f);
						gxRotatef(rotation[1]*180.f/M_PI, 0.f, 1.f, 0.f);
						gxScalef(scale, scale, scale);
						drawCube(cube);
					}
					gxPopMatrix();
				}
				gxMatrixMode(GL_PROJECTION);
				gxPopMatrix();

				gxMatrixMode(GL_MODELVIEW);

				gxPushMatrix();
				{
					gxTranslatef(0.f, 750.f, 0.f);
					gxScalef(800.f, -10.f, 1.f);
					//gxScalef(2400.f, -10.f, 1.f);
					setColor(colorWhite);
					gxBegin(GL_LINE_LOOP);
					{
						gxVertex2f(0.f, 400.f);

						if (mouse.isDown(BUTTON_RIGHT))
						{
							for (int i = 0; i < kFFTBucketCount; ++i)
							{
								const float p = s_fftBuckets[i];
								const float x = i / float(kFFTBucketCount - 1);
								const float y = p;
								gxVertex2f(x, y);
							}
						}
						else
						{
							for (int i = 0; i < kFFTComplexSize; ++i)
							{
								const float p = fftPowerValue(i);
								const float x = i / float(kFFTComplexSize - 1);
								const float y = p;
								gxVertex2f(x, y);
							}
						}

						gxVertex2f(800.f, 400.f);
					}
					gxEnd();
				}
				gxPopMatrix();

				effect.debugDraw();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
