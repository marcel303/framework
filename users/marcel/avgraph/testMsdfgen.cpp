#include "framework.h"

extern const int GFX_SX;
extern const int GFX_SY;

static int sampleMethod = 3;
static bool useSuperSampling = true;

void testMsdfgen()
{
	const int kNumSampleMethods = 5;
	const char * sampleMethodNames[] =
	{
		"Marcel1",
		"Marcel2",
		"PHoux1a",
		"PHoux1b",
		"PHoux1_Marcel"
	};
	
	do
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_d))
			useSuperSampling = !useSuperSampling;
		if (keyboard.wentDown(SDLK_a))
			sampleMethod = (sampleMethod + 1 + kNumSampleMethods) % kNumSampleMethods;
		if (keyboard.wentDown(SDLK_z))
			sampleMethod = (sampleMethod - 1 + kNumSampleMethods) % kNumSampleMethods;
		
		framework.beginDraw(255, 255, 255, 0);
		{
			setColor(colorBlack);
			setFont("calibri.ttf");
			drawText(5, GFX_SY - 5, 18, +1, -1, "super sampling: %d, sample method: %d - %s", useSuperSampling, sampleMethod, sampleMethodNames[sampleMethod]);
			
			setFontMSDF("calibri.ttf");
			
			gxPushMatrix();
			{
				gxTranslatef(200, 100, 0);
				
				if (!mouse.isDown(BUTTON_LEFT))
				{
					const float scale = mouse.y / float(GFX_SY) * 8.f;
					const float angle = (mouse.x - GFX_SX/2) / float(GFX_SX) * 20.f;
					const float slide = std::sin(framework.time) * 20.f;
					gxScalef(scale, scale, 1.f);
					gxRotatef(angle, 0.f, 0.f, 1.f);
					gxTranslatef(slide, 0.f, 0.f);
				}
				
				hqBegin(HQ_LINES, true);
				setColor(200, 200, 200);
				hqLine(0, 0, 1, 0, GFX_SY, 1);
				hqLine(0, 0, 1, GFX_SY, 0, 1);
				hqEnd();
				
				setFont("calibri.ttf");
				
				bool verify = keyboard.isDown(SDLK_v);
				
				if (!verify)
					beginTextBatchMSDF();
				{
					int x = 0;
					int y = 0;
					
					for (int i = 0; i < 10; ++i)
					{
						setColor(colorBlue);
						if (verify)
							drawText(x, y, 24, +1, +1, "Hello - World");
						else
							drawTextMSDF(x, y, 24, +1, +1, "Hello - World");
						y += 15;
						
						setColor(colorRed);
						if (verify)
							drawText(x, y, 24, +1, +1, "Lalala / %s", sampleMethodNames[sampleMethod]);
						else
							drawTextMSDF(x, y, 24, +1, +1, "Lalala / %s", sampleMethodNames[sampleMethod]);
						y += 15;
						
						x += 10;
					}
				}
				if (!verify)
					endTextBatchMSDF();
			}
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
