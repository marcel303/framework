#include "framework.h"

static const char * ps = R"SHADER(

include engine/ShaderPS.txt

uniform sampler2D tex;

shader_in vec2 v_texcoord0;

vec2 laplacian2D(in ivec2 texelCoord)
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
	ivec2 texelSize = textureSize(tex, 0);
	ivec2 texelCoord = ivec2(v_texcoord0 * texelSize);
	
	vec2 AB = texelFetch(tex, texelCoord, 0).xy;
	float A = AB.x;
	float B = AB.y;
	
	vec2 laplacian = laplacian2D(texelCoord);

	float A_1 = A + (0.9  * laplacian.x - A * B * B + 0.0545 * (1 - A));
	float B_1 = B + (0.18 * laplacian.y + A * B * B - (0.062 + 0.0545) * B);

	shader_fragColor = vec4(A_1, B_1, 1.0, 1.0);
}

)SHADER";

int main(int argc, const char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	Surface surface(800, 600, false, true, SURFACE_RG16F);
	surface.setSwizzle(0, 1, GX_SWIZZLE_ONE, GX_SWIZZLE_ONE);
	
	auto restart = [&]()
	{
		surface.clear(255, 0, 0);
		surface.swapBuffers();
		
		pushSurface(&surface);
		{
			surface.clear();
			
			for (int i = 0; i < 4; ++i)
			{
				setColorf(0, 1, 0);
				gxPushMatrix();
				gxTranslatef(rand() % 800, rand() % 600, 0);
				gxScalef(random<float>(0.f, 10.f), random<float>(0.f, 10.f), 1);
				fillCircle(0, 0, 10, 3 + (rand() % 3));
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
			shader.setTexture("tex", 0, surface.getTexture());
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
