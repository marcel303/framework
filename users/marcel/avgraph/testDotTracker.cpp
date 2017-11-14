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

#include "vfxNodes/dotTracker.h"
#include "framework.h"
#include "testBase.h"
#include "Timer.h"
#include <algorithm>

extern const int GFX_SX;
extern const int GFX_SY;

const int kNumPoints = 256;

void testDotTracker()
{
	TrackedDot dots[kNumPoints];
	DotTracker dotTracker;
	
	TrackedDot dots_reference[kNumPoints];
	DotTracker dotTracker_reference;

	int delay = 0;
	
	float time = 0.f;
	
	uint64_t timeAvg = 0;
	
	bool validateReference = false;
	
	bool fast = true;
	
	float maxRadius = 10.f;
	
	do
	{
		SDL_Delay(delay);
		
		//
		
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_a, true))
			delay += 20;
		if (keyboard.wentDown(SDLK_z, true))
			delay -= 5;
		
		if (keyboard.wentDown(SDLK_v))
			validateReference = !validateReference;
		
		if (keyboard.wentDown(SDLK_f))
			fast = !fast;
		
		//
		
		const float dt = std::max(1.f / 1000.f, std::min(framework.timeStep * 10.f, 1.f / 15.f));
		
		time += dt;
		
		//

		for (int i = 0; i < kNumPoints; ++i)
		{
			dots[i].x = std::sin(time / 10.f * (.123f + i / 100.f) + i / 11.f) * GFX_SX/3 + GFX_SX/2;
			dots[i].y = std::cos(time / 10.f * (.234f + i / 111.f) + i / 13.f) * GFX_SY/3 + GFX_SY/2;
		}
		
		std::random_shuffle(dots, dots + kNumPoints);
		
		//
		
		bool validationPassed = true;
		
		if (validateReference)
		{
			dotTracker_reference = dotTracker;
			
			memcpy(dots_reference, dots, sizeof(dots_reference));
		}
		
		auto t1 = g_TimerRT.TimeUS_get();
		
		if (validateReference)
		{
			dotTracker_reference.identify_reference(dots_reference, kNumPoints, dt, maxRadius);
		}
		
		if (fast)
			dotTracker.identify(dots, kNumPoints, dt, maxRadius);
		else
			dotTracker.identify_hgrid(dots, kNumPoints, dt, maxRadius);
		
		auto t2 = g_TimerRT.TimeUS_get();
		
		printf("identify took %.2fms. timeStep: %.2f\n", (t2 - t1) / 1000.0, dt);
		
		timeAvg = (timeAvg * 90 + (t2 - t1) * 10) / 100;
		
		if (validateReference)
		{
			for (int i = 0; i < kNumPoints; ++i)
			{
				Assert(dots[i].id == dots_reference[i].id);
				
				if (dots[i].id != dots_reference[i].id)
					validationPassed = false;
			}
		}
		
		//

		framework.beginDraw(255, 255, 255, 0);
		{
			setFont("calibri.ttf");
			
			setColor(100, 100, 100);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < kNumPoints; ++i)
				{
					hqFillCircle(dots[i].x, dots[i].y, 15.f);
				}
			}
			hqEnd();

			setColor(255, 200, 200);
			for (int i = 0; i < kNumPoints; ++i)
			{
				drawText(dots[i].x, dots[i].y, 14, 0, 0, "%d", dots[i].id);
			}
			
			setColor(0, 0, 0);
			drawText(5, 5, 14, +1, +1, "nextAllocId: %d, delay: %dms, time: %.2fms", dotTracker.nextAllocId, delay, timeAvg / 1000.0);
			drawText(5, 25, 14, +1, +1, "validate: %d [V to toggle]. validationPassed: %d. fast: %d (F to toggle)", validateReference, validationPassed, fast);
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
