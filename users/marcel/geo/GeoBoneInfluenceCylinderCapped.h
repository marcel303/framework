#pragma once

#include "GeoBoneInfluence.h"
#include "GeoPlane.h"

namespace Geo
{

	class BoneInfluenceCylinderCapped : public BoneInfluence
	{

	public:
		
		BoneInfluenceCylinderCapped();
		virtual ~BoneInfluenceCylinderCapped() override;
		
	public:
		
		float m_radius; ///< Radius of the cylinder.
		float m_min;    ///< Cylinder extent along the X axis, minimum value.
		float m_max;    ///< Cylinder extent along the X axis, maximum value.
		
	public:
		
		virtual float CalculateInfluence(Vec3Arg position) const override; ///< Calculate influence given position of point, min and max extent and radius.
		
		virtual bool Finalize() override; ///< Precomputed internal variables needed to compute influences.
		
	protected:
		
		Vec3 point1;     ///< First point on line segment.
		Vec3 point2;     ///< Second point on line segment.
		Plane planeLine; ///< Edge plane composed of point1 and point2. Required by nearest point on line algoritm.
		float m_radius2; ///< Radius, squared.
		
	protected:
		
		Vec3 GetNearestPointOnLine(Vec3Arg position) const; ////< Find the nearest point on line segment described by min and max. This may be a point on the line segment or one of it's outer vertices.

	};

}
