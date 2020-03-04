#include "GeoBuilder.h"
#include <math.h>

namespace Geo
{

	Builder& Builder::sphere(Mesh& mesh, int div1, int div2)
	{

		for (int i = 0; i < div1; ++i)
		{

			float angle1 = M_PI / div1 * i;
			float angle2 = M_PI / div1 * (i + 1);

			float h1 = cosf(angle1);
			float h2 = cosf(angle2);

			float r1 = sinf(angle1);
			float r2 = sinf(angle2);

			for (int j = 0; j < div2; ++j)
			{

				float angle1 = 2.0f * M_PI / div2 * j;
				float angle2 = 2.0f * M_PI / div2 * (j + 1);

				// Do not add first triangle if this is the sphere's top.
				// This prevents a triangle with area 0 being created...
				// First triangle uses h1 twice.
				
				if (i != 0)
				{

					Poly* poly = mesh.Add();

					Vertex* vertex1_1 = poly->Add();
					Vertex* vertex1_2 = poly->Add();
					Vertex* vertex1_3 = poly->Add();

					vertex1_1->position[0] = sinf(angle1) * r1;
					vertex1_1->position[1] = cosf(angle1) * r1;
					vertex1_1->position[2] = h1;

					vertex1_2->position[0] = sinf(angle2) * r1;
					vertex1_2->position[1] = cosf(angle2) * r1;
					vertex1_2->position[2] = h1;

					vertex1_3->position[0] = sinf(angle2) * r2;
					vertex1_3->position[1] = cosf(angle2) * r2;
					vertex1_3->position[2] = h2;

					transform(poly);
					
				}
				
				// Do not add second triangle if this is the sphere's bottom.
				// This prevents a triangle with area 0 being created...
				// Second triangle uses h2 twice.
				
				if (i != div1 - 1)
				{

					Poly* poly = mesh.Add();

					Vertex* vertex2_1 = poly->Add();
					Vertex* vertex2_2 = poly->Add();
					Vertex* vertex2_3 = poly->Add();

					vertex2_1->position[0] = sinf(angle2) * r2;
					vertex2_1->position[1] = cosf(angle2) * r2;
					vertex2_1->position[2] = h2;

					vertex2_2->position[0] = sinf(angle1) * r2;
					vertex2_2->position[1] = cosf(angle1) * r2;
					vertex2_2->position[2] = h2;

					vertex2_3->position[0] = sinf(angle1) * r1;
					vertex2_3->position[1] = cosf(angle1) * r1;
					vertex2_3->position[2] = h1;

					transform(poly);
					
				}

			}

		}
		
		return *this;

	}

}
