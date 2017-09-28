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
#include "testBase.h"
#include "Timer.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testChaosGame()
{
	const int kNumIterations = 100000;
	
	int64_t * output = new int64_t[kNumIterations * 2];
	
	for (int i = 0; i < kNumIterations * 2; ++i)
		output[i] = 0;
	
	const int kNumPoints = 4;
	
	int64_t pointX[kNumPoints];
	int64_t pointY[kNumPoints];
	
	pointX[0] = 0ll   << 32ll;
	pointY[0] = 0ll   << 32ll;
	
	pointX[1] = 600ll << 32ll;
	pointY[1] = 0ll   << 32ll;
	
	pointX[2] = 300ll << 32ll;
	pointY[2] = 600ll << 32ll;
	
	for (int i = 3; i < kNumPoints; ++i)
	{
		pointX[i] = int64_t(rand() % 600) << 32ll;
		pointY[i] = int64_t(rand() % 600) << 32ll;
	}
	
	bool showInstructions = true;
	
	do
	{
		framework.process();
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			showInstructions = false;
			
			auto t1 = g_TimerRT.TimeUS_get();
			
			pointX[2] = int64_t(mouse.x) << 32ll;
			pointY[2] = int64_t(mouse.y) << 32ll;
			
			int64_t x = pointX[0];
			int64_t y = pointY[0];
			
			int64_t * __restrict outputItr = output;
			
			for (int i = 0; i < kNumIterations; ++i)
			{
				const int index = rand() % kNumPoints;
				
				x = (x * 32 + pointX[index] * 40) >> 6;
				y = (y * 32 + pointY[index] * 40) >> 6;
				
				*outputItr++ = x;
				*outputItr++ = y;
			}
			
			auto t2 = g_TimerRT.TimeUS_get();
			
			printf("eval took %lldus\n", (t2 - t1));
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				const float scale = mouse.isDown(BUTTON_LEFT) ? 1.f : (.1f + mouse.x / float(GFX_SX) * (20.f - .1f));
				const float xOffset = mouse.isDown(BUTTON_LEFT) ? 0.f : mouse.y;
				gxScalef(scale, scale, 1.f);
				gxTranslatef(-xOffset, 0.f, 0.f);
				
				pushBlend(BLEND_ADD);
				setColorf(.3f, .2f, .1f, 1.f);
				
				hqBegin(HQ_FILLED_CIRCLES);
				{
					const int64_t * __restrict outputItr = output;
					
					//const float percentage = std::fmod(framework.time / 4.f, 1.f);
					//const int numPointsToDraw = kNumIterations * percentage;
					const int numPointsToDraw = kNumIterations;
					
					for (int i = 0; i < numPointsToDraw; ++i)
					{
						const int x = outputItr[0] >> 32;
						const int y = outputItr[1] >> 32;
						
						//gxVertex2f(x, y);
						hqFillCircle(x, y, .4f);
						
						outputItr += 2;
					}
				}
				hqEnd();
				
				popBlend();
			}
			gxPopMatrix();
			
			if (showInstructions)
			{
				setFont("calibri.ttf");
				setColorf(.9f, .6f, .3f);
				drawText(GFX_SX/2, GFX_SY/2, 48, 0, 0, "Click anywhere to engage in the chaos game!");
				drawText(GFX_SX/2, GFX_SY/2+40, 24, 0, 0, "(Search YouTube for 'numberphile' and 'chaos game')");
			}
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
	
	delete[] output;
	output = nullptr;
}
