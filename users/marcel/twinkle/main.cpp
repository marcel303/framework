#include "Calc.h"
#include "framework.h"

#define SX 1920
#define SY 1080

#define WORLD_RADIUS 400
#define WORLD_X 0
#define WORLD_Y 0
#define MAX_LEMMINGS 100

#define LEMMING_SPRITE Spriter("Art Assets/Animation/Shoon/Shoon_anim_000.scml")
#define HEART_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define PLAYER_SPRITE Spriter("Art Assets/Animation/Shoon/Shoon_anim_000.scml")
#define SUN_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define PARTICLE_SPRITE Spriter("Art Assets/Animation/Particules/Particules.scml")
#define DIAMOND_SPRITE Spriter("Art Assets/Animation/Diamonds/Diamond.scml")

class Sun
{
public:
	Sun()
	{
		angle = 0.0f;
		distance = 300.0f;
		speed = 0.05f;
		sun_state = 1;
		prays_needed = 2;
	}

	void spawn()
	{
		spriteState.startAnim(SUN_SPRITE, 0);

		color = colorYellow;
		targetColor = color;
	}

	void change_color_to(Color new_color)
	{
		targetColor = new_color;
		//color.r += (new_color.r > color.r ? 00.1 : -00.1);
		//color.g += (new_color.g > color.g ? 00.1 : -00.1);
		//color.b += (new_color.b > color.b ? 00.1 : -00.1);
		color = color.interp(targetColor, 0.01f);
	}

	void tick(float dt)
	{
		angle += dt * speed;
		spriteState.updateAnim(SUN_SPRITE, dt);
		if (prays_needed > 0)
			sun_state = 0;
		if (prays_needed < 0)
			sun_state = 2;
		if (!prays_needed)
			sun_state = 1;
		if (sun_state == 2)
			change_color_to(colorRed);
		if (sun_state == 0)
			change_color_to(colorBlue);
		if (sun_state == 1)
			change_color_to(colorYellow);
		//if(keyboard.wentDown(SDLK_SPACE))
			//sun_state = (sun_state + 1) % 3;
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteState.scale = 0.5f;
		setColor(color);
		SUN_SPRITE.draw(spriteState);
	}

	SpriterState spriteState;
	float angle;
	float distance;
	float speed;
	int sun_state; // 0 not happy, 1 it's okey, 2, too happy
	int prays_needed;
	Color color;
	Color targetColor;
};

class Lemming
{
public:
	Lemming()
	{
		isActive = false;
		pray = false;
		angle = 0.0f;
		state = 2;
		sunHit = false;
		sunHitProcessed = false;
	}

	void spawn()
	{
		spriteState.startAnim(LEMMING_SPRITE, state);
		spriteStateDiamond.startAnim(DIAMOND_SPRITE, 0);
		spriteStateParticles.startAnim(PARTICLE_SPRITE, 0);
	}

	void tick(float dt)
	{
		bool animIsDone = spriteState.updateAnim(LEMMING_SPRITE, dt);
		if (animIsDone)
		{
			if (!state)
				state = 1;
			if (state == 3)
				state = 2;
			spriteState.startAnim(LEMMING_SPRITE, state);
		}
		if (state == 1)
		{
			spriteStateDiamond.updateAnim(DIAMOND_SPRITE, dt);
			spriteStateParticles.updateAnim(PARTICLE_SPRITE, dt);
		}
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.7f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		LEMMING_SPRITE.draw(spriteState);
		if (state == 1)
		{
			spriteStateParticles.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + 100);
			spriteStateParticles.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + 100);
			spriteStateParticles.scale = 0.7f;
			spriteStateParticles.angle = angle * Calc::rad2deg + 90;
			setColor(colorWhite);
			PARTICLE_SPRITE.draw(spriteStateParticles);
			spriteStateDiamond.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + 200);
			spriteStateDiamond.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + 200);
			spriteStateDiamond.scale = 0.7f;
			spriteStateDiamond.angle = angle * Calc::rad2deg + 90;
			setColor(colorWhite);
			DIAMOND_SPRITE.draw(spriteStateDiamond);
		}
	}

	void doPray()
	{
		pray = (pray ? false : true);
		state = (pray ? 0 : 3);
		spriteState.startAnim(LEMMING_SPRITE, state);
	}

	bool isActive;
	SpriterState spriteState;
	SpriterState spriteStateParticles;
	SpriterState spriteStateDiamond;
	float angle;
	bool pray;
	int state;
	bool sunHit;
	bool sunHitProcessed;
};

class Player
{
public:
	SpriterState spriteState;
	float angle;
	float max_speed;
	float speed;
	Lemming *selected_lemming;


	Player()
	{
		max_speed = 1.0;
		speed = 0.0;
		angle = 0.0;
	}

	void spawn()
	{
		spriteState.startAnim(PLAYER_SPRITE, 0);
	}

