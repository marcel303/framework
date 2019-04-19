#include "framework.h"

/*
Coding Challenge #34: Diffusion-Limited Aggregation
https://www.youtube.com/watch?v=Cl_Gjj80gPE
*/

const int GFX_SX = 400;
const int GFX_SY = 400;

const int kPointRadius = 6;
const int kWalkerRadius = 6;

const int kCollisionRadius = kPointRadius + kWalkerRadius;

struct Point
{
	int x;
	int y;
};

struct Walker
{
	int x;
	int y;
	
	bool dead = false;

	void update()
	{
		const int dx = (rand() % 5) - 2;
		const int dy = (rand() % 5) - 2;

		x += dx;
		y += dy;
		
		if (x < 0 || x >= GFX_SX ||
			y < 0 || y >= GFX_SY)
		{
			dead = true;
		}
	}
};

int main(int argc, const char * argv[])
{
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;

	std::vector<Point> points;

	Point middle;
	middle.x = GFX_SX/2;
	middle.y = GFX_SY/2;
	points.push_back(middle);

	std::vector<Walker> walkers;

	bool first_frame = true;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		// add a new walker when there are not walkers anymore
		
		while (points.size() + walkers.size() < 200)
		{
			Walker walker;
			
			if (first_frame)
			{
				// add walkers all over the screen initially, to speed up them getting stuck
				
				walker.x = rand() % GFX_SX;
				walker.y = rand() % GFX_SY;
			}
			else
			{
				// add walkers in a circle around the middle otherwise, to avoid spawning new walkers inside the structure
				// that's being created and create more of a 'outside influence'
				
				const float angle = random<float>(0.f, float(M_PI) * 2.f);
				const float distance = GFX_SY*4/5/2;
				walker.x = GFX_SX/2 + cosf(angle) * distance;
				walker.y = GFX_SY/2 + sinf(angle) * distance;
			}
			
			walkers.push_back(walker);
		}
		
		first_frame = false;
		
		
		// update walkers
		
		const int kCollisionRadiusSquared = kCollisionRadius * kCollisionRadius;
		
	for (int i = 0; i < 20; ++i) // loop to speed up the animation
		for (auto & walker : walkers)
		{
			walker.update();
			
			if (walker.dead == false)
			{
				for (auto & point : points)
				{
					const int dx = point.x - walker.x;
					const int dy = point.y - walker.y;
					
					const int distanceSquared = dx * dx + dy * dy;
					
					if (distanceSquared <= kCollisionRadiusSquared)
					{
						walker.dead = true;
					
						Point point;
						point.x = walker.x;
						point.y = walker.y;
						points.push_back(point);
					
						break;
					}
				}
			}
		}
		
		auto pos = std::remove_if(walkers.begin(), walkers.end(), [](Walker & walker) { return walker.dead; });
		walkers.erase(pos, walkers.end());

		framework.beginDraw(200, 200, 200, 0);
		{
			// draw points
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (size_t i = 0; i < points.size(); ++i)
				{
					auto & point = points[i];
					setColor(Color::fromHSL(
						i / float(points.size())/2.f + .5f,
						.5f + .5f + i / float(points.size()),
						.5f - .5f * i / float(points.size())));
					hqFillCircle(point.x, point.y, kPointRadius);
				}
			}
			hqEnd();
			
			// draw walkers
			
			setColor(100, 100, 100);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (auto & walker : walkers)
				{
					hqFillCircle(walker.x, walker.y, kWalkerRadius);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
