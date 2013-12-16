// ----------------------------------------

cbuffer InstanceData : register(b0)
{
	matrix transform;
	float4 color;
	float blurRadius;
	float2 sunPos;
	float targetSx;
	float targetSy;
};

// ----------------------------------------

Texture2D gTexture : register(t0);
SamplerState gTextureSampler : register(s0);

Texture2D gRandomTexture : register(t1);
SamplerState gRandomTextureSampler : register(s1);

// ----------------------------------------
// debug vs
// ----------------------------------------

struct VSIN
{
	float3 position : POSITION;
};

struct VSOUT
{
	float4 position : SV_POSITION;
};

VSOUT vsmain(VSIN input)
{
	VSOUT output;
	
	output.position = mul(float4(input.position, 1.0f), transform);
	
	return output;
}

// ----------------------------------------
// debug ps
// ----------------------------------------

struct PSIN
{
	float4 position : SV_POSITION;
};

struct PSOUT
{
	float4 color : SV_Target;
};

PSOUT psmain(PSIN input)
{
	PSOUT output;
	
	output.color = color;
	
	return output;
}

// ----------------------------------------
// passthrough
// ----------------------------------------

struct VS_PASSTHROUGH_IN
{
	float4 position : POSITION;
	float2 texcoord : TEXTURE0;
};

struct VS_PASSTHROUGH_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXTURE0;
};

VS_PASSTHROUGH_OUT vs_passthrough(VS_PASSTHROUGH_IN input)
{
	VS_PASSTHROUGH_OUT result;
	
	result.position = mul(input.position, transform);
	result.texcoord = input.texcoord;
	
	return result;
}

// ----------------------------------------
// texture effect 1
//   basic pixel shader with 1 texcoord
// ----------------------------------------

struct PS_EFFECT1_IN
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXTURE0;
};

struct PS_EFFECT1_OUT
{
	float4 color : SV_Target;
};

// ----------------------------------------
// blit
// ----------------------------------------

PS_EFFECT1_OUT ps_blit(PS_EFFECT1_IN input)
{
	PS_EFFECT1_OUT result;
	
	result.color = float4(gTexture.Load(int3(input.texcoord, 0)).rgb, 1.0f);
	
	return result;
}

// ----------------------------------------
// blur
// ----------------------------------------

PS_EFFECT1_OUT ps_blur(PS_EFFECT1_IN input)
{
	PS_EFFECT1_OUT result;
	
	// blur. take N * N samples and take average
	
	const int N = 5;
	const int N2 = (N - 1) / 2;
	const int NSq = N * N;
	
	result.color = 0.0f;

	[unroll] for (int x = -N2; x <= +N2; ++x)
	{
		[unroll] for (int y = -N2; y <= +N2; ++y)
		{
			const float2 texcoord = input.texcoord + float2(x, y) * blurRadius;
			
			result.color += gTexture.Load(int3(texcoord, 0));
		}
	}
	
	result.color /= NSq;
	
	return result;
}

// ----------------------------------------
// god rays
// ----------------------------------------

PS_EFFECT1_OUT ps_godrays(PS_EFFECT1_IN input)
{
	PS_EFFECT1_OUT result;
	
	// god rays
	
	const int GN = 20;
	
	float4 baseColor = gTexture.Load(int3(input.texcoord, 0));
	
	// setup strobe
	float2 pos = sunPos;
	float2 dstPos = input.texcoord;
	float2 delta = dstPos - pos;
	float2 step = delta / GN;
	float decay = length(delta) / GN / 400.0f;
	//float decay = length(delta) / GN / 1000.0f;
	
#if 0
	// perturb start position a little
	float4 t = gRandomTexture.Load(int3(input.texcoord, 0)) - 0.5f;
	pos += float2(t.x * step.x, t.y * step.y) * 2.0f;
	delta = dstPos - pos;
	step = delta / GN;
	decay = length(delta) / GN / 400.0f;
#else
	// perturb start position a little
	float4 t = gRandomTexture.Sample(gRandomTextureSampler, input.texcoord / 256.0f) - 0.5f;
	pos += float2(t.x * step.x, t.y * step.y) * 2.0f;
#endif
	
	#if 0
	float godIntensity = 0.0f;
	
	for (int i = 0; i < GN; ++i)
	{
		float itensity = gTexture.Load(int3(pos, 0)).a;
		godIntensity = max(godIntensity - decay, itensity);
		pos += step;
	}
	#else
	float godIntensity = 1.0f;
	
	for (int i = 0; i < GN; ++i)
	{
		float itensity = 1.0f - gTexture.Load(int3(pos, 0)).a;
		godIntensity = max(godIntensity - decay, itensity);
		//float itensity = gTexture.Load(int3(pos, 0)).a;
		//godIntensity = min(godIntensity - decay, itensity);
		pos += step;
	}
	#endif
	
	//result.color = baseColor * godIntensity;
	//result.color = baseColor + godIntensity;
	//result.color = float4(baseColor.rgb, baseColor.a + godIntensity);
	result.color = float4(baseColor.rgb, godIntensity);
	//result.color = baseColor;

	//result.color = t;

	return result;
}

// ----------------------------------------
// blur alpha
// ----------------------------------------

PS_EFFECT1_OUT ps_blur_alpha(PS_EFFECT1_IN input)
{
	PS_EFFECT1_OUT result;
	
	// blur. take N * N samples and take average
	
	const int N = 5;
	const int N2 = (N - 1) / 2;
	const int NSq = N * N;
	
	result.color = gTexture.Load(int3(input.texcoord, 0));
	
	if (N > 1)
	{
		float alpha = 0.0f;

		[unroll] for (int x = -N2; x <= +N2; ++x)
		{
			[unroll] for (int y = -N2; y <= +N2; ++y)
			{
				// todo: use random texture to perturb texcoords?

				float2 texcoord = input.texcoord + float2(x, y) * 4.0f;
				
				float a = gTexture.Load(int3(texcoord, 0)).a;
				
				alpha += a;
			}
		}
		
		result.color.a = alpha / NSq;
	}
	
	result.color += result.color.a;
	//result.color += result.color.a * 0.7f;
	//result.color *= result.color.a * 0.7f;
	
	return result;
}

// ----------------------------------------
// geometry shader
// ----------------------------------------

#if 0
typedef struct
{
	float4 position : SV_POSITION;
} GSIN;

[maxvertexcount(243)]
void gsmain(triangle GSIN input[3], inout TriangleStream<PSIN> OutputStream)
{
	const int n = 1;

	[unroll] for (int x = -n; x <= +n; x += 1)
	{
		[unroll] for (int y = -n; y <= +n; y += 1)
		{
			float4 offset = float4(x, y, 0.0f, 0.0f) * 0.05f;
			
			for (int i = 0; i < 3; i++)
			{
				PSIN result;
			
				result.position = input[i].position + offset;
				
				OutputStream.Append(result);
			}
	
			OutputStream.RestartStrip();
		}
	}
	
	OutputStream.RestartStrip();
}
#endif
