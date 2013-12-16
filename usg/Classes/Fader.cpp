#include "Calc.h"
#include "Fader.h"

Fader::Fader()
{
	Initialize();
}

void Fader::Initialize()
{
	m_DownSpeed = 0.0f;
	m_UpSpeed = 0.0f;
	m_Reference = 0.0f;
	m_Value = 0.0f;
}

void Fader::Setup(float downSpeed, float upSpeed, float reference, float value)
{
	m_DownSpeed = downSpeed;
	m_UpSpeed = upSpeed;
	m_Reference = reference;
	m_Value = value;
}

void Fader::Update(float dt)
{
	float delta = m_Reference - m_Value;
	float shift = 0.0f;
	
	if (delta > 0.0f)
		shift = +m_UpSpeed * dt;
	else
		shift = -m_DownSpeed * dt;
	
	if (fabsf(shift) > fabsf(delta))
		shift = delta;
	
	m_Value += shift;
}

void Fader::Reference_set(float reference)
{
	m_Reference = reference;
}

float Fader::Value_get() const
{
	return m_Value;
}
