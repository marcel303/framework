#include "gltf.h"
#include "gltf-draw.h"

#define SHADER_METALLIC_ROUGHNESS 1
#define SHADER_SPECULAR_GLOSSINESS 0

namespace gltf
{
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
	
	void drawMesh(const Scene & scene, const Mesh & mesh, const bool isOpaquePass)
	{
		for (auto & primitive : mesh.primitives)
		{
			if (primitive.mode != kPrimitiveType_Triangles)
			{
				logWarning("primitive type not supported");
				continue;
			}
			
			if (primitive.material < 0 || primitive.material >= scene.materials.size())
			{
				logWarning("invalid material index");
				continue;
			}
			
			auto & material = scene.materials[primitive.material];
			
			//Assert(material.alphaMode != "MASK"); // todo : implement !
			
			const BLEND_MODE blendMode =
				material.alphaMode == "OPAQUE" || material.alphaMode == "MASK"
				? BLEND_OPAQUE
				: BLEND_ALPHA;
			
		#if SHADER_METALLIC_ROUGHNESS
			// PBR metallic roughness material
			const GxTextureId textureId = keyboard.isDown(SDLK_1) ? 0 : tryGetTextureId(scene, material.pbrMetallicRoughness.baseColorTexture.index);
			const GxTextureId metallicRoughnessTextureId = keyboard.isDown(SDLK_2) ? 0 : tryGetTextureId(scene, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
			const GxTextureId normalTextureId = keyboard.isDown(SDLK_3) ? 0 : tryGetTextureId(scene, material.normalTexture.index);
			const GxTextureId occlusionTextureId = keyboard.isDown(SDLK_4) ? 0 : tryGetTextureId(scene, material.occlusionTexture.index);
			const GxTextureId emissiveTextureId = keyboard.isDown(SDLK_5) ? 0 : tryGetTextureId(scene, material.emissiveTexture.index);
		#endif
		
		#if SHADER_SPECULAR_GLOSSINESS
			// PBR specular glossiness material
			const GxTextureId diffuseTextureId = keyboard.isDown(SDLK_1) ? 0 : tryGetTextureId(scene, material.pbrSpecularGlossiness.diffuseTexture.index);
			const GxTextureId specularGlossinessTextureId = keyboard.isDown(SDLK_2) ? 0 : tryGetTextureId(scene, material.pbrSpecularGlossiness.specularGlossinessTexture.index);
			const GxTextureId normalTextureId = keyboard.isDown(SDLK_3) ? 0 : tryGetTextureId(scene, material.normalTexture.index);
			const GxTextureId occlusionTextureId = keyboard.isDown(SDLK_4) ? 0 : tryGetTextureId(scene, material.occlusionTexture.index);
			const GxTextureId emissiveTextureId = keyboard.isDown(SDLK_5) ? 0 : tryGetTextureId(scene, material.emissiveTexture.index);
		#endif
		
			Color color = colorWhite;
			
			if (!keyboard.isDown(SDLK_u))
				color = material.pbrMetallicRoughness.baseColorFactor;
			
			const bool isOpaqueMaterial = (blendMode == BLEND_OPAQUE);
			
			if (isOpaquePass != isOpaqueMaterial)
				continue;
			
			setBlend(blendMode);
			
		#if 0 // todo : enable accelerated code path for all GX implementations
			if (!keyboard.isDown(SDLK_m))
			{
				auto gxMesh_itr = meshes.find(&mesh);
				
				if (gxMesh_itr != meshes.end())
				{
					GxMesh * gxMesh = gxMesh_itr->second;
					
				#if 0
					Shader shader("shader");
					setShader(shader);
					
					shader.setTexture("source", 0, textureId);
					shader.setImmediate("color", color.r, color.g, color.b, color.a);
					shader.setImmediate("params", textureId != 0, 0.f, 0.f, 0.f);
					shader.setImmediate("time", framework.time);
				#endif
				
				#if SHADER_SPECULAR_GLOSSINESS
					Shader shader("shader-pbr-specularGlossiness");
					setShader(shader);
					
					shader.setImmediate("scene_camPos",
						camera.position[0],
						camera.position[1],
						camera.position[2]);
					
					const float dx = cosf(framework.time);
					const float dz = sinf(framework.time);
					
					shader.setImmediate("scene_lightDir",
						dx,
						-1.f,
						dz);
					
					shader.setTexture("diffuseTexture", 0, diffuseTextureId);
					shader.setTexture("normalTexture", 1, normalTextureId);
					shader.setTexture("occlusionTexture", 2, occlusionTextureId);
					shader.setTexture("specularGlossinessTexture", 3, specularGlossinessTextureId);
					shader.setTexture("emissiveTexture", 4, emissiveTextureId);

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
				#endif
				
				#if SHADER_METALLIC_ROUGHNESS
					Shader shader("shader-pbr");
					setShader(shader);
					
					shader.setImmediate("scene_camPos",
						camera.position[0],
						camera.position[1],
						camera.position[2]);
					
					shader.setImmediate("scene_lightDir",
						.5f,
						1.f,
						.5f);
					
					shader.setTexture("baseColorTexture", 0, textureId);
					shader.setTexture("normalTexture", 1, normalTextureId);
					shader.setTexture("occlusionTexture", 2, occlusionTextureId);
					shader.setTexture("metallicRoughnessTexture", 3, metallicRoughnessTextureId);
					shader.setTexture("emissiveTexture", 4, emissiveTextureId);

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
				#endif
			
					setShader(shader);
					{
						pushWireframe(keyboard.isDown(SDLK_w));
						pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
						gxMesh->draw(GX_TRIANGLES);
						popCullMode();
						popWireframe();
					}
					clearShader();
				}
				
				continue;
			}
		#endif
		
		#if SHADER_METALLIC_ROUGHNESS
			gxSetTexture(textureId);
		#endif
		#if SHADER_SPECULAR_GLOSSINESS
			gxSetTexture(diffuseTextureId);
		#endif
			
			setColor(color);
			
			// draw mesh, without the use of vertex and index buffers
			
			const Accessor * indexAccessor;
			const BufferView * indexBufferView;
			const Buffer * indexBuffer;
			
			if (!resolveBufferView(scene, primitive.indices, indexAccessor, indexBufferView, indexBuffer))
			{
				logWarning("failed to resolve buffer view");
				continue;
			}
			
			if (indexAccessor->componentType != kElementType_U32)
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
					continue;
				}
				
				if (texcoord0Accessor->type != "VEC2")
				{
					logWarning("texcoord element type not supported");
					continue;
				}
			}
			
			pushWireframe(keyboard.isDown(SDLK_w));
			pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
			gxBegin(GX_TRIANGLES);
			{
				for (int i = 0; i < indexAccessor->count; ++i)
				{
					const uint8_t * index_mem = &indexBuffer->data.front() + indexBufferView->byteOffset + indexAccessor->byteOffset;
					Assert(index_mem < &indexBuffer->data.front() + indexBuffer->byteLength);
					const uint32_t * index_ptr = (uint32_t*)index_mem;
					const uint32_t index = index_ptr[i];
					
					//
					
					const uint8_t * position_mem = &positionBuffer->data.front() + positionBufferView->byteOffset + positionAccessor->byteOffset;
					position_mem += index * 3 * sizeof(float);
					Assert(position_mem < &positionBuffer->data.front() + positionBuffer->byteLength);
					const float * position_ptr = (float*)position_mem;
					
					const float position_x = position_ptr[0];
					const float position_y = position_ptr[1];
					const float position_z = position_ptr[2];
					
					//
					
					if (texcoord0Accessor != nullptr)
					{
						const uint8_t * texcoord0_mem = &texcoord0Buffer->data.front() + texcoord0BufferView->byteOffset + texcoord0Accessor->byteOffset;
						texcoord0_mem += index * 2 * sizeof(float);
						Assert(texcoord0_mem < &texcoord0Buffer->data.front() + texcoord0Buffer->byteLength);
						const float * texcoord0_ptr = (float*)texcoord0_mem;
						
						const float texcoord0_x = texcoord0_ptr[0];
						const float texcoord0_y = texcoord0_ptr[1];
						
						gxTexCoord2f(texcoord0_x, texcoord0_y);
					}
					
					//
					
					gxVertex3f(position_x, position_y, position_z);
				}
			}
			gxEnd();
			popCullMode();
			popWireframe();
			
			gxSetTexture(0);
		}
	}

	void drawNodeTraverse(const Scene & scene, const Node & node, const bool isOpaquePass)
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
			
			drawNodeTraverse(scene, child, isOpaquePass);
		}
		
		if (node.mesh >= 0 && node.mesh < scene.meshes.size())
		{
			auto & mesh = scene.meshes[node.mesh];
			
			drawMesh(scene, mesh, isOpaquePass);
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
}
