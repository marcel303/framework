#if _SHADER_
include engine/ShaderCommon.txt
#else
#pragma once
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

// FSFX post process

struct FsfxData
{
	float alpha;
	float _time;
	float _param1;
	float _param2;
	float _param3;
	float _param4;
	float _pcmVolume;
};

#if _SHADER_

layout (std140) uniform FsfxBlock
{
	FsfxData fsfxData;
};

#define time fsfxData._time
#define param1 fsfxData._param1
#define param2 fsfxData._param2
#define param3 fsfxData._param3
#define param4 fsfxData._param4
#define pcmVolume fsfxData._pcmVolume

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
	float alpha;
	float strength;
	float darken;
};

#if _SHADER_

layout (std140) uniform FlowmapBlock
{
	FlowmapData flowmapData;
};

#endif

// smoke effect

struct SmokeData
{
	float alpha;
	float strength;
	float darken;
	float darkenAlpha;
	float mul;
};

#if _SHADER_

layout (std140) uniform SmokeBlock
{
	SmokeData smokeData;
};

#endif

// luminance post process

struct LuminanceData
{
	float alpha;
	float power;
	float scale;
	float darken;
};

struct LuminanceData2
{
	float darkenAlpha;
};

#if _SHADER_

layout (std140) uniform LuminanceBlock
{
	LuminanceData luminanceData;
};

layout (std140) uniform LuminanceBlock2
{
	LuminanceData2 luminanceData2;
};

#endif

// 2D color lut post process

struct ColorLut2DData
{
	float alpha;
	float lutStart;
	float lutEnd;
	float numTaps;
};

#if _SHADER_

layout (std140) uniform ColorLut2DBlock
{
	ColorLut2DData colorLut2DData;
};

#endif

// vignette effect post process

struct VignetteData
{
	float alpha;
	float innerRadius;
	float distanceRcp;
};

#if _SHADER_

layout (std140) uniform VignetteBlock
{
	VignetteData vignetteData;
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

// particle system

struct ShaderParticle
{
	float value;
	float angle;
	float x;
	float y;
	float sx;
	float sy;
};

#if _SHADER_

layout (std140) uniform ShaderParticleBlock
{
	ShaderParticle shaderParticles[];
};

#endif
