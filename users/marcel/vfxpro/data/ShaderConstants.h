#if _SHADER_
include engine/ShaderCommon.txt
#endif

// lighting

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

layout (std140) uniform lightsBlock
{
	Light lights[kMaxLights];
};

#endif

// boxblur post process

struct BoxblurData
{
	float radiusX;
	float radiusY;
};

#if _SHADER_

layout (std140) uniform BoxblurBlock
{
	BoxblurData boxblurData;
};

#endif

// flowmap effect

struct FlowmapData
{
	float strength;
};

#if _SHADER_

layout (std140) uniform FlowmapBlock
{
	FlowmapData flowmapData;
};

#endif

// luminance post process

struct LuminanceData
{
	float power;
	float scale;
};

#if _SHADER_

layout (std140) uniform LuminanceBlock
{
	LuminanceData luminanceData;
};

#endif

// distortion bars post process

struct DistortionBarsData
{
	float px;
	float py;
	float pd;
	float qx;
	float qy;
	float qd;
	float pScale;
	float qScale;
};

#if _SHADER_

layout (std140) uniform DistotionBarsBlock
{
	DistortionBarsData distortionBarsData;
};

#endif
