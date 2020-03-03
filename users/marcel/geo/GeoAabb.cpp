#include "GeoAabb.h"

namespace Geo
{

	void Aabb::Add(const Geo::Aabb& aabb)
	{

		for (int i = 0; i < 3; ++i)
		{
			if (aabb.min[i] < min[i])
			{
				min[i] = aabb.min[i];
			}
			if (aabb.max[i] > max[i])
			{
				max[i] = aabb.max[i];
			}
		}

	}

	Vec3 Aabb::CalculateSize() const
	{

		Vec3 size;
		
		size[0] = max[0] - min[0];
		size[1] = max[1] - min[1];
		size[2] = max[2] - min[2];
		
		return size;

	}

	bool Aabb::IsIntersecting(const Aabb& aabb) const
	{

		for (int i = 0; i < 3; ++i)
		{
			if (aabb.max[i] < min[i])
			{
				return false;
			}
			if (aabb.min[i] > max[i])
			{
				return false;
			}
		}
		
		return true;

	}

	Aabb& Aabb::operator=(const Aabb& aabb)
	{

		min = aabb.min;
		max = aabb.max;
		
		return (*this);
	}

	Aabb Aabb::operator+(const Aabb& aabb)
	{

		Aabb temp;
		
		temp = (*this);
		temp.Add(aabb);
		
		return temp;
		
	}

	void Aabb::operator+=(const Aabb& aabb)
	{

		Add(aabb);
		
	}

}
