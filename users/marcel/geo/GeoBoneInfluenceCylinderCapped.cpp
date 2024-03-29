#include "GeoBoneInfluenceCylinderCapped.h"

#include <stdio.h>

namespace Geo
{

	BoneInfluenceCylinderCapped::BoneInfluenceCylinderCapped()
		: BoneInfluence()
	{

		type = bitCylinderCapped;
		
		m_radius = 1.0f;
		
		m_min = + 0.0f;
		m_max = + 1.0f;
		
	}

	BoneInfluenceCylinderCapped::~BoneInfluenceCylinderCapped()
	{

	}

	float BoneInfluenceCylinderCapped::CalculateInfluence(Vec3Arg position) const
	{

		Vec3 point = GetNearestPointOnLine(position);
		
		Vec3 delta = position - point;

		if (delta.CalcSizeSq() < m_radius2)
		{
			return 1.0f;
		}
			
		// FIXME: Write general attenuation function, member of BoneInfluence.
		
		float influence = 1.0f / delta.CalcSizeSq() * m_radius2;
		
		return influence;
		
	}

	bool BoneInfluenceCylinderCapped::Finalize()
	{

		BoneInfluence::Finalize();
		
		point1.Set(m_min, 0.0f, 0.0f);
		point2.Set(m_max, 0.0f, 0.0f);
		
		planeLine.normal = point2 - point1;
		planeLine.distance = planeLine.normal * point1;

		float size2_i = 1.0f / planeLine.normal.CalcSizeSq();
		
		planeLine.normal *= size2_i;
		planeLine.distance *= size2_i;

		m_radius2 = m_radius * m_radius;
		
		return true;
		
	}

	Vec3 BoneInfluenceCylinderCapped::GetNearestPointOnLine(Vec3Arg position) const
	{

		float distance = planeLine * position;
		
		if (distance < 0.0f)
		{
			distance = 0.0f;
		}
		
		if (distance > 1.0f)
		{
			distance = 1.0f;
		}
		
		return point1 + (point2 - point1) * distance;
		
	}

}
