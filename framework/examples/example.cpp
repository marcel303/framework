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
#include "StringEx.h"
#include <algorithm>

#define FULLSCREEN 0
#define TEST_SURFACE 1
#define SPRITE_SCALE 1

const int sx = 1920;
const int sy = 1080;

static Sprite * createRandomSprite()
{
	const int index = rand() % 4;
	char filename[32];
	sprintf_s(filename, sizeof(filename), "%s%d.png", "rpg", index);
	
	Sprite * sprite = new Sprite(filename, 0, 0, "rpg.txt");
	
	char anim[32];
	const int guy = rand() % 8;
	const int dir = rand() % 4;
	char dirName[4] = { 'u', 'd', 'l', 'r' };
	sprintf_s(anim, sizeof(anim), "%d%c", guy, dirName[dir]);
	
	sprite->startAnim(anim);
	sprite->x = rand() % sx;
	sprite->y = rand() % sy;
	sprite->scale = 4 * SPRITE_SCALE;
	sprite->animSpeed = 1.f + (rand() % 100) / 100.f;
	
	return sprite;
}

static bool compareSprites(const Sprite * sprite1, const Sprite * sprite2)
{
	return sprite1->y < sprite2->y;
}

static void sortSprites(Sprite ** sprites, int numSprites)
{
	std::sort(sprites, sprites + numSprites, compareSprites);
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

#if FULLSCREEN
	framework.fullscreen = true;
#else
	framework.minification = 2;
#endif

	for (int i = 1; i < argc; ++i)
	{
		if (std::string(argv[i]) =="devmode")
		{
			framework.minification = 2;
			framework.fullscreen = false;
		}
	}

	if (!framework.init(argc, (const char **)argv, sx, sy))
		return -1;
	framework.fillCachesWithPath(".", true);
	
	mouse.showCursor(false);
	
	Surface surface(sx, sy, false);
	
	bool down = false;
	float x = sx/2.f;
	float y = sy/4.f;
	
	Music bgm("bgm.ogg");
	//bgm.play();

	Shader shader("shader1");
	Shader postprocess("shader2");
	
	Sprite sprite("sprite.png");
	sprite.startAnim("walk-l");
	sprite.pauseAnim();
	
	const int numSprites = 100;
	Sprite * sprites[numSprites];
	for (int i = 0; i < numSprites; ++i)
		sprites[i] = createRandomSprite();
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (keyboard.isDown(SDLK_a))
		{
			for (int i = 0; i < numSprites; ++i)
			{
				delete sprites[i];
				sprites[i] = createRandomSprite();
			}
		}
		
		bool newDown = keyboard.isDown(SDLK_SPACE);
		
		if (newDown != down)
		{
			down = newDown;
			if (down)
			{
				Sound("test.wav").play();
				y += 10;
			}
			else
			{
				Sound("test.wav").play(50);
			}
		}
		
		if (keyboard.wentDown(SDLK_o))
			bgm.stop();
		if (keyboard.wentDown(SDLK_p))
			bgm.play();
		
		float dx = 0.f;
		float dy = 0.f;
		
		if (keyboard.isDown(SDLK_LEFT))
			dx -= 1.f;
		if (keyboard.isDown(SDLK_RIGHT))
			dx += 1.f;
		if (keyboard.isDown(SDLK_UP))
			dy -= 1.f;
		if (keyboard.isDown(SDLK_DOWN))
			dy += 1.f;
		
		for (int i = 0; i < MAX_GAMEPAD; ++i)
		{
			if (gamepad[i].isConnected)
			{
				if (gamepad[i].isDown(DPAD_LEFT))
					dx -= 1.f;
				if (gamepad[i].isDown(DPAD_RIGHT))
					dx += 1.f;
				if (gamepad[i].isDown(DPAD_UP))
					dy -= 1.f;
				if (gamepad[i].isDown(DPAD_DOWN))
					dy += 1.f;
				
				dx += gamepad[i].getAnalog(0, ANALOG_X);
				dy += gamepad[i].getAnalog(0, ANALOG_Y);
				dx += gamepad[i].getAnalog(1, ANALOG_X);
				dy += gamepad[i].getAnalog(1, ANALOG_Y);
			}
		}
		
		dx = clamp(dx, -1.f, +1.f);
		dy = clamp(dy, -1.f, +1.f);
		
		const char * wantAnim = 0;
		
		if (dx < 0)
			wantAnim = "walk-l";
		if (dx > 0)
			wantAnim = "walk-r";
		if (dy < 0)
			wantAnim = "walk-u";
		if (dy > 0)
			wantAnim = "walk-d";
		
		if (!wantAnim)
			sprite.pauseAnim();
		else if (sprite.getAnim() == wantAnim)
			sprite.resumeAnim();
		else
			sprite.startAnim(wantAnim);
		
		sprite.update(framework.timeStep);
		
		x += dx;
		y += dy;
		
		framework.beginDraw(165, 125, 65, 0);
		{
		#if TEST_SURFACE
			if (keyboard.isDown(SDLK_e))
			{
				pushSurface(&surface);
				surface.clear();
			}
		#endif
			
			setBlend(BLEND_ALPHA);
			
			sprite.x = x;
			sprite.y = y;
			sprite.scale = 3 * SPRITE_SCALE;
			
			for (int i = 0; i < numSprites; ++i)
			{
				const char dirName = sprites[i]->getAnim()[1];
				int dx = 0;
				int dy = 0;
				if (dirName == 'l') dx = -1;
				if (dirName == 'r') dx = +1;
				if (dirName == 'u') dy = -1;
				if (dirName == 'd') dy = +1;
				sprites[i]->x += dx * sprites[i]->animSpeed * 0.7f;
				sprites[i]->y += dy * sprites[i]->animSpeed * 0.7f;
				
				sprites[i]->animSpeed = std::max(0.f, sprites[i]->animSpeed - framework.timeStep * 0.1f);
				
				if ((rand() % 100) == 0)
					sprites[i]->angle = (rand() % 40) - 20.f;
				if ((rand() % 300) == 0)
					sprites[i]->scale = ((rand() % 200) + 50) / 100.f * 3.f * SPRITE_SCALE;
				
				if (sprites[i]->animSpeed == 0.f)
				{
					delete sprites[i];
					sprites[i] = createRandomSprite();
				}
				
				sprites[i]->update(framework.timeStep);
			}
			
			const int numSortedSprites = numSprites + 1;
			Sprite * sortedSprites[numSortedSprites];
			Sprite ** sortedSprite = sortedSprites;
			for (int i = 0; i < numSprites; ++i)
				*sortedSprite++ = sprites[i];
			*sortedSprite++ = &sprite;
			sortSprites(sortedSprites, numSortedSprites);

			for (int i = 0; i < numSortedSprites; ++i)
			{
				if (sortedSprites[i] == &sprite)
					setColorMode(COLOR_ADD);
					
				if (sortedSprites[i] == &sprite)
					setColor(255, 191, 127, 255, sine<int>(0, 63, framework.time * 10.f * 180.f/M_PI));
				else
					setColor(255, 191, 127, 2048 * sortedSprites[i]->animSpeed);
				
				sortedSprites[i]->draw();
				
				if (sortedSprites[i] == &sprite)
					setColorMode(COLOR_MUL);
			}
			
			for (int i = 0; i < MAX_GAMEPAD; ++i)
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);

				setColor(255, 255, 0, 255);
				drawText(5.f, 5.f + i * 40.f, 35, +1, +1, "gamepad[%d]: %s", i, gamepad[i].isConnected ? "connected" : "not connected");
				
				popFontMode();
			}
			
			setColor(255, 0, 0, 255);
			
		#if TEST_SURFACE
			if (keyboard.isDown(SDLK_e))
			{
				popSurface();
				
				setShader(postprocess);
				postprocess.setTexture("texture0", 0, surface.getTexture());
				surface.postprocess(postprocess);
				
				setShader(shader);
				shader.setTexture("texture0", 0, surface.getTexture(), false);
				shader.setImmediate("modifier", sine<float>(-.5f, +.5f, framework.time));
				
				setBlend(BLEND_ALPHA);
				setColorMode(COLOR_MUL);
				const int numSteps = keyboard.isDown(SDLK_w) ? 15 : 1;
				for (int i = 0; i < numSteps; ++i)
				{
					setColor(
						sine<int>(127, 255, framework.time * 1.234f),
						sine<int>(127, 255, framework.time * 2.345f),
						sine<int>(127, 255, framework.time * 3.456f), i == (numSteps - 1) ? 255 : 15);
					
					gxPushMatrix();
					gxTranslatef(+sx/2.f, +sy/2.f, 0.f);
					if (keyboard.isDown(SDLK_w))
					{
						const float scale = sine<float>(1.f, 2.f, framework.time * 2.321f);
						gxScalef(scale, scale, 1.f);
						gxRotatef(sine<float>(-25.f, +25.f, framework.time / 4.567f) + i / 2.f, 0.f, 0.f, 1.f);
					}
					gxTranslatef(-sx/2.f, -sy/2.f, 0.f);
					drawRect(0.f, 0.f, sx, sy);
					gxPopMatrix();
				}
				
				clearShader();
			}
		#endif
			
			Font font("calibri.ttf");
			setFont(font);

			setBlend(BLEND_ALPHA);
			setColor(0, 31, 63, 255);
			const int borderSize = 2;
			for (int i = -borderSize; i <= +borderSize; ++i)
				for (int j = -borderSize; j <= +borderSize; ++j)
				drawText(mouse.x + i, mouse.y + j, 35, 0, -2, "(%d, %d)", mouse.x, mouse.y);
			setColor(255, 255, 255, 255);
			drawText(mouse.x, mouse.y, 35, 0, -2, "(%d, %d)", mouse.x, mouse.y);
			
			Sprite("cursor.png", 12, 15).drawEx(mouse.x, mouse.y);
		}
		framework.endDraw();
	}
	
	for (int i = 0; i < numSprites; ++i)
		delete sprites[i];
	
	framework.shutdown();
	
	return 0;
}
