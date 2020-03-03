#pragma once

#include "GeoMesh.h"
#include "GeoTreeBsp.h"

namespace Geo
{

	namespace Csg3D
	{

		TreeBsp* CreateBspTree(Mesh* mesh);

		void Filter(TreeBsp* tree, Mesh* mesh, Mesh* outInside, Mesh* outOutside);

		void Add(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh); ///< Calculate mesh1 + mesh2.
		void Subtract(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh); ///< Calculate mesh1 - mesh2.
		void Intersect(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh); ///< Calculate mesh1 & mesh2.

		void AddInplace(Mesh* mesh1, Mesh* mesh2); ///< Add mesh2 to mesh1, storing results in mesh1.
		void SubtractInplace(Mesh* mesh1, Mesh* mesh2); ///< Subtract mesh2 from mesh1, storing results in mesh1.
		void IntersectInplace(Mesh* mesh1, Mesh* mesh2); ///< Intersect mesh1 with mesh2, storing results in mesh1.

	}

}
