#include "framework.h"
#include <array>
#include <earcut.hpp>

using Point = std::array<float, 2>;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(800, 600))
		return -1;

	std::vector<Point> points;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		if (mouse.wentDown(BUTTON_LEFT))
			points.clear();
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			Point p;
			p.at(0) = mouse.x;
			p.at(1) = mouse.y;
			points.push_back(p);
		}
		
		framework.beginDraw(255, 255, 255, 0);
		{
			std::vector<std::vector<Point>> polygon;
			polygon.push_back(points);
			
			auto tess = mapbox::earcut<uint32_t>(polygon);
			
			setColor(255, 255, 200);
			gxBegin(GX_TRIANGLES);
			{
				for (auto index : tess)
				{
					auto & p = points[index];
					
					gxVertex2f(p.at(0), p.at(1));
				}
			}
			gxEnd();
			
			setColor(255, 200, 100);
			hqBegin(HQ_LINES);
			{
				const auto numTriangles = tess.size() / 3;
				
				for (size_t i = 0; i < numTriangles; ++i)
				{
					const auto index1 = tess[i * 3 + 0];
					const auto index2 = tess[i * 3 + 1];
					const auto index3 = tess[i * 3 + 2];
					
					auto & p1 = points[index1];
					auto & p2 = points[index2];
					auto & p3 = points[index3];
					
					hqLine(p1[0], p1[1], 1.f, p2[0], p2[1], 1.f);
					hqLine(p2[0], p2[1], 1.f, p3[0], p3[1], 1.f);
					hqLine(p3[0], p3[1], 1.f, p1[0], p1[1], 1.f);
				}
			}
			hqEnd();
			
			setColor(100, 100, 100);
			gxBegin(GX_LINE_LOOP);
			{
				for (auto & point : points)
				{
					gxVertex2f(point.at(0), point.at(1));
				}
			}
			gxEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
