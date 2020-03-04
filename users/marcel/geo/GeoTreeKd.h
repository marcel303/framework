#pragma once

#include "GeoAabb.h"
#include "GeoTree.h"
#include "Types.h"
#include <vector>

namespace Geo
{

	/// Geometry: Kd tree type.
	/**
	 * Kd tree.
	 */
	class TreeKd : public Tree
	{

	public:
		
		TreeKd();
		virtual ~TreeKd() override;
		
	protected:
		
		Plane splitPlane; ///< Splitting plane, or undefined when leaf node.
		int splitAxis; ///< Splitting axis.
		int splitTreshold; ///< Splitting treshold. Default 2.
		
	protected:
		
		Aabb aabb; ///< Axis aligned bounding box of mesh extents.
		
	public:

		void SetSplitTreshold();
		
		virtual void Generate(int depthMax, GenerateStatistics* generateStatistics) override; ///< Generate Kd tree, using current set of polygons in mesh. depthLeft is the maximum tree depth.
		
	public:

		class KdSplitInfo
		{
		
		public:
			
			float d;
			float cost;
			int nFront;
			int nBack;
			
		};
		
	public:
		
		int CalculateSplitAxis() const;
		void CalculateSplitVector(std::vector<KdSplitInfo>& vSplitInfo) const;
		void CalculateSplitCosts(std::vector<KdSplitInfo>& vSplitInfo) const;
		bool CalculateSplitPlane(Plane* plane) const; ///< Find optimal splitting plane.
		
	};

}
