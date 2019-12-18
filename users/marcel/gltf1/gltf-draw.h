#pragma once

#include "Vec3.h"

namespace gltf
{
	struct Mesh;
	struct Node;
	struct Scene;

	struct BoundingBox
	{
		Vec3 min;
		Vec3 max;
		bool hasMinMax = false;
	};

	void drawMesh(const Scene & scene, const Mesh & mesh, const bool isOpaquePass);

	void drawNodeTraverse(const Scene & scene, const Node & node, const bool isOpaquePass);
	void drawNodeMinMaxTraverse(const Scene & scene, const Node & node);
	void calculateNodeMinMaxTraverse(const Scene & scene, const Node & node, BoundingBox & boundingBox);
}
