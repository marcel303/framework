#pragma once

#include "Ease.h"
#include <list>
#include <map>
#include <string>

class TweenFloat;

// todo : groups of tween values .to(...) :: set multiple targets at once, easier to chain and sync tweens

class TweenFloatModifier
{
public:
	virtual float applyModifier(TweenFloat * tweenFloat, const float value) = 0;
};

class TweenFloat
{
	struct AnimValue
	{
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

	explicit operator float() const
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
