#include "forwardLighting.h"
#include "lightVolumeBuilder.h"

#include "framework.h"

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
	const float intensity)
{
	Light light;
	light.type = kLightType_Point;
	light.position = position;
	light.attenuationBegin = attenuationBegin;
	light.attenuationEnd = attenuationEnd;
	light.color = color * intensity;

	addLight(light);
}

void ForwardLightingHelper::reset()
{
	lights.clear();
	
	lightsParamsBuffer.free();
	
	freeTexture(indexTextureId);
	freeTexture(lightIdsTextureId);
	
	isPrepared = false;
}

void ForwardLightingHelper::prepareShaderData(const Mat4x4 & worldToView)
{
	AssertMsg(isPrepared == false, "please call reset() when done rendering a frame", 0);
	
	// fill light params buffer with information for all of the lights
	
	{
		Vec4 * params = new Vec4[lights.size() * 2];

		for (size_t i = 0; i < lights.size(); ++i)
		{
			const Vec3 & position_world = lights[i].position;
			const Vec3 position_view = worldToView.Mul4(position_world);
			
			params[i * 2 + 0][0] = lights[i].type;
			params[i * 2 + 0][1] = position_view[0];
			params[i * 2 + 0][2] = position_view[1];
			params[i * 2 + 0][3] = position_view[2];
			
			params[i * 2 + 1][0] = lights[i].attenuationBegin;
			params[i * 2 + 1][1] = lights[i].attenuationEnd;
			params[i * 2 + 1][2] = 0.f;
			params[i * 2 + 1][3] = 0.f;
		}

		lightsParamsBuffer.setData(params, lights.size() * 2 * sizeof(Vec4));

		delete [] params;
		params = nullptr;
	}

	// generate light volume data using the (current) set of lights

	{
		LightVolumeBuilder builder;

		for (size_t i = 0; i < lights.size(); ++i)
		{
			const Vec3 & position_world = lights[i].position;
			const Vec3 position_view = worldToView.Mul4(position_world);
			
			builder.addPointLight(
				i,
				position_view,
				lights[i].attenuationEnd);
		}

		auto data = builder.generateLightVolumeData();

		// create textures from data

		indexTextureId = createTextureFromRG32F(
			data.index_table,
			data.index_table_sx,
			data.index_table_sy,
			false,
			false);

		lightIdsTextureId = createTextureFromR32F(
			data.light_ids,
			data.light_ids_sx,
			1,
			false,
			false);

		// dispose of the data

		data.free();
	}
	
	isPrepared = true;
}

void ForwardLightingHelper::setShaderData(Shader & shader, int & nextTextureUnit) const
{
	// todo : set shader uniforms, textures and buffers

	shader.setImmediate("useLightVolume", 1.f);
	shader.setBuffer("lightParamsBuffer", lightsParamsBuffer);
	shader.setImmediate("numLights", lights.size());
	shader.setTexture("lightVolume", nextTextureUnit++, indexTextureId, false, false);
	shader.setTexture("lightIds", nextTextureUnit++, lightIdsTextureId, false, false);
}
