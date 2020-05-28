#pragma once

#include "framework.h"

namespace gltf
{
	struct Material;
	struct MaterialShaders;
	struct Scene;
	
	struct MetallicRoughnessParams
	{
		GxImmediateIndex u_hasVertexColors = -1;
		
		GxImmediateIndex u_alphaMask = -1;
		GxImmediateIndex u_alphaMaskCutoff = -1;

		GxImmediateIndex u_baseColorFactor = -1;

		GxImmediateIndex u_metallicFactor = -1;
		GxImmediateIndex u_roughnessFactor = -1;
	
		GxImmediateIndex u_emissiveFactor = -1;

		GxImmediateIndex baseColorTexture = -1;
		GxImmediateIndex baseColorTextureCoord = -1;

		GxImmediateIndex metallicRoughnessTexture = -1;
		GxImmediateIndex metallicRoughnessTextureCoord = -1;
	
		GxImmediateIndex normalTexture = -1;
		GxImmediateIndex normalTextureCoord = -1;
		GxImmediateIndex u_normalTextureScale = -1;
	
		GxImmediateIndex occlusionTexture = -1;
		GxImmediateIndex occlusionTextureCoord = -1;
		GxImmediateIndex u_occlusionStrength = -1;

		GxImmediateIndex emissiveTexture = -1;
		GxImmediateIndex emissiveTextureCoord = -1;
		
		void init(const Shader & shader);
		
		void setShaderParams(
			Shader & shader,
			const Material & material,
			const Scene & scene,
			const bool hasVertexColors,
			int & nextTextureUnit) const;
		
		void setUseVertexColors(Shader & shader, const bool useVertexColors) const;
		void setBaseColor(Shader & shader, const Color & color) const;
		void setBaseColorTexture(Shader & shader, const GxTextureId textureId, const int texcoordIndex, int & nextTextureUnit) const;
		void setMetallicRoughness(Shader & shader, const float metallic, const float roughness) const;
		void setMetallicRoughnessTexture(Shader & shader, const GxTextureId textureId, const int texcoordIndex, int & nextTextureUnit) const;
		void setEmissive(Shader & shader, const float emissive) const;
		void setEmissive(Shader & shader, const Color & emissive) const;
	};
	
	struct SpecularGlossinessParams
	{
		GxImmediateIndex u_hasVertexColors = -1;

		GxImmediateIndex u_alphaMask = -1;
		GxImmediateIndex u_alphaMaskCutoff = -1;

		GxImmediateIndex u_diffuseFactor = -1;
		
		GxImmediateIndex u_specularFactor = -1;
		GxImmediateIndex u_glossinessFactor = -1;

		GxImmediateIndex u_emissiveFactor = -1;

		GxImmediateIndex diffuseTexture = -1;
		GxImmediateIndex diffuseTextureCoord = -1;
		
		GxImmediateIndex specularGlossinessTexture = -1;
		GxImmediateIndex specularGlossinessTextureCoord = -1;
		
		GxImmediateIndex normalTexture = -1;
		GxImmediateIndex normalTextureCoord = -1;
		GxImmediateIndex u_normalTextureScale = -1;
		
		GxImmediateIndex occlusionTexture = -1;
		GxImmediateIndex occlusionTextureCoord = -1;
		GxImmediateIndex u_occlusionStrength = -1;
		
		GxImmediateIndex emissiveTexture = -1;
		GxImmediateIndex emissiveTextureCoord = -1;
		
		void init(const Shader & shader);
		
		void setShaderParams(
			Shader & shader,
			const Material & material,
			const Scene & scene,
			const bool hasVertexColors,
			int & nextTextureUnit) const;
		
		void setUseVertexColors(Shader & shader, const bool useVertexColors);
		void setBaseColor(Shader & shader, const Color & color);
		void setSpecularGlossiness(Shader & shader, const Color & specular, const float glossiness) const;
		void setEmissive(Shader & shader, const float emissive) const;
		void setEmissive(Shader & shader, const Color & emissive) const;
	};
	
	// functions for working with the default shaders provided by the GLTF library
	// note that you'd normally would want to use the provided material shader includes
	// to create your own shaders with custom forward shading etc. but having some
	// built-in shaders comes in handy sometimes also,
	
	void setDefaultMaterialShaders(MaterialShaders & shaders);
	void setDefaultMaterialLighting(
		Shader & shader,
		const Mat4x4 & worldToView,
		const Vec3 & directionalDirection = Vec3(0, -1, 0),
		const Vec3 & directionalColor = Vec3(1.f),
		const Vec3 & ambientColor = Vec3());
	void setDefaultMaterialLighting(
		MaterialShaders & shaders,
		const Mat4x4 & worldToView,
		const Vec3 & directionalDirection = Vec3(0, -1, 0),
		const Vec3 & directionalColor = Vec3(1.f),
		const Vec3 & ambientColor = Vec3());
}
