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
			
			setFont("calibri.ttf");
			
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
