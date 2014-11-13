#include <stdio.h> // FIXME.
#include "CDCube.h"
#include "Debug.h"
#include "MxPlane.h"
#include "Vec3.h"

namespace CD
{
	Cube::Cube()
		: Object(TYPE_CUBE)
	{
		Setup(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f));
	}

	Cube::Cube(const Vec3& min, const Vec3& max)
		: Object(TYPE_CUBE)
	{
		Setup(min, max);
	}

	Cube::~Cube()
	{
	}

	void Cube::Setup(const Vec3& min, const Vec3& max)
	{
		m_min = min;
		m_max = max;

		// Calculate planes.

		m_absMin = m_min + GetPosition();
		m_absMax = m_max + GetPosition();

		m_planes[0] = Mx::Plane(Vec3(-1.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f) * Vec3(m_absMin[0], m_absMin[1], m_absMin[2]));
		m_planes[1] = Mx::Plane(Vec3(+1.0f, 0.0f, 0.0f), Vec3(+1.0f, 0.0f, 0.0f) * Vec3(m_absMax[0], m_absMin[1], m_absMin[2]));
		m_planes[2] = Mx::Plane(Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f) * Vec3(m_absMin[0], m_absMin[1], m_absMin[2]));
		m_planes[3] = Mx::Plane(Vec3(0.0f, +1.0f, 0.0f), Vec3(0.0f, +1.0f, 0.0f) * Vec3(m_absMin[0], m_absMax[1], m_absMin[2]));
		m_planes[4] = Mx::Plane(Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, 0.0f, -1.0f) * Vec3(m_absMin[0], m_absMin[1], m_absMin[2]));
		m_planes[5] = Mx::Plane(Vec3(0.0f, 0.0f, +1.0f), Vec3(0.0f, 0.0f, +1.0f) * Vec3(m_absMin[0], m_absMin[1], m_absMax[2]));
	}

	void Cube::OnPositionChange(const Vec3& position)
	{
		Setup(m_min, m_max);
	}

	bool Cube::HitRay(const Vec3& position, const Vec3& direction, float maxDistance, float& out_distance)
	{
		Vec3 min = position;
		Vec3 max = position + direction * maxDistance;

		FixAABB(min, max);

		if (!HitAABB(min, max))
		{
			//printf("%s: Early out: Ray AABB.\n", __FUNCTION__);
			return false;
		}

		if (IsInside(position, 0.0f))
		{
			out_distance = 0.0f;
			//printf("%s: Early out: Origin inside.\n", __FUNCTION__);
			return true;
		}

		float distance = -1.0f;

		for (int i = 0; i < 6; ++i)
		{
			float d = m_planes[i] * position;

			if (d > 0.0f)
			{
				float dd = m_planes[i].m_normal * direction;

				if (dd < 0.0f)
				{
					float t = -d / dd;

					if (distance == -1.0f || t < distance)
					{
						// Check if intersection point is in AABB.

						Vec3 p = position + direction * t;

						if (IsInside(p, 0.001f))
						{
							distance = t;
						}
					}
				}
				else
				{
					//printf("%s: Out: Ray not headed for intersection.\n", __FUNCTION__);
					return false;
				}
			}
		}

		if (distance == -1.0f)
		{
			//printf("%s: Out: No intersection.\n", __FUNCTION__);
			return false;
		}

		out_distance = distance;

		//printf("%s: Out: Intersection.\n", __FUNCTION__);
		return true;
	}

	bool Cube::IntersectPoint(const Vec3& position, Contact& out_contact)
	{
		if (IsInside(position, 0.0f))
		{
			float distance = 1.0f;

			for (int i = 0; i < 6; ++i)
			{
				float distance2 = m_planes[i] * position;

				if (distance == 1.0f || distance2 > distance)
				{
					distance = distance2;

					out_contact.m_plane.m_normal = m_planes[i].m_normal;
					out_contact.m_plane.m_distance = distance;
				}
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Cube::IntersectCube(const Vec3& min, const Vec3& max, Contact& out_contact)
	{
		// TODO.
		if (!HitAABB(min, max))
			return false;

		// Seek minpen, 6x.
		float d = 1000000.0f;
		int axis = 0;
		int sign = 0;
		for (int i = 0; i < 3; ++i)
		{
			float d1 = max[i] - m_absMin[i];
			if (d1 <= d) { d = d1; axis = i; sign = -1; }
			float d2 = m_absMax[i] - min[i];
			if (d2 <= d) { d = d2; axis = i; sign = +1; }
		}

		if (sign == 0)
		{
			DB_ERR("WTF?\n");
			return false;
		}

		out_contact.m_plane.m_normal[(axis + 0) % 3] = float(sign);
		out_contact.m_plane.m_normal[(axis + 1) % 3] = 0.0f;
		out_contact.m_plane.m_normal[(axis + 2) % 3] = 0.0f;
		out_contact.m_plane.m_distance = d;

		/*
		printf("Contact: (%f, %f, %f) - %f.\n",
			out_contact.m_plane.m_normal[0],
			out_contact.m_plane.m_normal[1],
			out_contact.m_plane.m_normal[2],
			out_contact.m_plane.m_distance);
		*/

		return true;
	}

	void Cube::Debug_Render()
	{
		// todo
	}

	// FIXME: Move to AABB class.
	void Cube::FixAABB(Vec3& min, Vec3& max)
	{
		#define FIX(v1, v2) \
		{ \
			if (v1 > v2) \
			{ \
				float temp = v1; \
				v1 = v2; \
				v2 = temp; \
			} \
		}
		FIX(min[0], max[0])
		FIX(min[1], max[1])
		FIX(min[2], max[2])
	}

	// FIXME: Move to AABB class.
	bool Cube::HitAABB(const Vec3& min, const Vec3& max)
	{
		if (max[0] < m_absMin[0]) return false;
		if (max[1] < m_absMin[1]) return false;
		if (max[2] < m_absMin[2]) return false;
		if (min[0] > m_absMax[0]) return false;
		if (min[1] > m_absMax[1]) return false;
		if (min[2] > m_absMax[2]) return false;

		return true;
	}

	bool Cube::IsInside(const Vec3& position, float eps)
	{
		if (position[0] + eps < m_absMin[0]) return false;
		if (position[1] + eps < m_absMin[1]) return false;
		if (position[2] + eps < m_absMin[2]) return false;
		if (position[0] - eps > m_absMax[0]) return false;
		if (position[1] - eps > m_absMax[1]) return false;
		if (position[2] - eps > m_absMax[2]) return false;

		return true;
	}
}
