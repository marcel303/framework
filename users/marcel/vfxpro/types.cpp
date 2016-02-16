#include "Calc.h"
#include "Debugging.h"
#include "types.h"

static void RegisterTweenFloat(TweenFloat * tween);
static void UnregisterTweenFloat(TweenFloat * tween);
static void TickTweenFloats(const float dt);

static float interp(const float from, const float to, const float time)
{
	return from * (1.f - time) + to * time;
}

TweenFloat::TweenFloat()
	: m_value(0.f)
	, m_from(0.f)
	, m_timeElapsed(0.f)
	, m_prev(nullptr)
	, m_next(nullptr)
{
	RegisterTweenFloat(this);
}

TweenFloat::TweenFloat(const float value)
	: m_value(value)
	, m_from(value)
	, m_timeElapsed(0.f)
	, m_prev(nullptr)
	, m_next(nullptr)
{
	RegisterTweenFloat(this);
}

TweenFloat::~TweenFloat()
{
	UnregisterTweenFloat(this);
}

void TweenFloat::to(const float value, const float time)
{
	AnimValue animValue;
	animValue.value = value;
	animValue.time = time;
	m_animValues.push_back(animValue);
}

void TweenFloat::clear()
{
	m_from = m_value;
	m_timeElapsed = 0.f;

	m_animValues.clear();
}

bool TweenFloat::isDone() const
{
	return m_animValues.empty();
}

void TweenFloat::tick(const float dt)
{
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

			m_value = interp(m_from, v.value, timeElapsed / v.time);

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
}

//

static TweenFloat * g_tweenFloats = nullptr;

static void RegisterTweenFloat(TweenFloat * tween)
{
	Assert(tween->m_prev == nullptr);
	Assert(tween->m_next == nullptr);

	if (g_tweenFloats)
	{
		g_tweenFloats->m_prev = tween;
		tween->m_next = g_tweenFloats;
	}

	g_tweenFloats = tween;
}

static void UnregisterTweenFloat(TweenFloat * tween)
{
	if (tween == g_tweenFloats)
	{
		g_tweenFloats = tween->m_next;
	}

	if (tween->m_prev)
		tween->m_prev->m_next = tween->m_next;
	if (tween->m_next)
		tween->m_next->m_prev = tween->m_prev;

	tween->m_prev = nullptr;
	tween->m_next = nullptr;
}

void TickTweenFloats(const float dt)
{
	for (TweenFloat * tween = g_tweenFloats; tween != nullptr; tween = tween->m_next)
	{
		tween->tick(dt);
	}
}
