#include "framework.h"

const int GFX_SX = 400;
const int GFX_SY = 400;

/*
Shader based Reaction-Diffusion algorithm.
At its core it uses a shader which runs for all pixels of a surface.
The surface itself is double-buffered, meaning it can be used in a ping-pong fashion
to store both the previous frame (read-only) the the new frame (written to by the shader).

http://karlsims.com/rd.html
*/

static const char * ps = R"SHADER(

include engine/ShaderPS.txt

uniform sampler2D tex;

shader_in vec2 v_texcoord0;

vec2 laplacian2D(ivec2 texelCoord)
{
	vec2 AB =
		+0.05 * texelFetchOffset(tex, texelCoord, 0, ivec2(-1, -1)).xy
		+0.2  * texelFetchOffset(tex, texelCoord, 0, ivec2( 0, -1)).xy
		+0.05 * texelFetchOffset(tex, texelCoord, 0, ivec2(+1, -1)).xy
 		+0.2  * texelFetchOffset(tex, texelCoord, 0, ivec2(-1,  0)).xy
 		-1.0  * texelFetchOffset(tex, texelCoord, 0, ivec2( 0,  0)).xy
 		+0.2  * texelFetchOffset(tex, texelCoord, 0, ivec2(+1,  0)).xy
		+0.05 * texelFetchOffset(tex, texelCoord, 0, ivec2(-1, +1)).xy
		+0.2  * texelFetchOffset(tex, texelCoord, 0, ivec2( 0, +1)).xy
		+0.05 * texelFetchOffset(tex, texelCoord, 0, ivec2(+1, +1)).xy;

	return AB;
}

void main()
{
	vec2 texelSize = textureSize(tex, 0);
	ivec2 texelCoord = ivec2(v_texcoord0 * texelSize);
	
	vec2 AB = texelFetch(tex, texelCoord, 0).xy;
	float A = AB.x;
	float B = AB.y;
	
	vec2 laplacian = laplacian2D(texelCoord);

// A' = A + (dA * laplacianA) - (A * B * B) + (feed * (1 - A))
// B' = B + (dB * laplacianB) + (A * B * B) - ((k + feed) * B)

// where k is the kill rate

#if 1
	// interpolating feed and k values over the screen
	float dA = 1.0;
	float dB = 0.5;
	float feed = mix(0.03, 0.06, v_texcoord0.y);
	float k = mix(0.055, 0.07, v_texcoord0.x);
#elif 0
	// 'common values'
	float dA = 1.0;
	float dB = 0.5;
	float feed = 0.055;
	float k = 0.062;
#elif 1
	// 'mitosis'
	float dA = 1.0;
	float dB = 0.45;
	float feed = 0.0367;
	float k = 0.0649;
#elif 0
	// 'coral growth'
	float dA = 1.0;
	float dB = 0.45;
	float feed = 0.0545;
	float k = 0.062;
#endif

	float dT = 1.0;

	float A_1 = A + ((dA * laplacian.x) - (A * B * B) + ((    feed) * (1.0 - A))) * dT;
	float B_1 = B + ((dB * laplacian.y) + (A * B * B) - ((k + feed) * (      B))) * dT;

	shader_fragColor = vec4(A_1, B_1, 0.0, 0.0);
}

)SHADER";

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	Surface surface(GFX_SX, GFX_SY, false, true, SURFACE_RG16F);
	surface.setSwizzle(0, 1, GX_SWIZZLE_ONE, GX_SWIZZLE_ONE);
	
	auto restart = [&]()
	{
		pushSurface(&surface);
		{
			// seed A with 1.0
			
			surface.clearf(1, 0, 0);
			
			// seed a small area A with 0.0 and B with 1.0
			
			setColorf(0, 1, 0);
			
			for (int i = 0; i < 4; ++i)
			{
				gxPushMatrix();
				{
					gxTranslatef(rand() % GFX_SX, rand() % GFX_SY, 0);
					gxScalef(random<float>(0.f, 10.f), random<float>(0.f, 10.f), 1);
					
					fillCircle(0, 0, 10, 3 + (rand() % 3));
				}
				gxPopMatrix();
			}
		}
		popSurface();
	};

	restart();

	shaderSource("ReactionDiffusion.ps", ps);

	Shader shader("ReactionDiffusion", "engine/Generic.vs", "ReactionDiffusion.ps");

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
			restart();

		setBlend(BLEND_OPAQUE); // we don't want alpha blending to slow down our post processing passes below, so disable it here
		
		for (int i = 0; i < 20; ++i)
		{
			// apply reaction-diffusion shader

			setShader(shader);
			shader.setTexture("tex", 0, surface.getTexture(), false, true);
			surface.postprocess();
			clearShader();
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			surface.blit(BLEND_OPAQUE);
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
