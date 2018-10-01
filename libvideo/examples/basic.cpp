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
#include "video.h"

static void drawProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration);

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	if (framework.init(0, nullptr, 800, 400))
	{
		MediaPlayer mp;

		mp.openAsync("lucy.mp4", MP::kOutputMode_RGBA);

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			mp.presentTime += framework.timeStep;
			
			mp.tick(mp.context, true);
			
			if (mp.presentedLastFrame(mp.context) || mouse.wentDown(BUTTON_LEFT))
			{
				auto openParams = mp.context->openParams;
				mp.close(false);
				mp.presentTime = 0.0;
				mp.openAsync(openParams);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				// draw the video frame
				
				const GLuint texture = mp.getTexture();

				if (texture != 0)
				{
					setColor(colorWhite);
					gxSetTexture(texture);
					pushBlend(BLEND_OPAQUE);
					drawRect(0, 0, 800, 400);
					popBlend();
					gxSetTexture(0);
				}
				
				// draw the progress bar
				
				int sx;
				int sy;
				double duration;
				
				if (mp.getVideoProperties(sx, sy, duration))
				{
					drawProgressBar(20, 400-20-20, 200, 20, mp.presentTime, duration);
				}
			}
			framework.endDraw();
		}
		
		mp.close(true);
		
		framework.shutdown();
	}

	return 0;
}

static void drawProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration)
{
	const double t = time / duration;
	
	setColor(63, 127, 255, 127);
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	hqFillRoundedRect(x, y, x + sx * t, y + sy, 6);
	hqEnd();
	
	setColor(63, 127, 255, 255);
	hqBegin(HQ_STROKED_ROUNDED_RECTS);
	hqStrokeRoundedRect(x, y, x + sx, y + sy, 6, 2);
	hqEnd();
	
	setFont("calibri.ttf");
	pushFontMode(FONT_SDF);
	setColor(colorWhite);
	
	const int hours = int(floor(time / 3600.0));
	const int minutes = int(floor(fmod(time / 60.0, 60.0)));
	const int seconds = int(floor(fmod(time, 60.0)));
	const int hundreds = int(floor(fmod(time, 1.0) * 100.0));
	
	const int d_hours = int(floor(duration / 3600.0));
	const int d_minutes = int(floor(fmod(duration / 60.0, 60.0)));
	const int d_seconds = int(floor(fmod(duration, 60.0)));
	const int d_hundreds = int(floor(fmod(duration, 1.0) * 100.0));
	
	drawText(x + 10, y + sy/2, 12, +1, 0, "%02d:%02d:%02d.%02d / %02d:%02d:%02d.%02d", hours, minutes, seconds, hundreds, d_hours, d_minutes, d_seconds, d_hundreds);
	popFontMode();
}
