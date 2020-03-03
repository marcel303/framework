#pragma once

#include "GeoMesh.h"
#include "GeoTreeBsp.h"

namespace Geo
{

	struct Csg3D
	{

		static TreeBsp* CreateBspTree(const Mesh& mesh);

		static void Filter(const TreeBsp& tree, const Mesh& mesh, Mesh& out_insideMesh, Mesh& out_outsideMesh);

		static void Add(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh); ///< Calculate mesh1 + mesh2.
		static void Subtract(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh); ///< Calculate mesh1 - mesh2.
		static void Intersect(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh); ///< Calculate mesh1 & mesh2.

		static void AddInplace(Mesh& mesh1, const Mesh& mesh2); ///< Add mesh2 to mesh1, storing results in mesh1.
		static void SubtractInplace(Mesh& mesh1, const Mesh& mesh2); ///< Subtract mesh2 from mesh1, storing results in mesh1.
		static void IntersectInplace(Mesh& mesh1, const Mesh& mesh2); ///< Intersect mesh1 with mesh2, storing results in mesh1.

	};

}
