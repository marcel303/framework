#include "delaunay/delaunay.h"
#include "delaunay/triangle.h"
#include "delaunay/vector2.h"
#include "framework.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testDelaunay()
{
	const int numPoints = 20;
	
	typedef Vector2<float> Pointf;
	typedef Triangle<float> Trianglef;
	typedef Edge<float> Edgef;
	
	Delaunay<float> triangulation;
	std::vector<Trianglef> triangles;
	std::vector<Edgef> edges;
	
	bool randomize = true;
	
	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_r))
		{
			randomize = true;
		}
		
		//
		
		const float dt = framework.timeStep;
		
		if (randomize)
		{
			randomize = false;
			
			//
			
			std::vector<Pointf> points;
			
			for (int azimuth = -180; azimuth <= 180; azimuth += 5)
			{
				points.push_back(Pointf(azimuth, -90));
			}
			
			for (int elevation = -40; elevation <= 90; elevation += 10)
			{
				for (int azimuth = 0; azimuth <= 180; azimuth += 5)
				{
					Pointf p(azimuth, elevation);
					
					points.push_back(p);
					
					if (azimuth != 0)
					{
						Pointf pMirrored(-azimuth, elevation);
						
						points.push_back(pMirrored);
					}
				}
			}
			
			/*
			points.push_back(Pointf(0.f,    0.f   ));
			points.push_back(Pointf(GFX_SX, 0.f   ));
			points.push_back(Pointf(0.f,    GFX_SY));
			points.push_back(Pointf(GFX_SX, GFX_SY));
			
			for (int i = 0; i < numPoints; ++i)
			{
				Pointf p;
				p.x = random<float>(0.f, GFX_SX);
				p.y = random<float>(0.f, GFX_SY);
				points.push_back(p);
			}
			*/
			
			triangulation = Delaunay<float>();
			
			triangles = triangulation.triangulate(points);
			edges = triangulation.getEdges();
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				for (auto & triangle : triangles)
				{
					auto toScreen = [](Pointf in)
					{
						in.x = (in.x + 180.f) / 360.f * GFX_SX;
						in.y = (in.y + 90.f) / 180.f * GFX_SY;
						
						return in;
					};
					
					Pointf p1 = toScreen(triangle.p1);
					Pointf p2 = toScreen(triangle.p2);
					Pointf p3 = toScreen(triangle.p3);
					
					setColorf(
						p1.x / GFX_SX,
						p1.y / GFX_SY,
						0.f);
					
					hqFillTriangle(
						p2.x, p2.y,
						p1.x, p1.y,
						p3.x, p3.y);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	//exit(0);
}
