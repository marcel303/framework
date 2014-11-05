#ifndef CDPLANE_H
#define CDPLANE_H
#pragma once

#include "Debug.h" // FIXME, cpp
#include "MxPlane.h"

namespace CD
{
	class Plane
	{
	public:
		Plane()
		{
		}

		void Setup(const Mx::Plane& plane)
		{
			m_plane = plane;
		}

		virtual bool HitRay(const Vec3& position, const Vec3& direction, float maxDistance, float& out_distance)
		{
			const float d1 = m_plane * position;

			//if (d1 < 0.0f)
				//return;

			const float dd = m_plane.m_normal * direction;

			//if (dd > 0.0)
				//return;

			if (d1 >= 0.0f && dd >= 0.0f)
				return false;
			if (d1 <= 0.0f && dd <= 0.0f)
				return false;

			out_distance = - d1 / dd;

			Assert(out_distance >= 0.0f);

			return true;
		}

	private:
		Mx::Plane m_plane;
	};
}

#endif
