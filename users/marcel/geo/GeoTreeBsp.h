#pragma once

#include "GeoTree.h"

namespace Geo
{

	/// Geometry: BSP tree type.
	/**
	 * Binary space partition tree.
	 */
	class TreeBsp : public Tree
	{

	public:
		
		TreeBsp();
		virtual ~TreeBsp() override;
		
	protected:
		
		Plane splitPlane; ///< Splitting plane, or undefined when leaf node.
		
	public:
		
		const Plane& GetSplitPlane() const;
		
	public:

		virtual void Generate(int depthMax, GenerateStatistics* generateStatistics) override; ///< Generate BSP tree, using current set of polygons in mesh. depthLeft is the maximum tree depth.
		
	public:

		bool FindSplitPlane(Plane* plane) const; ///< Find optimal splitting plane.
		
	};

}
