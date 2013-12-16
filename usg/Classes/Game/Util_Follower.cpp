#include "Util_Follower.h"

namespace GameUtil
{
	Follower::Follower()
	{
		m_Target = 0;
		m_MinDistance = 0.0f;
		m_MaxDistance = 0.0f;
		m_Position.SetZero();
	}
	
	void Follower::Setup(Follower* target, float minDistance, float maxDistance, Vec2F position)
	{
		m_Target = target;
		m_MinDistance = minDistance;
		m_MaxDistance = maxDistance;
		m_Position = position;
	}
	
	Vec2F Follower::Delta_get() const
	{
		if (!m_Target)
			return Vec2F(0.0f, 0.0f);
		else
			return m_Target->m_Position - m_Position;
	}
	
	void Follower::Solve()
	{
		if (!m_Target)
			return;
		
		Vec2F delta = m_Target->m_Position - m_Position;

		float length = delta.Length_get();

		delta.Normalize();

#if 1
		float offset1 = length - m_MaxDistance;
		float offset2 = length - m_MinDistance;
#else
		float offset1 = length - m_TargetDistance;
		float offset2 = length - m_TargetDistance;
#endif

		// Pivot follows if moved away too far.

		if (offset1 > 0.0f)
		{
			m_Position += delta * offset1;
		}

		// Push pivot away if got too near.

		if (offset2 < 0.0f)
		{
			m_Position += delta * offset2;
		}
	}
};
