#pragma once

#include "Vec3.h"

// forward declarations

namespace gltf
{
	struct Mesh;
	struct Node;
	struct Scene;
}

namespace gltf
{
	struct BoundingBox
	{
		Vec3 min;
		Vec3 max;
		bool hasMinMax = false;
	};
	
	// -- draw using GX api --

	void drawMesh(const Scene & scene, const Mesh & mesh, const bool isOpaquePass);

	void drawNodeTraverse(const Scene & scene, const Node & node, const bool isOpaquePass);
	void drawNodeMinMaxTraverse(const Scene & scene, const Node & node);
	void calculateNodeMinMaxTraverse(const Scene & scene, const Node & node, BoundingBox & boundingBox);
}

// -- draw using buffer cache --

#include <map>

class GxVertexBuffer;
class GxIndexBuffer;
class GxMesh;

namespace gltf
{
	struct BufferCache
	{
		std::map<int, GxVertexBuffer*> vertexBuffers;
		std::map<int, GxIndexBuffer*> indexBuffers;
		std::map<const gltf::Mesh*, GxMesh*> meshes;
		
		bool init(const gltf::Scene & scene);
	};
	
	void drawMesh(const Scene & scene, const BufferCache * bufferCache, const Mesh & mesh, const bool isOpaquePass);

	void drawNodeTraverse(const Scene & scene, const BufferCache * bufferCache, const Node & node, const bool isOpaquePass);
}
