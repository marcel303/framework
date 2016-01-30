#pragma once

#include "config.h"
#include "Mat4x4.h"
#include "simplexnoise.h"
#include <cmath>
#include <limits>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

#if VIDEO_RECORDING_MODE
	#define SX 30
	#define SY 30
	#define SZ 30
#else
	#define SX 12
	#define SY 21
	#define SZ 24
#endif

#define ArraySize(x) (sizeof(x) / sizeof(x[0]))

// forward declarations

struct Coord;
struct Cube;
struct CubeBuffer;
struct Particle;
class Transform;

	// shape distance functions
static float computeMinParticleDistance(const Coord & c, const Particle * particles, const int numParticles);
static float computeCircleDistance(const Coord & c);
static float computeSphereDistance(const Coord & c);
static float computeCubeDistance(const Coord & c);
static float computeToroidDistance(const Coord & c, const float thickness, const float outer);
static float computeCircleHullDistance(const Coord & c);
static float computePointDistance(const Coord & c);
static float computeLineDistance(const Coord & c);
static float computeLineSegmentDistance(const Coord & c, const float x1, const float x2, const float r);
static float computePerlinNoise(const Coord & c, const float w);
static float computeTorus82(const Coord & c, const float a, const float b);
static float computeTorus88(const Coord & c, const float a, const float b);

	// coordinate space deformation functions
static Coord repeat(const Coord & coord, const float repeatX, const float repeatY, const float repeatZ);
static Coord twistX(const Coord & c, const float scale);
static Coord twistY(const Coord & c, const float scale);
static Coord twistZ(const Coord & c, const float scale);

	// CSG operations
static float csgUnion(const float d1, const float d2);
static float csgSubtraction(const float d1, const float d2);
static float csgIntersection(const float d1, const float d2);
static float csgSoftUnion(const float a, const float b, const float k);
static float csgSoftIntersection(const float a, const float b, const float k);

// math functions

static float min(const int x, const int y) { return x < y ? x : y; }
static float min(const float x) { return x; }
static float min(const float x, const float y) { return x < y ? x : y; }
static float min(const float x, const float y, const float z) { return min(x, min(y, z)); }
static float min(const float x, const float y, const float z, const float w) { return min(x, min(y, min(z, w))); }
static float min(const float x, const float y, const float z, const float w, const float s) { return min(x, min(y, min(z, min(w, s)))); }

static float max(const int x, const int y) { return x > y ? x : y; }
static float max(const float x, const float y) { return x > y ? x : y; }
static float max(const float x, const float y, const float z) { return max(x, max(y, z)); }

static float clamp(const float x, const float min, const float max) { return x < min ? min : x > max ? max : x; }
static float lerp(const float a, const float b, const float t) { return a * (1.f - t) + b * t; }

static float rand(const float min, const float max) { return min + (max - min) * (rand() % 1024) / 1023.f; }

static float length1(const float x) { return std::sqrtf(x * x); }
static float length2(const float x, const float y) { return std::sqrtf(x * x + y * y); }
static float length3(const float x, const float y, const float z) { return std::sqrtf(x * x + y * y + z * z); }

static float dot3(
	const float x1, const float y1, const float z1,
	const float x2, const float y2, const float z2)
{
	return
		x1 * x2 +
		y1 * y2 +
		z1 * z2;
}

//

struct Coord
{
	float x, y, z;

	Coord()
	{
	}

	Coord(const float x, const float y, const float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Coord operator+(const Coord & other) const
	{
		return Coord(x + other.x, y + other.y, z + other.z);
	}
};

//

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

	void fadeLinear(const float speed, const float dt)
	{
		for (int x = 0; x < SX; ++x)
		{
			for (int y = 0; y < SY; ++y)
			{
				for (int z = 0; z < SZ; ++z)
				{
					m_value[x][y][z] = max(0.f, m_value[x][y][z] - speed * dt);
				}
			}
		}
	}

	void fadeIncremental(const float lossPerSecond, const float dt)
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

//

struct Cube : CubeBuffer
{
	Cube()
		: CubeBuffer()
	{
	}
};

//

class Transform
{
	Mat4x4 m_matrix;
	Mat4x4 m_matrixInverse;
	bool m_dirty;

public:
	Transform()
		: m_dirty(false)
	{
		m_matrix.MakeIdentity();
		m_matrixInverse.MakeIdentity();
	}

	void reset()
	{
		matrix().MakeIdentity();
	}

