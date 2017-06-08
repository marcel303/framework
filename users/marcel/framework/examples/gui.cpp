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

#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, (const char**)argv, VIEW_SX, VIEW_SY))
		return -1;

	int buttonId = 0;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		// process
		
		framework.process();
		
		ui.process();
		
		if (keyboard.wentDown(SDLK_a))
		{
			char name[32];
			sprintf(name, "btn_%d", buttonId++);
			
			Dictionary & button = ui[name];
			button.parse("type:button image:button.png image_over:button-over.png image_down:button-down.png action:sound sound:button2.ogg");
			button.setInt("x", rand() % VIEW_SX);
			button.setInt("y", rand() % VIEW_SY);
		}
		
		// draw
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setBlend(BLEND_ALPHA);

			ui.draw();
			
			Font font("calibri.ttf");
			setFont(font);
			
			setColor(255, 255, 255, 255);
			drawText(VIEW_SX/2, VIEW_SY/4, 30, 0, 0, "press A to add a button");
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}