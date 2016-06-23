#include "Calc.h"
#include "Debugging.h"
#include "framework.h"
#include "types.h"

static float interp(const float from, const float to, const float time)
{
	return from * (1.f - time) + to * time;
}

TweenFloat::TweenFloat()
	: m_value(0.f)
	, m_valueWithModifier(0.f)
	, m_modifier(nullptr)
	, m_from(0.f)
	, m_timeElapsed(0.f)
	, m_timeWait(0.f)
	, m_prev(nullptr)
	, m_next(nullptr)
{
}

TweenFloat::TweenFloat(const float value)
	: m_value(value)
	, m_valueWithModifier(0.f)
	, m_modifier(nullptr)
	, m_from(value)
	, m_timeElapsed(0.f)
	, m_timeWait(0.f)
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

		if (m_timeWait < 0.f)
		{
			dt -= m_timeWait;
			m_timeWait = 0.f;
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

//

ParticleSystem::ParticleSystem(const int numElements)
	: numParticles(0)
	, numFree(0)
{
	resize(numElements);
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::resize(const int numElements)
{
	numParticles = numElements;

	freeList.resize(numElements, true);
	for (int i = 0; i < numElements; ++i)
		freeList[i] = i;
	numFree = numElements;

	alive.resize(numElements, true);
	autoKill.resize(numElements, true);

	x.resize(numElements, true);
	y.resize(numElements, true);
	vx.resize(numElements, true);
	vy.resize(numElements, true);
	sx.resize(numElements, true);
	sy.resize(numElements, true);
	angle.resize(numElements, true);
	vangle.resize(numElements, true);
	life.resize(numElements, true);
	lifeRcp.resize(numElements, true);
	hasLife.resize(numElements, true);
}

bool ParticleSystem::alloc(const bool _autoKill, float _life, int & id)
{
	if (numFree == 0)
	{
		id = -1;

		return false;
	}
	else
	{
		id = freeList[--numFree];

		fassert(!alive[id]);

		alive[id] = true;
		autoKill[id] = _autoKill;

		x[id] = 0.f;
		y[id] = 0.f;
		vx[id] = 0.f;
		vy[id] = 0.f;
		sx[id] = 1.f;
		sy[id] = 1.f;

		angle[id] = 0.f;
		vangle[id] = 0.f;

		if (_life == 0.f)
		{
			life[id] = 1.f;
			lifeRcp[id] = 1.f;
			hasLife[id] = false;
		}
		else
		{
			life[id] = _life;
			lifeRcp[id] = 1.f / _life;
			hasLife[id] = true;
		}

		return true;
	}
}

void ParticleSystem::free(const int id)
{
	if (isValidIndex(id))
	{
		fassert(alive[id]);

		alive[id] = false;

		freeList[numFree++] = id;
	}
}

void ParticleSystem::tick(const float dt)
{
	for (int i = 0; i < numParticles; ++i)
	{
		if (alive[i])
		{
			if (hasLife[i])
			{
				life[i] = life[i] - dt;
			}

			if (life[i] < 0.f)
			{
				life[i] = 0.f;

				if (autoKill[i])
				{
					free(i);

					continue;
				}
			}

			x[i] += vx[i] * dt;
			y[i] += vy[i] * dt;

			angle[i] += vangle[i] * dt;
		}
	}
}

void ParticleSystem::draw(const float alpha)
{
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numParticles; ++i)
		{
			if (alive[i])
			{
				const float value = life[i] * lifeRcp[i];

				gxColor4f(1.f, 1.f, 1.f, alpha * value);

				const float s = std::sinf(angle[i]);
				const float c = std::cosf(angle[i]);

				const float sx_2 = sx[i] * .5f;
				const float sy_2 = sy[i] * .5f;

				const float s_sx_2 = s * sx_2;
				const float s_sy_2 = s * sy_2;
				const float c_sx_2 = c * sx_2;
				const float c_sy_2 = c * sy_2;

				gxTexCoord2f(0.f, 1.f); gxVertex2f(x[i] + (-c_sx_2 - s_sy_2), y[i] + (+s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x[i] + (+c_sx_2 - s_sy_2), y[i] + (-s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x[i] + (+c_sx_2 + s_sy_2), y[i] + (-s_sx_2 + c_sy_2));
				gxTexCoord2f(0.f, 0.f); gxVertex2f(x[i] + (-c_sx_2 + s_sy_2), y[i] + (+s_sx_2 + c_sy_2));
			}
		}
	}
	gxEnd();
}

//

BlendMode parseBlendMode(const std::string & blend)
{
	if (blend == "add")
		return kBlendMode_Add;
	else if (blend == "subtract")
		return kBlendMode_Subtract;
	else if (blend == "alpha")
		return kBlendMode_Alpha;
	else if (blend == "premultiplied_alpha" || blend == "pre_alpha")
		return kBlendMode_PremultipliedAlpha;
	else if (blend == "opaque")
		return kBlendMode_Opaque;
	else if (blend == "alpha_test")
		return kBlendMode_AlphaTest;
	else
	{
		logWarning("unknown blend type: %s", blend.c_str());
		return kBlendMode_Add;
	}
}
