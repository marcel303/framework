#include "framework.h"

/*
Coding Challenge #64.2: Inverse Kinematics
https://www.youtube.com/watch?v=hbgDqyy8bIw
*/

struct Segment
{
	Vec2 a;
	float length;
	float angle;
	Vec2 b;

	void init(const float _x, const float _y, const float _length, const float _angle)
	{
		a.Set(_x, _y);
		length = _length;
		angle = _angle;
	}

	void follow(const float tx, const float ty)
	{
		const Vec2 target(tx, ty);
		const Vec2 dir = target - a;
		
		angle = atan2f(dir[1], dir[0]);
		
		const Vec2 dir2 = dir.CalcNormalized() * -length;
		
		a = target + dir2;
	}
	
	void calculateB()
	{
		const float dx = length * cosf(angle);
		const float dy = length * sinf(angle);

		b.Set(a[0] + dx, a[1] + dy);
	}

	void update()
	{
		calculateB();
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	std::vector<Segment> segments;
	segments.resize(400);
	
	for (auto & segment : segments)
	{
		segment.init(400, 300, 2, 0);
	}

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		for (size_t i = 0; i < segments.size(); ++i)
		{
			auto & segment = segments[i];
			
			if (i == 0)
			{
				//const float target_x = 400.f + 200.f * cosf(framework.time * 10.f / 1.234f);
				//const float target_y = 300.f + 200.f * cosf(framework.time * 30.f / 2.345f);
				//segment.follow(target_x, target_y);
				
				segment.follow(mouse.x, mouse.y);
			}
			else
			{
				Segment & parent = segments[i - 1];
				
				segment.follow(parent.a[0], parent.a[1]);
			}
			
			segment.update();
		}
		
		framework.beginDraw(200, 200, 200, 0);
		{
			setColor(40, 40, 40);
			
			hqBegin(HQ_LINES);
			{
				float strokeSize = 3.f;
				
				for (auto & segment : segments)
				{
					const float strokeSize1 = strokeSize;
					strokeSize *= .995f;
					const float strokeSize2 = strokeSize;
					
					hqLine(segment.b[0], segment.b[1], strokeSize1, segment.a[0], segment.a[1], strokeSize2);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
