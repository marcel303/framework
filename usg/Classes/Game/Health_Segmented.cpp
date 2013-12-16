#include "Calc.h"
#include "Health_Segmented.h"

namespace Game
{
	void Health_Segmented::Initialize(float health, float segmentMultiplier, int subSegmentCount)
	{
		Health_set(health);
		m_SegmentMultiplier = segmentMultiplier;
		SubSegmentCount_set(subSegmentCount);
	}
	
	void Health_Segmented::Damage(float damage)
	{
		damage = GetSegmentDamage(damage);
		
		m_Health -= damage;
		
		if (m_Health < 0.0f)
			m_Health = 0.0f;
	}
	
	float Health_Segmented::GetSegmentDamage(float damage)
	{
		int subSegmentCount = SubSegmentCount_get();
		
		float strength = powf(m_SegmentMultiplier, (float)subSegmentCount);
		
		damage /= strength;
		
		return damage;
	}
	
	int Health_Segmented::SubSegmentCount_get() const
	{
		return m_SubSegmentCount;
	}
	
	void Health_Segmented::SubSegmentCount_set(int count)
	{
		m_SubSegmentCount = count;
	}
	
	float Health_Segmented::Health_get() const
	{
		return m_Health;
	}
	
	void Health_Segmented::Health_set(float health)
	{
		m_Health = health;
	}
}
