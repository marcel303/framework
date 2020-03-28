#pragma once

#include "framework.h"
#include "Vec3.h"
#include <vector>

class Mat4x4;

enum LightType
{
	kLightType_Point,
	kLightType_Spot
};

struct Light
{
	LightType type;
	Vec3 position;
	Vec3 direction;
	float attenuationBegin;
	float attenuationEnd;
	float spotAngle;
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
	
	float worldToVolumeScale = 0.f;

public:

	~ForwardLightingHelper();
	
	void addLight(const Light & light);
	void addPointLight(
		Vec3Arg position,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity);
	void addSpotLight(
		Vec3Arg position,
		Vec3Arg direction,
		const float angle,
		const float farDistance,
		Vec3Arg color,
		const float intensity);
	void reset();

	void prepareShaderData(const int resolution, const float extents, const Mat4x4 & worldToView);
	void setShaderData(Shader & shader, int & nextTextureUnit) const;
	
};
