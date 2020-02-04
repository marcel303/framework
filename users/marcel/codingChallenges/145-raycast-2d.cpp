// The Coding Train
// Coding Challenge #145: 2D Raycasting
// https://www.youtube.com/watch?v=TOEi6T2mtHo

#include "framework.h"
#include <limits>
#include <vector>

struct LineSegment
{
	float x1;
	float y1;
	float x2;
	float y2;
};

std::vector<LineSegment> lineSegments;

LineSegment lineSegmentBeingRecorded;

static void rayCast(float x, float y, float dx, float dy, float & t)
{
	for (auto & lineSegment : lineSegments)
	{
		// calculate a normal vector for this line segment and make it into a plane equation
		
		const float line_dx = lineSegment.x2 - lineSegment.x1;
		const float line_dy = lineSegment.y2 - lineSegment.y1;
		
		const float line_nx = -line_dy;
		const float line_ny = +line_dx;
		const float line_nd = lineSegment.x1 * line_nx + lineSegment.y1 * line_ny;
		
		// calculate distance from (x, y) to the line segment's plane
		
		const float point_t =  x * line_nx +  y * line_ny - line_nd;
		const float delta_t = dx * line_nx + dy * line_ny;
		const float intersection_t = - point_t / delta_t;
		
		// do we intersect the plane going forward ?
		
		if (intersection_t >= 0.f)
		{
			// calculate the intersection point
			
			const float intersection_x = x + dx * intersection_t;
			const float intersection_y = y + dy * intersection_t;
			
			// check if the intersection point lies within the line segment
			// calculate two additional planes, this time perpendicular to the line segment
			// the planes have their origins in (x1, y1) and (x2, y2) of the line segment
			
			const float line_tx = line_dx;
			const float line_ty = line_dy;
			const float line_td1 = line_tx * lineSegment.x1 + line_ty * lineSegment.y1;
			const float line_td2 = line_tx * lineSegment.x2 + line_ty * lineSegment.y2;
			
			// check if the intersection point lies after and before the first and second plane
			
			const float line_t = intersection_x * line_tx + intersection_y * line_ty;
			
			if ((line_t >= line_td1 && line_t <= line_td2) || mouse.isDown(BUTTON_LEFT))
			{
				t = fminf(t, intersection_t);
			}
		}
	}
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.allowHighDpi = false;
	
	if (!framework.init(800, 800))
		return -1;

	// precomputed ray directions for the visualization
	
	std::vector<Vec2> ray_directions;
	ray_directions.resize(200);
	
	for (size_t i = 0; i < ray_directions.size(); ++i)
	{
		const float a = float(2.f * M_PI * i / ray_directions.size());
		ray_directions[i].Set(cosf(a), sinf(a));
	}
	
	for (int i = 0; i < 2; ++i)
	{
		LineSegment lineSegment;
		lineSegment.x1 = random<float>(0.f, 800.f);
		lineSegment.y1 = random<float>(0.f, 800.f);
		lineSegment.x2 = random<float>(0.f, 800.f);
		lineSegment.y2 = random<float>(0.f, 800.f);
		lineSegments.push_back(lineSegment);
	}
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			lineSegmentBeingRecorded.x1 = mouse.x;
			lineSegmentBeingRecorded.y1 = mouse.y;
		}
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			lineSegmentBeingRecorded.x2 = mouse.x;
			lineSegmentBeingRecorded.y2 = mouse.y;
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			lineSegments.push_back(lineSegmentBeingRecorded);
		}
		
		if (mouse.wentDown(BUTTON_RIGHT))
		{
			lineSegments.clear();
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			setColor(100, 100, 100);
			gxBegin(GX_LINES);
			{
				for (auto & ray_direction : ray_directions)
				{
					const float dx = ray_direction[0];
					const float dy = ray_direction[1];

					float t = 1000.f;
					rayCast(
						mouse.x,
						mouse.y,
						dx,
						dy,
						t);

					gxVertex2f(mouse.x, mouse.y);
					gxVertex2f(mouse.x + dx * t, mouse.y + dy * t);
				}
			}
			gxEnd();

			for (auto & lineSegment : lineSegments)
			{
				setColor(colorWhite);

				drawLine(
					lineSegment.x1,
					lineSegment.y1,
					lineSegment.x2,
					lineSegment.y2);
			}

			setColor(colorYellow);
			drawLine(
				lineSegmentBeingRecorded.x1,
				lineSegmentBeingRecorded.y1,
				lineSegmentBeingRecorded.x2,
				lineSegmentBeingRecorded.y2);
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
