#include "AngleController.h"

AngleController::AngleController()
{
	Setup(0.0f, 0.0f, 0.0f);
}

AngleController::AngleController(float angle, float targetAngle, float speed)
{
	Setup(angle, targetAngle, speed);
}

void AngleController::Setup(float angle, float targetAngle, float speed)
{
	m_Angle = angle;
	m_TargetAngle = targetAngle;
	m_Speed = speed;
	m_HasReachedTarget = true;
}

void AngleController::Update(float dt)
{
	const float maxChange = m_Speed * dt;
	
	const float angleDiff = GetShortestArc(m_Angle, m_TargetAngle);
	
	if (fabsf(angleDiff) < maxChange)
	{
		m_Angle += angleDiff;

		m_HasReachedTarget = true;
	}
	else
	{
		if (angleDiff > 0.0f)
			m_Angle += maxChange;
		else
			m_Angle -= maxChange;

		m_HasReachedTarget = false;
	}
}

float AngleController::GetShortestArc(float angle, float targetAngle)
{
	// normalize angles
	
	targetAngle = fmodf(targetAngle, Calc::m2PI);
	angle = fmodf(angle, Calc::m2PI);
	
	// calculate difference
	
	float angleDiff = targetAngle - angle;
	
#if 1
	angleDiff = fmodf(angleDiff, Calc::m2PI);
	
	//if (angleDiff > Calc::mPI)
	//	angleDiff -= Calc::m2PI;
#endif
	
	if (angleDiff > Calc::mPI)
		angleDiff -= Calc::m2PI;
	if (angleDiff < -Calc::mPI)
		angleDiff += Calc::m2PI;
	
	return angleDiff;
}
