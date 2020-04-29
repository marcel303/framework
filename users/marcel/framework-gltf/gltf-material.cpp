#include "gltf.h"
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

		// set texture maps

		//shader.setTexture("baseColorTexture", nextTextureUnit++, baseColorTextureId, true, false);
		baseColorTextureCoord = shader.getImmediateIndex("baseColorTextureCoord");

		//shader.setTexture("metallicRoughnessTexture", nextTextureUnit++, metallicRoughnessTextureId, true, false);
		metallicRoughnessTextureCoord = shader.getImmediateIndex("metallicRoughnessTextureCoord");
		
		//shader.setTexture("normalTexture", nextTextureUnit++, normalTextureId, true, false);
		normalTextureCoord = shader.getImmediateIndex("normalTextureCoord");
		
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
			shader.setImmediate(u_emissiveFactor, emissive);
	}
	
	//
	
	void SpecularGlossinessParams::init(const Shader & shader)
	{
	}
	
	void SpecularGlossinessParams::setShaderParams(
		Shader & shader,
		const Material & material,
		const Scene & scene,
		const bool hasVertexColors,
		int & nextTextureUnit) const
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
}
