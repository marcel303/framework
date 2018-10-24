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

/*
Acknowledgement for the use of the Stanford Lucy model:
	Source: Stanford University Computer Graphics Laboratory
	http://graphics.stanford.edu/data/3Dscanrep/
*/

#include "framework.h"

#define VIEW_SX 1200
#define VIEW_SY 600

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	mouse.showCursor(false);
	mouse.setRelative(true);
	
	Model model("stanford-lucy.fbx");

	bool wireframe = false;
	bool rotate = false;
	
	float angle = 0.f;
	
	Camera3d camera;
	camera.maxForwardSpeed = 1000.f;
	camera.maxStrafeSpeed = 1000.f;
	camera.maxUpSpeed = 1000.f;
	camera.position[2] = -1000.f;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		SDL_WarpMouseInWindow(nullptr, VIEW_SX/2, VIEW_SY/2);
		
		if (keyboard.wentDown(SDLK_1))
			wireframe = !wireframe;
		if (keyboard.wentDown(SDLK_2))
			rotate = !rotate;
		
		camera.tick(framework.timeStep, true);
		
		model.tick(framework.timeStep);
		
		if (rotate)
		{
			angle += framework.timeStep * 10.f;
		}
		
		framework.beginDraw(31, 31, 31, 0, 1.f);
		{
			const float fov = 50.f;
			projectPerspective3d(fov, 1.f, 10000.f);
			
			camera.pushViewMatrix();
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			checkErrorGL();
			
			pushBlend(BLEND_OPAQUE);
			setColor(255, 255, 255);
			
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			checkErrorGL();
			
			gxPushMatrix();
			{
				gxRotatef(angle, 0.f, 1.f, 0.f); // animated rotation
				gxRotatef(angle, 1.f, 0.f, 0.f);
				gxRotatef(angle, 1.f, 0.f, 1.f);
				gxScalef(.01f, .01f, .01f); // scale the model down in size
				gxScalef(+1, -1, -1); // make sure the model is facing us initially
				
				Shader shader("basic-lighting");
				shader.setImmediate("time", framework.time);
				shader.setImmediate("cameraPosition", camera.position[0], camera.position[1], camera.position[2]);
				model.overrideShader = &shader;
				
				for (int i = 0; i < 3; ++i)
					model.drawEx(Vec3((i - 1) * 100000.f, 0.f, 0.f), Vec3(0, 1, 0), i * framework.time, 1.f, DrawMesh | DrawUnSkinned);
			}
			gxPopMatrix();
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			checkErrorGL();
			
			popBlend();
			
			glDepthMask(GL_FALSE);
			gxPushMatrix();
			gxTranslatef(0, -800, 0);
			gxScalef(1000, 1000, 1000);
			setAlpha(10);
			drawGrid3dLine(100, 100, 0, 2);
			gxPopMatrix();
			glDepthMask(GL_TRUE);
			
			glDisable(GL_DEPTH_TEST);
			checkErrorGL();
			
			camera.popViewMatrix();
			
			// show instructions
			
			projectScreen2d();
			
			gxTranslatef(11, 11, 0);
			
			setColor(0, 0, 0, 180);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(0, 0, 270, 60, 14);
			hqEnd();
			
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			{
				setColor(colorWhite);
				
				const int fontSize = 14;
				const int incrementY = fontSize + 2;
				int x = 17;
				int y = 17;
				
				y -= incrementY;
				y += incrementY; drawText(x, y, fontSize, +1, +1, "1: wireframe [%s]", wireframe ? "on" : "off");
				y += incrementY; drawText(x, y, fontSize, +1, +1, "2: rotate models [%s]", rotate ? "on" : "off");
			}
			popFontMode();
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
