#pragma once

#include "GeoTree.h"
#include "Types.h"

namespace Geo
{

	/// Geometry: Quad tree type.
	/**
	 * Quad tree.
	 */
	class TreeQuad : public Tree
	{

	public:
		
		TreeQuad();
		virtual ~TreeQuad() override;
		
	protected:
		
		Vec3 splitPosition; ///< Center position of split.
		int splitAxis; ///< Axis that does not get split. Default X axis. Usually Y axis for terrain.
		int splitTreshold; ///< Minimum number of polygons before split. Default 2.
		
	public:

		void SetSplitAxis(int axis);
		void SetSplitTreshold(int treshold);
		
		virtual void Generate(int depthMax, GenerateStatistics* generateStatistics) override; ///< Generate quad tree, using current set of polygons in mesh. depthLeft is the maximum tree depth.
		
	};

}
