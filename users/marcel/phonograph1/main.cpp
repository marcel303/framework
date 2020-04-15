#include "framework.h"
#include "Noise.h"
#include <vector>

static std::vector<float> groove;

static void drawGroove()
{
	setColor(colorWhite);
	
#if 1
	gxBegin(GX_QUADS);
	{
		const float depth = .1f;
		const float grooveWidth = .1f;
		const float grooveWidth_2 = grooveWidth / 2.f;
		
		for (int i = 0; i < groove.size() - 1; ++i)
		{
			const float x1 = groove[i + 0] / 10.f;
			const float x2 = groove[i + 1] / 10.f;
			
			const float y1 = (i + 0) / 100.f;
			const float y2 = (i + 1) / 100.f;
			
			const Vec3 p1 = Vec3(x1, y1, 0.f);
			const Vec3 p2 = Vec3(x1, y1, depth);
			const Vec3 p3 = Vec3(x2, y2, 0.f);
			
			const Vec3 d1 = p2 - p1;
			const Vec3 d2 = p3 - p1;
			const Vec3 n = (d1 % d2).CalcNormalized();
			
			gxNormal3f(+n[0], +n[1], +n[2]);
			gxVertex3f(x1 - grooveWidth_2, y1, 0.f);
			gxVertex3f(0.f, y1, depth);
			gxVertex3f(0.f, y2, depth);
			gxVertex3f(x2 - grooveWidth_2, y2, 0.f);
			
			gxNormal3f(-n[0], -n[1], -n[2]);
			gxVertex3f(x1 + grooveWidth_2, y1, 0.f);
			gxVertex3f(0.f, y1, depth);
			gxVertex3f(0.f, y2, depth);
			gxVertex3f(x2 + grooveWidth_2, y2, 0.f);
		}
	}
	gxEnd();
#else
	gxBegin(GX_LINE_STRIP);
	{
		for (int i = 0; i < groove.size(); ++i)
		{
			gxVertex3f(groove[i] / 100.f, i / 100.f, 0.f);
		}
	}
	gxEnd();
#endif
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	for (int i = 0; i < 1024; ++i)
	{
		const float value = raw_noise_1d(i / 40.f);
		
		groove.push_back(value);
	}
	
	Camera3d camera;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				pushDepthTest(true, DEPTH_LESS);
				
				setColor(colorWhite);
				drawGrid3dLine(10, 10, 0, 2);
				
				pushShaderOutputs("n");
				drawGroove();
				popShaderOutputs();
				
				popDepthTest();
			}
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
