#include <vector>
#include "GeoEdge.h"

namespace Geo
{

	Edge::Edge()
	{

		vertex[0] = nullptr;
		vertex[1] = nullptr;
		
		prev = nullptr;
		next = nullptr;
		
	}

	void Edge::RevertWinding()
	{

		std::swap(vertex[0], vertex[1]);
		
	}

	void Edge::Finalize()
	{

		planeEdge.Setup(vertex[0]->position, vertex[1]->position);
		
		const float size2 = planeEdge.normal.CalcSizeSq();
		planeEdge.normal /= size2;
		planeEdge.distance /= size2;
		
	}

}
