#include "GeoBuilder.h"
#include <math.h>

namespace Geo
{

	/// Geometry: Primitives: Circle generator.
	/**
	 * Creates a circle in the OXY plane, with radius 1.0 and slices corner points.
	 * The higher the number of slices, the better the approximation of a true circle will be.
	 * The generated polygons will be stored in the specified mesh.
	 * @param mesh Mesh to put generated polygon in.
	 * @param slices Number of divisions. More division make to shape look smoother.
	 */
	Builder& Builder::circle(Mesh& mesh, int sides)
	{

		float a = 0.0f;
		float s = 2.0f * M_PI / sides;

		Poly* poly = mesh.Add();
		
		for (int i = 0; i < sides; ++i)
		{

			Vertex* vertex = poly->Add();

			vertex->position[0] = sinf(a);
			vertex->position[1] = cosf(a);

			a += s;
			
		}

		transform(poly);
		
		return *this;

	}

}
