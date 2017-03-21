#include "Calc.h"
#include "Debugging.h"
#include "TweenFloat.h"

static float interp(const float from, const float to, const float time)
{
	return from * (1.f - time) + to * time;
}

TweenFloat::TweenFloat()
	: m_value(0.f)
	, m_from(0.f)
	, m_timeElapsed(0.f)
	, m_timeWait(0.f)
	, m_modifier(nullptr)
	, m_valueWithModifier(0.f)
	, m_animValues()
	, m_prev(nullptr)
	, m_next(nullptr)
{
}

TweenFloat::TweenFloat(const float value)
	: m_value(value)
	, m_from(value)
	, m_timeElapsed(0.f)
	, m_timeWait(0.f)
	, m_modifier(nullptr)
	, m_valueWithModifier(value)
	, m_animValues()
	, m_prev(nullptr)
	, m_next(nullptr)
{
}

TweenFloat::~TweenFloat()
{
}

void TweenFloat::to(const float value, const float time, const EaseType easeType, const float easeParam)
{
	AnimValue animValue;
	animValue.value = value;
	animValue.time = time;
	animValue.easeType = easeType;
	animValue.easeParam = easeParam;
	m_animValues.push_back(animValue);
}

void TweenFloat::clear()
{
	m_from = m_value;
	m_timeElapsed = 0.f;

	m_animValues.clear();
}

void TweenFloat::wait(const float time)
{
	m_timeWait = time;
}

bool TweenFloat::isDone() const
{
	return m_animValues.empty();
}

void TweenFloat::tick(const float _dt)
{
	float dt = _dt;

	if (m_timeWait > 0.f)
	{
		m_timeWait -= dt;

		if (m_timeWait <= 0.f)
		{
			dt -= m_timeWait;
			m_timeWait = 0.f;
		}
		else
		{
			dt = 0.f;
		}
	}

	if (m_animValues.empty())
	{
		Assert(m_timeElapsed == 0.f);
	}
	else
	{
		m_timeElapsed += dt;

		for (;;)
		{
			AnimValue & v = m_animValues.front();

			const float timeElapsed = Calc::Min(m_timeElapsed, v.time);

			if (v.time > 0.f)
				m_value = interp(m_from, v.value, EvalEase(timeElapsed / v.time, v.easeType, v.easeParam));
			else
				m_value = v.value;

			if (m_timeElapsed >= v.time)
			{
				m_from = v.value;
				m_value = v.value;
				m_timeElapsed -= v.time;

				m_animValues.pop_front();

				if (m_animValues.empty())
				{
					m_timeElapsed = 0.f;

					break;
				}
			}
			else
			{
				break;
			}
		}
	}

	applyModifiers();
}

float TweenFloat::getFinalValue() const
{
	if (m_animValues.empty())
		return m_value;
	else
		return m_animValues.back().value;
}

void TweenFloat::addModifier(TweenFloatModifier * modifier)
{
	Assert(m_modifier == nullptr);

	m_modifier = modifier;
}

void TweenFloat::applyModifiers()
{
	m_valueWithModifier = m_modifier ? m_modifier->applyModifier(this, m_value) : m_value;
}