	void tick(float dt)
	{
		// input

		float accel = 2;
		float currentAccel = 0;

		if (keyboard.isDown(SDLK_RIGHT))
			currentAccel += accel;
		if (keyboard.isDown(SDLK_LEFT))
			currentAccel -= accel;
		if (keyboard.wentUp(SDLK_SPACE))
			selected_lemming = 0;

		speed += dt * currentAccel;

		if (speed < -max_speed)
			speed = -max_speed;
		if (speed > max_speed)
			speed = max_speed;

		if (currentAccel == 0)
			speed = speed * powf(0.01f, dt);

		angle += speed * dt;
		if (selected_lemming)
		{
			selected_lemming->angle = angle;
		}
		spriteState.updateAnim(PLAYER_SPRITE, dt);

		// lemmings

	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.7f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		PLAYER_SPRITE.draw(spriteState);
	}

};

Lemming	*check_collision_player(Player player, Lemming lemmings[], int nbr_lem)
{
	Lemming	* result = 0;
	int nearest = INT_MAX;

	int dx;
	int dy;
	for (int i = 0; i < nbr_lem; ++i)
	{
		if (!lemmings[i].isActive)
			continue;

		dx = player.spriteState.x - lemmings[i].spriteState.x;
		dy = player.spriteState.y - lemmings[i].spriteState.y;
		int d = sqrtf(dx * dx + dy * dy);
		if (d < 100 && d < nearest)
		{
			result = (&lemmings[i]);
			nearest = d;
		}
	}
	return (result);
}

int check_collision_sun(Sun sun, Lemming lemmings[], int nbr_lem)
{
	const float angle = 10.f * Calc::deg2rad;
	const float cosAngle = cosf(angle);

	for (int i = 0; i < nbr_lem; ++i)
	{
		lemmings[i].sunHit = false;

		if (!lemmings[i].isActive)
			continue;
		if (!lemmings[i].pray)
			continue;

		float dx1 = sun.spriteState.x;
		float dy1 = sun.spriteState.y;
		{
			float d1 = sqrtf(dx1 * dx1 + dy1 * dy1);
			dx1 /= d1;
			dy1 /= d1;
		}

		float dx2 = lemmings[i].spriteState.x;
		float dy2 = lemmings[i].spriteState.y;
		{
			float d2 = sqrtf(dx2 * dx2 + dy2 * dy2);
			dx2 /= d2;
			dy2 /= d2;
		}

		float dot = dx1 * dx2 + dy1 * dy2;

		if (dot >= 0.f && dot >= cosAngle)
			lemmings[i].sunHit = true;
	}
	return (0);
}

Player player;
Lemming lemmings[MAX_LEMMINGS];
Sun sun;

int main(int argc, char * argv[])
{
	if (1 == 2)
	{
		framework.fullscreen = false;
		framework.minification = 2;
	}
	else
	{
		framework.fullscreen = true;
	}

	if (framework.init(0, 0, SX, SY))
	{
		SpriterState heartState;
		heartState.x = WORLD_X;
		heartState.y = WORLD_Y;
		heartState.animSpeed = 0.1f;
		heartState.startAnim(HEART_SPRITE, 0);

		for (int i = 0; i < MAX_LEMMINGS / 10; ++i)
		{
			lemmings[i].isActive = true;
			lemmings[i].angle = random(0.f, 1.f) * 2.f * M_PI;
			lemmings[i].spawn();
		}
		sun.spawn();
		player.spawn();

		bool stop = false;

		while (!stop)
		{
			// process

			framework.process();

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				stop = true;
			}

			// logic


			const float dt = 1.f / 60.f;

			heartState.updateAnim(HEART_SPRITE, dt);

			player.tick(dt);
			sun.tick(dt);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
				if (lemmings[i].isActive)
					lemmings[i].tick(dt);
			Lemming *le = check_collision_player(player, lemmings, MAX_LEMMINGS);
			if (le)
			{
				if (keyboard.wentDown(SDLK_SPACE))
					player.selected_lemming = le;
				if (keyboard.wentDown(SDLK_UP))
					le->doPray();
			}

			check_collision_sun(sun, lemmings, MAX_LEMMINGS);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
			{
				if (!lemmings[i].isActive)
					continue;
				if (!lemmings[i].sunHitProcessed && lemmings[i].sunHit)
				{
					--sun.prays_needed;
					lemmings[i].sunHitProcessed = true;
				}
			}

			// draw

			framework.beginDraw(0, 0, 0, 0);
			{
				setColor(colorWhite);

				gxPushMatrix();

				Sprite background("Art Assets/Wallpaper.jpg");
				background.drawEx(0, 0);

				gxTranslatef(SX/2, SY/2, 0);

				gxScalef(0.7f, 0.7f, 1);
				Sprite earth("Art Assets/planete.png");
				earth.drawEx(-earth.getWidth() / 2, -earth.getHeight() / 2, 0.0, 1.0, 1.0, true, FILTER_POINT);

				HEART_SPRITE.draw(heartState);

				for (int i = 0; i < MAX_LEMMINGS; ++i)
					if (lemmings[i].isActive)
						lemmings[i].draw();

				player.draw();
				sun.draw();

				gxPopMatrix();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
