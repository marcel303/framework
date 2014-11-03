#ifndef REPLICATIONPRIORITY_H
#define REPLICATIONPRIORITY_H
#pragma once

namespace Replication
{
	const static float PRI_LOW      = 0.5f;
	const static float PRI_NORMAL   = 1.0f;
	const static float PRI_HIGH     = 2.0f;
	const static float PRI_VERYHIGH = 10.0f;

	class Priority
	{
	public:
		Priority();

		void Init(float base, float skipBonus = PRI_NORMAL);

		inline void Update(float modifier, bool culled)
		{
			m_modifier = modifier;
			m_culled = culled;
		}

		inline float Calc(int skipCount) const
		{
			return m_base * m_modifier + skipCount * m_skipBonus;
		}

		float m_base;
		float m_modifier;
		float m_skipBonus;
		bool m_culled; // If true, don't send updates at all.
	};
}

#endif
