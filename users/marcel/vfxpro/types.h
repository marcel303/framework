#pragma once

#define TWEENFLOAT_ALLOW_IMPLICIT_CONVERSION 1

#include "Debugging.h"
#include "Ease.h"
#include "framework.h"
#include "TweenFloat.h"
#include <list>
#include <map>
#include <string>

//

template <typename T>
bool isValidIndex(const T & value) { return value != ((T)-1); }

//

template <typename T>
struct Array
{
	T * data;
	int size;

	Array()
		: data(nullptr)
		, size(0)
	{
	}

	Array(int numElements)
		: data(nullptr)
		, size(0)
	{
		resize(numElements);
	}

	~Array()
	{
		resize(0, false);
	}

	void resize(int numElements, bool zeroMemory)
	{
		if (data != nullptr)
		{
			delete [] data;
			data = nullptr;
			size = 0;
		}

		if (numElements != 0)
		{
			data = new T[numElements];
			size = numElements;

			if (zeroMemory)
			{
				memset(data, 0, sizeof(T) * numElements);
			}
		}
	}

	int getSize() const
	{
		return size;
	}

	void setSize(const int size)
	{
		resize(size, false);
	}

	T & operator[](int index)
	{
		Assert(index >= 0);

		return data[index];
	}
};

//

struct EffectTimer
{
	float time;

	EffectTimer()
		: time(0.f)
	{
	}

	void tick(const float dt)
	{
		time += dt;
	}

	bool consume(const float amount)
	{
		if (time < amount)
			return false;
		else
		{
			time -= amount;
			return true;
		}
	}
};

//

struct ParticleSystem
{
	int numParticles;

	Array<int> freeList;
	int numFree;

	Array<bool> alive;
	Array<bool> autoKill;

	Array<float> x;
	Array<float> y;
	Array<float> vx;
	Array<float> vy;
	Array<float> sx;
	Array<float> sy;
	Array<float> angle;
	Array<float> vangle;
	Array<float> life;
	Array<float> lifeRcp;
	Array<bool> hasLife;

	ParticleSystem(const int numElements);
	~ParticleSystem();

	void resize(const int numElements);
	bool alloc(const bool _autoKill, float _life, int & id);
	void free(const int id);

	void tick(const float dt);
	void draw(const float alpha);
};

//

enum BlendMode
{
	kBlendMode_Add,
	kBlendMode_Subtract,
	kBlendMode_Alpha,
	kBlendMode_PremultipliedAlpha,
	kBlendMode_Opaque,
	kBlendMode_AlphaTest,
	kBlendMode_Multiply
};

BlendMode parseBlendMode(const std::string & blend);

//

struct ColorCurve
{
	static const int kMaxKeys = 10;

	struct Key
	{
		float t;
		Color color;

		Key();

		bool operator<(const Key & other) const;
		bool operator==(const Key & other) const;
		bool operator!=(const Key & other) const;
	};

	Key keys[kMaxKeys];
	int numKeys;

	ColorCurve();

	bool allocKey(Key *& key);
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);
	void setLinear(const Color & v1, const Color & v2);
	void setLinearAlpha(float v1, float v2);
	void sample(const float t, Color & result) const;
};

//

struct LeapState
{
	LeapState()
	{
		memset(this, 0, sizeof(*this));
	}

	bool hasPalm;
	float palmX;
	float palmY;
	float palmZ;
};
