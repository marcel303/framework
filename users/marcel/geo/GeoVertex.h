#pragma once

#include "GeoPlane.h"
#include "Vec3.h"
#include "Vec4.h"

namespace Geo
{

	/// Geometry: Vertex type.
	/**
	 * A vertex is a position and plane and zero or more associated (varying) values.
	 */
	class Vertex
	{

	public:
		
		Vertex();
		Vertex(const Vertex& vertex);
		~Vertex();
		
	public:
		
		static const int kMaxVaryings = 8; ///< Maximum number of 'varying' (interpolated) components.
		
	public:
		
		Vec3 position; ///< Position of vertex.
		Plane planeFaceAverage; ///< Average face normal (and plane distance).

		Vec4 varying[kMaxVaryings]; ///< Varying components. These are user-defined values that get interpolated when splitting edges. These can be anything, like for instance texture coordinates, a normal vector or a color.
		
	public:
		
		PlaneClassification Classify(const Plane& plane) const;
		
	public:
		
		Vertex& operator=(const Vertex& vertex);
		
	public:
		
		Vertex operator+(const Vertex& vertex) const;
		Vertex operator-(const Vertex& vertex) const;
		Vertex operator*(const float v) const;
		
	public:
		
		Vertex& operator+=(const Vertex& vertex);
		Vertex& operator-=(const Vertex& vertex);
		Vertex& operator*=(const float v);

	};

}
