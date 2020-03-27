#pragma once

#include "framework.h"
#include "Vec3.h"
#include <vector>

class Mat4x4;

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

public:

	ShaderBuffer lightsParamsBuffer;
	bool isPrepared = false;
	
	std::vector<Light> lights;
	
	GxTextureId indexTextureId = 0;
	GxTextureId lightIdsTextureId = 0;

public:

	~ForwardLightingHelper();
	
	void addLight(const Light & light);
	void addPointLight(
			Vec3Arg position,
			const float attenuationBegin,
			const float attenuationEnd,
			Vec3Arg color,
			const float intensity);
	void reset();

	void prepareShaderData(const Mat4x4 & worldToView);
	void setShaderData(Shader & shader, int & nextTextureUnit) const;
	
};
