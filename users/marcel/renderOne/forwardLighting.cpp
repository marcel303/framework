#include "forwardLighting.h"

#include "framework.h"

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
	
	delete lightsParamsBuffer;
	lightsParamsBuffer = nullptr;
}

void ForwardLightingHelper::prepareShaderData(const Mat4x4 & worldToView)
{
	AssertMsg(lightsParamsBuffer == nullptr, "please call reset() when done rendering a frame", 0);
	
	// todo : generate light volume data using the (current) set of lights
	
	// fill light params buffer with information for all of the lights
	
	lightsParamsBuffer = new ShaderBuffer();
	
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

		lightsParamsBuffer->setData(params, lights.size() * 2 * sizeof(Vec4));

		delete [] params;
		params = nullptr;
	}
}

void ForwardLightingHelper::setShaderData(Shader & shader) const
{
	// todo : set shader uniforms, textures and buffers

	shader.setImmediate("useLightVolume", 0.f);
	shader.setBuffer("lightParamsBuffer", *lightsParamsBuffer);
	shader.setImmediate("numLights", lights.size());
}
