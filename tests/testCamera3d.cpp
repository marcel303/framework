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

#include "ui.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testCamera3d()
{
	setAbout("This test illustrates how to use the Camera3d object to fly through a 3D scene.");
	
	Camera3d camera;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	camera.gamepadIndex = 0;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	enum State
	{
		kState_Play,
		kState_Menu
	};
	
	State state = kState_Play;
	
	UiState uiState;
	uiState.sx = 260;
	uiState.x = (GFX_SX - uiState.sx) / 2;
	uiState.y = GFX_SY * 2 / 3;
	
	do
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		if (state == kState_Play)
		{
			if (keyboard.wentDown(SDLK_TAB))
				state = kState_Menu;
			else
			{
				camera.tick(dt, true);
			}
		}
		else if (state == kState_Menu)
		{
			if (keyboard.wentDown(SDLK_TAB))
				state = kState_Play;
			else
			{
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				
				gxPushMatrix();
				{
					Mat4x4 cameraMatrix = camera.getViewMatrix();
					//Mat4x4 cameraMatrix = camera.getWorldMatrix();
					cameraMatrix.SetTranslation(0, 0, 0);
					gxMultMatrixf(cameraMatrix.m_v);
					gxMultMatrixf(cameraMatrix.m_v);
					gxMultMatrixf(cameraMatrix.m_v);
					gxScalef(.3f, .3f, .3f);
					
					setColor(colorWhite);
					gxSetTexture(getTexture("happysun.jpg"));
					drawRect3d(0, 1);
					drawRect3d(2, 0);
					gxSetTexture(0);
				}
				gxPopMatrix();
				
				setColor(colorWhite);
				gxSetTexture(getTexture("picture.jpg"));
				if (mouse.isDown(BUTTON_LEFT))
					drawGrid3dLine(100, 100, 0, 2);
				else
					drawGrid3d(10, 10, 0, 2);
				gxSetTexture(0);
				
				glDisable(GL_DEPTH_TEST);
			}
			camera.popViewMatrix();
			
			//
			
			projectScreen2d();
			
			if (state == kState_Play)
			{
				drawText(30, 30, 24, +1, +1, "press TAB to open menu");
			}
			else if (state == kState_Menu)
			{
				drawText(30, 30, 24, +1, +1, "press TAB to close menu");
				
				setColor(31, 63, 127, 127);
				drawRect(0, 0, GFX_SX, GFX_SY);
				
				makeActive(&uiState, true, true);
				pushMenu("camera");
				{
					doLabel("camera", 0.f);
					doTextBox(fov, "field of view", dt);
					doTextBox(near, "near distance", dt);
					doTextBox(far, "far distance", dt);
				}
				popMenu();
			}
			
			popFontMode();
			
			drawTestUi();
		}
		framework.endDraw();

	}
	while (tickTestUi());
}
