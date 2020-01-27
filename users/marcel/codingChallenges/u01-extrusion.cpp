#include "framework.h"

static void drawExtrusion(Vec2Arg p1, Vec2Arg p2, const Vec2 * points, const int numPoints, const bool closed)
{
	hqBegin(HQ_LINES);
	
	float angle1;
	
	for (int i = 0; i < (closed ? (numPoints + 1) : (numPoints - 1)); ++i)
	{
		const Vec2 & point1 = points[(i + 0) % numPoints];
		const Vec2 & point2 = points[(i + 1) % numPoints];
		
		const Vec2 delta = point2 - point1;
		
		const float angle2 = - atan2(delta[1], delta[0]);
		
		if (closed)
		{
			if (i == 0)
			{
				angle1 = angle2;
				continue;
			}
		}
		else
		{
			if (i == 0)
				angle1 = angle2;
		}
		
		Mat4x4 rotationMatrix1;
		rotationMatrix1.MakeRotationZ(angle1);
		rotationMatrix1.MakeRotationZ(angle1);
		
		Mat4x4 rotationMatrix2;
		rotationMatrix2.MakeRotationZ(angle2);
		rotationMatrix2.MakeRotationZ(angle2);
		
		const Vec2 p1_transformed_1 = point1 + rotationMatrix1.Mul(p1);
		const Vec2 p2_transformed_1 = point1 + rotationMatrix1.Mul(p2);
		
		const Vec2 p1_transformed_2 = point2 + rotationMatrix2.Mul(p1);
		const Vec2 p2_transformed_2 = point2 + rotationMatrix2.Mul(p2);
		
		const float strokeSize = 1.f;
		const float strokeSize2 = .5f;
		
		hqLine(p1_transformed_1[0], p1_transformed_1[1], strokeSize, p1_transformed_2[0], p1_transformed_2[1], strokeSize);
		hqLine(p2_transformed_1[0], p2_transformed_1[1], strokeSize, p2_transformed_2[0], p2_transformed_2[1], strokeSize);
		
		hqLine(p1_transformed_1[0], p1_transformed_1[1], strokeSize2, p2_transformed_1[0], p2_transformed_1[1], strokeSize2);
		
		angle1 = angle2;
	}
	
	hqEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 800))
		return -1;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		Vec2 p1(0.f, -30.f + sinf(framework.time * 10.f / 12.34f) * 26.f);
		Vec2 p2(0.f, +30.f + sinf(framework.time * 10.f / 13.45f) * 26.f);
		
		Vec2 points[200];
		for (int i = 0; i < 200; ++i)
		{
			const float t = 2.f * float(M_PI) * i / float(200);
			
			//points[i][0] = i* 4;
			points[i][1] = sinf(t * 6.f + framework.time) * 10.f;
			//points[i][0] += sinf(t * 8.f + framework.time * 10.f / 1.23) * 4.f;
			//points[i][1] += sinf(t * 10.f + framework.time / 2.34) * 2.f;
			
			points[i][0] += sinf(t) * 200.f;
			points[i][1] += cosf(t) * 200.f;
		}
		
		framework.beginDraw(10, 12, 14, 0);
		{
			gxTranslatef(400, 400, 0);
			pushColorPost(POST_PREMULTIPLY_RGB_WITH_ALPHA);
			pushBlend(BLEND_MAX);
			drawExtrusion(p1, p2, points, 200, true);
			popBlend();
			popColorPost();
		}
		framework.endDraw();
	}
	
	return 0;
}
