#include <algorithm>
#include "framework.h"

#define TEST_SURFACE 1
#define SPRITE_SCALE 1

const int sx = 1920;
const int sy = 1080;

//const int sx = 1920/2;
//const int sy = 1080/2;

static Sprite * createRandomSprite()
{
	const int index = rand() % 4;
	char filename[32];
	sprintf(filename, "%s%d.png", "rpg", index);
	
	Sprite * sprite = new Sprite(filename, 0, 0, "rpg.txt");
	
	char anim[32];
	const int guy = rand() % 8;
	const int dir = rand() % 4;
	char dirName[4] = { 'u', 'd', 'l', 'r' };
	sprintf(anim, "%d%c", guy, dirName[dir]);
	
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
	changeDirectory("data");
	
	framework.fullscreen = true;
	for (int i = 1; i < argc; ++i)
	{
		if (std::string(argv[i]) =="devmode")
		{
			framework.reloadCachesOnActivate = true;
			framework.minification = 2;
			framework.fullscreen = false;
		}
	}
	if (!framework.init(argc, (const char **)argv, sx, sy))
		return -1;
	framework.fillCachesWithPath(".");
	
	mouse.showCursor(false);
	
	Surface surface(sx, sy);
	
	bool down = false;
	float x = sx/2.f;
	float y = sy/4.f;
	
	Music bgm("bgm.ogg");
	bgm.play();

	Shader shader("shader1");
	Shader postprocess("shader2");
	
	Sprite background("background.png");
	background.startAnim("default");
	
	Sprite sprite("sprite.png");
	sprite.startAnim("walk-l");
	sprite.pauseAnim();
	
	Sprite sprite2("hawk.png");
	sprite2.scale = 1.f;
	sprite2.x = sx - (sprite2.getWidth() / 2) * sprite2.scale;
	sprite2.y = sy - (sprite2.getHeight() / 2) * sprite2.scale;
	sprite2.pivotX = sprite2.getWidth() / 2.f;
	sprite2.pivotY = sprite2.getHeight() / 2.f;
	sprite2.filter = FILTER_LINEAR;
	
	const int numSprites = 100;
	Sprite * sprites[numSprites];
	for (int i = 0; i < numSprites; ++i)
		sprites[i] = createRandomSprite();
	
	ui.load("ui.txt");
	for (int i = 0; i < 7; ++i)
	{
		char name[32];
		sprintf(name, "button_%d", i);
		Dictionary & button = ui[name];
		button.parse("type:image image:button.png _image_over:button-over.png _image_down:button-down.png action:sound sound:test.wav");
		button.setInt("x", 10);
		button.setInt("y", 200 + i * 70);
	}
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		stage.process(framework.timeStep);
		
		ui.process();
		
		if (keyboard.isDown(SDLK_a))
		{
			for (int i = 0; i < numSprites; ++i)
			{
				delete sprites[i];
				sprites[i] = createRandomSprite();
			}
		}
		
		if (keyboard.wentDown(SDLK_d))
		{
			static int uiId = 0;
			uiId = (uiId + 1) % 2;
			ui.load(uiId == 0 ? "ui.txt" : "ui2.txt");
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
				Sound("test.wav").play(50, 200);
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
				if (gamepad[i].isDown[DPAD_LEFT])
					dx -= 1.f;
				if (gamepad[i].isDown[DPAD_RIGHT])
					dx += 1.f;
				if (gamepad[i].isDown[DPAD_UP])
					dy -= 1.f;
				if (gamepad[i].isDown[DPAD_DOWN])
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
		
		x += dx;
		y += dy;

		if (keyboard.isDown(SDLK_s))
		{
			const char * name = "sprite.png";
			const char * anim = "walk-once";
			
			for (int i = 0; i < 10; ++i)
			{
				StageObject * obj = new StageObject_SpriteAnim(name, anim);
				obj->sprite->x = rand() % sx;
				obj->sprite->y = rand() % sy;
				obj->sprite->scale = 1.5f * SPRITE_SCALE;
				
				const int objectId = stage.addObject(obj);
			
				if (i == 0)
					stage.removeObject(objectId);
			}
		}
		
		framework.beginDraw(165, 125, 65, 0);
		{
			#if TEST_SURFACE
			if (keyboard.isDown(SDLK_e))
			{
				pushSurface(&surface);
				surface.clear();
				//surface.mulf(1.f, 1.f, 1.f, .9f);
			}
			#endif
			
			setBlend(BLEND_ALPHA);
			
			{
				static float gradX1 = sx;
				static float gradY1 = sy;
				static float gradX2 = -sx/3;
				static float gradY2 = -sy/3;
				bool showLine = false;
				if (mouse.isDown(BUTTON_LEFT))
				{
					gradX1 = mouse.x;
					gradY1 = mouse.y;
					showLine = true;
				}
				if (mouse.isDown(BUTTON_RIGHT))
				{
					gradX2 = mouse.x;
					gradY2 = mouse.y;
					showLine = true;
				}
				setGradientf(gradX1, gradY1, Color(1.f, 0.f, 0.f, 0.f), gradX2, gradY2, Color(1.f, 1.f, 1.f, 1.f));
				const int border = 0;
				const int x1 = border;
				const int x2 = sx - border * 2;
				const int y = sy / 2;
				const int dy = sine<int>(0, sy, framework.time * 3.f);
				int y1 = y - dy / 2;
				int y2 = y + dy / 2;
				if (y1 > y2)
					std::swap(y1, y2);
				setDrawRect(x1, y1, x2 - x1, y2 - y1);
				gxSetTexture(0);
				drawRectGradient(0.f, 0.f, sx, sy);
				clearDrawRect();
				if (showLine)
				{
					setColor(0, 255, 0, 127);
					gxSetTexture(0);
					drawLine(gradX1, gradY1, gradX2, gradY2);
				}
			}
			
			setColor(255, 255, 255);
			background.drawEx(sx - 132, 132, 0, 2);
			
			stage.draw();
			
			sprite.x = x;
			sprite.y = y;
			sprite.scale = 4 * SPRITE_SCALE;
			
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
				if (sortedSprites[i] == &sprite || sortedSprites[i] == &sprite2)
					setColorMode(COLOR_ADD);
					
				if (sortedSprites[i] == &sprite)
					setColor(255, 191, 127, 255, sine<int>(0, 255, framework.time));
				else
					setColor(255, 191, 127, 2048 * sortedSprites[i]->animSpeed);
				
				sortedSprites[i]->draw();
				
				if (sortedSprites[i] == &sprite || sortedSprites[i] == &sprite2)
					setColorMode(COLOR_MUL);
			}
			
			setColorMode(COLOR_ADD);
			setColor(255, 191, 127, 255, sine<int>(0, 255, framework.time));
			sprite2.draw();
			setColorMode(COLOR_MUL);
			
			for (int i = 0; i < MAX_GAMEPAD; ++i)
			{
				Font font("calibri.ttf");
				setFont(font);

				setColor(255, 255, 0, 255);
				drawText(5.f, 5.f + i * 40.f, 35, 0, 0, "gamepad[%d]: %s", i, gamepad[i].isConnected ? "connected" : "not connected");
			}

			setColor(255, 255, 255, 255);
			ui.draw();
			
			setColor(255, 0, 0, 255);
			//drawLine(0, 0, sx, sy);
			//drawLine(sx, 0, 0, sy);
			
			#if TEST_SURFACE
			if (keyboard.isDown(SDLK_e))
			{
				popSurface();
				
				surface.postprocess(postprocess);
				
				setShader(shader);
				shader.setTexture("texture0", 0, surface.getTexture());
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
