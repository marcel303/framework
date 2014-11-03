#include "ReplicationPriority.h"

namespace Replication
{
	Priority::Priority()
	{
		m_base = 1.0f;
		m_modifier = 1.0f;
		m_skipBonus = 1.0f;
		m_culled = false;
	}

	void Priority::Init(float base, float skipBonus)
	{
		m_base = base;
		m_skipBonus = skipBonus;
	}
}
