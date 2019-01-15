#include "dataTypes.h"
#include "framework.h"

/*
Coding Challenge #129: Koch Fractal Snowflake
https://www.youtube.com/watch?v=X8bXDKqMsXE
*/

typedef FastList<Vec2, 1000000> PointList;

void addSubdivisions(PointList & points, const float scale)
{
	auto p1_itr = points.head;
	auto p2_itr = p1_itr->next;

	while (p2_itr != nullptr)
	{
		auto & p1 = p1_itr->value;
		auto & p2 = p2_itr->value;
		
		const Vec2 delta = p2 - p1;
		
		const Vec2 normal(delta[1], -delta[0]);
		
		const Vec2 sub1 = p1 + delta * 1.f / 3.f;
		const Vec2 sub2 = p1 + delta * 1.f / 2.f + normal * scale;
		const Vec2 sub3 = p1 + delta * 2.f / 3.f;
		
		points.insert_after(p1_itr, sub3);
		points.insert_after(p1_itr, sub2);
		points.insert_after(p1_itr, sub1);
		
		p1_itr = p2_itr;
		p2_itr = p2_itr->next;
	}
}

int main(int argc, char * argv[])
{
	if (!framework.init(800, 800))
		return -1;

	PointList points;
	
	while (!framework.quitRequested)
	{
		framework.process();

		// clear the list of points
		
		points.reset();
		
		// add the initial triangle (the first points is duplicated which is why we start out with four points
		
		for (int i = 0; i < 4; ++i)
		{
			const float angle = float(M_PI) * 2.f * i / 3.f;
			const float x = 300.f * cosf(angle);
			const float y = 300.f * sinf(angle);
			
			points.push_back(Vec2(x, y));
		}

		// create subdivisions, by splitting up each line segment between point A and B into four line segments
		// in his video Daniel works with explicit line segments. I chose to use just a list of points here and
		// construct the line segments when rendering, because it's both more efficient to compute and (for me)
		// easier to understand and implement
		
		// note : we play around with the scale factor here
		const float scale = (sinf(framework.time) + .5f) / 1.5f * .7f;
		//const float scale = sinf(M_PI / 3.f) / 3.f;
		
		for (int i = 0; i < 5; ++i)
			addSubdivisions(points, scale);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxTranslatef(400, 400, 0);
			
			// zoom in and out the fractal when the left mouse button is pressed
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				const float scale = (sinf(framework.time / 3.21f) + 2.f) / 3.f * 2.f;
				
				gxScalef(scale, scale, 1);
			}
			
			hqBegin(HQ_LINES);
			{
				setColor(colorWhite);
				auto p1_itr = points.head;
				auto p2_itr = p1_itr->next;

				while (p2_itr != nullptr)
				{
					auto & p1 = p1_itr->value;
					auto & p2 = p2_itr->value;

					hqLine(p1[0], p1[1], 1.f, p2[0], p2[1], 1.f);
					
					p1_itr = p2_itr;
					p2_itr = p2_itr->next;
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
