#include "framework.h"
#include "reflection.h"
#include "reflection-bindtofile.h"
#include "Vec2.h"
#include "Vec3.h"
#include <vector>

void test_bindObjectToFile()
{
	TypeDB typeDB;
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("Vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("Vec3", kDataType_Float3);

	struct Polygon
	{
		std::vector<Vec2> points;
	};
	
	typeDB.addStructured<Polygon>("Polygon")
		.add("points", &Polygon::points);
	
	Polygon polygon;
	
	Vec3 color = Vec3(0.f, 1.f, 0.f);
	
	if (bindObjectToFile(&typeDB, &polygon, "out/polygon.txt") == false)
		logError("failed to bind object to file");
	if (bindObjectToFile(&typeDB, &color, "out/polygon-color.txt") == false)
		logError("failed to bind object to file");
	
	// real-time editing needs to be enabled to let framework detect file changes
	framework.enableRealTimeEditing = true;
	
	framework.init(640, 480);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		tickObjectToFileBinding();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setColorf(
				color[0],
				color[1],
				color[2]);
			
			gxBegin(GX_LINE_LOOP);
			{
				for (auto & point : polygon.points)
					gxVertex2f(point[0], point[1]);
			}
			gxEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
}
