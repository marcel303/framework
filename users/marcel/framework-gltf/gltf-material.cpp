#include "gltf.h"
#include "gltf-draw.h" // MaterialShaders
#include "gltf-material.h"

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
	
	//
	
	void MetallicRoughnessParams::init(const Shader & shader)
	{
		// PBR metallic roughness material

		u_hasVertexColors = shader.getImmediateIndex("u_hasVertexColors");

		u_alphaMask = shader.getImmediateIndex("u_alphaMask");
		u_alphaMaskCutoff = shader.getImmediateIndex("u_alphaMaskCutoff");

		u_baseColorFactor = shader.getImmediateIndex("u_baseColorFactor");
		
		u_metallicFactor = shader.getImmediateIndex("u_metallicFactor");
		u_roughnessFactor = shader.getImmediateIndex("u_roughnessFactor");
		
		u_emissiveFactor = shader.getImmediateIndex("u_emissiveFactor");

		// texture maps

		//shader.setTexture("baseColorTexture", nextTextureUnit++, baseColorTextureId, true, false);
		baseColorTextureCoord = shader.getImmediateIndex("baseColorTextureCoord");

		//shader.setTexture("metallicRoughnessTexture", nextTextureUnit++, metallicRoughnessTextureId, true, false);
		metallicRoughnessTextureCoord = shader.getImmediateIndex("metallicRoughnessTextureCoord");
		
		//shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		normalTextureCoord = shader.getImmediateIndex("normalTextureCoord");
	// todo : add normal scale. see GLTF spec
		u_normalTextureScale = shader.getImmediateIndex("u_normalTextureScale");
		
		//shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		occlusionTextureCoord = shader.getImmediateIndex("occlusionTextureCoord");
		u_occlusionStrength = shader.getImmediateIndex("u_occlusionStrength");

		//shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		emissiveTextureCoord = shader.getImmediateIndex("emissiveTextureCoord");
	}
	
	void MetallicRoughnessParams::setShaderParams(
		Shader & shader,
		const Material & material,
		const Scene & scene,
		const bool hasVertexColors,
		int & nextTextureUnit) const
	{
		// PBR metallic roughness material

		if (u_hasVertexColors != -1) shader.setImmediate(u_hasVertexColors, hasVertexColors ? 1.f : 0.f);

		if (u_alphaMask != -1)       shader.setImmediate(u_alphaMask, material.alphaMode == "MASK");
		if (u_alphaMaskCutoff != -1) shader.setImmediate(u_alphaMaskCutoff, material.alphaCutoff);

		if (u_baseColorFactor != -1)
		{
			shader.setImmediate(u_baseColorFactor,
				material.pbrMetallicRoughness.baseColorFactor.r,
				material.pbrMetallicRoughness.baseColorFactor.g,
				material.pbrMetallicRoughness.baseColorFactor.b,
				material.pbrMetallicRoughness.baseColorFactor.a);
		}
		
		if (u_metallicFactor != -1)  shader.setImmediate(u_metallicFactor, material.pbrMetallicRoughness.metallicFactor);
		if (u_roughnessFactor != -1) shader.setImmediate(u_roughnessFactor, material.pbrMetallicRoughness.roughnessFactor);
		
		if (u_emissiveFactor != -1)
		{
			shader.setImmediate(u_emissiveFactor,
				material.emissiveFactor[0],
				material.emissiveFactor[1],
				material.emissiveFactor[2]);
		}
		
		// set texture maps

		// todo : set texture samplers

		const GxTextureId baseColorTextureId         = tryGetTextureId(scene, material.pbrMetallicRoughness.baseColorTexture.index);
		const GxTextureId metallicRoughnessTextureId = tryGetTextureId(scene, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
		
		const GxTextureId normalTextureId    = tryGetTextureId(scene, material.normalTexture.index);
		const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
		const GxTextureId emissiveTextureId  = tryGetTextureId(scene, material.emissiveTexture.index);
	
		shader.setTexture("baseColorTexture", nextTextureUnit++, baseColorTextureId, true, false);
		if (baseColorTextureCoord != -1)
		{
			shader.setImmediate(baseColorTextureCoord,
				baseColorTextureId == 0
				? -1
				: material.pbrMetallicRoughness.baseColorTexture.texCoord);
		}

		shader.setTexture("metallicRoughnessTexture", nextTextureUnit++, metallicRoughnessTextureId, true, false);
		if (metallicRoughnessTextureCoord != -1)
		{
			shader.setImmediate(
				metallicRoughnessTextureCoord,
				metallicRoughnessTextureId == 0
				? -1
				: material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
		}
		
		shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		if (normalTextureCoord != -1)
		{
			shader.setImmediate(
				normalTextureCoord,
				normalTextureId == 0
				? -1
				: material.normalTexture.texCoord);
		}
		if (u_normalTextureScale != -1)
			shader.setImmediate(u_normalTextureScale, material.normalTexture.scale);
		
		shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		if (occlusionTextureCoord != -1)
		{
			shader.setImmediate(
				occlusionTextureCoord,
				occlusionTextureId == 0
				? -1
				: material.occlusionTexture.texCoord);
		}
		if (u_occlusionStrength != -1)
			shader.setImmediate(u_occlusionStrength, material.occlusionTexture.strength);

		shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		if (emissiveTextureCoord != -1)
		{
			shader.setImmediate(
				emissiveTextureCoord,
				emissiveTextureId == 0
				? -1
				: material.emissiveTexture.texCoord);
		}
	}
	
	void MetallicRoughnessParams::setUseVertexColors(Shader & shader, const bool useVertexColors)
	{
		if (u_hasVertexColors != -1)
			shader.setImmediate(u_hasVertexColors, useVertexColors ? 1.f : 0.f);
	}
	
	void MetallicRoughnessParams::setBaseColor(Shader & shader, const Color & color)
	{
		if (u_baseColorFactor != -1)
			shader.setImmediate(u_baseColorFactor, color.r, color.g, color.b, color.a);
	}
	
	void MetallicRoughnessParams::setMetallicRoughness(Shader & shader, const float metallic, const float roughness) const
	{
		if (u_metallicFactor != -1)
			shader.setImmediate(u_metallicFactor, metallic);
		if (u_roughnessFactor != -1)
			shader.setImmediate(u_roughnessFactor, roughness);
	}
	
	void MetallicRoughnessParams::setEmissive(Shader & shader, const float emissive) const
	{
		if (u_emissiveFactor != -1)
			shader.setImmediate(u_emissiveFactor, emissive, emissive, emissive);
	}
	
	void MetallicRoughnessParams::setEmissive(Shader & shader, const Color & emissive) const
	{
		if (u_emissiveFactor != -1)
			shader.setImmediate(u_emissiveFactor, emissive.r, emissive.g, emissive.b);
	}
	
	//
	
	void SpecularGlossinessParams::init(const Shader & shader)
	{
		// PBR specular glossiness material

		u_hasVertexColors = shader.getImmediateIndex("u_hasVertexColors");

		u_alphaMask = shader.getImmediateIndex("u_alphaMask");
		u_alphaMaskCutoff = shader.getImmediateIndex("u_alphaMaskCutoff");

		u_diffuseFactor = shader.getImmediateIndex("u_diffuseFactor");
		
		u_specularFactor = shader.getImmediateIndex("u_specularFactor");
		u_glossinessFactor = shader.getImmediateIndex("u_glossinessFactor");
		
		u_emissiveFactor = shader.getImmediateIndex("u_emissiveFactor");
		
		// texture maps

		//shader.setTexture("diffuseTexture", nextTextureUnit++, diffuseTextureId, true, false);
		diffuseTextureCoord = shader.getImmediateIndex("diffuseTextureCoord");

		//shader.setTexture("specularGlossinessTexture", nextTextureUnit++, specularGlossinessTextureId, true, false);
		specularGlossinessTextureCoord = shader.getImmediateIndex("specularGlossinessTextureCoord");
		
		//shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		normalTextureCoord = shader.getImmediateIndex("normalTextureCoord");
	// todo : add normal scale. see GLTF spec
		u_normalTextureScale = shader.getImmediateIndex("u_normalTextureScale");
		
		//shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		occlusionTextureCoord = shader.getImmediateIndex("occlusionTextureCoord");
		u_occlusionStrength = shader.getImmediateIndex("u_occlusionStrength");

		//shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		emissiveTextureCoord = shader.getImmediateIndex("emissiveTextureCoord");
	}
	
	void SpecularGlossinessParams::setShaderParams(
		Shader & shader,
		const Material & material,
		const Scene & scene,
		const bool hasVertexColors,
		int & nextTextureUnit) const
	{
		// PBR specular glossiness material
		
		if (u_hasVertexColors != -1) shader.setImmediate(u_hasVertexColors, hasVertexColors ? 1.f : 0.f);

		if (u_alphaMask != -1)       shader.setImmediate(u_alphaMask, material.alphaMode == "MASK");
		if (u_alphaMaskCutoff != -1) shader.setImmediate(u_alphaMaskCutoff, material.alphaCutoff);

		if (u_diffuseFactor != -1)
		{
			shader.setImmediate(u_diffuseFactor,
				material.pbrSpecularGlossiness.diffuseFactor.r,
				material.pbrSpecularGlossiness.diffuseFactor.g,
				material.pbrSpecularGlossiness.diffuseFactor.b,
				material.pbrSpecularGlossiness.diffuseFactor.a);
		}
		
		if (u_specularFactor != -1)
		{
			shader.setImmediate(u_specularFactor,
				material.pbrSpecularGlossiness.specularFactor[0],
				material.pbrSpecularGlossiness.specularFactor[1],
				material.pbrSpecularGlossiness.specularFactor[2]);
		}
		
		if (u_glossinessFactor != -1)
			shader.setImmediate(u_glossinessFactor, material.pbrSpecularGlossiness.glossinessFactor);
		
		if (u_emissiveFactor != -1)
		{
			shader.setImmediate(u_emissiveFactor,
				material.emissiveFactor[0],
				material.emissiveFactor[1],
				material.emissiveFactor[2]);
		}

		// set texture maps

		// todo : set texture samplers

		const GxTextureId diffuseTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.diffuseTexture.index);
		const GxTextureId specularGlossinessTextureId = tryGetTextureId(scene, material.pbrSpecularGlossiness.specularGlossinessTexture.index);
		
		const GxTextureId normalTextureId = tryGetTextureId(scene, material.normalTexture.index);
		const GxTextureId occlusionTextureId = tryGetTextureId(scene, material.occlusionTexture.index);
		const GxTextureId emissiveTextureId = tryGetTextureId(scene, material.emissiveTexture.index);

		shader.setTexture("diffuseTexture", nextTextureUnit++, diffuseTextureId, true, false);
		shader.setImmediate(diffuseTextureCoord, diffuseTextureId == 0 ? -1 : material.pbrSpecularGlossiness.diffuseTexture.texCoord);
		
		shader.setTexture("specularGlossinessTexture", nextTextureUnit++, specularGlossinessTextureId, true, false);
		shader.setImmediate(specularGlossinessTextureCoord, specularGlossinessTextureId == 0 ? -1 : material.pbrSpecularGlossiness.specularGlossinessTexture.texCoord);
		
		shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		if (normalTextureCoord != -1)
		{
			shader.setImmediate(
				normalTextureCoord,
				normalTextureId == 0
				? -1
				: material.normalTexture.texCoord);
		}
		if (u_normalTextureScale != -1)
			shader.setImmediate(u_normalTextureScale, material.normalTexture.scale);
		
		shader.setTexture("occlusionTexture", nextTextureUnit++, occlusionTextureId, true, false);
		if (occlusionTextureCoord != -1)
		{
			shader.setImmediate(
				occlusionTextureCoord,
				occlusionTextureId == 0
				? -1
				: material.occlusionTexture.texCoord);
		}
		if (u_occlusionStrength != -1)
			shader.setImmediate(u_occlusionStrength, material.occlusionTexture.strength);

		shader.setTexture("emissiveTexture", nextTextureUnit++, emissiveTextureId, true, false);
		if (emissiveTextureCoord != -1)
		{
			shader.setImmediate(
				emissiveTextureCoord,
				emissiveTextureId == 0
				? -1
				: material.emissiveTexture.texCoord);
		}
	}
	
	void SpecularGlossinessParams::setUseVertexColors(Shader & shader, const bool useVertexColors)
	{
		if (u_hasVertexColors != -1)
			shader.setImmediate(u_hasVertexColors, useVertexColors ? 1.f : 0.f);
	}
	
	void SpecularGlossinessParams::setBaseColor(Shader & shader, const Color & color)
	{
		if (u_diffuseFactor != -1)
			shader.setImmediate(u_diffuseFactor, color.r, color.g, color.b, color.a);
	}
	
	void SpecularGlossinessParams::setSpecularGlossiness(Shader & shader, const Color & specular, const float glossiness) const
	{
		if (u_specularFactor != -1)
			shader.setImmediate(u_specularFactor, specular.r, specular.g, specular.b);
		if (u_glossinessFactor != -1)
			shader.setImmediate(u_glossinessFactor, glossiness);
	}
	
	void SpecularGlossinessParams::setEmissive(Shader & shader, const float emissive) const
	{
		if (u_emissiveFactor != -1)
			shader.setImmediate(u_emissiveFactor, emissive, emissive, emissive);
	}
	
	void SpecularGlossinessParams::setEmissive(Shader & shader, const Color & emissive) const
	{
		if (u_emissiveFactor != -1)
			shader.setImmediate(u_emissiveFactor, emissive.r, emissive.g, emissive.b);
	}
	
	//
	
	void setDefaultMaterialShaders(MaterialShaders & shaders)
	{
		static Shader metallicRoughnessShader("gltf/shaders/pbr-metallicRoughness");
		static Shader specularGlossinessShader("gltf/shaders/pbr-specularGlossiness");
		
		shaders.metallicRoughnessShader = &metallicRoughnessShader;
		shaders.specularGlossinessShader = &specularGlossinessShader;
		shaders.init();
		
		// set max ambient lighting
		setDefaultMaterialLighting(shaders, Mat4x4(true), Vec3(), Vec3(), Vec3(1.f));
	}
	
	void setDefaultMaterialLighting(
		MaterialShaders & materialShaders,
		const Mat4x4 & worldToView,
		const Vec3 & directionalDirection,
		const Vec3 & directionalColor,
		const Vec3 & ambientColor)
	{
		Shader * shaders[2] =
			{
				materialShaders.metallicRoughnessShader,
				materialShaders.specularGlossinessShader
			};
		
		const Vec3 lightDirection_view = worldToView.Mul3(directionalDirection);
		
		for (int i = 0; i < 2; ++i)
		{
			if (shaders[i] != nullptr)
			{
				setShader(*shaders[i]);
				{
					shaders[i]->setImmediate("scene_lightParams1",
						lightDirection_view[0],
						lightDirection_view[1],
						lightDirection_view[2],
						0.f);
				
					shaders[i]->setImmediate("scene_lightParams2",
						directionalColor[0],
						directionalColor[1],
						directionalColor[2], 1.f);
					
					shaders[i]->setImmediate("scene_ambientLightColor",
						ambientColor[0],
						ambientColor[1],
						ambientColor[2]);
				}
				clearShader();
			}
		}
	}
}
