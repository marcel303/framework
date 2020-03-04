#pragma once

#include "GeoMesh.h"

namespace Geo
{

	/// Geometry: Tree type.
	/**
	 * A tree is a composition of one or more children and a mesh that contains geometry data.
	 * Non-leaf nodes generally do not have any geometric data associated with them, while leaf nodes do.
	 */
	class Tree
	{

	public:
		
		Tree();
		virtual ~Tree();
		
	public:
		
		Tree* parent; ///< Parent node, or 0 if root.

		Mesh mesh; ///< Mesh containing geometric data.
		
		std::list<Tree*> children; ///< List of child nodes, or empty if leaf.

	public:
		
		Aabb aabb; ///< Extents. Valid after finalization.
		
	public:
		
		Tree* Link(Tree* child); ///< Add child node.
		void Remove(Tree* child); ///< Remove child node, freeing it's memory.
		void Clear(); ///< Clear list of child nodes. Freeing all memory.
		
	public:
		
		void Finalize();
		
	public:
		
		/// Structure that holds statistics about the tree generation process.
		class GenerateStatistics
		{
		
		public:
			
			GenerateStatistics();
			
		public:
			
			int nLeafs; ///< Number of leafs.
			int nNodes; ///< Number of nodes.
			int nSplits; ///< Number of split polygons.
			int nPoly; ///< Number of polygons.
			int nVertex; ///< Number of vertices.
			
		};
		
		void Split(const Plane& plane, Tree* outTreeFront, Tree* outTreeBack, GenerateStatistics* generateStatistics = 0); ///< Split the node by the given plane into two child nodes, that each contain part of the geometric data. When geometry spans the plane, the geometry is split into parts and filtered to the appropriate nodes.
		
		virtual void Generate(int depthMax, GenerateStatistics* statistics = 0); ///< Generate tree structure. Use this function to create a tree from the geometric data using a specific algortim. Derived classes implement their own generation method.

		/// Structure containing information about a possible split.
		class SplitInfo
		{
		
		public:
			
			SplitInfo();
			
		public:
		
			int nOn; ///< Number of polygons on splitting plane.
			int nFront; ///< Number of polygons in front of splitting plane.
			int nBack; ///< Number of polygons behind splitting plane.
			int nSpan; ///< Number of polygons spanning splitting plane.
		
		};
		
		SplitInfo GetSplitInfo(const Plane& plane) const; ///< Calculate split information for given plane. Used to decide which splitting plane to use.

	};

}
