#pragma once

#include "Calc.h"

class AngleController
{
public:
	AngleController();
	AngleController(float angle, float targetAngle, float speed);
	
	void Setup(float angle, float targetAngle, float speed);
	
	void Update(float dt);
	
	inline float Angle_get() const
	{
		return m_Angle;
	}
	
	inline void Angle_set(float angle)
	{
		m_Angle = angle;
	}
	
	inline float TargetAngle_get() const
	{
		return m_TargetAngle;
	}
	
	inline void TargetAngle_set(float angle)
	{
		m_TargetAngle = angle;
	}
	
	inline void Speed_set(float speed)
	{
		m_Speed = speed;
	}
	
	inline bool HasReachedTarget_get() const
	{
		return m_HasReachedTarget;
	}

	static float GetShortestArc(float angle, float targetAngle);
	
private:
	float m_Angle;
	float m_TargetAngle;
	float m_Speed;
	bool m_HasReachedTarget;
};
