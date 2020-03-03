#include "GeoBuilder.h"
#include <math.h>

namespace Geo
{

	namespace Builder
	{

		/// Geometry: Primitives: Cylinder generator.
		/**
		 * Creates a mesh around the z axis with radius 1.0 and height from -1.0 to +1.0.
		 * The higher the number of slices, the better the approximation of a true cylinder will be.
		 * The generated polygons will be stored in the specified mesh.
		 * @param mesh Mesh to put generated polygon in.
		 * @param slices Number of divisions. More divisions make the shape look smoother.
		 */
		void MakeCylinder(Mesh& mesh, int slices)
		{

			float s = 2.0f * M_PI / slices;
			float a = 0.0f;

			Poly* cb = mesh.Add();
			Poly* ct = mesh.Add();
			
			for (int i = 0; i < slices; ++i)
			{

				// Side.
				
				Poly* poly = mesh.Add();

				Vertex* vertex1 = poly->Add(); vertex1->position.Set(sinf(a    ), cosf(a    ), +1.0f);
				Vertex* vertex2 = poly->Add(); vertex2->position.Set(sinf(a + s), cosf(a + s), +1.0f);
				Vertex* vertex3 = poly->Add(); vertex3->position.Set(sinf(a + s), cosf(a + s), -1.0f);
				Vertex* vertex4 = poly->Add(); vertex4->position.Set(sinf(a    ), cosf(a    ), -1.0f);

				Transform(poly);

				// Top and bottom.
				
				Vertex* vertex;
				
				vertex = cb->Add();
				vertex->position.Set(sinf(+a), cosf(+a), -1.0f);
				
				vertex = ct->Add();
				vertex->position.Set(sinf(-a), cosf(-a), +1.0f);
				
				a += s;

			}
			
			Transform(cb);
			Transform(ct);

		}

	} // Builder.

} // Geo.
