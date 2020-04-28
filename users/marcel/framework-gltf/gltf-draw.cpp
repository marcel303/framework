#include "gltf.h"
#include "gltf-draw.h"

#include "gx_mesh.h"

#define ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE 1

#if ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE
	#include <algorithm>
#endif

namespace gltf
{
	static Material defaultMaterial;
	
	//
	
	static GxTextureId tryGetTextureId(const Scene & scene, const int textureIndex)
	{
		GxTextureId result = 0;
		
		if (textureIndex >= 0 && textureIndex < scene.textures.size())
		{
			auto & texture = scene.textures[textureIndex];
			
			if (texture.source >= 0 && texture.source < scene.images.size())
			{
				auto & image = scene.images[texture.source];
				
				result = getTexture(image.path.c_str());
			}
		}
		
		return result;
	}
	
	static bool translatePrimitiveType(const PrimitiveType type, GX_PRIMITIVE_TYPE & result)
	{
		switch (type)
		{
		case kPrimitiveType_Points:
			result = GX_POINTS;
			break;
		case kPrimitiveType_Lines:
			result = GX_LINES;
			break;
		case kPrimitiveType_LineStrip:
			result = GX_LINE_STRIP;
			break;
		case kPrimitiveType_Triangles:
			result = GX_TRIANGLES;
			break;
		case kPrimitiveType_TriangleStrip:
			result = GX_TRIANGLE_STRIP;
			break;
		case kPrimitiveType_TriangleFan:
			result = GX_TRIANGLE_FAN;
			break;
		
		default:
			Assert(false);
			return false;
		}
		
		return true;
	}

	void setShaderParams_metallicRoughness(Shader & shader, const Material & material, const Scene & scene, const bool hasVertexColors, int & nextTextureUnit)
	{
		// PBR metallic roughness material

		shader.setImmediate("u_hasVertexColors", hasVertexColors ? 1.f : 0.f);

		shader.setImmediate("u_alphaMask", material.alphaMode == "MASK");
		shader.setImmediate("u_alphaMaskCutoff", material.alphaCutoff);

		shader.setImmediate("u_baseColorFactor",
			material.pbrMetallicRoughness.baseColorFactor.r,
			material.pbrMetallicRoughness.baseColorFactor.g,
			material.pbrMetallicRoughness.baseColorFactor.b,
			material.pbrMetallicRoughness.baseColorFactor.a);

		shader.setImmediate("u_metallicFactor",
			material.pbrMetallicRoughness.metallicFactor);
		shader.setImmediate("u_roughnessFactor",
			material.pbrMetallicRoughness.roughnessFactor);
		
		shader.setImmediate("u_emissiveFactor",
			material.emissiveFactor[0],
			material.emissiveFactor[1],
			material.emissiveFactor[2]);

		// set texture maps

		// todo : set texture samplers

		const GxTextureId baseColorTextureId = tryGetTextureId(scene, material.pbrMetallicRoughness.baseColorTexture.index);
		const GxTextureId metallicRoughnessTextureId = tryGetTextureId(scene, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
		
		const GxTextureId normalTextureId = tryGetTextureId(scene, material.normalTexture.index);
		const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
		const GxTextureId emissiveTextureId = tryGetTextureId(scene, material.emissiveTexture.index);
	
		shader.setTexture("baseColorTexture", nextTextureUnit++, baseColorTextureId, true, false);
		shader.setImmediate("baseColorTextureCoord", baseColorTextureId == 0 ? -1 : material.pbrMetallicRoughness.baseColorTexture.texCoord);

		shader.setTexture("metallicRoughnessTexture", nextTextureUnit++, metallicRoughnessTextureId, true, false);
		shader.setImmediate("metallicRoughnessTextureCoord", metallicRoughnessTextureId == 0 ? -1 : material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
		
		shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		shader.setImmediate("normalTextureCoord", normalTextureId == 0 ? -1 : material.normalTexture.texCoord);
		
		shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		shader.setImmediate("occlusionTextureCoord", occlusionTextureId == 0 ? -1 : material.occlusionTexture.texCoord);
		shader.setImmediate("u_occlusionStrength", material.occlusionTexture.strength);

		shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		shader.setImmediate("emissiveTextureCoord", emissiveTextureId == 0 ? -1 : material.emissiveTexture.texCoord);
	}

	void setShaderParams_specularGlossiness(Shader & shader, const Material & material, const Scene & scene, const bool hasVertexColors, int & nextTextureUnit)
	{
		// PBR specular glossiness material

		shader.setImmediate("u_hasVertexColors", hasVertexColors ? 1.f : 0.f);

		shader.setImmediate("u_alphaMask", material.alphaMode == "MASK");
		shader.setImmediate("u_alphaMaskCutoff", material.alphaCutoff);

		shader.setImmediate("u_diffuseFactor",
			material.pbrSpecularGlossiness.diffuseFactor.r,
			material.pbrSpecularGlossiness.diffuseFactor.g,
			material.pbrSpecularGlossiness.diffuseFactor.b,
			material.pbrSpecularGlossiness.diffuseFactor.a);
		
		shader.setImmediate("u_specularFactor",
			material.pbrSpecularGlossiness.specularFactor[0],
			material.pbrSpecularGlossiness.specularFactor[1],
			material.pbrSpecularGlossiness.specularFactor[2]);
		shader.setImmediate("u_glossinessFactor",
			material.pbrSpecularGlossiness.glossinessFactor);

		shader.setImmediate("u_emissiveFactor",
			material.emissiveFactor[0],
			material.emissiveFactor[1],
			material.emissiveFactor[2]);

		// set texture maps

		// todo : set texture samplers

		const GxTextureId diffuseTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.diffuseTexture.index);
		const GxTextureId specularGlossinessTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.specularGlossinessTexture.index);
		
		const GxTextureId normalTextureId = tryGetTextureId(scene, material.normalTexture.index);
		const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
		const GxTextureId emissiveTextureId = tryGetTextureId(scene, material.emissiveTexture.index);

		shader.setTexture("diffuseTexture", nextTextureUnit++, diffuseTextureId, true, false);
		shader.setImmediate("diffuseTextureCoord", diffuseTextureId == 0 ? -1 : material.pbrSpecularGlossiness.diffuseTexture.texCoord);
		
		shader.setTexture("specularGlossinessTexture", nextTextureUnit++, specularGlossinessTextureId, true, false);
		shader.setImmediate("specularGlossinessTextureCoord", specularGlossinessTextureId == 0 ? -1 : material.pbrSpecularGlossiness.specularGlossinessTexture.texCoord);
		
		shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		shader.setImmediate("normalTextureCoord", normalTextureId == 0 ? -1 : material.normalTexture.texCoord);
		
		shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		shader.setImmediate("occlusionTextureCoord", occlusionTextureId == 0 ? -1 : material.occlusionTexture.texCoord);
		shader.setImmediate("u_occlusionStrength", material.occlusionTexture.strength);
		
		shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		shader.setImmediate("emissiveTextureCoord", emissiveTextureId == 0 ? -1 : material.emissiveTexture.texCoord);
	}
	
