#pragma once

namespace Game
{
	class Health_Segmented
	{
	public:
		void Initialize(float health, float segmentMultiplier, int subSegmentCount);
		
		void Damage(float damage);
		
		float GetSegmentDamage(float damage);
		
		int SubSegmentCount_get() const;
		void SubSegmentCount_set(int count);
		float Health_get() const;
		void Health_set(float health);
		
	private:
		float m_Health;
		float m_SegmentMultiplier;
		int m_SubSegmentCount;
	};
}
