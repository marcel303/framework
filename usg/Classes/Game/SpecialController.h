#pragma once

#include "Types.h"

namespace Game
{
	class SpecialController
	{
	public:
		SpecialController();
		void Initialize();
		
		void Setup(Vec2F pos, float angle1, float angle2, float radius1, float radius2);
		
		bool TouchBegin(int finger, Vec2F pos);
		void TouchMove(int finger, Vec2F pos);		
		void TouchEnd(int finger);
		
		bool IsActive_get() const;
		
	private:
		Vec2F m_Pos;
		float m_Angle1;
		float m_Angle2;
		float m_Radius1;
		float m_Radius2;
		int m_Finger;
		bool m_IsActive;
		
		XBOOL HitTest(const Vec2F& pos) const;
	};
	
	class RoundButtonController
	{
	public:
		RoundButtonController();
		void Initialize();
		
		void Setup(Vec2F pos, float radius);
		
		bool TouchBegin(int finger, Vec2F pos);
		void TouchMove(int finger, Vec2F pos);		
		void TouchEnd(int finger);
		
		bool IsActive_get() const;
		
	//private:
	public:
		Vec2F m_Pos;
		float m_Radius;
	private:
		int m_Finger;
		bool m_IsActive;
		
		XBOOL HitTest(const Vec2F& pos) const;
	};
}
