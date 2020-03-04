#include "Debugging.h"
#include "GeoPlane.h"

namespace Geo
{

	float epsilon = 1e-6f;

	Plane::Plane()
	{

		distance = 0.0f;
		
	}

	Plane::Plane(const Plane& plane)
	{

		normal = plane.normal;
		distance = plane.distance;
		
	}

	Plane::Plane(Vec3Arg vector1, Vec3Arg vector2)
	{

		Setup(vector1, vector2);
		
	}

	Plane::Plane(Vec3Arg vector1, Vec3Arg vector2, Vec3Arg vector3)
	{

		Setup(vector1, vector2, vector3);
		
	}

	Plane::~Plane()
	{

	}

	void Plane::Normalize()
	{

		float length = normal.CalcSize();
		
		normal /= length;
		distance /= length;
		
	}

	void Plane::Setup(Vec3Arg vector1, Vec3Arg vector2)
	{

		normal = vector2 - vector1;
		distance = vector1 * normal;
		
	}

	void Plane::Setup(Vec3Arg vector1, Vec3Arg vector2, Vec3Arg vector3)
	{

		Vec3 delta1 = vector2 - vector1;
		Vec3 delta2 = vector3 - vector2;
		
		normal = delta1 % delta2;
		distance = vector1 * normal;

	}

	PlaneClassification Plane::Classify(Vec3Arg vector) const
	{

		float temp = normal * vector - distance;
		
		if (temp >= - epsilon && temp <= + epsilon)
		{
			return pcOn;
		}
		
		if (temp >= - epsilon)
		{
			return pcFront;
		}
		
		if (temp <= + epsilon)
		{
			return pcBack;
		}
		
		Assert(false);
		
		return pcUnknown;
		
	}

	Plane Plane::operator-() const
	{

		Plane temp;
		
		temp.normal = - normal;
		temp.distance = - distance;
		
		return temp;
		
	}

	Plane& Plane::operator=(const Plane& plane)
	{

		normal = plane.normal;
		distance = plane.distance;
		
		return (*this);
		
	}

	float Plane::operator*(Vec3Arg vector) const
	{

		return normal * vector - distance;
		
	}

}
