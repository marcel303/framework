#pragma once

#include "Types.h"

namespace Game
{
	// analog controller / input class. converts finger touches to analog stick controls
	
	class AnalogController
	{
	public:
		AnalogController();
		void Initialize();
		
		void Setup(Vec2F pos, float radius, float deadZone);
		
		bool TouchBegin(int finger, Vec2F pos, bool calibrate);
		void TouchMove(int finger, Vec2F pos);		
		void TouchEnd(int finger);
		
		bool HitTest(const Vec2F& pos) const;
		
		void DisableTouch();
		
		Vec2F Direction_get() const;
		float Distance_get() const;
		float DistanceNorm_get() const;
		bool IsActive_get() const;
		
		inline Vec2F Position_get() const
		{
			return m_Pos;
		}
		
		inline float Radius_get() const
		{
			return m_Radius;
		}
		
		inline Vec2F TouchPosition_get() const
		{
			return m_CurrentPos;
		}
		
	private:
		float ActiveDistance_get(const Vec2F& pos) const;
		
		Vec2F m_Pos;
		float m_Radius;
		float m_DeadZone;
		
		int m_Finger;
		Vec2F m_StartPos;
		Vec2F m_CurrentPos;
		Vec2F m_Direction;
	};
}
