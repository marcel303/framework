#include "GeoBuilder.h"
#include <math.h>

namespace Geo
{

	namespace Builder
	{

		/// Geometry: Primitives: Cone generator.
		/**
		 * Creates a cone around the z axis with radius 1.0. The top will be at z=+1.0, the bottom at z=-1.0.
		 * The higher the number of slices, the better the approximation of a true cone will be.
		 * The generated polygons will be stored in the specified mesh.
		 * @param mesh Mesh to put generated polygon in.
		 * @param slices Number of divisions. More divisions make the shape look smoother.
		 */
		void MakeCone(Mesh& mesh, int slices)
		{

			float s = 2.0f * M_PI / slices;
			float a = 0.0f;

			Poly* cb = mesh.Add();
			
			for (int i = 0; i < slices; ++i)
			{

				Poly* poly = mesh.Add();

				Vertex* vertex1 = poly->Add(); vertex1->position.Set(0.0f,        0.0f,        +1.0f);
				Vertex* vertex2 = poly->Add(); vertex2->position.Set(sinf(a + s), cosf(a + s), -1.0f);
				Vertex* vertex3 = poly->Add(); vertex3->position.Set(sinf(a  ),   cosf(a  ),   -1.0f);

				Transform(poly);
		 
				Vertex* vertex = cb->Add();
				vertex->position.Set(sinf(a), cosf(a), -1.0f);
				
				a += s;

			}
			
			Transform(cb);
			
		}

	} // Builder.

} // Geo.
