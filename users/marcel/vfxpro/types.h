#pragma once

#include <list>

//

template <typename T>
bool isValidIndex(const T & value) { return value != ((T)-1); }

//

template <typename T>
struct Array
{
	T * data;

	Array()
		: data(nullptr)
	{
	}

	Array(int numElements)
		: data(nullptr)
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
		}

		if (numElements != 0)
		{
			data = new T[numElements];

			if (zeroMemory)
			{
				memset(data, 0, sizeof(T) * numElements);
			}
		}
	}

	T & operator[](int index)
	{
		assert(index >= 0);

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

class TweenFloat
{
	struct AnimValue
	{
		// todo : curve type (linear, pow, etc..)

		float value;
		float time;
	};

	float m_value;
	float m_from;
	float m_timeElapsed;

	std::list<AnimValue> m_animValues;

public:
	TweenFloat * m_prev;
	TweenFloat * m_next;

	TweenFloat();
	explicit TweenFloat(const float value);
	~TweenFloat();

	void to(const float value, const float time);
	void clear();
	bool isDone() const;
	void tick(const float dt);

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
