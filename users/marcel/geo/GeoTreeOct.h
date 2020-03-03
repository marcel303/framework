#pragma once

#include "GeoTree.h"
#include "Types.h"

namespace Geo
{

	/// Geometry: Quad tree type.
	/**
	 * Quad tree.
	 */
	class TreeOct : public Tree
	{

	public:
		
		TreeOct();
		virtual ~TreeOct() override;
		
	protected:
		
		Vector splitPosition; ///< Center position of split.
		int splitTreshold; ///< Minimum number of polygons before split. Default 2.
		
	public:

		void SetSplitTreshold(int treshold);
		
		virtual void Generate(int depthMax, GenerateStatistics* generateStatistics) override; ///< Generate quad tree, using current set of polygons in mesh. depthLeft is the maximum tree depth.
		
	};

}
