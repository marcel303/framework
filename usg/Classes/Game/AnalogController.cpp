#include "AnalogController.h"

namespace Game
{
	AnalogController::AnalogController()
	{
		Initialize();
	}
	
	void AnalogController::Initialize()
	{
		m_Radius = 1.0f;
		m_Finger = -1;
		m_Direction.Set(1.0f, 0.0f);
	}
	
	void AnalogController::Setup(Vec2F pos, float radius, float deadZone)
	{
		m_Pos = pos;
		m_Radius = radius;
		m_DeadZone = deadZone;
	}
	
	bool AnalogController::TouchBegin(int finger, Vec2F pos, bool calibrate)
	{
		// is inside circle?
		
		if (!HitTest(pos))
			return false;
		
		m_Finger = finger;
		m_StartPos = calibrate ? pos : m_Pos;
		m_CurrentPos = pos;
		m_Direction = m_StartPos.UnitDelta(m_CurrentPos);
		
		return true;
	}
	
	void AnalogController::TouchMove(int finger, Vec2F pos)
	{
		if (finger != m_Finger)
			return;
		
		m_CurrentPos = pos;
		m_Direction = m_StartPos.UnitDelta(m_CurrentPos);
	}
	
	void AnalogController::TouchEnd(int finger)
	{
		if (finger != m_Finger)
			return;
		
		m_Finger = -1;
	}
	
	bool AnalogController::HitTest(const Vec2F& pos) const
	{
		const float distance = m_Pos.Distance(pos) - m_Radius;
		
		return distance <= 0.0f;
	}
	
	void AnalogController::DisableTouch()
	{
		m_Finger = -1;
	}
	
	Vec2F AnalogController::Direction_get() const
	{
		return m_Direction;
	}
	
	float AnalogController::Distance_get() const
	{
		if (m_Finger == -1)
			return 0.0f;
		
		return ActiveDistance_get(m_CurrentPos);
	}
	
	float AnalogController::DistanceNorm_get() const
	{
		if (m_Finger == -1)
			return 0.0f;
		
		return ActiveDistance_get(m_CurrentPos) / (m_Radius - m_DeadZone);
	}
	
	bool AnalogController::IsActive_get() const
	{
		return m_Finger != -1 && Distance_get() >= 0.0f;
	}
	
	float AnalogController::ActiveDistance_get(const Vec2F& pos) const
	{
		float distance = m_StartPos.Distance(pos) - m_DeadZone;
		
		if (distance < 0.0f)
			distance = 0.0f;
		
		return distance;
	}
};
