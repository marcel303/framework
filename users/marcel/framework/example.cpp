#include <SDL/SDL.h>
#include "framework.h"

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
	sprite->x = rand() % 1920;
	sprite->y = rand() % 1280;
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
	framework.setMinification(2);
	framework.init(argc, argv, 1920, 1280);
	
	bool down = false;
	int x = 1920/2;
	int y = 1280/4;
	
	Music("bgm.ogg").play();
	
	Sprite background("background.png");
	background.startAnim("default");
	
	Sprite sprite("sprite.png");
	sprite.startAnim("walk-l");
	sprite.pauseAnim();
	
	const int numSprites = 100;
	Sprite * sprites[numSprites];
	for (int i = 0; i < numSprites; ++i)
		sprites[i] = createRandomSprite();
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
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
				Sound("test.wav").play(50, 200);
			}
		}
		
		int dx = 0;
		int dy = 0;
		
		if (keyboard.isDown(SDLK_LEFT))
			dx--;
		if (keyboard.isDown(SDLK_RIGHT))
			dx++;
		if (keyboard.isDown(SDLK_UP))
			dy--;
		if (keyboard.isDown(SDLK_DOWN))
			dy++;
		
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

		for (int i = 0; i < 4; ++i)
		{
			if (gamepad[i].isConnected)
			{
				if (gamepad[i].isDown[DPAD_LEFT])
					x--;
				if (gamepad[i].isDown[DPAD_RIGHT])
					x--;
			}
		}
		
		framework.beginDraw(165, 125, 65, 0);
		{
			setBlend(BLEND_ALPHA);
			
			setColor(255, 255, 255);
			background.drawEx(1920 - 132, 132, 0, 2);
			
			sortSprites(sprites, numSprites);
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
					sprites[i]->angle = (rand() % 40) - 20;
				if ((rand() % 300) == 0)
					sprites[i]->scale = ((rand() % 200) + 50) / 100.f * 4.f;
				
				setColor(255, 191, 127, 2048 * sprites[i]->animSpeed);
				sprites[i]->draw();
				
				if (sprites[i]->animSpeed == 0.f)
				{
					delete sprites[i];
					sprites[i] = createRandomSprite();
				}
			}
			
			setColor(255, 255, 255, 255);
			sprite.drawEx(x, y, 0, 4, BLEND_ALPHA);
			
			Font font("test.ttf");
			setFont(font);
			
			setColor(0, 0, 0, 255);
			drawText(mouse.x, mouse.y, 28, 0, 0, "PROTO[%d]TYPE", rand() % 10);
			drawText(mouse.x, mouse.y + 30, 20, 0, 0, "(demo only)", rand() % 10);
			
			setColor(255, 0, 0, 255);
			//drawLine(0, 0, 1920, 1280);
			//drawLine(1920, 0, 0, 1280);
		}
		framework.endDraw();
	}
	
	for (int i = 0; i < numSprites; ++i)
		delete sprites[i];
	
	framework.shutdown();
	
	return 0;
}
