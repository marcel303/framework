#pragma once

#include "Vec3.h"
#include <vector>

class Mat4x4;
class Shader;
class ShaderBuffer;

enum LightType
{
	kLightType_Point
};

struct Light
{
	LightType type;
	Vec3 position;
	float attenuationBegin;
	float attenuationEnd;
	Vec3 color;
};

class ForwardLightingHelper
{

private:

	ShaderBuffer * lightsParamsBuffer = nullptr;
	
	std::vector<Light> lights;

public:

	void addLight(const Light & light);
	void addPointLight(
			Vec3Arg position,
			const float attenuationBegin,
			const float attenuationEnd,
			Vec3Arg color,
			const float intensity);
	void reset();

	void prepareShaderData(const Mat4x4 & worldToView);
	void setShaderData(Shader & shader) const;
	
};
