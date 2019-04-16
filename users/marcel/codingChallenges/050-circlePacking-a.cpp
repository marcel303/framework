#include "framework.h"

/*
Circle packing coding challenge
https://youtu.be/QHEQuoIKgNE
*/

struct Circle
{
	float x;
	float y;
	float radius;
};

int main(int argc, const char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	std::vector<Circle> circles;
	
	auto isValid = [&](const float x, const float y, const float radius, const Circle * circle_to_ignore)
	{
		for (auto & circle : circles)
		{
			if (&circle == circle_to_ignore)
				continue;
			
			const float dx = circle.x - x;
			const float dy = circle.y - y;
			const float d = hypotf(dx, dy);
			
			if (d < radius + circle.radius + 3.f)
				return false;
		}
		
		return true;
	};
	
	float timer = 0.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
			circles.clear();
		
		// grow our current circles
		
		for (auto & circle : circles)
		{
			const float new_radius = circle.radius + 10.f * framework.timeStep;
			
			if (isValid(circle.x, circle.y, new_radius, &circle))
				circle.radius = new_radius;
		}
		
		// add a new circle whenever the timer elapses
		
		timer -= framework.timeStep;
		
		while (timer <= 0.f)
		{
			timer += .01f;
			
			// add a new circle
			
			Circle c;
			c.x = random<float>(0.f, 800.f);
			c.y = random<float>(0.f, 600.f);
			c.radius = 1.f;
			
			if (isValid(c.x, c.y, c.radius, nullptr))
			{
				circles.push_back(c);
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setColor(colorWhite);
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				for (auto & circle : circles)
				{
					hqStrokeCircle(circle.x, circle.y, circle.radius, fminf(1.f + circle.radius / 10.f, 3.f));
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
