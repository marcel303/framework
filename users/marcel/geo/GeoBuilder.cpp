#include "GeoBuilder.h"

namespace Geo
{

namespace Builder
{

MatrixStack matrixStack;

void Geo::Builder::Transform(Poly* poly)
{

	for (std::list<Vertex*>::iterator i = poly->cVertex.begin(); i != poly->cVertex.end(); ++i)
	{

		(*i)->position = matrixStack.GetMatrix() * (*i)->position;

	}

}

}; // Builder.

}; // Geo.