	//
	
	static void drawMeshPrimitive(const Scene & scene, const BufferCache * bufferCache, const MeshPrimitive & primitive, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		GX_PRIMITIVE_TYPE gxPrimitiveType;
		
		if (!translatePrimitiveType((PrimitiveType)primitive.mode, gxPrimitiveType))
		{
			logWarning("primitive type not supported");
			return;
		}
		
		// fetch material

		auto & material =
			primitive.material < 0 || primitive.material >= scene.materials.size()
			? defaultMaterial
			: scene.materials[primitive.material];
		
		// omit material im this pass ?

		const bool isOpaqueMaterial = (material.alphaMode != "BLEND");
		
		if (isOpaquePass != isOpaqueMaterial)
			return;
		
		// determine which shader to use for the material

		Shader * shader_ptr = nullptr;
		
		if (material.pbrSpecularGlossiness.isSet)
		{
			if (materialShaders.pbr_specularGlossiness == nullptr)
				shader_ptr = materialShaders.fallbackShader;
			else
				shader_ptr = materialShaders.pbr_specularGlossiness;
		}
		else
		{
			if (materialShaders.pbr_metallicRoughness == nullptr)
				shader_ptr = materialShaders.fallbackShader;
			else
				shader_ptr = materialShaders.pbr_metallicRoughness;
		}
		
		Shader & shader = *shader_ptr;
		setShader(shader);
	
		// set material parameters

		const bool hasVertexColors = primitive.attributes.count("COLOR_0") != 0;
		
		int nextTextureUnit = 0;

		if (material.pbrSpecularGlossiness.isSet)
			setShaderParams_specularGlossiness(shader, material, scene, hasVertexColors, nextTextureUnit);
		else
			setShaderParams_metallicRoughness(shader, material, scene, hasVertexColors, nextTextureUnit);
		
		// draw

		pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
		{
			do
			{
				if (bufferCache != nullptr)
				{
					// find the cached mesh for this primitive and draw
					
					auto gxMesh_itr = bufferCache->primitives.find(&primitive);
					
					Assert(gxMesh_itr != bufferCache->primitives.end());
					if (gxMesh_itr != bufferCache->primitives.end())
					{
						GxMesh * gxMesh = gxMesh_itr->second;
						gxMesh->draw();
					}
				}
				else
				{
					// draw mesh, without the use of vertex and index buffers
					
					const Accessor * indexAccessor;
					const BufferView * indexBufferView;
					const Buffer * indexBuffer;
					
					if (primitive.indices == -1)
					{
						indexAccessor = nullptr;
					}
					else
					{
						if (!resolveBufferView(scene, primitive.indices, indexAccessor, indexBufferView, indexBuffer))
						{
							logWarning("failed to resolve buffer view");
							break;
						}
						
						if (indexAccessor->componentType != kComponentType_U8 &&
							indexAccessor->componentType != kComponentType_U16 &&
							indexAccessor->componentType != kComponentType_U32)
						{
							logWarning("component type not supported");
							break;
						}
					}
					
					//
					
					const Accessor * positionAccessor;
					const BufferView * positionBufferView;
					const Buffer * positionBuffer;
					
					auto position_itr = primitive.attributes.find("POSITION");
					
					if (position_itr == primitive.attributes.end())
					{
						logWarning("position attribute not found");
						break;
					}
					
					const int positionAccessorIndex = position_itr->second;
					
					if (!resolveBufferView(scene, positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
					{
						logWarning("failed to resolve buffer view");
						break;
					}
					
					if (positionAccessor->type != "VEC3")
					{
						logWarning("position element type not supported");
						break;
					}
					
					//
					
					const Accessor * color0Accessor = nullptr;
					const BufferView * color0BufferView;
					const Buffer * color0Buffer;
					
					auto color0_itr = primitive.attributes.find("COLOR_0");
					int color0_numComponents = 0;
					
					if (color0_itr != primitive.attributes.end())
					{
						const int color0AccessorIndex = color0_itr->second;
						
						if (!resolveBufferView(scene, color0AccessorIndex, color0Accessor, color0BufferView, color0Buffer))
						{
							logWarning("failed to resolve buffer view");
						}
						else if (color0Accessor->type == "VEC3")
							color0_numComponents = 3;
						else if (color0Accessor->type == "VEC4")
							color0_numComponents = 4;
						else
						{
							logWarning("color element type not supported");
							color0Accessor = nullptr;
						}
					}
					
					//
					
					const Accessor * texcoord0Accessor = nullptr;
					const BufferView * texcoord0BufferView;
					const Buffer * texcoord0Buffer;
					
					auto texcoord0_itr = primitive.attributes.find("TEXCOORD_0");
					
					if (texcoord0_itr != primitive.attributes.end())
					{
						const int texcoord0AccessorIndex = texcoord0_itr->second;
						
						if (!resolveBufferView(scene, texcoord0AccessorIndex, texcoord0Accessor, texcoord0BufferView, texcoord0Buffer))
						{
							logWarning("failed to resolve buffer view");
						}
						else if (texcoord0Accessor->type != "VEC2")
						{
							logWarning("texcoord element type not supported");
							texcoord0Accessor = nullptr;
						}
					}
					
					const Accessor * normalAccessor = nullptr;
					const BufferView * normalBufferView;
					const Buffer * normalBuffer;
					
					auto normal_itr = primitive.attributes.find("NORMAL");
					
					if (normal_itr != primitive.attributes.end())
					{
						const int normalAccessorIndex = normal_itr->second;
						
						if (!resolveBufferView(scene, normalAccessorIndex, normalAccessor, normalBufferView, normalBuffer))
						{
							logWarning("failed to resolve buffer view");
						}
						else if (normalAccessor->type != "VEC3")
						{
							logWarning("normal element type not supported");
							normalAccessor = nullptr;
						}
					}
					
					// note : we turn off the occlusion texture here, as it depends on texcoord1, rather than texcoord0. we cannot set texcoord1 using the basic GX api, so to avoid artifacts we just disable the occlusion texture altogether
					shader.setImmediate("occlusionTextureCoord", -1.f);
					
					gxBegin(gxPrimitiveType);
					{
						const int numElements =
							indexAccessor == nullptr
							? positionAccessor->count
							: indexAccessor->count;
						
						for (int i = 0; i < numElements; ++i)
						{
							uint32_t index;
							
							if (indexAccessor == nullptr)
							{
								index = i;
							}
							else
							{
								const uint8_t * __restrict index_mem = indexBuffer->data + indexBufferView->byteOffset + indexAccessor->byteOffset;
								Assert(index_mem < indexBuffer->data + indexBuffer->byteLength);
								
								if (indexAccessor->componentType == kComponentType_U32)
								{
									const uint32_t * __restrict index_ptr = (uint32_t*)index_mem;
									index = index_ptr[i];
								}
								else if (indexAccessor->componentType == kComponentType_U16)
								{
									const uint16_t * __restrict index_ptr = (uint16_t*)index_mem;
									index = index_ptr[i];
								}
								else if (indexAccessor->componentType == kComponentType_U8)
								{
									const uint8_t * __restrict index_ptr = (uint8_t*)index_mem;
									index = index_ptr[i];
								}
								else
								{
									Assert(false);
									continue;
								}
							}
							
							//
							
							if (color0Accessor != nullptr)
							{
								const uint8_t * __restrict color0_mem = color0Buffer->data + color0BufferView->byteOffset + color0Accessor->byteOffset;
								if (color0BufferView->byteStride == 0)
									color0_mem += index * color0_numComponents * sizeof(float);
								else
									color0_mem += index * color0BufferView->byteStride;
								Assert(color0_mem < color0Buffer->data + color0Buffer->byteLength);
								const float * __restrict color0_ptr = (float*)color0_mem;
								
								if (color0_numComponents == 3)
									gxColor4f(color0_ptr[0], color0_ptr[1], color0_ptr[2], 1.f);
								else if (color0_numComponents == 4)
									gxColor4f(color0_ptr[0], color0_ptr[1], color0_ptr[2], color0_ptr[3]);
							}
							
							//
							
							if (texcoord0Accessor != nullptr)
							{
								const uint8_t * __restrict texcoord0_mem = texcoord0Buffer->data + texcoord0BufferView->byteOffset + texcoord0Accessor->byteOffset;
								if (texcoord0BufferView->byteStride == 0)
									texcoord0_mem += index * 2 * sizeof(float);
								else
									texcoord0_mem += index * texcoord0BufferView->byteStride;
								Assert(texcoord0_mem < texcoord0Buffer->data + texcoord0Buffer->byteLength);
								const float * __restrict texcoord0_ptr = (float*)texcoord0_mem;
								
								const float texcoord0_x = texcoord0_ptr[0];
								const float texcoord0_y = texcoord0_ptr[1];
								
								gxTexCoord2f(texcoord0_x, texcoord0_y);
							}
							
							//
							
							if (normalAccessor != nullptr)
							{
								const uint8_t * __restrict normal_mem = normalBuffer->data + normalBufferView->byteOffset + normalAccessor->byteOffset;
								if (normalBufferView->byteStride == 0)
									normal_mem += index * 3 * sizeof(float);
								else
									normal_mem += index * normalBufferView->byteStride;
								Assert(normal_mem < normalBuffer->data + normalBuffer->byteLength);
								const float * __restrict normal_ptr = (float*)normal_mem;
								
								gxNormal3fv(normal_ptr);
							}
							
							//
							
							const uint8_t * __restrict position_mem = positionBuffer->data + positionBufferView->byteOffset + positionAccessor->byteOffset;
							if (positionBufferView->byteStride == 0)
								position_mem += index * 3 * sizeof(float);
							else
								position_mem += index * positionBufferView->byteStride;
							Assert(position_mem < positionBuffer->data + positionBuffer->byteLength);
							const float * __restrict position_ptr = (float*)position_mem;
							
							gxVertex3fv(position_ptr);
						}
					}
					gxEnd();
				}
			}
			while (false);
		}
		popCullMode();
	}

	void drawMesh(const Scene & scene, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		drawMesh(scene, nullptr, mesh, materialShaders, isOpaquePass);
	}
	
	void drawMesh(const Scene & scene, const BufferCache * bufferCache, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		for (auto & primitive : mesh.primitives)
		{
			drawMeshPrimitive(scene, bufferCache, primitive, materialShaders, isOpaquePass);
		}
	}
	
	void drawNodeTraverse(const Scene & scene, const Node & node, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		drawNodeTraverse(scene, nullptr, node, materialShaders, isOpaquePass);
	}
	
	void drawNodeTraverse(const Scene & scene, const BufferCache * bufferCache, const Node & node, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		gxPushMatrix();
		gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
		Mat4x4 rotationMatrix = node.rotation.toMatrix();
		gxMultMatrixf(rotationMatrix.m_v);
		gxScalef(node.scale[0], node.scale[1], node.scale[2]);
		gxMultMatrixf(node.matrix.m_v);
		
		for (auto child_index : node.children)
		{
			if (child_index < 0 || child_index >= scene.nodes.size())
			{
				logWarning("invalid child index");
				continue;
			}
			
			auto & child = scene.nodes[child_index];
			
			drawNodeTraverse(scene, bufferCache, child, materialShaders, isOpaquePass);
		}
		
		if (node.mesh >= 0 && node.mesh < scene.meshes.size())
		{
			auto & mesh = scene.meshes[node.mesh];
			
			drawMesh(scene, bufferCache, mesh, materialShaders, isOpaquePass);
		}
		
		gxPopMatrix();
	}

	void drawNodeMinMaxTraverse(const Scene & scene, const Node & node)
	{
		gxPushMatrix();
		gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
		Mat4x4 rotationMatrix = node.rotation.toMatrix();
		gxMultMatrixf(rotationMatrix.m_v);
		gxScalef(node.scale[0], node.scale[1], node.scale[2]);
		gxMultMatrixf(node.matrix.m_v);
		
		for (auto child_index : node.children)
		{
			if (child_index < 0 || child_index >= scene.nodes.size())
			{
				logWarning("invalid child index");
				continue;
			}
			
			auto & child = scene.nodes[child_index];
			
			drawNodeMinMaxTraverse(scene, child);
		}
		
		if (node.mesh >= 0 && node.mesh < scene.meshes.size())
		{
			auto & mesh = scene.meshes[node.mesh];
			
			for (auto & primitive : mesh.primitives)
			{
				const Accessor * positionAccessor;
				const BufferView * positionBufferView;
				const Buffer * positionBuffer;
				
				auto position_itr = primitive.attributes.find("POSITION");
				
				if (position_itr == primitive.attributes.end())
				{
					logWarning("position attribute not found");
					continue;
				}
				
				const int positionAccessorIndex = position_itr->second;
				
				if (!resolveBufferView(scene, positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
				{
					logWarning("failed to resolve buffer view");
					continue;
				}
				
				if (positionAccessor->type != "VEC3")
				{
					logWarning("position element type not supported");
					continue;
				}
				
				const Vec3 min(
					positionAccessor->min[0],
					positionAccessor->min[1],
					positionAccessor->min[2]);
				const Vec3 max(
					positionAccessor->max[0],
					positionAccessor->max[1],
					positionAccessor->max[2]);
				
				const Vec3 mid = (min + max) / 2.f;
				const Vec3 size = (max - min) / 2.f;
				
				setColor(127, 63, 255);
				lineCube(mid, size);
			}
		}
		
		gxPopMatrix();
	}

	void calculateNodeMinMaxTraverse(const Scene & scene, const Node & node, BoundingBox & boundingBox)
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
		Mat4x4 rotationMatrix = node.rotation.toMatrix();
		gxMultMatrixf(rotationMatrix.m_v);
		gxScalef(node.scale[0], node.scale[1], node.scale[2]);
		gxMultMatrixf(node.matrix.m_v);
		
		for (auto child_index : node.children)
		{
			if (child_index < 0 || child_index >= scene.nodes.size())
			{
				logWarning("invalid child index");
				continue;
			}
			
			auto & child = scene.nodes[child_index];
			
			calculateNodeMinMaxTraverse(scene, child, boundingBox);
		}
		
		if (node.mesh >= 0 && node.mesh < scene.meshes.size())
		{
			auto & mesh = scene.meshes[node.mesh];
			
			for (auto & primitive : mesh.primitives)
			{
				const Accessor * positionAccessor;
				const BufferView * positionBufferView;
				const Buffer * positionBuffer;
				
				auto position_itr = primitive.attributes.find("POSITION");
				
				if (position_itr == primitive.attributes.end())
				{
					logWarning("position attribute not found");
					continue;
				}
				
				const int positionAccessorIndex = position_itr->second;
				
				if (!resolveBufferView(scene, positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
				{
					logWarning("failed to resolve buffer view");
					continue;
				}
				
				if (positionAccessor->type != "VEC3")
				{
					logWarning("position element type not supported");
					continue;
				}
				
				const Vec3 min(
					positionAccessor->min[0],
					positionAccessor->min[1],
					positionAccessor->min[2]);
				const Vec3 max(
					positionAccessor->max[0],
					positionAccessor->max[1],
					positionAccessor->max[2]);
				
				const Vec3 minMax[2] = { min, max };
				
				Mat4x4 nodeToWorld;
				
				gxGetMatrixf(GX_MODELVIEW, nodeToWorld.m_v);
				
				// 1) construct all eight vertices of the bounding box giving by min, max
				// 2) transform the min, max from node space into world-space
				// 3) compare with bounding box min, max and expand bounding box
				
				for (int x = 0; x < 2; ++x)
				{
					for (int y = 0; y < 2; ++y)
					{
						for (int z = 0; z < 2; ++z)
						{
							const Vec3 point_node(
								minMax[x][0],
								minMax[y][1],
								minMax[z][2]);
							
							const Vec3 point_world = nodeToWorld.Mul4(point_node);
							
							if (boundingBox.hasMinMax)
							{
								for (int i = 0; i < 3; ++i)
								{
									boundingBox.min[i] = fminf(boundingBox.min[i], point_world[i]);
									boundingBox.max[i] = fmaxf(boundingBox.max[i], point_world[i]);
								}
							}
							else
							{
								boundingBox.hasMinMax = true;
								boundingBox.min = point_world;
								boundingBox.max = point_world;
							}
						}
					}
				}
			}
		}
		
		gxPopMatrix();
	}
	
	void calculateSceneMinMaxTraverse(const Scene & scene, const int activeScene, BoundingBox & boundingBox)
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		gxLoadIdentity();
		
		for (size_t sceneRootIndex = 0; sceneRootIndex < scene.sceneRoots.size(); ++sceneRootIndex)
		{
			if (activeScene != -1 && activeScene != sceneRootIndex)
				continue;
			
			auto & sceneRoot = scene.sceneRoots[sceneRootIndex];
			
			for (auto & node_index : sceneRoot.nodes)
			{
				if (node_index >= 0 && node_index < scene.nodes.size())
				{
					auto & node = scene.nodes[node_index];
					
					calculateNodeMinMaxTraverse(scene, node, boundingBox);
				}
			}
		}
		
		gxPopMatrix();
	}
	
#if ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE
	static Vec3 calculateMeshPrimitiveCenter(const Scene & scene, const MeshPrimitive & primitive)
	{
		Vec3 result;
		
		const Accessor * positionAccessor;
		const BufferView * positionBufferView;
		const Buffer * positionBuffer;
		
		auto position_itr = primitive.attributes.find("POSITION");

		if (position_itr == primitive.attributes.end())
		{
			logWarning("position attribute not found");
			return result;
		}
		
		const int positionAccessorIndex = position_itr->second;

		if (!resolveBufferView(scene, positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
		{
			logWarning("failed to resolve buffer view");
			return result;
		}

		if (positionAccessor->type != "VEC3")
		{
			logWarning("position element type not supported");
			return result;
		}
		
		for (int i = 0; i < 3; ++i)
		{
			result[i] = (positionAccessor->min[i] + positionAccessor->max[i]) / 2.f;
		}
		
		return result;
	}
#endif
	
	void drawScene(const Scene & scene, const MaterialShaders & materialShaders, const bool isOpaquePass, const int activeScene, const DrawOptions * drawOptions)
	{
		drawScene(scene, nullptr, materialShaders, isOpaquePass, activeScene, drawOptions);
	}
	
	
#if ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE
	static void computeNodeToViewTransformsTraverse(const Scene & scene, const int node_index, Mat4x4 * __restrict nodeToViewTransforms)
	{
		auto & node = scene.nodes[node_index];
		
		gxPushMatrix();
		{
			// apply local node transform
			
			gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
			Mat4x4 rotationMatrix = node.rotation.toMatrix();
			gxMultMatrixf(rotationMatrix.m_v);
			gxScalef(node.scale[0], node.scale[1], node.scale[2]);
			gxMultMatrixf(node.matrix.m_v);
			
			// fetch the global node to view transform
			
			auto & nodeToViewTransform = nodeToViewTransforms[node_index];
			gxGetMatrixf(GX_MODELVIEW, nodeToViewTransform.m_v);
			
			// traverse children
			
			for (auto child_index : node.children)
			{
				if (child_index < 0 || child_index >= scene.nodes.size())
				{
					logWarning("invalid child index");
					continue;
				}
				
				computeNodeToViewTransformsTraverse(scene, child_index, nodeToViewTransforms);
			}
		}
		gxPopMatrix();
	}
#endif
	
	void drawScene(const Scene & scene, const BufferCache * bufferCache, const MaterialShaders & materialShaders, const bool isOpaquePass, const int in_activeScene, const DrawOptions * in_drawOptions)
	{
		const int activeScene =
			in_activeScene == -2
			? scene.activeScene
			: in_activeScene;
		
		const DrawOptions drawOptions =
			in_drawOptions
			? *in_drawOptions
			: DrawOptions();
		
		pushAlphaToCoverage(drawOptions.alphaMode == kAlphaMode_AlphaToCoverage);
		
		for (size_t sceneRootIndex = 0; sceneRootIndex < scene.sceneRoots.size(); ++sceneRootIndex)
		{
			if (activeScene != -1 && activeScene != sceneRootIndex)
				continue;
			
			auto & sceneRoot = scene.sceneRoots[sceneRootIndex];
			
		#if ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE
			if (drawOptions.sortPrimitivesByViewDistance == false)
		#endif
			{
				for (auto & node_index : sceneRoot.nodes)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						auto & node = scene.nodes[node_index];
						
						drawNodeTraverse(scene, bufferCache, node, materialShaders, isOpaquePass);
					}
				}
				
				clearShader();
			}
		#if ENABLE_SORT_PRIMITIVES_BY_VIEW_DISTANCE
			else
			{
				// sort primitives by view distance
				
				// 0. compute node to view transforms
				
				Mat4x4 * nodeToViewTransforms = (Mat4x4*)alloca(scene.nodes.size() * sizeof(Mat4x4));
				
				for (auto & node_index : sceneRoot.nodes)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						computeNodeToViewTransformsTraverse(scene, node_index, nodeToViewTransforms);
					}
				}
				
				// 1. gather a full list of primitives
				
				// 1.1. compute the total number of primitives
				
				int totalNumPrimitives = 0;
				
				std::function<void(int node_index)> count_traverse;
				
				count_traverse = [&](int node_index)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						auto & node = scene.nodes[node_index];
						
						if (node.mesh >= 0 && node.mesh < scene.meshes.size())
						{
							auto & mesh = scene.meshes[node.mesh];
							totalNumPrimitives += mesh.primitives.size();
						}
						
						for (auto & child_index : node.children)
						{
							count_traverse(child_index);
						}
					}
				};
				
				for (auto & node_index : sceneRoot.nodes)
				{
					count_traverse(node_index);
				}
				
				// 1.2. allocate temporary storage for the primitives
				
				const MeshPrimitive ** prims = (const MeshPrimitive**)alloca(totalNumPrimitives * sizeof(MeshPrimitive*));
				Vec3 * primCenters = (Vec3*)alloca(totalNumPrimitives * sizeof(Vec3));
				float * primDistances = (float*)alloca(totalNumPrimitives * sizeof(float));
				int * primNodeIndices = (int*)alloca(totalNumPrimitives * sizeof(int));
				
				// 1.3. fetch primitives into a linear array
				
				int primIndex = 0;
				
				std::function<void(int node_index)> collect_traverse;
				
				collect_traverse = [&](int node_index)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						auto & node = scene.nodes[node_index];
						auto & nodeToViewTransform = nodeToViewTransforms[node_index];
						
						if (node.mesh >= 0 && node.mesh < scene.meshes.size())
						{
							auto & mesh = scene.meshes[node.mesh];
							
							for (auto & prim : mesh.primitives)
							{
								Assert(primIndex < totalNumPrimitives);
								
								prims[primIndex] = &prim;
								primCenters[primIndex] = calculateMeshPrimitiveCenter(scene, prim);
								primDistances[primIndex] = nodeToViewTransform.Mul4(primCenters[primIndex])[2];
								primNodeIndices[primIndex] = node_index;
								
								primIndex++;
							}
						}
						
						for (auto & child_index : node.children)
						{
							collect_traverse(child_index);
						}
					}
				};
				
				for (auto & node_index : sceneRoot.nodes)
				{
					collect_traverse(node_index);
				}
		
				// 2. sort primitives by view distance
				
				// 2.1. create a list of indices for prims, so we can sort prim indices rather than by value
				
				int * sortedPrimIndices = (int*)alloca(totalNumPrimitives * sizeof(int));
				for (int i = 0; i < totalNumPrimitives; ++i)
					sortedPrimIndices[i] = i;
				
				// 2.2 sort prims by view distance
				
				std::sort(sortedPrimIndices, sortedPrimIndices + totalNumPrimitives,
					[&](const int index1, const int index2) -> bool
					{
						const float z1 = primDistances[index1];
						const float z2 = primDistances[index2];
						return z1 > z2;
					});
				
				for (int i = 0; i < totalNumPrimitives; ++i)
				{
					const int primIndex = sortedPrimIndices[i];
					const MeshPrimitive * prim = prims[primIndex];
					const int nodeIndex = primNodeIndices[primIndex];
					const Mat4x4 & nodeToViewTransform = nodeToViewTransforms[nodeIndex];
					
					gxPushMatrix();
					{
						gxSetMatrixf(GX_MODELVIEW, nodeToViewTransform.m_v);
						
						drawMeshPrimitive(scene, bufferCache, *prim, materialShaders, isOpaquePass);
						
					#if false // for debugging: draw a cube at the prim center
						if (isOpaquePass)
						{
							setColor(colorWhite);
							fillCube(primCenters[primIndex], Vec3(.1f, .1f, .1f));
						}
					#endif
					}
					gxPopMatrix();
				}
				
				clearShader();
			}
		#endif
		}
		
		popAlphaToCoverage();
	}
}