	void scale(const float x, const float y, const float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeScaling(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void translate(const float x, const float y, const float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeTranslation(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void rotateX(const float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationX(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateY(const float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationY(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateZ(const float r)
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

//

struct Particle
{
	bool active;
	float x, y, z;

	Particle()
	{
		memset(this, 0, sizeof(*this));
	}
};

// shape distance functions

static float computeMinParticleDistance(const Coord & c, const Particle * particles, const int numParticles)
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

static float computeCubeDistance(const Coord & c, const float sx, const float sy, const float sz)
{
	// actually a round box

	return length3(
		max(0.f, abs(c.x) - sx),
		max(0.f, abs(c.y) - sy),
		max(0.f, abs(c.z) - sz));
}

static float computeToroidDistance(const Coord & c, const float thickness, const float outer)
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

static float computeLineSegmentDistance(const Coord & c, const float x1, const float x2, const float r)
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

static float computePerlinNoise(const Coord & c, const float w)
{
	const float value = octave_noise_4d(4.f, .5f, .5f, c.x, c.y, c.z, w);

	return value;
}

static float computeTorus82(const Coord & c, const float a, const float b)
{
	const float dx = (c.x * c.x + c.z * c.z) - a;
	const float dy = c.y;
	const float d = dx * dx * dx * dx + dy * dy * dy * dy;
	return d - b;
}

static float computeTorus88(const Coord & c, const float a, const float b)
{
	const float dx = (c.x * c.x * c.x * c.x + c.z * c.z * c.z * c.z) - a;
	const float dy = c.y;
	const float d = dx * dx * dx * dx + dy * dy * dy * dy;
	return d - b;
}

// coordinate space deformation functions

static Coord repeat(const Coord & coord, const float repeatX, const float repeatY, const float repeatZ)
{
	return Coord(
		repeatX == 0.f ? coord.x : fmodf(coord.x, repeatX),
		repeatY == 0.f ? coord.y : fmodf(coord.y, repeatY),
		repeatZ == 0.f ? coord.z : fmodf(coord.z, repeatZ));
}

static Coord twistX(const Coord & c, const float scale)
{
	Coord result;
	Mat4x4 m;
	m.MakeRotationX(-2.f * (float)M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
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
	m.MakeRotationY(-2.f * (float)M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
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
	m.MakeRotationZ(-2.f * (float)M_PI * c.y * scale); // -2.f because we want to create an inverse rotation matrix
	const Vec3 t = m * Vec3(c.x, c.y, c.z);
	result.x = t[0];
	result.y = t[1];
	result.z = t[2];
	return result;
}

// CSG operations

static float csgUnion(const float d1, const float d2)
{
	return min(d1, d2);
}

static float csgSubtraction(const float d1, const float d2)
{
	return max(-d1, d2);
}

static float csgIntersection(const float d1, const float d2)
{
	return max(d1, d2);
}

static float csgSoftUnion(const float a, const float b, const float k)
{
	const float h = clamp(.5f + .5f * (b - a) / k, 0.f, 1.f);
	return lerp(b, a, h) - k * h * (1.f - h);
}

static float csgSoftIntersection(const float a, const float b, const float k)
{
	const float h = clamp(.5f + .5f * (b - a) / k, 0.f, 1.f);
	return lerp(a, b, h) + k * h * (1.f - h);
}

//

class EffectCtx
{
public:
	virtual float fftBucketValue(int index) const = 0;
	virtual int fftBucketCount() const = 0;

	virtual bool keyIsDown(const SDLKey key) const = 0;
	virtual bool keyWentDown(const SDLKey key) const = 0;
	virtual bool keyWentUp(const SDLKey key) const = 0;

	virtual int mouseX() const = 0;
	virtual int mouseY() const = 0;

	virtual void setColor(const int r, const int g, const int b, const int a = 255) = 0;
	virtual void drawText(const float x, const float y, const int size, const float alignX, const float alignY, const char * format, ...) = 0;
	virtual void drawLine(const float x1, const float y1, const float x2, const float y2) = 0;
};

class EffectCtxImpl : public EffectCtx
{
public:
	virtual float fftBucketValue(int index) const;
	virtual int fftBucketCount() const;

	virtual bool keyIsDown(const SDLKey key) const;
	virtual bool keyWentDown(const SDLKey key) const;
	virtual bool keyWentUp(const SDLKey key) const;

	virtual int mouseX() const;
	virtual int mouseY() const;

	virtual void setColor(const int r, const int g, const int b, const int a);
	virtual void drawText(const float x, const float y, const int size, const float alignX, const float alignY, const char * format, ...);
	virtual void drawLine(const float x1, const float y1, const float x2, const float y2);
};

class Effect
{
public:
	virtual void tick(const float dt) = 0;
	virtual void evaluateRaw(int x, int y, int z, float & value) = 0;
	virtual float evaluate(const Coord & c) = 0;
	virtual void debugDraw() = 0;
};
