#pragma once

#include "GeoPoly.h"
#include <list>

namespace Geo
{

	/// Geometry: Mesh type.
	/**
	 * A mesh is a collection of polygons.
	 */
	class Mesh
	{

	public:
		
		Mesh();
		Mesh(const Mesh& mesh); ///< Copy constructor.
		~Mesh();
		
	public:
		
		std::list<Poly*> polys; ///< List of polygons.
		
	public:
		
		Aabb aabb; ///< Axis aligned bounding box.
		Vector center;
		Vector polyCenter;
		Vector vertexCenter;
		
	public:
		
		Poly* Add(); ///< Add new polygon. Return pointer to newly allocated polygon.
		void Remove(Poly* poly); ///< Remove polygon. Freeing it's memory.
		Poly* Link(Poly* poly); ///< Add existing polygon.
		Poly* Unlink(Poly* poly); ///< Remove polygon from list, without freeing it's memory.
		void Clear(); ///< Remove all polygons.
		void Move(Mesh& mesh); ///< Move polygons to another mesh.
		
	public:
		
		void RevertPolyWinding(); ///< Revert winding for every polygon. See Poly::RevertWinding.
		Aabb CalculateExtents() const; ///< Calculate axis aligned bounding box (AABB).
		Vector CalculateCenter() const; ///< Calculate center, using extents AABB.
		Vector CalculateVertexCenter() const; ///< Calculate center, using average vertex position.
		Vector CalculatePolyCenter() const; ///< Calculate center, using average of polygon centers.
		const Aabb& GetExtents() const; ///< Return precomputed extends. Requires finalization.
		const Vector& GetCenter() const; ///< Return precomputed center. Requires finalization.
		const Vector& GetPolyCenter() const; ///< Return precomputed average polygon center. Requires finalization.
		const Vector& GetVertexCenter() const; ///< Return precomputed average vertex center. Requires finalization.
		bool IsConvex() const; ///< Returns true if the mesh is convex. Mesh is convex when all poly's are in front off each other.
		PlaneClassification Classify(const Plane& plane) const; ///< Classify mesh against plane. See Plane::Classify.

	public:
		
		bool bFinalized;
		
	public:
		
		void Finalize(); ///< Finalize mesh. Once done making modification, finalize the mesh to make sure edges, normals, etc are properly generated.
		void Unfinalize(); ///< Unfinalize mesh.
		
	public:
		
		Mesh& operator=(const Mesh& mesh); ///< Assign mesh.
		
	};

}
