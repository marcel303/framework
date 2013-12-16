// ----------------------------------------
// passthrough
// ----------------------------------------

cbuffer gPassthroughCB : register(b0)
{
	float4x4 gPassthroughWVP;
};

struct VSPassthroughIn
{
	float4 position : POSITION;
	float2 texcoord : TEXTURE0;
};

struct VSPassthroughOut
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXTURE0;
};

struct PSPassthroughOut
{
	float3 color : SV_Target0;
};

VSPassthroughOut vs_passthrough(VSPassthroughIn input)
{
	VSPassthroughOut output;

	output.position = mul(input.position, gPassthroughWVP);
	output.texcoord = input.texcoord;

	return output;
}

PSPassthroughOut ps_passthrough(VSPassthroughOut input)
{
	PSPassthroughOut output;
	
	output.color = float3(1.0f, 1.0f, 1.0f);

	return output;
}

// ----------------------------------------
// geometry
// ----------------------------------------

cbuffer gGeometryCB : register(b0)
{
	float4x4 gGeometryWVP;
	float4x4 gGeometryW;
};

Texture2D gGeometryNormalTexture : register(t0);
SamplerState gGeometryNormalTextureSampler : register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D gGeometryAlbedoTexture : register(t1);
SamplerState gGeometryAlbedoTextureSampler : register(s1)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct VSGeometryIn
{
	float4 position : POSITION;
#if DEPTH_ONLY == 0
	float3 normal : NORMAL;
	float2 texcoord : TEXTURE0;
#endif
};

struct VSGeometryOut
{
	float4 position : SV_POSITION;
#if DEPTH_ONLY == 0
	float2 texcoord : TEXTURE0;
	float3 normal : TEXTURE1;
#endif
};

struct PSGeometryOut
{
#if DEPTH_ONLY == 0
	float3 normal : SV_Target0;
	float4 albedo : SV_Target1;
#endif
};

VSGeometryOut vs_geometry(VSGeometryIn input)
{
	VSGeometryOut output;

	output.position = mul(input.position, gGeometryWVP);
#if DEPTH_ONLY == 0
	output.texcoord = input.texcoord;
	output.normal = mul(input.normal, (float3x3)gGeometryW);
#endif

	return output;
}

PSGeometryOut ps_geometry(VSGeometryOut input)
{
	PSGeometryOut output;

#if DEPTH_ONLY == 0
	output.normal = normalize(input.normal);
	output.albedo = gGeometryAlbedoTexture.Sample(gGeometryAlbedoTextureSampler, input.texcoord);
#endif

	return output;
}

// ----------------------------------------
// light
// ----------------------------------------

cbuffer gLightCB : register(b0)
{
	float4x4 gLightWVP;
	float4x4 gLightInvP;
};

Texture2D gLightNormalTexture : register(t0);
Texture2D gLightDepthTexture : register(t1);
Texture2D gLightItensityTexture : register(t2);
SamplerState gLightItensityTextureSampler : register(s2)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

void vs_light(
	float4 position : POSITION,
	float2 texcoord : TEXTURE0,
	out float4 rPosition : SV_POSITION,
	out float2 rLightTexcoord : TEXTURE0)
{
	rPosition = mul(position, gLightWVP);
	//rLightTexcoord = position.xy;
	rLightTexcoord = texcoord;
}

void ps_light(
	float4 position : SV_POSITION,
	float2 lightTexcoord : TEXTURE0,
	out float4 rLightColor : SV_Target)
{
	const float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
	//const float3 lightDir = normalize(float3(0.0f, 1.0f, 0.0f));

	const float3 normal = gLightNormalTexture.Load(int3(position.xy, 0)).xyz;
	const float depth = gLightDepthTexture.Load(int3(position.xy, 0)).x;

	const float lightIntensity = gLightItensityTexture.Sample(gLightItensityTextureSampler, lightTexcoord).x;
	const float lightDiffuse = dot(lightDir, normal);

#if 1
	if (lightDiffuse <= 0.0f)
		discard;
	if (lightIntensity == 0.0f)
		discard;

	rLightColor = lightDiffuse * lightIntensity;
	//rLightColor = lightIntensity;
	return;
#endif

#if 0
	float z = depth * 10.0f;
	rLightColor = z;
#elif 0
	float4 depthPos = mul(float4(position.xy, depth, 1.0f), gLightInvP);
	float z = depthPos.z / depthPos.w;
	rLightColor = z;
#elif 1
	float z = depth * (3.0f - 1.0f) + 1.0f;
	if (z > 1.1f)
		rLightColor = 0.7f;
	else
		rLightColor = 1.0f;
#else
	//float z = depth * (3.0f - 1.0f) + 1.0f;
	//float z = (depth - 1.0f) / (3.0f - 1.0f);
	//float z = depth / 10.0f;
	float z = depth;
	rLightColor = z;
#endif
}

// ----------------------------------------
// composite
// ----------------------------------------

cbuffer gCompositeCB : register(b0)
{
	float4x4 gCompositeWVP;
};

struct VSCompositeIn
{
	float4 position : POSITION;
	float2 texcoord : TEXTURE0;
};

struct VSCompositeOut
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXTURE0;
};

struct PSCompositeOut
{
	float4 color : SV_Target0;
};

Texture2D gCompositeAlbedo : register(t0);
Texture2D gCompositeLight : register(t1);

VSCompositeOut vs_composite(VSCompositeIn input)
{
	VSCompositeOut output;

	output.position = mul(input.position, gCompositeWVP);
	output.texcoord = input.texcoord;

	return output;
}

PSCompositeOut ps_composite(VSCompositeOut input)
{
	PSCompositeOut output;

	//input.texcoord = (input.texcoord + 1.0f) * 256.0f;

	const float4 albedo = gCompositeAlbedo.Load(int3(input.texcoord.xy, 0));
	const float4 light = gCompositeLight.Load(int3(input.texcoord.xy, 0));
	
	output.color = light * albedo;
	//output.color = albedo;
	//output.color = albedo * 0.05f + light;
	//output.color = albedo + light;
	//output.color = float4(input.texcoord.xy, 0.0f, 1.0f);

	return output;
}
