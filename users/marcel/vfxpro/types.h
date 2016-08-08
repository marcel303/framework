#pragma once

#include "Debugging.h"
#include "Ease.h"
#include "framework.h"
#include <list>
#include <map>
#include <string>

//

class TweenFloat;

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

// todo : groups of tween values .to(...) :: set multiple targets at once, easier to chain and sync tweens

class TweenFloatModifier
{
public:
	virtual float applyModifier(TweenFloat * tweenFloat, float value) = 0;
};

class TweenFloat
{
	struct AnimValue
	{
		// todo : curve type (linear, pow, etc..)

		float value;
		float time;

		EaseType easeType;
		float easeParam;
	};

	float m_value;
	float m_from;
	float m_timeElapsed;

	float m_timeWait;

	TweenFloatModifier * m_modifier;
	float m_valueWithModifier;

	std::list<AnimValue> m_animValues;

public:
	TweenFloat * m_prev;
	TweenFloat * m_next;

	TweenFloat();
	explicit TweenFloat(const float value);
	~TweenFloat();

	void to(const float value, const float time, const EaseType easeType, const float easeParam);
	void clear();
	void wait(const float time);
	bool isDone() const;
	bool isActive() const { return !isDone(); }
	void tick(const float dt);

	float getFinalValue() const;

	void addModifier(TweenFloatModifier * modifier);
	void applyModifiers();

	void operator=(const float value)
	{
		clear();

		m_value = value;
		m_from = value;

		applyModifiers();
	}

	operator float() const
	{
		return m_valueWithModifier;
	}
};

struct TweenFloatCollection
{
	std::map<std::string, TweenFloat*> m_tweenVars;

	void addVar(const char * name, TweenFloat & var)
	{
		m_tweenVars[name] = &var;
	}

	virtual TweenFloat * getVar(const char * name)
	{
		auto i = m_tweenVars.find(name);

		if (i == m_tweenVars.end())
			return nullptr;
		else
			return i->second;
	}

	void tweenTo(const char * name, const float value, const float time, const EaseType easeType, const float easeParam)
	{
		auto i = m_tweenVars.find(name);

		if (i != m_tweenVars.end())
		{
			TweenFloat * v = i->second;

			v->to(value, time, easeType, easeParam);
		}
	}

	void tick(const float dt)
	{
		for (auto i = m_tweenVars.begin(); i != m_tweenVars.end(); ++i)
		{
			TweenFloat * var = i->second;

			var->tick(dt);
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
