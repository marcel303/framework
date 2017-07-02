#include "framework.h"
#include "graph.h"
#include "soundmix.h"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1300;
const int GFX_SY = 800;

int main(int argc, char * argv[])
{
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		bool stop = false;
		
		PcmData sound;
		sound.load("sound.ogg", 0);
		
		AudioSourcePcm pcm;
		pcm.init(&sound, 0);
		
		AudioSourceSine sine;
		sine.init(0.f, 400.f);
		
		AudioSourceMix mix;
		mix.add(&pcm, 1.f);
		mix.add(&sine, 1.f);
		
		do
		{
			framework.process();
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = false;
			
			//

			framework.beginDraw(0, 0, 0, 0);
			{
				pushFontMode(FONT_SDF);
				{
					setFont("calibri.ttf");
					
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD -");
				}
				popFontMode();
			}
			framework.endDraw();
		} while (stop == false);

		framework.shutdown();
	}

	return 0;
}
