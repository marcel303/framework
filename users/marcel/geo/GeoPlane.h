#pragma once

#include "MathVector.h"

namespace Geo
{

	extern float epsilon; // FIXME: Move to Geo.h.

	/// Geometry: Plane classification.
	enum PlaneClassification
	{
		pcUnknown, ///< Unknown classification. Used to initialize PlaneClassification instances.
		pcFront, ///< Item classified being in front of plane.
		pcBack, ///< Item classified being behind plane.
		pcOn, ///< Item classified being on the plane's surface.
		pcSpan ///< Item classified spanning the plane's surface.
	};

	/// Geometry: Plane type.
	/**
	 * A plane is defined by a normal vector and a distance.
	 */
	class Plane
	{

	public:
		
		Plane();
		Plane(const Plane& plane); ///< Copy constructor.
		Plane(const Vector& vector1, const Vector& vector2); ///< Create plane from 2 vectors. normal = vector2 - vector1. distance = normal * vector1.
		Plane(const Vector& vector1, const Vector& vector2, const Vector& vector3); ///< Create plane from 3 vectors. Vector 1, 2 and 3 specify a triagle. normal = (vector2 - vector1) % (vector3 - vector2). distance = normal * vector1.
		~Plane();
		
	public:
		
		Vector normal; ///< Plane normal.
		float distance; ///< Plane distance.
		
	public:
		
		void Normalize(); ///< Normalize plane normal / distance.
		void Setup(const Vector& vector1, const Vector& vector2); ///< Create plane from 2 vectors. normal = vector2 - vector1. distance = normal * vector1.
		void Setup(const Vector& vector1, const Vector& vector2, const Vector& vector3); ///< Create plane from 3 vectors. Vector 1, 2 and 3 specify a triagle. normal = (vector2 - vector1) % (vector3 - vector2). distance = normal * vector1.
		PlaneClassification Classify(const Vector& vector) const; ///< Classify point.
		
	public:
		
		Plane operator-() const; ///< Negate normal / distance. Return result.
		Plane& operator=(const Plane& plane); ///< Assign plane.
		
	public:
		
		float operator*(const Vector& vector) const; ///< Return normal * vector - distance, a.k.a. the distance of a point to a plane.

	};

}
