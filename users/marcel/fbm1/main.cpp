#include "framework.h"
#include "Noise.h"
#include <math.h>

// fractional brownian noise
// see: https://www.iquilezles.org/www/articles/fbm/fbm.htm

float fbm_slow(Vec2Arg x, float H, int numOctaves)
{
    float t = 0.f;
	
    for (int i = 0; i < numOctaves; ++i)
    {
        float f = powf(2.f, float(i));
        float a = powf(f, -H);
		
        t += a * raw_noise_2d(f*x[0], f*x[1]);
    }
	
    return t;
}

float fbm_fast(Vec2Arg x, float H, int numOctaves)
{
    float G = exp2f(-H);
    float f = 1.f;
    float a = 1.f;
    float t = 0.f;
	
    for (int i = 0; i < numOctaves; ++i)
    {
        t += a * raw_noise_2d(f*x[0], f*x[1]);
		
        f *= 2.f;
        a *= G;
    }
	
    return t;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 400))
		return -1;
	
	Mat4x4 viewMatrix;
	viewMatrix.MakeLookat(Vec3(0, 0, -2), Vec3(0, 0, 0), Vec3(0, 1, 0));
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			gxSetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
			pushDepthTest(true, DEPTH_LESS);
			
			const float H = lerp<float>(0.f, 1.f, mouse.x / 800.f);
			
			for (float x = -1.f; x <= +1.f; x += .1f)
			{
				for (float y = -1.f; y <= +1.f; y += .1f)
				{
					const float value = fbm_slow(Vec2(x, y), H, 8);
					
					setLumif(fabsf(value));
					setAlphaf(1.f);
					fillCube(Vec3(x, y, 0), Vec3(value * .03f));
				}
			}
			
			popDepthTest();
		}
		framework.endDraw();
	}
	
	return 0;
}
