#pragma once

#include "framework.h"
#include "Vec3.h"
#include <vector>

class Mat4x4;

namespace rOne
{
	enum LightType
	{
		kLightType_Point,
		kLightType_Spot,
		kLightType_Directional
	};

	struct Light
	{
		LightType type;
		Vec3 position;
		Vec3 direction;
		float attenuationBegin = 0.f;
		float attenuationEnd = 1.f;
		float spotAngle = 0.f;
		Vec3 color = Vec3(1, 1, 1);
		float intensity = 1.f;
		float userData;
	};

	class ForwardLightingHelper
	{

	public:

		std::vector<Light> lights;
		
		bool isPrepared = false;
		
		ShaderBuffer lightParamsBuffer;
		
		GxTextureId indexTextureId = 0;
		GxTextureId lightIdsTextureId = 0;
		
		float worldToVolumeScale = 0.f;
		bool infiniteSpaceMode = false;

	public:

		~ForwardLightingHelper();
		
		void addLight(const Light & light);
		void addPointLight(
			Vec3Arg position,
			const float attenuationBegin,
			const float attenuationEnd,
			Vec3Arg color,
			const float intensity,
			const float userData = -1.f);
		void addSpotLight(
			Vec3Arg position,
			Vec3Arg direction,
			const float angle,
			const float farDistance,
			Vec3Arg color,
			const float intensity,
			const float userData = -1.f);
		void addDirectionalLight(
			Vec3Arg direction,
			Vec3Arg color,
			const float intensity,
			const float userData = -1.f);
		void reset();

		void prepareShaderData(
			const int resolution,
			const float extents,
			const bool infiniteSpaceMode,
			const Mat4x4 & worldToView);
			
		void setShaderData(Shader & shader, int & nextTextureUnit) const;
		
	};
}
