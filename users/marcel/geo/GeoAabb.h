#pragma once

#include "MathVector.h"

namespace Geo
{

	class Aabb
	{

	public:
		
		Vector min; ///< Minimal extents. 'Upper-left' corner of bounding box.
		Vector max; ///< Maximal extents. 'Lower-right' corner of bounding box.
		
	public:
		
		void Add(const Aabb& aabb); ///< Include bounding box into current bounding box, expanding current volume to cover new area.
		Vector CalculateSize() const; ///< Calculate size. Equals difference between min and max.
		bool IsIntersecting(const Aabb& aabb) const; ///< Returns true if bounding boxes overlap.
		
	public:
		
		Aabb& operator=(const Aabb& aabb); ///< Copy.
		Aabb operator+(const Aabb& aabb); ///< Expand bounding box such that it covers the added region.
		void operator+=(const Aabb& aabb); ///< Expand bounding box such that it covers the added region.
		
	};

}
