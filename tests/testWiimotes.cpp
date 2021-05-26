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

#if VFXGRAPH_ENABLE_WIIMOTE

#include "framework.h"
#include "objects/wiimote.h"

#include "ui.h"

void testWiimotes()
{
	Wiimotes wiimotes;
	
	wiimotes.findAndConnect();
	
	UiState uiState;
	uiState.x = 200;
	uiState.y = 200;
	uiState.sx = 400;
	
	do
	{
		framework.process();
		
		wiimotes.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			auto & wiimoteData = wiimotes.wiimoteData;
			
			setColor(colorWhite);
			drawText(100, 100, 12, +1, +1, "connected: %d", wiimoteData.connected);
			
			makeActive(&uiState, true, true);
			pushMenu("LEDs");
			{
				if (doButton("1", .00f, .25f, false))
					wiimotes.setLedEnabled(0, !wiimotes.getLedEnabled(0));
				if (doButton("2", .25f, .25f, false))
					wiimotes.setLedEnabled(1, !wiimotes.getLedEnabled(1));
				if (doButton("3", .50f, .25f, false))
					wiimotes.setLedEnabled(2, !wiimotes.getLedEnabled(2));
				if (doButton("4", .75f, .25f, true))
					wiimotes.setLedEnabled(3, !wiimotes.getLedEnabled(3));
			}
			popMenu();
			
			drawText(g_drawX + 4, g_drawY + 4, 12, +1, +1, "LED values: %d", wiimoteData.ledValues);
			g_drawY += 20;
			
			for (int i = 0; i < 16; ++i)
			{
				drawText(g_drawX + 4 + i * 16, g_drawY + 4, 12, +1, +1, "%d", (wiimoteData.buttons & (1 << i)) ? 1 : 0);
			}
			g_drawY += 20;
			
			drawText(g_drawX + 4, g_drawY + 4, 12, +1, +1, "motion: %d, %d, %d",
				wiimoteData.motion.forces[0],
				wiimoteData.motion.forces[1],
				wiimoteData.motion.forces[2]);
			g_drawY += 20;
			
			{
				const float x = (wiimoteData.motion.forces[0] / float(1 << 6)) - 1.f;
				const float y = (wiimoteData.motion.forces[1] / float(1 << 6)) - 1.f;
				const float z = (wiimoteData.motion.forces[2] / float(1 << 6)) - 1.f;
				setColor(colorGreen);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * x, g_drawY + 20);
				g_drawY += 20;
				setColor(colorRed);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * y, g_drawY + 20);
				g_drawY += 20;
				setColor(colorBlue);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * z, g_drawY + 20);
				g_drawY += 20;
			}
			
			{
				const float yaw = (wiimoteData.motionPlus.yaw / float(1 << 12)) - 1.f;
				const float pitch = (wiimoteData.motionPlus.pitch / float(1 << 12)) - 1.f;
				const float roll = (wiimoteData.motionPlus.roll / float(1 << 12)) - 1.f;
				setColor(colorGreen);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * yaw, g_drawY + 20);
				g_drawY += 20;
				setColor(colorRed);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * pitch, g_drawY + 20);
				g_drawY += 20;
				setColor(colorBlue);
				drawRect(g_drawX + uiState.sx/2, g_drawY, g_drawX + uiState.sx/2 * roll, g_drawY + 20);
				g_drawY += 20;
			}
			
			setColor(colorWhite);
			drawText(g_drawX + 4, g_drawY + 4, 12, +1, +1, "motion+: %d, %d, %d",
				wiimoteData.motionPlus.yaw,
				wiimoteData.motionPlus.pitch,
				wiimoteData.motionPlus.roll);
			g_drawY += 20;
			
			popFontMode();
		}
		framework.endDraw();
	}
	while (!keyboard.wentDown(SDLK_ESCAPE));
	
	wiimotes.shut();
	
	exit(0);
}

#endif
