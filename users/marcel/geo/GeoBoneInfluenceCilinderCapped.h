#pragma once

#include "GeoBoneInfluence.h"
#include "GeoPlane.h"

namespace Geo
{

	class BoneInfluenceCilinderCapped : public BoneInfluence
	{

	public:
		
		BoneInfluenceCilinderCapped();
		virtual ~BoneInfluenceCilinderCapped() override;
		
	public:
		
		float m_radius; // Radius of cilinder.
		float m_min; // Cilinder extent along the X axis, minimum value.
		float m_max; // Cilinder extent along the X axis, maximum value.
		
	public:
		
		virtual float CalculateInfluence(const Vector& position) const override; ///< Calculate influence given position of point, min and max extent and radius.
		
		virtual bool Finalize() override; ///< Precomputed internal variables needed to compute influences.
		
	protected:
		
		Vector point1; ///< First point on line segment.
		Vector point2; ///< Second point on line segment.
		Plane planeLine; ///< Edge plane composed of point1 and point2. Required by nearest point on line algoritm.
		float m_radius2; ///< Radius, squared.
		
	protected:
		
		Vector GetNearestPointOnLine(const Vector& position) const; ////< Find the nearest point on line segment described by min and max. This may be a point on the line segment or one of it's outer vertices.

	};

}
