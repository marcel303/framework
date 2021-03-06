#pragma once

#include "GeoVertex.h"

namespace Geo
{

	/// Geometry: Edge type.
	/**
	 * An edge references two vertices. vertex[0] and vertex[1].
	 * A series of edges defines a shape, for instance (but not necessarily) a polygon.
	 */
	class Edge
	{

	public:
		
		Edge();

	public:
		
		Vertex* vertex[2]; ///< Pointers to the two vertices the edge is comprised of.

		Plane planeEdge;    ///< Plane following the edge. planeEdge * vertex[0] = 0.0. planeEdge * vertex[1] = 1.0.
		Plane planeOutward; ///< Plane pointing outwards of the polygon. Perpendicular to both polygon normal and planeEdge. planeOutward * vertex[0] = 0.0. planeOutward * vertex[1] = 0.0. planeOutward * poly->normal = 0.0. planeOutward * planeEdge = 0.0.
		
		Edge* next; ///< Pointer to next edge in the polygon. Cyclic.
		Edge* prev; ///< Pointer to previous edge in the polygon. Cyclic.
		
	public:
		
		void RevertWinding(); ///< Swaps the order of the vertices. Eg replace vertex[0] with vertex[1] and vertex[1] with vertex[0].
		
	public:
		
		void Finalize(); ///< Calculates planeEdge.

	};

}
