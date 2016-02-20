#pragma once

#include "Debugging.h"
#include <list>

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

enum EaseType
{
	kEaseType_Linear,
	kEaseType_PowIn,
	kEaseType_PowOut,
	kEaseType_PowInOut,
	kEaseType_SineIn,
	kEaseType_SineOut,
	kEaseType_SineInOut,
	kEaseType_BackIn,
	kEaseType_BackOut,
	kEaseType_BackInOut,
	kEaseType_BounceIn,
	kEaseType_BounceOut,
	kEaseType_BounceInOut,
	kEaseType_Count
};

static float evalEase(float t, EaseType type, float param1)
{
	Assert(t >= 0.f && t <= 1.f);

	float result = t;

	switch (type)
	{
	case kEaseType_Linear:
		break;

	case kEaseType_PowIn:
		result = powf(t, param1);
		break;

	case kEaseType_PowOut:
		result = 1.f - powf(1.f - t, param1);
		break;

	case kEaseType_PowInOut:
		if ((t *= 2.f) < 1.f)
			result = .5f * powf(t, param1);
		else
			result = 1.f - .5f * fabsf(powf(2.f - t, param1));
		break;

	case kEaseType_SineIn:
		result = 1.f - cosf(t * M_PI/2.f);
		break;

	case kEaseType_SineOut:
		result = sinf(t * M_PI/2.f);
		break;

	case kEaseType_SineInOut:
		result = -.5f * (cosf(M_PI * t) - 1.f);
		break;

	case kEaseType_BackIn:
		result = t * t * ((param1 + 1.f) * t - param1);
		break;

	case kEaseType_BackOut:
		result = (--t * t * ((param1 + 1.f) * t + param1) + 1.f);
		break;

	case kEaseType_BackInOut:
		if ((t *= 2.f) < 1.f)
			result = .5f * (t * t * ((param1 + 1.f) * t - param1));
		else
			result = .5 * ((t -= 2.f) * t * ((param1 + 1.f) * t + param1) + 2.f);
		break;

	case kEaseType_BounceIn:
		result = 1.f - evalEase(1.f - t, kEaseType_BounceOut, 0.f);
		break;

	case kEaseType_BounceOut:
		if (t < 1.f / 2.75f)
			result = (7.5625f * t * t);
		else if (t < 2.f / 2.75f)
			result = (7.5625f * (t -= 1.5f / 2.75f) * t + .75f);
		else if (t < 2.5f / 2.75f)
			result = (7.5625f * (t -= 2.25f / 2.75f) * t + .9375f);
		else
			result = (7.5625f * (t -= 2.625f / 2.75f) * t + .984375f);
		break;

	case kEaseType_BounceInOut:
		if (t < .5f)
			result = evalEase(t * 2.f, kEaseType_BounceIn, 0.f) * .5f;
		else
			result = evalEase(t * 2.f - 1.f, kEaseType_BounceOut, 0.f) * .5f + .5f;
		break;

		// todo : elastic in/out
	}

	return result;
}

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

	void operator=(const float value)
	{
		clear();

		m_value = value;
	}

	operator float() const
	{
		return m_value;
	}
};

void TickTweenFloats(const float dt);
