#pragma once

#include "GeoMesh.h"
#include "MathMatrixStack.h"

/**
 * The geometry builder provides methods to dynamically generate geometry like cubes, spheres and cylinders.
 * All generated geometry is transformed by the geometry builder transformation matrix.
\code
// Example:

Geo::Mesh mesh;

Geo::Builder::matrixStack.Push();
{
	// Create a (4, 6, 2) sized box with center at (0, 0, 0).
	Geo::Builder::matrixStack.identity();
	Geo::Builder::matrixStack.scale(2.0f, 3.0f, 1.0f);
	Geo::Builder::MakeCube(mesh);
}
Geom::Builder::matrixStack.Pop();
\endcode
 */

namespace Geo
{

	struct Builder
	{
		
		MatrixStack matrixStack; ///< Generated geometric primitives are transformed by this matrix before being outputted.
		
		Builder& push();
		Builder& pop();
		Builder& scale(float s);
		Builder& scale(float sx, float sy, float sz);
		Builder& rotate(float angle, float axisX, float axisY, float axisZ);
		Builder& translate(float x, float y, float z);

		Builder& transform(Poly* poly);
		
		/**
		 * Generate a cube from (-1, -1, -1) to (1, 1, 1).
		 */
		Builder& cube(Mesh& mesh);
		
		/**
		 * Circlar top & bottom are located at z = -1.0 and z = +1.0.
		 * Cylinder is oriented along the z axis.
		 * @param sides The number of sides the cylinder has. Higher means better silhouette.
		 */
		Builder& cylinder(Mesh& mesh, int sides);
		
		/**
		 * Generate a circle with radius 1.0, in the XY plane.
		 * @param sides The number of edges.
		 */
		Builder& circle(Mesh& mesh, int sides);
		
		/**
		 * Generate a cone shape. Cone converges in (0.0, 0.0, 1.0).
		 * @param sides Subdivision factor.
		 */
		Builder& cone(Mesh& mesh, int sides);
		
		/**
		 * Generate a sphere with radius 1.0.
		 * @param div1 Subdivisions along the z axis.
		 * @param div2 Subdivisions in the XY plane.
		 */
		Builder& sphere(Mesh& mesh, int div1, int div2);

		Builder& donut(Mesh& mesh, int sides, int slices, float r1, float r2);
		
	};

}
