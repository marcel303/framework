#include "Calc.h"
#include "Debugging.h"
#include "SpecialController.h"

namespace Game
{
	SpecialController::SpecialController()
	{
		Initialize();
	}
	
	void SpecialController::Initialize()
	{
		m_Angle1 = 0.0f;
		m_Angle2 = 0.0f;
		m_Radius1 = 0.0f;
		m_Radius2 = 0.0f;
		m_Finger = -1;
		m_IsActive = false;
	}
	
	void SpecialController::Setup(Vec2F pos, float angle1, float angle2, float radius1, float radius2)
	{
		if (angle1 < 0.0f)
		{
			angle1 += Calc::m2PI;
			angle2 += Calc::m2PI;
		}
		
		Assert(angle1 <= angle2);
		Assert(angle1 >= 0.0f && angle1 <= Calc::m2PI);
		Assert(angle2 >= 0.0f && angle2 <= Calc::m2PI);
		Assert(radius1 <= radius2);
		
		//
		
		m_Pos = pos;
		m_Angle1 = angle1;
		m_Angle2 = angle2;
		m_Radius1 = radius1;
		m_Radius2 = radius2;
	}
	
	bool SpecialController::TouchBegin(int finger, Vec2F pos)
	{
		if (m_Finger != -1)
			return false;
		
		if (!HitTest(pos))
			return false;
		
		m_Finger = finger;
		m_IsActive = true;
		
		return true;
	}
	
	void SpecialController::TouchMove(int finger, Vec2F pos)
	{
	}
	
	void SpecialController::TouchEnd(int finger)
	{
		if (finger != m_Finger)
			return;
		
		m_Finger = -1;
		m_IsActive = false;
	}
	
	bool SpecialController::IsActive_get() const
	{
		return m_IsActive;
	}
	
	XBOOL SpecialController::HitTest(const Vec2F& pos) const
	{
		RectF rect(m_Pos + Vec2F(-24.0f, -69.0f), Vec2F(95.0f, 70.0f));
		
		return rect.IsInside(pos);
	}
	
	//
	
#pragma message("RoundButtonController: move to cpp/h")
	
	RoundButtonController::RoundButtonController()
	{
		Initialize();
	}
	
	void RoundButtonController::Initialize()
	{
		m_Finger = -1;
		m_IsActive = false;
	}
	
	void RoundButtonController::Setup(Vec2F pos, float radius)
	{
		m_Pos = pos;
		m_Radius = radius;
	}
	
	bool RoundButtonController::TouchBegin(int finger, Vec2F pos)
	{
		if (m_Finger != -1)
			return false;
		
		if (!HitTest(pos))
			return false;
		
		m_Finger = finger;
		m_IsActive = true;
		
		return true;
	}
	
	void RoundButtonController::TouchMove(int finger, Vec2F pos)
	{
	}
	
	void RoundButtonController::TouchEnd(int finger)
	{
		if (finger != m_Finger)
			return;
		
		m_Finger = -1;
		m_IsActive = false;
	}
	
	bool RoundButtonController::IsActive_get() const
	{
		return m_IsActive;
	}
	
	XBOOL RoundButtonController::HitTest(const Vec2F& pos) const
	{
		float distance = (pos - m_Pos).Length_get();
		
		return distance <= m_Radius;
	}
}
