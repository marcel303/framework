/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cmath>
#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
	if (!framework.init(argc, (const char**)argv, VIEW_SX, VIEW_SY))
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
