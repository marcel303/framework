#include <vector>
#include "GeoEdge.h"

namespace Geo
{

Edge::Edge()
{

	vertex[0] = 0;
	vertex[1] = 0;
	
	prev = 0;
	next = 0;
	
}

Edge::~Edge()
{

}

void Edge::RevertWinding()
{

	std::swap(vertex[0], vertex[1]);
	
}

void Edge::Finalize()
{

	planeEdge.Setup(vertex[0]->position, vertex[1]->position);
	
	float size2 = planeEdge.normal.GetSize2();
	planeEdge.normal /= size2;
	planeEdge.distance /= size2;
	
}

};