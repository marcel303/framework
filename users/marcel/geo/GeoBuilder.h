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
		
		Mesh* mesh;
		
		Builder();
		
		Builder& push();
		Builder& pop();

		Builder& scale(float s);
		Builder& scale(float sx, float sy, float sz);
		Builder& rotate(float angle, float axisX, float axisY, float axisZ);
		Builder& translate(float x, float y, float z);
		
		Builder& pushScale(float s);
		Builder& pushScale(float sx, float sy, float sz);
		Builder& pushRotate(float angle, float axisX, float axisY, float axisZ);
		Builder& pushTranslate(float x, float y, float z);

		Builder& transform(Poly* poly);
		
		Builder& begin(Mesh& mesh);
		Builder& end();
		
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
		
		Builder& grid(Mesh& mesh, int axis, int resolution1, int resolution2, Vec3Arg origin);

		//
		
		Builder& cube() { return cube(*mesh); }
		Builder& cylinder(int sides) { return cylinder(*mesh, sides); }
		Builder& circle(int sides) { return circle(*mesh, sides); }
		Builder& cone(int sides) { return cone(*mesh, sides); }
		Builder& sphere(int div1, int div2) { return sphere(*mesh, div1, div2); }
		Builder& donut(int sides, int slices, float r1, float r2) { return donut(*mesh, sides, slices, r1, r2); }
		Builder& grid(int axis, int resolution1, int resolution2, Vec3Arg origin) { return grid(*mesh, axis, resolution1, resolution2, origin); }
		
	};

}
