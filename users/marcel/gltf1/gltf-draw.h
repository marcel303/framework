#pragma once

#include "framework.h"

// forward declarations

namespace gltf
{
	struct Mesh;
	struct Node;
	struct Scene;
}

namespace gltf
{
	enum AlphaMode
	{
		kAlphaMode_AlphaBlend,
		kAlphaMode_AlphaToCoverage
	};
	
	struct BoundingBox
	{
		Vec3 min;
		Vec3 max;
		bool hasMinMax = false;
	};
	
	struct MaterialShaders
	{
		Shader * pbr_specularGlossiness = nullptr;
		Shader * pbr_metallicRoughness = nullptr;
		
		Shader * fallbackShader = nullptr;
	};
	
	struct DrawOptions
	{
		AlphaMode alphaMode = kAlphaMode_AlphaBlend;
		bool sortPrimitivesByViewDistance = false;
	};
	
	// -- draw using GX api --

	void drawMesh(const Scene & scene, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass);

	void drawNodeTraverse(const Scene & scene, const Node & node, const MaterialShaders & materialShaders, const bool isOpaquePass);
	void drawNodeMinMaxTraverse(const Scene & scene, const Node & node);
	void calculateNodeMinMaxTraverse(const Scene & scene, const Node & node, BoundingBox & boundingBox);
	
	void calculateSceneMinMaxTraverse(const Scene & scene, const int activeScene, BoundingBox & boundingBox);
	
	void drawScene(const Scene & scene, const MaterialShaders & materialShaders, const bool isOpaquePass, const int activeScene = -2, const DrawOptions * drawOptions = nullptr);
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
		std::map<const MeshPrimitive*, GxMesh*> primitives;
		
		bool init(const Scene & scene);
	};
	
	void drawMesh(const Scene & scene, const BufferCache * bufferCache, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass);

	void drawNodeTraverse(const Scene & scene, const BufferCache * bufferCache, const Node & node, const MaterialShaders & materialShaders, const bool isOpaquePass);
	
	void drawScene(const Scene & scene, const BufferCache * bufferCache, const MaterialShaders & materialShaders, const bool isOpaquePass, const int activeScene = -2, const DrawOptions * drawOptions = nullptr);
}
