#include "forwardLighting.h"
#include "lightVolumeBuilder.h"
#include "srgbFunctions.h"

#include "framework.h"
#include <math.h>

namespace rOne
{
	ForwardLightingHelper::~ForwardLightingHelper()
	{
		reset();
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
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized", 0);
		
		Light light;
		light.type = kLightType_Spot;
		light.position = position;
		light.direction = direction;
		light.attenuationBegin = 0.f;
		light.attenuationEnd = farDistance;
		light.spotAngle = cosf(angle / 2.f);
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
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized", 0);
		
		Light light;
		light.type = kLightType_Directional;
		light.direction = direction;
		light.color = srgbToLinear(color) * intensity;
		light.userData = userData;

		addLight(light);
	}

	void ForwardLightingHelper::reset()
	{
		lights.clear();
		
		lightParamsBuffer.free();
		
		freeTexture(indexTextureId);
		freeTexture(lightIdsTextureId);
		
		isPrepared = false;
	}

	void ForwardLightingHelper::prepareShaderData(
		const int resolution,
		const float extents,
		const bool in_infiniteSpaceMode,
		const Mat4x4 & worldToView)
	{
		AssertMsg(isPrepared == false, "please call reset() when done rendering a frame", 0);
		
		infiniteSpaceMode = in_infiniteSpaceMode;
		
		// fill light params buffer with information for all of the lights
		
		if (!lights.empty())
		{
			Vec4 * params = new Vec4[lights.size() * 4];

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
				params[i * 4 + 3][2] = lights[i].spotAngle;
				params[i * 4 + 3][3] = lights[i].userData;
			}

			lightParamsBuffer.setData(params, lights.size() * 4 * sizeof(Vec4));

			delete [] params;
			params = nullptr;
		}
		else
		{
			// bind some dummy data when there is no data. this to avoid gpu-driver issues
			
			Vec4 data[4];
			lightParamsBuffer.setData(data, sizeof(data));
		}

		// generate light volume data using the (current) set of lights

		{
			LightVolumeBuilder builder;

			for (size_t i = 0; i < lights.size(); ++i)
			{
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
		shader.setBuffer("lightParamsBuffer", lightParamsBuffer);
		shader.setImmediate("numLights", lights.size());
		shader.setTexture("lightVolume", nextTextureUnit++, indexTextureId, false, false);
		shader.setTexture("lightIds", nextTextureUnit++, lightIdsTextureId, false, false);
		shader.setImmediate("worldToVolumeScale", worldToVolumeScale);
		shader.setImmediate("infiniteSpaceMode", infiniteSpaceMode ? 1.f : 0.f);
		
		int directionalLightId = -1;
		for (size_t i = 0; i < lights.size(); ++i)
			if (lights[i].type == kLightType_Directional)
				directionalLightId = i;
		shader.setImmediate("directionalLightId", directionalLightId);
	}
}
