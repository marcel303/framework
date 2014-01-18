#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, argv, VIEW_SX, VIEW_SY))
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