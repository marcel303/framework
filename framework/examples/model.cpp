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

#include <time.h>
static int getTimeUS() { return clock() * 1000000 / CLOCKS_PER_SEC; }

#define VIEW_SX 1200
#define VIEW_SY 600

#if 0
	#define X1 0
	#define X2 0
	#define Y1 0
	#define Y2 0
	#define Z1 0
	#define Z2 0
#else
	#define X1 -3
	#define X2 +3
	#define Y1 -3
	#define Y2 +3
	#define Z1 -3
	#define Z2 +3
#endif

class CoordKey
{
public:
	int values[3];
	
	CoordKey(int x, int y, int z)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
	}
	
	bool operator<(const CoordKey & other) const
	{
		for (int i = 0; i < 3; ++i)
			if (values[i] != other.values[i])
				return values[i] < other.values[i];
		return false;
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	framework.enableDepthBuffer = true;
	
	if (!framework.init(argc, (const char **)argv, VIEW_SX, VIEW_SY))
		return -1;
	
	pushFontMode(FONT_SDF);
	
	std::map<CoordKey, Model*> models;
	
	logDebug("loading model");
	
	for (int x = X1; x <= X2; ++x)
	{
		for (int y = Y1; y <= Y2; ++y)
		{
			for (int z = Z1; z <= Z2; ++z)
			{
				Model * model = new Model("model.txt");
				model->x = x * 200.f;
				model->y = y * 200.f;
				model->z = z * 100.f;
				
				models[CoordKey(x, y, z)] = model;
			}
		}
	}
	
	const std::vector<std::string> animList = models[CoordKey(0, 0, 0)]->getAnimList();
	
	for (size_t i = 0; i < animList.size(); ++i)
		logDebug("anim: %s", animList[i].c_str());
	
	int drawFlags = DrawMesh | DrawColorNormals;
	bool stressTest = false;
	bool wireframe = false;
	bool rotate = false;
	bool loop = false;
	bool autoPlay = false;
	float animSpeed = 1.f;
	
	float angle = 0.f;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		bool startRandomAnim = false;
		
		if (keyboard.wentDown(SDLK_s))
			stressTest = !stressTest;
		if (keyboard.wentDown(SDLK_w))
			wireframe = !wireframe;
		if (keyboard.wentDown(SDLK_b))
			drawFlags ^= DrawBones;
		if (keyboard.wentDown(SDLK_m))
			drawFlags = drawFlags ^ DrawPoseMatrices;
		if (keyboard.wentDown(SDLK_n))
			drawFlags = drawFlags ^ DrawNormals;
		if (keyboard.wentDown(SDLK_h))
			drawFlags ^= DrawHardSkinned;
		if (keyboard.wentDown(SDLK_u))
			drawFlags ^= DrawUnSkinned;
		if (keyboard.wentDown(SDLK_1))
			drawFlags ^= DrawColorBlendWeights;
		if (keyboard.wentDown(SDLK_2))
			drawFlags ^= DrawColorBlendIndices;
		if (keyboard.wentDown(SDLK_3))
			drawFlags ^= DrawColorTexCoords;
		if (keyboard.wentDown(SDLK_4))
			drawFlags ^= DrawColorNormals;
		if (keyboard.wentDown(SDLK_SPACE))
			startRandomAnim = true;
		if (keyboard.wentDown(SDLK_a))
			rotate = !rotate;
		if (keyboard.wentDown(SDLK_l))
			loop = !loop;
		if (keyboard.wentDown(SDLK_p))
			autoPlay = !autoPlay;
		if (keyboard.wentDown(SDLK_UP))
			animSpeed *= 1.5f;
		if (keyboard.wentDown(SDLK_DOWN))
			animSpeed /= 1.5f;
		
		for (int x = X1; x <= X2; ++x)
		{
			for (int y = Y1; y <= Y2; ++y)
			{
				for (int z = Z1; z <= Z2; ++z)
				{
					if (stressTest == false && (x != 0 || y != 0 || z != 0))
						continue;
					
					bool startRandomAnimForModel = startRandomAnim;
					
					Model * model = models[CoordKey(x, y, z)];
					
					model->tick(framework.timeStep);
					
					if (autoPlay &&  !model->animIsActive)
						startRandomAnimForModel = true;
					
					if (startRandomAnimForModel)
					{
						const std::string & name = animList[rand() % animList.size()];
						
						model->startAnim(name.c_str(), loop ? -1 : 1);
						model->animSpeed = animSpeed;
					}
				}
			}
		}
		
		if (rotate)
		{
			angle += framework.timeStep * 10.f;
		}
		
		// set 3D transform
		
		const float fov = 90.f * M_PI / 180.f;
		const float aspect = VIEW_SY / float(VIEW_SX);
		
		Mat4x4 transform3d;
		transform3d.MakePerspectiveGL(fov, aspect, .1f, +2000.f);
		setTransform3d(transform3d);
		
		framework.beginDraw(31, 31, 31, 0);
		{
			glClearDepth(1.f);
			glClear(GL_DEPTH_BUFFER_BIT);
			
			// switch to 3D drawing mode
			
			setTransform(TRANSFORM_3D);
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			checkErrorGL();
			
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			checkErrorGL();
			
			gxPushMatrix();
			{
				gxTranslatef(0.f, -100.f, 600.f);
				gxRotatef(angle, 0.f, 1.f, 0.f);
				
				pushBlend(BLEND_OPAQUE);
				setColor(255, 255, 255);
				
				int time = 0;
				time -= getTimeUS();
				
				for (int x = X1; x <= X2; ++x)
				{
					for (int y = Y1; y <= Y2; ++y)
					{
						for (int z = Z1; z <= Z2; ++z)
						{
							if (stressTest == false && (x != 0 || y != 0 || z != 0))
								continue;
					
							Model * model = models[CoordKey(x, y, z)];
							
							model->draw(drawFlags);
						}
					}
				}
				
				time += getTimeUS();
				logDebug("draw took %.2fms", time / 1000.f);
				
				setFont("calibri.ttf");
				setColor(200, 200, 200);
				const Vec2 s = transformToScreen(Vec3(0.f, 0.f, 0.f));
				debugDrawText(s[0], s[1], 14, 0, 0, "root");
				setColor(255, 255, 255);
				
				popBlend();
			}
			gxPopMatrix();
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			checkErrorGL();
			
			glDisable(GL_DEPTH_TEST);
			checkErrorGL();
			
			// show instructions
			
			setTransform(TRANSFORM_SCREEN);
			
			gxTranslatef(11, 11, 0);
			
			setColor(0, 0, 0, 180);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(0, 0, 270, 310, 14);
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			
			const int fontSize = 14;
			const int incrementY = fontSize + 2;
			int x = 17;
			int y = 17;
			
			y -= incrementY;
			y += incrementY; drawText(x, y, fontSize, +1, +1, "S: stress test [%s]", stressTest ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "W: wireframe [%s]", wireframe ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "B: draw bones [%s]", (drawFlags & DrawBones) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "M: draw pose matrices [%s]", (drawFlags & DrawPoseMatrices) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "N: draw normals [%s]", (drawFlags & DrawNormals) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "H: hard skinned [%s]", (drawFlags & DrawHardSkinned) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "U: unskinned [%s]", (drawFlags & DrawUnSkinned) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "1: color blend weights [%s]", (drawFlags & DrawColorBlendWeights) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "2: color blend indices [%s]", (drawFlags & DrawColorBlendIndices) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "3: color texture coordinates [%s]", (drawFlags & DrawColorTexCoords) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "4: color normals [%s]", (drawFlags & DrawColorNormals) ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "SPACE: trigger a random animation");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "A: rotate view [%s]", rotate ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "L: loop animations [%s]", loop ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "P: auto play animations [%s]", autoPlay ? "on" : "off");
			y += incrementY; drawText(x, y, fontSize, +1, +1, "UP: increase animation speed [%.2f]", animSpeed);
			y += incrementY; drawText(x, y, fontSize, +1, +1, "DOWN: decrease animation speed [%.2f]", animSpeed);
		}
		framework.endDraw();
	}
	
	popFontMode();
	
	for (auto & modelItr : models)
	{
		auto & model = modelItr.second;
		
		delete model;
		model = nullptr;
	}
	
	framework.shutdown();
	
	return 0;
}
