#pragma once

#include "GeoAabb.h"
#include "GeoEdge.h"
#include "GeoPlane.h"
#include "GeoVertex.h"
#include <list>

namespace Geo
{

	/// Geometry: Poly type.
	/**
	 * A polygon is a collection of vertices.
	 * Polygons must be convex for most operations.
	 * Once done initializing vertices, call Finalize() to calculate plane and edges.
	 */
	class Poly
	{

	public:
		
		Poly();
		Poly(const Poly& poly);
		~Poly();
		
	public:
		
		std::list<Vertex*> cVertex; ///< List of vertices.
		std::list<Edge*> cEdge; ///< List of edges. Reconstructed from list of vertices when finalizing.
		
	public:
		
		Plane plane; ///< Face plane. Calculated when finalized.
		Aabb aabb; ///< Axis aligned bounding box.
		Vector vertexCenter; ///< Center based on average vertex position.
		
	public:
		
		Vertex* Add(); ///< Add vertex to tail.
		Vertex* AddHead(); ///< Add vertex to head.
		Vertex* AddTail(); ///< Add vertex to tail.
		Vertex* Link(Vertex* vertex); ///< Add already existing vertex to tail.
		Vertex* LinkHead(Vertex* vertex); ///< Add already existing vertex to head.
		Vertex* LinkTail(Vertex* vertex); ///< Add already existing vertex to tail.
		void Clear(); ///< Remove all vertices.
		
	public:
		
		void RevertWinding(); ///< Revert order of vertices. This will flip the face normal.
		
		PlaneClassification Classify(const Plane& plane); ///< Classify polygon / plane relation.
		PlaneClassification Classify(const Poly& poly); ///< Classify polygon / polygon relationship.
		
		void Split(const Plane& plane, Poly* polyFront, Poly* polyBack); ///< Split poly in 2 parts, 1 in front of plane, the other behind. Note: Only clip if asserted Classify(plane) == pcSpan, else nasty things shall happen.
		
		void ClipKeepFront(const Plane& plane, Poly* poly); ///< Clip poly by plane, keeping everything in front of plane.
		void ClipKeepBack(const Plane& plane, Poly* poly); ///< Clip poly by plane, keeping everything behind plane.
		
		/*
		Clip polygon by another polygon. The result is an inside polygon and an outside mesh. A little figure to illustrate:
		\code
		++++                ++++
		++---- => outside = ++    inside =   ++  poly =   ----
		++----              ++               ++           ----
		----                                            ----
		\endcode
		*/
		void Clip(const Poly* poly, Poly** inside, class Mesh** outside);
		void Triangulate(class Mesh* mesh); ///< Triangulate poly, resulting in 1 or more triangular polygons (mesh).
		void CalculateExtents(Vector& outMin, Vector& outMax) const; ///< Calculate axis aligned bounding box (AABB).
		Vector CalculateCenter() const; ///< Calculate center, using extents AABB.
		Vector CalculateVertexCenter() const; ///< Calculate center, using average vertex position.
		const Aabb& GetExtents() const; ///< Return precomputed extends. Requires finalization.
		Vector GetCenter() const; ///< Return precomputed center. Requires finalization.
		Vector GetVertexCenter() const; ///< Return precomputed average vertex center. Requires finalization.

	public:
		
		bool bFinalized;
		
	public:
		
		void Finalize(); ///< Calculate plane, edges and edge.planeOutward.
		void Unfinalize(); ///< Undo finalization.
		
	public:
		
		Poly& operator=(const Poly& poly); ///< Assign poly.
		
	};

}
