#include "forwardLighting.h"
#include "lightVolumeBuilder.h"
#include "srgbFunctions.h"

#include "framework.h"
#include <algorithm>
#include <math.h>

namespace rOne
{
	static const int kMaxLights = 1024; // due to a limitation with desktop Metal of 64kb max constant buffer memory, this is limited to 1024 only
	
	ForwardLightingHelper::ForwardLightingHelper()
	{
		lightParamsBuffer.alloc(kMaxLights * 4*sizeof(Vec4));
		lightExtrasBuffer.alloc(kMaxLights * 4*sizeof(Vec4));
	}
	
	ForwardLightingHelper::~ForwardLightingHelper()
	{
		reset();
		
		lightParamsBuffer.free();
		lightExtrasBuffer.free();
	}

	void ForwardLightingHelper::addLight(const Light & light)
	{
		lights.push_back(light);
	}

	void ForwardLightingHelper::addPointLight(
		Vec3Arg position,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		Light light;
		light.type = kLightType_Point;
		light.position = position;
		light.attenuationBegin = attenuationBegin;
		light.attenuationEnd = attenuationEnd;
		light.color = srgbToLinear(color) * intensity;
		light.userData = userData;

		addLight(light);
	}

	void ForwardLightingHelper::addSpotLight(
		Vec3Arg position,
		Vec3Arg direction,
		const float angle,
		const float farDistance,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized");
		AssertMsg(angle >= 0.f && angle <= float(M_PI), "spot angle must be between 0 and 180 degrees");
		
		Light light;
		light.type = kLightType_Spot;
		light.position = position;
		light.direction = direction;
		light.attenuationBegin = 0.f;
		light.attenuationEnd = farDistance;
		light.spotAngle = angle;
		light.color = srgbToLinear(color) * intensity;
		light.userData = userData;

		addLight(light);
	}
	
	void ForwardLightingHelper::addDirectionalLight(
		Vec3Arg direction,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized");
		
		Light light;
		light.type = kLightType_Directional;
		light.direction = direction;
		light.color = srgbToLinear(color) * intensity;
		light.isGlobalLight = true;
		light.userData = userData;

		addLight(light);
	}
	
	void ForwardLightingHelper::addAreaLight(
		const LightType lightType,
		const Mat4x4 & transform,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		Assert(
			lightType == kLightType_AreaBox ||
			lightType == kLightType_AreaSphere ||
			lightType == kLightType_AreaRect ||
			lightType == kLightType_AreaCircle);
		
		Light light;
		light.type = lightType;
		light.transform = transform;
		light.attenuationBegin = attenuationBegin;
		light.attenuationEnd = attenuationEnd;
		light.color = srgbToLinear(color) * intensity;
		light.userData = userData;
		
		addLight(light);
	}
	
	void ForwardLightingHelper::addAreaBoxLight(
		const Mat4x4 & transform,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		addAreaLight(
			kLightType_AreaBox,
			transform,
			attenuationBegin,
			attenuationEnd,
			color,
			intensity,
			userData);
	}
	
	void ForwardLightingHelper::addAreaSphereLight(
		const Mat4x4 & transform,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		addAreaLight(
			kLightType_AreaSphere,
			transform,
			attenuationBegin,
			attenuationEnd,
			color,
			intensity,
			userData);
	}
	
	void ForwardLightingHelper::addAreaRectLight(
		const Mat4x4 & transform,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		addAreaLight(
			kLightType_AreaRect,
			transform,
			attenuationBegin,
			attenuationEnd,
			color,
			intensity,
			userData);
	}
	
	void ForwardLightingHelper::addAreaCircleLight(
		const Mat4x4 & transform,
		const float attenuationBegin,
		const float attenuationEnd,
		Vec3Arg color,
		const float intensity,
		const float userData)
	{
		addAreaLight(
			kLightType_AreaCircle,
			transform,
			attenuationBegin,
			attenuationEnd,
			color,
			intensity,
			userData);
	}

	void ForwardLightingHelper::reset()
	{
		lights.clear();
		
		freeTexture(indexTextureId);
		freeTexture(lightIdsTextureId);
		
		numGlobalLights = 0;
		
		isPrepared = false;
	}

