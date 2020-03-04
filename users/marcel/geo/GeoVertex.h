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
		Vertex(const Vertex& vertex); ///< Copy constructor.
		~Vertex();
		
	public:
		
		static const int nVarying = 8; ///< Maximum number of 'varying' (interpolated) components.
		
	public:
		
		Vec3 position; ///< Position of vertex.
		Plane planeFaceAverage; ///< Average face normal (and plane distance).

		Vec4 varying[nVarying]; ///< Varying components. These are user-defined values that get interpolated when splitting edges. These can be anything, like for instance texture coordinates, a normal vector or a color.
		
	public:
		
		PlaneClassification Classify(const Plane& plane) const; ///< Classify vertex position against plane.
		
	public:
		
		Vertex& operator=(const Vertex& vertex); ///< Assign vertex.
		
	public:
		
		Vertex operator+(const Vertex& vertex) const; ///< Add vertices. Return result. Add position and varying values.
		Vertex operator-(const Vertex& vertex) const; ///< Subtract vertices. Return result. Subtract position and varying values.
		Vertex operator*(float v) const; ///< Scale vertex. Return result. Scale position and varying values.
		
	public:
		
		Vertex& operator+=(const Vertex& vertex); ///< Add vertex.
		Vertex& operator-=(const Vertex& vertex); ///< Subtract vertex.
		Vertex& operator*=(float v); ///< Scale vertex.

	};

}
