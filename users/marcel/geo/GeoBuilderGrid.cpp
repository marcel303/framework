#include "GeoBuilder.h"

namespace Geo
{

	/// Geometry: Primitives: Grid generator.
	/**
	 * Creates a grid perpendicular to the given axis.
	 * The generated polygons will be stored in the specified mesh.
	 * @param mesh Mesh to put generated polygon in.
	 * @param axis The grid will be perpendicular to this axis.
	 * @param resolution1 The number of subdivisions along the 'next' axis. For example, if axis is X, this is the nunber of subdivisions along the Y axis.
	 * @param resolution2 The number of subdivisions along the 'next next' axis. For example, if axis is X, this is the nunber of subdivisions along the Z axis.
	 * @param origin The origin of the grid.
	 */
	Builder& Builder::grid(Mesh& mesh, int axis, int resolution1, int resolution2, Vec3Arg origin)
	{
		
		for (int i1 = 0; i1 < resolution1; ++i1)
		{
		
			for (int i2 = 0; i2 < resolution2; ++i2)
			{
			
				float v11 = (i1 + 0) / (resolution1 / 2.0f) - 1.0f;
				float v12 = (i2 + 0) / (resolution2 / 2.0f) - 1.0f;

				float v21 = (i1 + 1) / (resolution1 / 2.0f) - 1.0f;
				float v22 = (i2 + 0) / (resolution2 / 2.0f) - 1.0f;
				
				float v31 = (i1 + 1) / (resolution1 / 2.0f) - 1.0f;
				float v32 = (i2 + 1) / (resolution2 / 2.0f) - 1.0f;
				
				float v41 = (i1 + 0) / (resolution1 / 2.0f) - 1.0f;
				float v42 = (i2 + 1) / (resolution2 / 2.0f) - 1.0f;
				
				Geo::Poly* poly = mesh.Add();
				
				Geo::Vertex* vertex1 = poly->Add();
				Geo::Vertex* vertex2 = poly->Add();
				Geo::Vertex* vertex3 = poly->Add();
				Geo::Vertex* vertex4 = poly->Add();
				
				vertex1->position[(axis + 1) % 3] = v11;
				vertex1->position[(axis + 2) % 3] = v12;
				
				vertex2->position[(axis + 1) % 3] = v21;
				vertex2->position[(axis + 2) % 3] = v22;

				vertex3->position[(axis + 1) % 3] = v31;
				vertex3->position[(axis + 2) % 3] = v32;

				vertex4->position[(axis + 1) % 3] = v41;
				vertex4->position[(axis + 2) % 3] = v42;
				
				vertex1->position += origin;
				vertex2->position += origin;
				vertex3->position += origin;
				vertex4->position += origin;
			
			}
			
		}
		
		return *this;

	}

}
