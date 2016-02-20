// light computation functions

const int kMaxLights = 4;

const int kLightType_None = 0;
const int kLightType_Directional = 1;
const int kLightType_Omni = 2;
const int kLightType_Spot = 3;

struct Light
{
#if !_SHADER_
	Light()
		: type(kLightType_None)
	{
	}

	void setup(int _type, float _x, float _y, float _z, float r, float g, float b, float falloff)
	{
		type = _type;
		x = _x;
		y = _y;
		z = _z;
		colorR = r;
		colorG = g;
		colorB = b;
		falloffRcp = 1.f / falloff;
	}
#endif

	float type;
	float x;
	float y;
	float z;
	float colorR;
	float colorG;
	float colorB;
	float falloffRcp;
};

#if _SHADER_

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
		const vec3 delta = vec3(light.x, light.y, light.z) - surfacePosition;
		const float distance = sqrt(dot(delta, delta));
		const vec3 direction = delta / distance;
		float falloff = saturate(1.f - distance * light.falloffRcp);

		diffuseIntensity = saturate(dot(direction, surfaceNormal)) * falloff;
		specularIntensity = 0.f;
	}
	else if (light.type == kLightType_Spot)
	{
		diffuseIntensity = 1.f;
		specularIntensity = 0.f;
	}
}

//

uniform Light lights[kMaxLights];

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

#endif
