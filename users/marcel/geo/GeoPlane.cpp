#include "Debugging.h"
#include "GeoPlane.h"

namespace Geo
{

float epsilon = 0.01f;

Plane::Plane()
{

	distance = 0.0f;
	
}

Plane::Plane(const Plane& plane)
{

	normal = plane.normal;
	distance = plane.distance;
	
}

Plane::Plane(const Vector& vector1, const Vector& vector2)
{

	Setup(vector1, vector2);
	
}

Plane::Plane(const Vector& vector1, const Vector& vector2, const Vector& vector3)
{

	Setup(vector1, vector2, vector3);
	
}

Plane::~Plane()
{

}

void Plane::Normalize()
{

	float length = normal.GetSize();
	
	normal /= length;
	distance /= length;
	
}

void Plane::Setup(const Vector& vector1, const Vector& vector2)
{

	normal = vector2 - vector1;
	distance = vector1 * normal;
	
}

void Plane::Setup(const Vector& vector1, const Vector& vector2, const Vector& vector3)
{

	Vector delta1 = vector2 - vector1;
	Vector delta2 = vector3 - vector2;
	
	normal = delta1 % delta2;
	distance = vector1 * normal;

}

PlaneClassification Plane::Classify(const Vector& vector) const
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

float Plane::operator*(const Vector& vector) const
{

	return normal * vector - distance;
	
}

};
