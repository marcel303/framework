#pragma once

#include "gltf.h"

class Shader;

namespace gltf
{
	void setShaderParams_metallicRoughness(
		Shader & shader,
		const Material & material,
		const Scene & scene,
		const bool hasVertexColors,
		int & nextTextureUnit);

	void setShaderParams_specularGlossiness(
		Shader & shader,
		const Material & material,
		const Scene & scene,
		const bool hasVertexColors,
		int & nextTextureUnit);
}
