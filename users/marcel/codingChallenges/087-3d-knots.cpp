#include "framework.h"
#include <math.h>

// Coding Challenge #87: 3D Knots
// https://www.youtube.com/watch?v=r6YMKr1X0VA

// http://paulbourke.net/geometry/knots/

static Vec3 knot4(const float t)
{
/*
family:
	x = r * cos(phi) * cos(theta)
	y = r * cos(phi) * sin(theta)
	z = r * sin(phi)

flavour:
	r(beta) = 0.8 + 1.6 * sin(6 * beta)
	theta(beta) = 2 * beta
	phi(beta) = 0.6 * pi * sin(12 * beta)
*/

	const float r = 0.8f + 1.6f * sinf(6 * t);
	const float theta = 2 * t;
	const float phi = 0.6f * float(M_PI) * sinf(12 * t);

	const float x = r * cosf(phi) * cosf(theta);
	const float y = r * cosf(phi) * sinf(theta);
	const float z = r * sinf(phi);
	
	return Vec3(x, y, z);
}

//

static inline void computeBasisVectors(
	const Vec3 & from,
	const Vec3 & to,
	const Vec3 & upAxis,
	Vec3 & out_tangent,
	Vec3 & out_bitangent)
{
	const Vec3 direction = to - from;
	
	out_tangent   = (direction % upAxis     ).CalcNormalized();
	out_bitangent = (direction % out_tangent).CalcNormalized();
}

static void drawTube_UpAxis(const Vec3 * points, const int numPoints, const Vec3 & upAxis, const float radius, const int numSegments)
{
	if (numPoints < 2)
	{
		return;
	}
	
	// compute the cosine-sine table describing a full circle (with wrapped initial point)
	
	float * cs = (float*)alloca((numSegments + 1) * 2 * sizeof(float));
	
	for (int s = 0; s <= numSegments; ++s)
	{
		cs[s * 2 + 0] = cosf(s / float(numSegments) * float(2.0 * M_PI));
		cs[s * 2 + 1] = sinf(s / float(numSegments) * float(2.0 * M_PI));
	}
	
	gxBegin(GX_QUADS);
	{
		Vec3 t[2];
		Vec3 b[2];
		
		int index1;
		int index2;
		
		for (int i = 0; i < numPoints - 1; ++i)
		{
			index1 = i - 1 >= 0             ? i - 1 : i;
			index2 = i + 1 <= numPoints - 1 ? i + 1 : i;
			computeBasisVectors(points[index1], points[index2], upAxis, t[0], b[0]);
			
			index1 = i;
			index2 = i + 2 <= numPoints - 1 ? i + 2 : i + 1;
			computeBasisVectors(points[index1], points[index2], upAxis, t[1], b[1]);
			
			const Vec3 & p1 = points[i + 0];
			const Vec3 & p2 = points[i + 1];
			
			for (int s = 0; s < numSegments; ++s)
			{
				const Vec3 n00 = t[0] * cs[(s + 0) * 2 + 0] + b[0] * cs[(s + 0) * 2 + 1];
				const Vec3 n10 = t[0] * cs[(s + 1) * 2 + 0] + b[0] * cs[(s + 1) * 2 + 1];
				const Vec3 n11 = t[1] * cs[(s + 1) * 2 + 0] + b[1] * cs[(s + 1) * 2 + 1];
				const Vec3 n01 = t[1] * cs[(s + 0) * 2 + 0] + b[1] * cs[(s + 0) * 2 + 1];
				
				const Vec3 p00 = p1 + n00 * radius;
				const Vec3 p10 = p1 + n10 * radius;
				const Vec3 p11 = p2 + n11 * radius;
				const Vec3 p01 = p2 + n01 * radius;
				
				gxNormal3fv(&n01[0]); gxVertex3fv(&p01[0]);
				gxNormal3fv(&n11[0]); gxVertex3fv(&p11[0]);
				gxNormal3fv(&n10[0]); gxVertex3fv(&p10[0]);
				gxNormal3fv(&n00[0]); gxVertex3fv(&p00[0]);
			}
		}
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	framework.allowHighDpi = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(800, 600))
		return -1;
	
	Camera3d camera;
	camera.position[2] = -2.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			camera.pushViewMatrix();
			
			pushDepthTest(true, DEPTH_LESS);
			pushCullMode(CULL_BACK, CULL_CCW);
			{
				gxPushMatrix();
				{
					gxRotatef(framework.time * 1.23f, 0, 1, 0);
					
					setColor(colorWhite);
					
				#if false
					gxBegin(GX_LINE_LOOP);
					{
						for (int i = 0; i < 10000; ++i)
						{
							const Vec3 p = knot4(float(M_PI) * i / 10000.f);
							
							gxVertex3f(p[0], p[1], p[2]);
						}
					}
					gxEnd();
				#endif
				
				#if true
					const int kNumPoints = 4000;
					
					Vec3 p[kNumPoints];
					
					const float T = lerp<float>(.4f, 1.f, (sinf(framework.time / 3.45f) + 1.f) / 2.f);
					
					for (int i = 0; i < kNumPoints; ++i)
					{
						// note : dividing by 'kNumPoints - 1' will duplicate the first point, ensuring the knot is closed when drawn
						
						p[i] = knot4(float(M_PI) * i / float(kNumPoints - 1) * T);
					}
					
					pushShaderOutputs("n");
					drawTube_UpAxis(p, kNumPoints, Vec3(0, 1, 0), lerp<float>(.02f, .13f, (sinf(framework.time) + 1.f) / 2.f), 20);
					popShaderOutputs();
				#endif
				}
				gxPopMatrix();
			}
			popCullMode();
			popDepthTest();
			
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
