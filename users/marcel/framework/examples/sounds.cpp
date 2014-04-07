#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, (const char**)argv, VIEW_SX, VIEW_SY))
		return -1;
	
	const char * sounds[3] = { "button1.ogg", "button2.ogg", "button3.ogg" };
	const char * sound = 0;

	Music music("bgm.ogg");
	bool playMusic = true;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		// process
		
		framework.process();
		
		int index = -1;
		
		if (keyboard.wentDown(SDLK_1))
			index = 0;
		if (keyboard.wentDown(SDLK_2))
			index = 1;
		if (keyboard.wentDown(SDLK_3))
			index = 2;
			
		if (keyboard.wentDown(SDLK_m))
		{
			if (playMusic)
				music.play();
			else
				music.stop();
			playMusic = !playMusic;
		}
		
		if (index != -1)
		{
			sound = sounds[index];
			
			Sound(sound).play();
		}
		
		// draw
				
		framework.beginDraw(0, 0, 0, 0);
		{
			setBlend(BLEND_ALPHA);
			
			Font font("calibri.ttf");
			setFont(font);
			
			setColor(255, 255, 255, 255);
			drawText(VIEW_SX/2, VIEW_SY/4, 30, 0, 0, "press 1, 2 or 3 to play a sound");
			setColor(255, 255, 255, 191);
			drawText(VIEW_SX/2, VIEW_SY/3, 30, 0, 0, "press M to start/stop music");
			
			if (sound)
			{
				setColor(255, 255, 0, 255);
				drawText(VIEW_SX/2, VIEW_SY/2, 20, 0, 0, "begun playing %s..", sound);
			}
			
			setColor(255, 255, 255, 191);
			drawText(VIEW_SX/2, VIEW_SY/2 + 25, 20, 0, 0, playMusic ? "music stopped" : "music playing", sound);
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}