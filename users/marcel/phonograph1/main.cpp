#include "framework.h"
#include "Noise.h"
#include <vector>

static std::vector<float> groove;

static const float depth = .1f;
static const float grooveWidth = .1f;
static const float grooveWidth_2 = grooveWidth / 2.f;

static void calculateGrooveFrame(const int sample, Mat4x4 & frame)
{
	const float angle = sample / 1000.f * float(2.0 * M_PI);
	const float radius = 2.f - sample / 4000.f;
	
	frame = Mat4x4(true).RotateY(angle).Translate(radius, 0, 0);
}

static Vec3 getGroovePositionAndVector(const int sample, Vec3 & vector)
{
#if 1
	Mat4x4 frame;
	calculateGrooveFrame(sample, frame);
	
	const float value = groove[sample] / 10.f;
	
	const float x = frame.Mul4(Vec3(value, 0, 0))[0];
	const float z = frame.Mul4(Vec3(value, 0, 0))[2];
	
	vector = frame.GetAxis(0).CalcNormalized();
	
	return Vec3(x, 0.f, z);
#else
	const float x = groove[sample] / 10.f;
	const float z = sample / 100.f;
	
	vector = Vec3(1, 0, 0);
	
	return Vec3(x, 0.f, z);
#endif
}

static void drawGroove()
{
	setColor(colorWhite);
	
#if 1
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < groove.size() - 1; ++i)
		{
			Vec3 vector1;
			Vec3 vector2;
			
			const Vec3 p1 = getGroovePositionAndVector(i + 0, vector1);
			const Vec3 p2 = getGroovePositionAndVector(i + 1, vector2);
			
			const Vec3 p3 = Vec3(p1[0], -depth, p1[2]);
			
			const Vec3 d1 = p2 - p1;
			const Vec3 d2 = p3 - p2;
			const Vec3 n = (d1 % d2).CalcNormalized();
			
			const Vec3 p1_l = p1 - vector1 * grooveWidth_2;
			const Vec3 p2_l = p2 - vector2 * grooveWidth_2;
			
			const Vec3 p1_r = p1 + vector1 * grooveWidth_2;
			const Vec3 p2_r = p2 + vector2 * grooveWidth_2;
			
			gxColor3f((+n[0] + 1.f) / 2.f, (+n[1] + 1.f) / 2.f, (+n[2] + 1.f) / 2.f);
			gxNormal3f(+n[0], +n[1], +n[2]);
			gxVertex3f(p1_l[0],    0.f, p1_l[2]);
			gxVertex3f(  p1[0], -depth,   p1[2]);
			gxVertex3f(  p2[0], -depth,   p2[2]);
			gxVertex3f(p2_l[0],    0.f, p2_l[2]);
			
			gxColor3f((-n[0] + 1.f) / 2.f, (-n[1] + 1.f) / 2.f, (-n[2] + 1.f) / 2.f);
			gxNormal3f(-n[0], -n[1], -n[2]);
			gxVertex3f(p1_r[0],    0.f, p1_r[2]);
			gxVertex3f(  p1[0], -depth,   p1[2]);
			gxVertex3f(  p2[0], -depth,   p2[2]);
			gxVertex3f(p2_r[0],    0.f, p2_r[2]);
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
	
	for (int i = 0; i < 4000; ++i)
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
				
				pushShaderOutputs("c");
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
