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
	
	Color color;
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
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
		
		// grow or shrink our current circles
		
		for (auto & circle : circles)
		{
			// or the circle when possible
			
			const float new_radius = circle.radius + 10.f * framework.timeStep;
		
			if (isValid(circle.x, circle.y, new_radius, &circle))
				circle.radius = new_radius;
			
			// shrink the circle if the mouse is near it
			
			const float dx = circle.x - mouse.x;
			const float dy = circle.y - mouse.y;
			
			const float d = hypotf(dx, dy);
			
			if (d <= circle.radius + 40.f)
			{
				circle.radius *= powf(.1f, framework.timeStep);
			}
		}
		
		// add a new circle whenever the timer elapses
		
		timer -= framework.timeStep;
		
		while (timer <= 0.f)
		{
			timer += .01f;
			
			for (int i = 0; i < 100; ++i) // retry a few times in case isValid returns false for our random location
			{
				// add a new circle
				
				Circle c;
				c.x = random<float>(0.f, 800.f);
				c.y = random<float>(0.f, 600.f);
				c.radius = 1.f;
				c.color = Color::fromHSL(powf(1.f / (circles.size() / 10.f + 1), .1f), .7f, .3f);
				
				if (isValid(c.x, c.y, c.radius, nullptr))
				{
					circles.push_back(c);
					
					break;
				}
			}
		}
		
		framework.beginDraw(200, 200, 200, 0);
		{
			setColor(colorWhite);
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				for (auto & circle : circles)
				{
					setColor(circle.color);
					
					for (int i = 0; i < 3; ++i)
						hqStrokeCircle(circle.x, circle.y, circle.radius / (i + 1), fminf(1.f + circle.radius / 10.f, 3.f));
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
