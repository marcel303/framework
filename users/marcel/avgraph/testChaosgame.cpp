#include "framework.h"
#include "Timer.h"

extern int GFX_SX;
extern int GFX_SY;

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
	
	do
	{
		framework.process();
		
		if (mouse.isDown(BUTTON_LEFT))
		{
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
			const float scale = mouse.isDown(BUTTON_LEFT) ? 1.f : (.1f + mouse.x / float(GFX_SX) * (20.f - .1f));
			const float xOffset = mouse.isDown(BUTTON_LEFT) ? 0.f : mouse.y;
			gxScalef(scale, scale, 1.f);
			gxTranslatef(-xOffset, 0.f, 0.f);
			
			setBlend(BLEND_ADD);
			setColorf(.3f, .2f, .1f, 1.f);
			//gxBegin(GL_POINTS);
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
			//gxEnd();
			hqEnd();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete[] output;
	output = nullptr;
}
