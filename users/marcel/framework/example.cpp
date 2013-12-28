#include <SDL/SDL.h>
#include "framework.h"

int main(int argc, char * argv[])
{
	framework.setMinification(2);
	framework.init(argc, argv, 1920, 1280);
	
	bool down = false;
	int x = 0;
	int y = 0;
	
	Music("bgm.ogg").play();
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		bool newDown = keyboard.isDown(SDLK_SPACE);
		
		if (newDown != down)
		{
			down = newDown;
			if (down)
			{
				Sound("player/down.wav").play();
				y += 10;
			}
			else
			{
				Sound sound("player/up.wav");
				sound.setVolume(50);
				sound.setSpeed(200);
				sound.play();
			}
		}
		
		if (keyboard.isDown(SDLK_LEFT))
			x--;
		if (keyboard.isDown(SDLK_RIGHT))
			x++;
		
		for (int i = 0; i < 4; ++i)
		{
			if (gamepad[i].isConnected())
			{
				if (gamepad[i].isDown(DPAD_LEFT))
					x--;
				if (gamepad[i].isDown(DPAD_RIGHT))
					x--;
			}
		}
		
		framework.beginDraw(0, 255, 255, 0);
		{
			setBlend(BLEND_ALPHA);
			
			setColor(255, 255, 255);
			Sprite("background.png").draw();
			
			setColor(255, 255, 255, 255 - y);
			Sprite("player/sprite.png", 20, 30).drawEx(x, y, 0, 1, BLEND_ALPHA);
			
			Font font("test.ttf");
			setFont(font);
			
			setColor(0, 0, 0, 255);
			drawText(mouse.getX(), mouse.getY(), 28, 0, 0, "PROTO[%d]TYPE", rand() % 10);
			drawText(mouse.getX(), mouse.getY() + 30, 20, 0, 0, "(demo only)", rand() % 10);
			
			setColor(255, 0, 0, 255);
			drawLine(0, 0, 1920, 1280);
			drawLine(1920, 0, 0, 1280);
		}
		framework.endDraw();
	}
	
	return 0;
}
