#include "GeoBuilder.h"

namespace Geo
{

	/// Geometry: Primitives: Cube generator.
	/**
	 * Creates an orthogonal cube with corners (-1.0, -1.0, -1.0), (+1.0, +1.0, +1.0).
	 * The generated polygons will be stored in the specified mesh.
	 * @param mesh Mesh to put generated polygon in.
	 */
	Builder& Builder::cube(Mesh& mesh)
	{

		const char vertex[8][3] =
			{
				{ -1, -1, -1 },
				{ +1, -1, -1 },
				{ +1, +1, -1 },
				{ -1, +1, -1 },
				{ -1, -1, +1 },
				{ +1, -1, +1 },
				{ +1, +1, +1 },
				{ -1, +1, +1 }
			};

		const char face[6][4] =
			{
				{ 3, 2, 1, 0 },	// OK
				{ 4, 5, 6, 7 }, // OK
				{ 0, 4, 7, 3 },	// OK
				{ 2, 6, 5, 1 },	// OK
				{ 0, 1, 5, 4 },	// OK
				{ 7, 6, 2, 3 }	// OK
			};

		for (int i = 0; i < 6; ++i)
		{

			Poly* poly = mesh.Add();

			for (int j = 0; j < 4; j++)
			{

				Vertex* temp = poly->Add();

				temp->position[0] = (float)vertex[(int)face[i][j]][0];
				temp->position[1] = (float)vertex[(int)face[i][j]][1];
				temp->position[2] = (float)vertex[(int)face[i][j]][2];
				
			}

			transform(poly);
			
		}
		
		return *this;

	}

}
