#include <algorithm>
#include "framework.h"

const int sx = 1920;
const int sy = 1080;

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
	sprite->x = float(rand() % sx);
	sprite->y = float(rand() % sy);
	sprite->scale = 4;
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
	framework.minification = 2;
	framework.fullscreen = true;
	if (!framework.init(argc, argv, sx, sy))
		exit(-1);
	framework.fillCachesWithPath(".");
	
	bool down = false;
	float x = sx/2.f;
	float y = sy/4.f;
	
	Music bgm("bgm.ogg");
	bgm.play();
	
	Sprite background("background.png");
	background.startAnim("default");
	
	Sprite sprite("sprite.png");
	sprite.startAnim("walk-l");
	sprite.pauseAnim();
	
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
		
		float dx = 0;
		float dy = 0;
		
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
				obj->sprite->x = float(rand() % sx);
				obj->sprite->y = float(rand() % sy);
				obj->sprite->scale = 3.f;
				
				const int objectId = stage.addObject(obj);
			
				if (i == 0)
					stage.removeObject(objectId);
			}
		}
		
		framework.beginDraw(165, 125, 65, 0);
		{
			setBlend(BLEND_ALPHA);
			
			setColor(255, 255, 255);
			background.drawEx(sx - 132, 132, 0, 2);
			
			stage.draw();
			
			sprite.x = x;
			sprite.y = y;
			sprite.scale = 4;
			
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
					sprites[i]->scale = ((rand() % 200) + 50) / 100.f * 4.f;
				
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
				setColor(255, 191, 127, int(2048 * sortedSprites[i]->animSpeed));
				sortedSprites[i]->draw();
			}
					
			for (int i = 0; i < MAX_GAMEPAD; ++i)
			{
				Font font("calibri.ttf");
				setFont(font);

				setColor(255, 255, 0, 255);
				drawText(5, 5 + i * 40, 35, 0, 0, "gamepad[%d]: %s", i, gamepad[i].isConnected ? "connected" : "not connected");
			}

			setColor(255, 255, 255, 255);
			ui.draw();
			
			Font font("calibri.ttf");
			setFont(font);

			setColor(255, 255, 255, 255);
			drawText(mouse.x, mouse.y, 35, 0, 0, "(%d, %d)", mouse.x, mouse.y);
			
			setColor(255, 0, 0, 255);
			//drawLine(0, 0, sx, sy);
			//drawLine(sx, 0, 0, sy);
		}
		framework.endDraw();
	}
	
	for (int i = 0; i < numSprites; ++i)
		delete sprites[i];
	
	framework.shutdown();
	
	return 0;
}
