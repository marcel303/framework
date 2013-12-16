#pragma once

#include "Types.h"

namespace GameUtil
{
	class Follower
	{
	public:
		Follower();
		
		void Setup(Follower* target, float minDistance, float maxDistance, Vec2F position);
		Vec2F Delta_get() const;
		void Solve();
		
		Follower* m_Target;
		float m_MinDistance;
		float m_MaxDistance;
		Vec2F m_Position;
	};
}
