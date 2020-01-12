#include "gltf.h"
#include "gltf-draw.h"

#include "gx_mesh.h"

// todo : remove keyboard controls
// todo : add options struct for controlling how to draw

#define TODO_SCENE_CAMERA 0

namespace gltf
{
	static Material defaultMaterial;
	
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
	
	void drawMesh(const Scene & scene, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		drawMesh(scene, nullptr, mesh, materialShaders, isOpaquePass);
	}
	
	void drawMesh(const Scene & scene, const BufferCache * bufferCache, const Mesh & mesh, const MaterialShaders & materialShaders, const bool isOpaquePass)
	{
		for (auto & primitive : mesh.primitives)
		{
			if (primitive.mode != kPrimitiveType_Triangles)
			{
				logWarning("primitive type not supported");
				continue;
			}
			
			auto & material =
				primitive.material < 0 || primitive.material >= scene.materials.size()
				? defaultMaterial
				: scene.materials[primitive.material];
			
			const bool isOpaqueMaterial =
				material.alphaMode == "OPAQUE" ||
				material.alphaMode == "MASK";
			
			if (isOpaquePass != isOpaqueMaterial)
				continue;
			
			//Assert(material.alphaMode != "MASK"); // todo : implement !
			
			const char * shaderName;
			
			if (material.pbrSpecularGlossiness.isSet)
			{
				if (materialShaders.pbr_specularGlossiness.empty())
					shaderName = materialShaders.fallbackShader.c_str();
				else
					shaderName = materialShaders.pbr_specularGlossiness.c_str();
			}
			else
			{
				if (materialShaders.pbr_metallicRoughness.empty())
					shaderName = materialShaders.fallbackShader.c_str();
				else
					shaderName = materialShaders.pbr_metallicRoughness.c_str();
			}
			
		#if 0
			Shader shader(materialShaders.fallbackShader.c_str());
			setShader(shader);
			shader.setTexture("diffuseTexture", 0, textureId, true, false);
		#else
			Shader shader(shaderName);
			setShader(shader);
			
			if (material.pbrSpecularGlossiness.isSet)
			{
				// PBR specular glossiness material
				const GxTextureId diffuseTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.diffuseTexture.index);
				const GxTextureId specularGlossinessTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.specularGlossinessTexture.index);
				const GxTextureId normalTextureId = tryGetTextureId(scene, material.normalTexture.index);
				const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
				const GxTextureId emissiveTextureId = tryGetTextureId(scene, material.emissiveTexture.index);
				
			#if TODO_SCENE_CAMERA
				shader.setImmediate("scene_camPos",
					camera.position[0],
					camera.position[1],
					camera.position[2]);
			#endif
		
				const float dx = cosf(framework.time);
				const float dz = sinf(framework.time);
		
				shader.setImmediate("scene_lightDir",
					dx,
					-1.f,
					dz);
		
				shader.setTexture("diffuseTexture", 0, diffuseTextureId, true, false);
				shader.setTexture("normalTexture", 1, normalTextureId, true, false);
				shader.setTexture("occlusionTexture", 2, occlusionTextureId, true, false);
				shader.setTexture("specularGlossinessTexture", 3, specularGlossinessTextureId, true, false);
				shader.setTexture("emissiveTexture", 4, emissiveTextureId, true, false);

				shader.setImmediate("material_diffuseFactor",
					material.pbrSpecularGlossiness.diffuseFactor.r,
					material.pbrSpecularGlossiness.diffuseFactor.g,
					material.pbrSpecularGlossiness.diffuseFactor.b,
					material.pbrSpecularGlossiness.diffuseFactor.a);
				shader.setImmediate("material_hasDiffuseTexture", diffuseTextureId != 0);
				shader.setImmediate("material_hasSpecularGlossinessTexture", specularGlossinessTextureId != 0);
				shader.setImmediate("material_glossinessFactor",
					material.pbrSpecularGlossiness.glossinessFactor);
				shader.setImmediate("material_specularFactor",
					material.pbrSpecularGlossiness.specularFactor[0],
					material.pbrSpecularGlossiness.specularFactor[1],
					material.pbrSpecularGlossiness.specularFactor[2]);
		
				shader.setImmediate("material_hasNormalTexture", normalTextureId != 0);
				shader.setImmediate("material_occlusionStrength", material.occlusionTexture.strength);
				shader.setImmediate("material_hasOcclusionTexture", occlusionTextureId != 0);
				shader.setImmediate("material_hasEmissiveTexture", emissiveTextureId != 0);
				shader.setImmediate("material_emissiveFactor",
					material.emissiveFactor[0],
					material.emissiveFactor[1],
					material.emissiveFactor[2]);
				shader.setImmediate("material_alphaMask", false);
				shader.setImmediate("material_alphaMaskCutoff", 0.f);
			}
			else
			{
				// PBR metallic roughness material
				const GxTextureId textureId = tryGetTextureId(scene, material.pbrMetallicRoughness.baseColorTexture.index);
				const GxTextureId metallicRoughnessTextureId = tryGetTextureId(scene, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
				const GxTextureId normalTextureId = tryGetTextureId(scene, material.normalTexture.index);
				const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
				const GxTextureId emissiveTextureId = tryGetTextureId(scene, material.emissiveTexture.index);
				
			#if TODO_SCENE_CAMERA
				shader.setImmediate("scene_camPos",
					camera.position[0],
					camera.position[1],
					camera.position[2]);
			#endif
		
			/*
				shader.setImmediate("scene_lightDir",
					.5f,
					1.f,
					.5f);
			*/
			
				shader.setTexture("baseColorTexture", 0, textureId, true, false);
				shader.setTexture("normalTexture", 1, normalTextureId, true, false);
				shader.setTexture("occlusionTexture", 2, occlusionTextureId, true, false);
				shader.setTexture("metallicRoughnessTexture", 3, metallicRoughnessTextureId, true, false);
				shader.setTexture("emissiveTexture", 4, emissiveTextureId, true, false);

				shader.setImmediate("material_baseColorFactor",
					material.pbrMetallicRoughness.baseColorFactor.r,
					material.pbrMetallicRoughness.baseColorFactor.g,
					material.pbrMetallicRoughness.baseColorFactor.b,
					material.pbrMetallicRoughness.baseColorFactor.a);
				shader.setImmediate("material_hasBaseColorTexture", textureId != 0);
				shader.setImmediate("material_hasMetallicRoughnessTexture", metallicRoughnessTextureId != 0);
				shader.setImmediate("material_hasNormalTexture", normalTextureId != 0);
				shader.setImmediate("material_occlusionStrength", material.occlusionTexture.strength);
				shader.setImmediate("material_hasOcclusionTexture", occlusionTextureId != 0);
				shader.setImmediate("material_hasEmissiveTexture", emissiveTextureId != 0);
				shader.setImmediate("material_metallicFactor",
					material.pbrMetallicRoughness.metallicFactor);
				shader.setImmediate("material_roughnessFactor",
					material.pbrMetallicRoughness.roughnessFactor);
				shader.setImmediate("material_emissiveFactor",
					material.emissiveFactor[0],
					material.emissiveFactor[1],
					material.emissiveFactor[2]);
				shader.setImmediate("material_alphaMask", material.alphaMode == "MASK");
				shader.setImmediate("material_alphaMaskCutoff", material.alphaCutoff);
			}
		#endif
		
			pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
			{
				if (bufferCache != nullptr)
				{
					auto gxMesh_itr = bufferCache->meshes.find(&mesh);
					
					if (gxMesh_itr != bufferCache->meshes.end())
					{
						GxMesh * gxMesh = gxMesh_itr->second;
						gxMesh->draw(GX_TRIANGLES);
					}
				}
				else
				{
					// draw mesh, without the use of vertex and index buffers
					
					const Accessor * indexAccessor;
					const BufferView * indexBufferView;
					const Buffer * indexBuffer;
					
					if (!resolveBufferView(scene, primitive.indices, indexAccessor, indexBufferView, indexBuffer))
					{
						logWarning("failed to resolve buffer view");
						continue;
					}
					
					if (indexAccessor->componentType != kElementType_U16 &&
						indexAccessor->componentType != kElementType_U32)
					{
						logWarning("component type not supported");
						continue;
					}
					
					//
					
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
					
					//
					
					const Accessor * color0Accessor = nullptr;
					const BufferView * color0BufferView;
					const Buffer * color0Buffer;
					
					auto color0_itr = primitive.attributes.find("COLOR_0");
					
					if (color0_itr != primitive.attributes.end())
					{
						const int color0AccessorIndex = color0_itr->second;
						
						if (!resolveBufferView(scene, color0AccessorIndex, color0Accessor, color0BufferView, color0Buffer))
						{
							logWarning("failed to resolve buffer view");
						}
						else if (color0Accessor->type != "VEC3")
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
					
					//
					
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
					
					gxBegin(GX_TRIANGLES);
					{
						for (int i = 0; i < indexAccessor->count; ++i)
						{
							const uint8_t * __restrict index_mem = indexBuffer->data + indexBufferView->byteOffset + indexAccessor->byteOffset;
							Assert(index_mem < indexBuffer->data + indexBuffer->byteLength);
							
							uint32_t index;
							
							if (indexAccessor->componentType == kElementType_U32)
							{
								const uint32_t * __restrict index_ptr = (uint32_t*)index_mem;
								index = index_ptr[i];
							}
							else if (indexAccessor->componentType == kElementType_U16)
							{
								const uint16_t * __restrict index_ptr = (uint16_t*)index_mem;
								index = index_ptr[i];
							}
							else
							{
								Assert(false);
								continue;
							}
							
							//
							
							if (color0Accessor != nullptr)
							{
								const uint8_t * __restrict color0_mem = color0Buffer->data + color0BufferView->byteOffset + color0Accessor->byteOffset;
								color0_mem += index * 3 * sizeof(float);
								Assert(color0_mem < color0Buffer->data + color0Buffer->byteLength);
								const float * __restrict color0_ptr = (float*)color0_mem;
								
								gxColor4f(color0_ptr[0], color0_ptr[1], color0_ptr[2], 1.f);
							}
							
							//
							
							if (texcoord0Accessor != nullptr)
							{
								const uint8_t * __restrict texcoord0_mem = texcoord0Buffer->data + texcoord0BufferView->byteOffset + texcoord0Accessor->byteOffset;
								texcoord0_mem += index * 2 * sizeof(float);
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
								normal_mem += index * 3 * sizeof(float);
								Assert(normal_mem < normalBuffer->data + normalBuffer->byteLength);
								const float * __restrict normal_ptr = (float*)normal_mem;
								
								gxNormal3fv(normal_ptr);
							}
							
							//
							
							const uint8_t * __restrict position_mem = positionBuffer->data + positionBufferView->byteOffset + positionAccessor->byteOffset;
							position_mem += index * 3 * sizeof(float);
							Assert(position_mem < positionBuffer->data + positionBuffer->byteLength);
							const float * __restrict position_ptr = (float*)position_mem;
							
							gxVertex3fv(position_ptr);
						}
					}
					gxEnd();
					
					gxSetTexture(0);
				}
			}
			popCullMode();
			clearShader();
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
	
	void drawScene(const Scene & scene, const MaterialShaders & materialShaders, const bool isOpaquePass, const int activeScene)
	{
		drawScene(scene, nullptr, materialShaders, isOpaquePass, activeScene);
	}
	
	void drawScene(const Scene & scene, const BufferCache * bufferCache, const MaterialShaders & materialShaders, const bool isOpaquePass, const int in_activeScene)
	{
		const int activeScene =
			in_activeScene < 0
			? scene.activeScene
			: in_activeScene;
		
		if (activeScene >= 0 && activeScene < scene.sceneRoots.size())
		{
			auto & sceneRoot = scene.sceneRoots[scene.activeScene];
			
			for (auto & node_index : sceneRoot.nodes)
			{
				if (node_index >= 0 && node_index < scene.nodes.size())
				{
					auto & node = scene.nodes[node_index];
					
					drawNodeTraverse(scene, bufferCache, node, materialShaders, isOpaquePass);
				}
			}
		}
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
				GxIndexBuffer * indexBuffer = nullptr;
				
				// create index buffers
				
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
					}
					else
					{
						if (accessor->componentType != gltf::kElementType_U16 &&
							accessor->componentType != gltf::kElementType_U32)
						{
							logWarning("index element type not supported");
							continue;
						}
						
						indexBuffer = new GxIndexBuffer();
						const uint8_t * index_mem = buffer->data + bufferView->byteOffset + accessor->byteOffset;
						Assert(index_mem < buffer->data + buffer->byteLength);
						
						const GX_INDEX_FORMAT format =
							accessor->componentType == gltf::kElementType_U16
							? GX_INDEX_16
							: GX_INDEX_32;
						
						indexBuffer->alloc(index_mem, accessor->count, format);
						
						Assert(indexBuffers[primitive.indices] == nullptr);
						indexBuffers[primitive.indices] = indexBuffer;
					}
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
						accessor->type == "SCALAR" ? GX_ELEMENT_FLOAT32 :
						accessor->type == "VEC2" ? GX_ELEMENT_FLOAT32 :
						accessor->type == "VEC3" ? GX_ELEMENT_FLOAT32 :
						accessor->type == "VEC4" ? GX_ELEMENT_FLOAT32 :
						(GX_ELEMENT_TYPE)-1;
					
					if (type == (GX_ELEMENT_TYPE)-1)
					{
						logWarning("element type not supported");
						continue;
					}
					
					GxVertexInput * v_ptr = nullptr;
					
					for (int i = 0; i < numVsInputs; ++i)
						if (vsInputs[i].id == id)
							v_ptr = &vsInputs[i];
					
					GxVertexInput & v = *v_ptr;
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
				}
				
				if (vertexBufferIndex < 0)
				{
					logWarning("invalid vertex buffer index");
					continue;
				}
				
				// create mesh
				
				Assert(vertexBuffers[vertexBufferIndex] != nullptr);
				
				GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
				
				GxMesh * gxMesh = new GxMesh();
				gxMesh->setVertexBuffer(vertexBuffer, vsInputs, numVsInputs, 0);
				gxMesh->setIndexBuffer(indexBuffer);
				
				Assert(meshes[&mesh] == nullptr);
				meshes[&mesh] = gxMesh;
			}
		}
		
		return true;
	}
}
