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
				const GLuint texture = mp.getTexture();

				if (texture != 0)
				{
					gxSetTexture(texture);
					pushBlend(BLEND_OPAQUE);
					drawRect(0, 0, 800, 400);
					popBlend();
					gxSetTexture(0);
				}
			}
			framework.endDraw();
		}
	}

	return 0;
}
