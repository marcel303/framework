#ifndef CDCUBE_H
#define CDCUBE_H
#pragma once

#include "CDObject.h"
#include "MxPlane.h"
#include "Vec3.h"

namespace CD
{
	class Cube : public Object
	{
	public:
		Cube();
		Cube(const Vec3& min, const Vec3& max);
		virtual ~Cube();

		void Setup(const Vec3& min, const Vec3& max);

		virtual void OnPositionChange(const Vec3& position);

		virtual bool HitRay(const Vec3& position, const Vec3& direction, float maxDistance, float& out_distance);
		virtual bool IntersectPoint(const Vec3& position, Contact& out_contact);
		virtual bool IntersectCube(const Vec3& min, const Vec3& max, Contact& out_contact);

		virtual void Debug_Render();

	private:
		// FIXME: Move to AABB class.
		void FixAABB(Vec3& min, Vec3& max);
		// FIXME: Move to AABB class.
		bool HitAABB(const Vec3& min, const Vec3& max);
		bool IsInside(const Vec3& position, float eps);

	public: //FIXME.
		Vec3 m_min;
		Vec3 m_max;
		Vec3 m_absMin;
		Vec3 m_absMax;

		Mx::Plane m_planes[6];
	};
}

#endif
