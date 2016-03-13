include ShaderConstants.h

// light computation functions

float saturate(in float x)
{
	return clamp(x, 0.f, 1.f);
}

void computeLight(in Light light, in vec3 surfacePosition, in vec3 surfaceNormal, out float diffuseIntensity, out float specularIntensity)
{
	if (light.type == kLightType_Directional)
	{
		diffuseIntensity = saturate(dot(vec3(light.x, light.y, light.z), surfaceNormal));
		specularIntensity = 0.f;
	}
	else if (light.type == kLightType_Omni)
	{
		vec3 delta = vec3(light.x, light.y, light.z) - surfacePosition;
		float distance = length(delta);
		vec3 direction = normalize(delta);
		float falloff = saturate(1.f - distance * light.falloffRcp);

		diffuseIntensity = abs(dot(direction, surfaceNormal)) * falloff;
		specularIntensity = 0.f;
	}
	else if (light.type == kLightType_Spot)
	{
		diffuseIntensity = 1.f;
		specularIntensity = 0.f;
	}
}

//

void computeLighting(in vec3 surfacePosition, in vec3 surfaceNormal, out vec3 totalDiffuse, out vec3 totalSpecular)
{
	totalDiffuse = vec3(0.f);
	totalSpecular = vec3(0.f);

	for (int i = 0; i < kMaxLights; ++i)
	{
		if (lights[i].type != kLightType_None)
		{
			float diffuseIntensity;
			float specularIntensity;

			computeLight(lights[i], surfacePosition, surfaceNormal, diffuseIntensity, specularIntensity);

			totalDiffuse  += diffuseIntensity  * vec3(lights[i].colorR, lights[i].colorG, lights[i].colorB);
			totalSpecular += specularIntensity * vec3(lights[i].colorR, lights[i].colorG, lights[i].colorB);
		}
	}
}
