#pragma once

#include "Types.h"

namespace Game
{
	enum ForceFieldType
	{
		ForceFieldType_Twirly, // snake motion
		ForceFieldType_Concentric,
	};
	
	class ForceField
	{
	public:
		ForceField()
		{
		}
		
		void Setup_Twirly(Vec2F center, float baseAngle, int waveCount)
		{
			m_Type = ForceFieldType_Twirly;
			m_Twirly_Center = center;
			m_Twirly_BaseAngle = baseAngle;
			m_Twirly_WaveCount = waveCount;
		}
		
		Vec2F Evaluate(Vec2F position)
		{
			return Vec2F();
		}
		
	private:
		ForceFieldType m_Type;
		
		Vec2F m_Twirly_Center;
		float m_Twirly_BaseAngle;
		int m_Twirly_WaveCount;
	};
};
