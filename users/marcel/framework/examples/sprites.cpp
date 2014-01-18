#include <cmath>
#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, argv, VIEW_SX, VIEW_SY))
		return -1;
	
	float angle = 0.f;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		angle += framework.timeStep * 10.f;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			for (int x = 0; x < 8; ++x)
			{
				for (int y = 0; y < 8; ++y)
				{
					const float px = VIEW_SX / 8 * (x + .5f);
					const float py = VIEW_SY / 8 * (y + .5f);
					const float scale = 2.f;
					const float angleSpeed = (x * 8 + y - 31.5f);
					
					Sprite sprite("sprite.png");
					sprite.x = px;
					sprite.y = py;
					sprite.angle = angle * angleSpeed;
					sprite.scale = 1.5f;
					sprite.pivotX = sprite.getWidth() / 2.f;
					sprite.pivotY = sprite.getHeight() / 2.f;
					
					setColorf(1.f, 1.f, 1.f, 1.f, 4.f / (std::abs(angleSpeed) + 1e-10f));
					sprite.draw();
				}
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}