	void ForwardLightingHelper::prepareShaderData(
		const int resolution,
		const float extents,
		const bool in_infiniteSpaceMode,
		const Mat4x4 & worldToView)
	{
		AssertMsg(isPrepared == false, "please call reset() when done rendering a frame");
		
		infiniteSpaceMode = in_infiniteSpaceMode;
		
		// sort lights so the global lights are at the beginning, and calculate the number of global lights
		
		std::sort(lights.begin(), lights.end(),
			[](const Light & light1, const Light & light2)
			{
				const bool isGlobal1 = light1.isGlobalLight;
				const bool isGlobal2 = light2.isGlobalLight;
				return isGlobal1 > isGlobal2;
			});
		
		Assert(numGlobalLights == 0);
		
		for (auto & light : lights)
		{
			if (light.isGlobalLight)
				numGlobalLights++;
			else
				break;
		}
		
		// fill light params buffer with information for all of the lights
		
		if (!lights.empty())
		{
			Vec4 * params = new Vec4[lights.size() * 4];
			Vec4 * extras = new Vec4[lights.size() * 4];

			for (size_t i = 0; i < lights.size(); ++i)
			{
				const Vec3 & position_world = lights[i].position;
				const Vec3 & direction_world = lights[i].direction;
				
				const Vec3 position_view = worldToView.Mul4(position_world);
				const Vec3 direction_view = worldToView.Mul3(direction_world);
				
				params[i * 4 + 0][0] = position_view[0];
				params[i * 4 + 0][1] = position_view[1];
				params[i * 4 + 0][2] = position_view[2];
				params[i * 4 + 0][3] = lights[i].type;
				
				params[i * 4 + 1][0] = direction_view[0];
				params[i * 4 + 1][1] = direction_view[1];
				params[i * 4 + 1][2] = direction_view[2];
				params[i * 4 + 1][3] = 0.f;
				
			// optimize : store light color as u32, as rgbe
				params[i * 4 + 2][0] = lights[i].color[0];
				params[i * 4 + 2][1] = lights[i].color[1];
				params[i * 4 + 2][2] = lights[i].color[2];
				params[i * 4 + 2][3] = 0.f;
				
				params[i * 4 + 3][0] = lights[i].attenuationBegin;
				params[i * 4 + 3][1] = lights[i].attenuationEnd;
				params[i * 4 + 3][2] = cosf(lights[i].spotAngle / 2.f);
				params[i * 4 + 3][3] = lights[i].userData;
				
				const bool isAreaLight =
					lights[i].type == kLightType_AreaBox ||
					lights[i].type == kLightType_AreaSphere ||
					lights[i].type == kLightType_AreaRect ||
					lights[i].type == kLightType_AreaCircle;
				
				if (isAreaLight)
				{
					const Mat4x4 lightToView = worldToView * lights[i].transform;
					
					// create packed area light params
					Mat4x4 lightToView_packed(true);
					
					const float scaleX = lightToView.GetAxis(0).CalcSize();
					const float scaleY = lightToView.GetAxis(1).CalcSize();
					const float scaleZ = lightToView.GetAxis(2).CalcSize();
					
					// create orthonormal 3x3 rotation matrix
					for (int i = 0; i < 3; ++i)
					{
						lightToView_packed(0, i) = lightToView(0, i) / scaleX;
						lightToView_packed(1, i) = lightToView(1, i) / scaleY;
						lightToView_packed(2, i) = lightToView(2, i) / scaleZ;
					}
					
					// pack scaling factors
					lightToView_packed(0, 3) = scaleX;
					lightToView_packed(1, 3) = scaleY;
					lightToView_packed(2, 3) = scaleZ;
					
					// pack translation
					lightToView_packed.SetTranslation(lightToView.GetTranslation());
					
					// store the packed data
					extras[i * 4 + 0] = lightToView_packed.GetColumn(0);
					extras[i * 4 + 1] = lightToView_packed.GetColumn(1);
					extras[i * 4 + 2] = lightToView_packed.GetColumn(2);
					extras[i * 4 + 3] = lightToView_packed.GetColumn(3);
				}
			}

			lightParamsBuffer.setData(params, lights.size() * 4 * sizeof(Vec4));
			lightExtrasBuffer.setData(extras, lights.size() * 4 * sizeof(Vec4));

			delete [] params;
			params = nullptr;
			
			delete [] extras;
			extras = nullptr;
		}

		// generate light volume data using the (current) set of lights

		{
			LightVolumeBuilder builder;

			for (size_t i = 0; i < lights.size(); ++i)
			{
				if (lights[i].isGlobalLight)
					continue;
				
				if (lights[i].type == kLightType_Point)
				{
					const Vec3 & position_world = lights[i].position;
					const Vec3 position_view = worldToView.Mul4(position_world);
					
					builder.addPointLight(
						i,
						position_view,
						lights[i].attenuationEnd);
				}
				else if (lights[i].type == kLightType_Spot)
				{
					const Vec3 & position_world = lights[i].position;
					const Vec3 & direction_world = lights[i].direction;
					const Vec3 position_view = worldToView.Mul4(position_world);
					const Vec3 direction_view = worldToView.Mul3(direction_world);
					
					builder.addSpotLight(
						i,
						position_view,
						direction_view,
						lights[i].spotAngle,
						lights[i].attenuationEnd);
				}
				else if (
					lights[i].type == kLightType_AreaBox ||
					lights[i].type == kLightType_AreaSphere ||
					lights[i].type == kLightType_AreaRect ||
					lights[i].type == kLightType_AreaCircle)
				{
					const Mat4x4 transform_view = worldToView * lights[i].transform;
					
					builder.addAreaLight(
						i,
						transform_view,
						lights[i].attenuationEnd);
				}
				else if (lights[i].type == kLightType_Directional)
				{
					continue;
				}
				else
				{
					Assert(false);
					continue;
				}
			}

			auto data = builder.generateLightVolumeData(resolution, extents, infiniteSpaceMode);

			// create textures from data

			indexTextureId = createTextureFromRG32F(
				data.index_table,
				data.index_table_sx,
				data.index_table_sy * data.index_table_sz, // todo : would be nice to use 3d textures here
				false,
				false);

			if (data.light_ids_sx * data.light_ids_sy > 0)
			{
				lightIdsTextureId = createTextureFromR32F(
					data.light_ids,
					data.light_ids_sx,
					data.light_ids_sy,
					false,
					false);
			}

			worldToVolumeScale = data.world_to_volume_scale;
			
			// dispose of the data

			data.free();
		}
		
		isPrepared = true;
	}

	void ForwardLightingHelper::setShaderData(Shader & shader, int & nextTextureUnit) const
	{
		shader.setBuffer("LightParamsBuffer", lightParamsBuffer);
		shader.setBuffer("LightExtrasBuffer", lightExtrasBuffer);
		shader.setImmediate("numLights", lights.size());
		shader.setImmediate("numGlobalLights", numGlobalLights);
		shader.setTexture("lightVolume", nextTextureUnit++, indexTextureId, false, false);
		shader.setTexture("lightIds", nextTextureUnit++, lightIdsTextureId, false, false);
		shader.setImmediate("worldToVolumeScale", worldToVolumeScale);
		shader.setImmediate("infiniteSpaceMode", infiniteSpaceMode ? 1.f : 0.f);
	}
}
