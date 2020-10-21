/*
	Copyright (C) 2020 Marcel Smit
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
	setupPaths(CHIBI_RESOURCE_PATHS);

	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	struct Prim
	{
		HQ_TYPE type;
	};
	
	Prim prims[] =
	{
		{ HQ_FILLED_RECTS },
		{ HQ_STROKED_RECTS },
		{ HQ_FILLED_CIRCLES },
		{ HQ_STROKED_CIRCLES },
		{ HQ_FILLED_TRIANGLES },
		{ HQ_STROKED_TRIANGLES }
	};
	
	const char * modes[] =
	{
		"useScreenSize: false, scaled: false",
		"useScreenSize: false, scaled: true",
		"useScreenSize: true,  scaled: false",
		"useScreenSize: true,  scaled: true"
	};
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setColor(colorWhite);
			hqSetGradient(GRADIENT_LINEAR,
				Mat4x4(true)
				.Rotate(float(-M_PI/2.f), Vec3(0, 0, 1))
				.Scale(1.f / VIEW_SY), Color(0, 0, 100), Color(0, 0, 0), COLOR_MUL);
			hqBegin(HQ_FILLED_RECTS);
			hqFillRect(0, 0, VIEW_SX, VIEW_SY);
			hqEnd();
			hqClearGradient();
			
			const int numPrims = sizeof(prims) / sizeof(prims[0]);
			const int numModes = sizeof(modes) / sizeof(modes[0]);
			
			for (int m = 0; m < 4; ++m)
			{
				const float my = VIEW_SY * m / numModes;
				
				setLumi(192);
				drawText(4, my + 4, 14, +1, +1, "%s", modes[m]);
				
				for (int i = 0; i < numPrims; ++i)
				{
					auto & prim = prims[i];
					
					const float x = VIEW_SX * (i + .5f) / numPrims;
					const float y = VIEW_SY * (m + .5f) / numModes;
					const float s = 20.f;
					const float strokeSize = 4.f;
					
					const bool useScreenSize = (m / 2) != 0;
					const bool scaled = (m % 2) != 0;
					
					gxPushMatrix();
					{
						gxTranslatef(x, y, 0);
						if (scaled)
							gxScalef(2, 2, 1);
						gxRotatef(framework.time * 10.f, 0, 0, 1);
						
						setColor(colorWhite);
						
						switch (prim.type)
						{
						case HQ_FILLED_RECTS:
							hqBegin(HQ_FILLED_RECTS, useScreenSize);
							hqFillRect(-s, -s, +s, +s);
							hqEnd();
							break;
							
						case HQ_STROKED_RECTS:
							hqBegin(HQ_STROKED_RECTS, useScreenSize);
							hqStrokeRect(-s, -s, +s, +s, strokeSize);
							hqEnd();
							break;
							
						case HQ_FILLED_CIRCLES:
							hqBegin(HQ_FILLED_CIRCLES, useScreenSize);
							hqFillCircle(0, 0, s);
							hqEnd();
							break;
							
						case HQ_STROKED_CIRCLES:
							hqBegin(HQ_STROKED_CIRCLES, useScreenSize);
							hqStrokeCircle(0, 0, s, strokeSize);
							hqEnd();
							break;
							
						case HQ_FILLED_TRIANGLES:
							hqBegin(HQ_FILLED_TRIANGLES, useScreenSize);
							hqFillTriangle(-s, -s, +s, -s, 0, +s);
							hqEnd();
							break;
							
						case HQ_STROKED_TRIANGLES:
							hqBegin(HQ_STROKED_TRIANGLES, useScreenSize);
							hqStrokeTriangle(-s, -s, +s, -s, 0, +s, strokeSize);
							hqEnd();
							break;
						}
					}
					gxPopMatrix();
				}
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