//

#include "data/engine/ShaderCommon.txt"
#include "gx_mesh.h"

namespace gltf
{
	bool BufferCache::init(const Scene & scene)
	{
		// create vertex buffers from buffer objects
		
		int bufferIndex = 0;
		
		for (auto & buffer : scene.buffers)
		{
			GxVertexBuffer * vertexBuffer = new GxVertexBuffer();
			vertexBuffer->alloc(buffer.data, buffer.byteLength);
			
			Assert(vertexBuffers[bufferIndex] == nullptr);
			vertexBuffers[bufferIndex++] = vertexBuffer;
		}
		
		Assert(bufferIndex == scene.buffers.size());
		
		// create index buffers and meshes for mesh primitives
		
		for (auto & mesh : scene.meshes)
		{
			for (auto & primitive : mesh.primitives)
			{
				GX_PRIMITIVE_TYPE gxPrimitiveType;
				
				if (!translatePrimitiveType(primitive.mode, gxPrimitiveType))
				{
					logWarning("primitive type not supported");
					continue;
				}
				
				// create mappings between vertex buffers and vertex shaders
				
				int vertexBufferIndex = -1;
				
				GxVertexInput vsInputs[] =
				{
					{ VS_POSITION,      3, GX_ELEMENT_FLOAT32, 0, 0, 12 },
					{ VS_NORMAL,        3, GX_ELEMENT_FLOAT32, 0, 0, 12 },
					{ VS_COLOR,         4, GX_ELEMENT_FLOAT32, 0, 0, 16 },
					{ VS_TEXCOORD0,     2, GX_ELEMENT_FLOAT32, 0, 0,  8 },
					{ VS_TEXCOORD1,     2, GX_ELEMENT_FLOAT32, 0, 0,  8 },
					{ VS_BLEND_INDICES, 4, GX_ELEMENT_UINT8,   0, 0,  4 },
					{ VS_BLEND_WEIGHTS, 4, GX_ELEMENT_UINT8,   1, 0,  4 }
				};
				const int numVsInputs = sizeof(vsInputs) / sizeof(vsInputs[0]);
				
				int numVertices = -1;
				
				for (auto & attribute : primitive.attributes)
				{
					const gltf::Accessor * accessor;
					const gltf::BufferView * bufferView;
					const gltf::Buffer * buffer;
		
					const std::string & attributeName = attribute.first;
					const int accessorIndex = attribute.second;
		
					if (!gltf::resolveBufferView(scene, accessorIndex, accessor, bufferView, buffer))
					{
						logWarning("failed to resolve buffer view");
						continue;
					}
					
					if (vertexBufferIndex == -1)
						vertexBufferIndex = bufferView->buffer;
					else if (bufferView->buffer != vertexBufferIndex)
						vertexBufferIndex = -2;
					
					if (numVertices == -1)
						numVertices = accessor->count;
					else if (numVertices != accessor->count)
						numVertices = -2;
					
					Assert(vertexBufferIndex >= 0);
					Assert(numVertices >= 0);
					if (vertexBufferIndex < 0 || numVertices < 0)
						continue;
		
					/*
					POSITION,
					NORMAL,
					TANGENT,
					TEXCOORD_0,
					TEXCOORD_1,
					COLOR_0,
					JOINS_0, (bone indices)
					WEIGHTS_0
					
					note : bitangent = cross(normal, tangent.xyz) * tangent.w
					*/
					
					const int id =
						attributeName == "POSITION" ? VS_POSITION :
						attributeName == "NORMAL" ? VS_NORMAL :
						attributeName == "COLOR_0" ? VS_COLOR :
						attributeName == "TEXCOORD_0" ? VS_TEXCOORD0 :
						attributeName == "TEXCOORD_1" ? VS_TEXCOORD1 :
						attributeName == "JOINTS_0" ? VS_BLEND_INDICES :
						attributeName == "WEIGHTS_0" ? VS_BLEND_WEIGHTS :
						-1;
					
					if (id == -1)
					{
						// note : usually when we get here it's because of the TANGENT attribute
						//logDebug("unknown attribute: %s", attributeName.c_str());
						continue;
					}
					
					const int numComponents =
						accessor->type == "SCALAR" ? 1 :
						accessor->type == "VEC2" ? 2 :
						accessor->type == "VEC3" ? 3 :
						accessor->type == "VEC4" ? 4 :
						-1;
					
					if (numComponents == -1)
					{
						logWarning("number of components not supported");
						continue;
					}
					
					const GX_ELEMENT_TYPE type =
						accessor->componentType == kComponentType_U8 ? GX_ELEMENT_UINT8 :
						accessor->componentType == kComponentType_U16 ? GX_ELEMENT_UINT16 :
						accessor->componentType == kComponentType_Float32 ? GX_ELEMENT_FLOAT32 :
						(GX_ELEMENT_TYPE)-1;
					
					if (type == (GX_ELEMENT_TYPE)-1)
					{
						logWarning("component type not supported");
						continue;
					}
					
					GxVertexInput * v_ptr = nullptr;
					
					for (int i = 0; i < numVsInputs; ++i)
						if (vsInputs[i].id == id)
							v_ptr = &vsInputs[i];
					
					Assert(v_ptr != nullptr);
					
					GxVertexInput v;
					v.id = id;
					v.numComponents = numComponents;
					v.type = type;
					v.normalize = accessor->normalized;
					v.offset = bufferView->byteOffset + accessor->byteOffset;
					
					if (bufferView->byteStride != 0)
						v.stride = bufferView->byteStride;
					else
					{
						const int bytesPerComponent =
							type == GX_ELEMENT_UINT8 ? 1 :
							type == GX_ELEMENT_UINT16 ? 2 :
							type == GX_ELEMENT_FLOAT32 ? 4 :
							-1;
						
						Assert(bytesPerComponent != -1);
						if (bytesPerComponent == -1)
						{
							logWarning("unknown component type size");
							continue;
						}
						
						v.stride = numComponents * bytesPerComponent;
					}
					
					*v_ptr = v;
				}
				
				if (vertexBufferIndex < 0)
				{
					logWarning("invalid vertex buffer index");
					continue;
				}
				
				if (primitive.indices == -1)
				{
					// create mesh
					
					Assert(vertexBuffers[vertexBufferIndex] != nullptr);
					
					GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
					
					GxMesh * gxMesh = new GxMesh();
					gxMesh->setVertexBuffer(vertexBuffer, vsInputs, numVsInputs, 0);
					gxMesh->addPrim(gxPrimitiveType, numVertices, false);
					
					Assert(primitives[&primitive] == nullptr);
					primitives[&primitive] = gxMesh;
				}
				else
				{
					// create index buffer
					
					GxIndexBuffer * indexBuffer = nullptr;
					
					auto indexBuffer_itr = indexBuffers.find(primitive.indices);
					
					if (indexBuffer_itr != indexBuffers.end())
					{
						indexBuffer = indexBuffer_itr->second;
					}
					else
					{
						const gltf::Accessor * accessor;
						const gltf::BufferView * bufferView;
						const gltf::Buffer * buffer;
						
						if (!gltf::resolveBufferView(scene, primitive.indices, accessor, bufferView, buffer))
						{
							logWarning("failed to resolve buffer view");
							continue;
						}
						else
						{
							if (accessor->componentType != gltf::kComponentType_U8 &&
							 	accessor->componentType != gltf::kComponentType_U16 &&
								accessor->componentType != gltf::kComponentType_U32)
							{
								logWarning("index component type not supported");
								continue;
							}
							
							indexBuffer = new GxIndexBuffer();
							const uint8_t * index_mem = buffer->data + bufferView->byteOffset + accessor->byteOffset;
							Assert(index_mem < buffer->data + buffer->byteLength);
							
							if (accessor->componentType == gltf::kComponentType_U8)
							{
								// U8 index format is not natively supported
								// create a copy of the indices using U16 format
								
								uint16_t * indices = new uint16_t[accessor->count];
								for (int i = 0; i < accessor->count; ++i)
									indices[i] = index_mem[i];
								
								const GX_INDEX_FORMAT format = GX_INDEX_16;
								
								indexBuffer->alloc(indices, accessor->count, format);
								
								delete [] indices;
								indices = nullptr;
							}
							else
							{
								const GX_INDEX_FORMAT format =
									accessor->componentType == gltf::kComponentType_U16
									? GX_INDEX_16
									: GX_INDEX_32;
								
								indexBuffer->alloc(index_mem, accessor->count, format);
							}
							
							Assert(indexBuffers[primitive.indices] == nullptr);
							indexBuffers[primitive.indices] = indexBuffer;
						}
					}
					
					// create mesh
					
					Assert(vertexBuffers[vertexBufferIndex] != nullptr);
					
					GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
					
					GxMesh * gxMesh = new GxMesh();
					gxMesh->setVertexBuffer(vertexBuffer, vsInputs, numVsInputs, 0);
					gxMesh->setIndexBuffer(indexBuffer);
					gxMesh->addPrim(gxPrimitiveType, indexBuffer->getNumIndices(), true);
					
					Assert(primitives[&primitive] == nullptr);
					primitives[&primitive] = gxMesh;
				}
			}
		}
		
		return true;
	}
}
