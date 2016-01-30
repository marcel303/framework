#include "Calc.h"
#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"

#include "audio.h"
#include "audiostream/AudioOutput.h"

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

int activeAudioSet = 0;

OPTION_DECLARE(bool, DEBUG_DRAW, false);
OPTION_DEFINE(bool, DEBUG_DRAW, "Debug Draw");

struct SoundInfo
{
	std::string fmt;
	int count;
	float volume;
};

static std::map<std::string, SoundInfo> s_sounds;

static void addSound(const char * name, const char * fmt, int count, float volume = 1.f)
{
	SoundInfo & s = s_sounds[name];
	s.fmt = fmt;
	s.count = count;
	s.volume = volume;
}

static void initSound()
{
	addSound("pickup", "Sound Assets/Effects/Test/jump%02d.ogg", 4, .05f);
}

static void playSound(const char * name)
{
	if (s_sounds.count(name) == 0)
	{
		logError("sound not found: %s", name);
		return;
	}

	SoundInfo & s = s_sounds[name];
	char temp[256];
	sprintf(temp, s.fmt.c_str(), rand() % s.count);
	Sound(temp).play(s.volume * 100);
}

class Sun
{
public:
	enum Emotion
	{
		kEmotion_Happy,
		kEmotion_Neutral,
		kEmotion_Sad
	};

	Sun()
	{
		angle = 0.0f;
		distance = 700.0f;
		speed = 0.1f;
		prays_needed = 2;
		emotion = kEmotion_Neutral;
	}

	void spawn()
	{
		spriteState.startAnim(SUN_SPRITE, 0);

		color = colorYellow;
		targetColor = color;
	}

	void tickEmotion()
	{
		if (emotion == kEmotion_Happy)
		{
			targetColor = colorRed;
			activeAudioSet = 3;
		}
		else if (emotion == kEmotion_Neutral)
		{
			targetColor = colorYellow;
			activeAudioSet = 2;
		}
		else if (emotion == kEmotion_Sad)
		{
			targetColor = colorBlue;
			activeAudioSet = 1;
		}

		color = color.interp(targetColor, 0.01f);
	}

	void tick(float dt)
	{
		angle += dt * speed;
		spriteState.updateAnim(SUN_SPRITE, dt);

		if (prays_needed > 0)
			emotion = kEmotion_Sad;
		else if (prays_needed < 0)
			emotion = kEmotion_Happy;
		else
			emotion = kEmotion_Neutral;

		tickEmotion();
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteState.scale = 1.5f;
		setColor(color);
		SUN_SPRITE.draw(spriteState);
	}

	SpriterState spriteState;
	float angle;
	float distance;
	float speed;
	int sun_state; // 0 not happy, 1 it's okey, 2, too happy
	int prays_needed;
	Emotion emotion;
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
		spriteState.scale = 0.4f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		LEMMING_SPRITE.draw(spriteState);
		if (state == 1)
		{
			spriteStateParticles.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + 100);
			spriteStateParticles.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + 100);
			spriteStateParticles.scale = 0.4f;
			spriteStateParticles.angle = angle * Calc::rad2deg + 90;
			setColor(colorWhite);
			PARTICLE_SPRITE.draw(spriteStateParticles);
			spriteStateDiamond.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + 200);
			spriteStateDiamond.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + 200);
			spriteStateDiamond.scale = 0.4f;
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
		max_speed = 0.6;
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
		spriteState.scale = 0.6f;
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
float cam = 0.0;

int main(int argc, char * argv[])
{
	if (1 == 1)
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
		OptionMenu optionsMenu;

		AudioOutput_OpenAL ao;
		MyAudioStream mas;
		mas.Open("Sound Assets/Music/Loops/Music_Layer_%02d_Loop.ogg", "Sound Assets/Music/Loops/Choir_Layer_%02d_Loop.ogg", "Sound Assets/AMB/AMB_STATE_%02d_LOOP.ogg");
		ao.Initialize(2, 48000, 1 << 14);
		ao.Play();
		AudioStream_Capture asc;
		asc.mSource = &mas;

		fftInit();

		struct AudioSet
		{
			float volume[12];
		} audioSets[4] =
		{
			{ 1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0 },
			{ 1, 1, 0, 0,  1, 1, 0, 0,  0, 1, 0, 0 },
			{ 1, 1, 1, 0,  1, 1, 1, 0,  0, 0, 1, 0 },
			{ 0, 0, 1, 1,  1, 1, 1, 1,  0, 0, 0, 1 }
		};

		initSound();

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
			const float dt = 1.f / 60.f;

			// process

			framework.process();

			asc.mTime = framework.time;
			ao.Update(&asc);
			fftProcess(framework.time);

			static bool doOptions = false;

			if (keyboard.wentDown(SDLK_F5))
				doOptions = !doOptions;

			if (doOptions)
			{
				OptionMenu * menu = &optionsMenu;

				menu->Update();

				if (keyboard.isDown(SDLK_UP) || gamepad[0].isDown(DPAD_UP))
					menu->HandleAction(OptionMenu::Action_NavigateUp, dt);
				if (keyboard.isDown(SDLK_DOWN) || gamepad[0].isDown(DPAD_DOWN))
					menu->HandleAction(OptionMenu::Action_NavigateDown, dt);
				if (keyboard.wentDown(SDLK_RETURN) || gamepad[0].wentDown(GAMEPAD_A))
					menu->HandleAction(OptionMenu::Action_NavigateSelect);
				if (keyboard.wentDown(SDLK_BACKSPACE) || gamepad[0].wentDown(GAMEPAD_B))
				{
					if (menu->HasNavParent())
						menu->HandleAction(OptionMenu::Action_NavigateBack);
					else
						doOptions = false;
				}
				if (keyboard.isDown(SDLK_LEFT) || gamepad[0].isDown(DPAD_LEFT))
					menu->HandleAction(OptionMenu::Action_ValueDecrement, dt);
				if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].isDown(DPAD_RIGHT))
					menu->HandleAction(OptionMenu::Action_ValueIncrement, dt);
			}

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				stop = true;
			}

			if (keyboard.wentDown(SDLK_q))
				activeAudioSet = 0;
			if (keyboard.wentDown(SDLK_w))
				activeAudioSet = 1;
			if (keyboard.wentDown(SDLK_e))
				activeAudioSet = 2;
			if (keyboard.wentDown(SDLK_r))
				activeAudioSet = 3;

			for (int c = 0; c < 12; ++c)
				mas.targetVolume[c] = audioSets[activeAudioSet].volume[c] / 8.f;

			if (keyboard.wentDown(SDLK_s))
				playSound("pickup");

			// logic

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
				//gxTranslatef((SX/2)-player.spriteState.x, SY/2-player.spriteState.y, 0);

				static float targetZoom = 1.f;
				static float zoom = 1.f;

				/*if (sun.spriteState.x + sun.spriteState.scaleX > SX || sun.spriteState.y + sun.spriteState.scaleY > SY)
					targetZoom -= 0.1f;
				else if (sun.spriteState.x + sun.spriteState.scaleX < SX - 5.0 || sun.spriteState.y + sun.spriteState.scaleY < SY - 5)
					targetZoom += 0.1f;*/

				const int PLAYER_SIZE = 200;
				const float px1 = player.spriteState.x - PLAYER_SIZE;
				const float py1 = player.spriteState.y - PLAYER_SIZE;
				const float px2 = player.spriteState.x + PLAYER_SIZE;
				const float py2 = player.spriteState.y + PLAYER_SIZE;

				const int SUN_SIZE = 400;
				const float sx1 = sun.spriteState.x - SUN_SIZE;
				const float sy1 = sun.spriteState.y - SUN_SIZE;
				const float sx2 = sun.spriteState.x + SUN_SIZE;
				const float sy2 = sun.spriteState.y + SUN_SIZE;

				const float vx1 = Calc::Min(px1, sx1);
				const float vy1 = Calc::Min(py1, sy1);
				const float vx2 = Calc::Max(px2, sx2);
				const float vy2 = Calc::Max(py2, sy2);

				const float midX = (vx1 + vx2) / 2.f;
				const float midY = (vy1 + vy2) / 2.f;

				//targetZoom = 1.0 / (abs((sun.spriteState.x - player.spriteState.x)));
				const float boxSizeX = vx2 - vx1;
				const float boxSizeY = vy2 - vy1;
				const float scaleX = SX / boxSizeX;
				const float scaleY = SY / boxSizeY;

				targetZoom = Calc::Min(scaleX, scaleY);

				if (keyboard.wentDown(SDLK_1))
					targetZoom = .25f;
				if (keyboard.wentDown(SDLK_2))
					targetZoom = .5;
				if (keyboard.wentDown(SDLK_3))
					targetZoom = 1.f;
				const float t = powf(.05f, dt);
				zoom = t * zoom + (1.f - t) * targetZoom;

				gxScalef(zoom, zoom, 1);
				gxTranslatef(-midX, -midY, 0.f);

				static Color earth_color = colorWhite;
				Color earth_target_color = colorWhite;
				if (sun.emotion == sun.kEmotion_Happy)
					earth_target_color = Color::fromHex("510a42");
				else if (sun.emotion == sun.kEmotion_Neutral)
					earth_target_color = Color::fromHex("ab5a10");
				else
					earth_target_color = Color::fromHex("9cc4d1");
				Sprite earth("Art Assets/planete.png");
				earth_color = earth_color.interp(earth_target_color, 0.02f);
				setColor(earth_color);
				earth.drawEx(-earth.getWidth() / 2, -earth.getHeight() / 2, 0.0, 1.0, 1.0, true, FILTER_POINT);

				HEART_SPRITE.draw(heartState);

				glBegin(GL_LINE_LOOP);
				{
					int n = Calc::Min(50, kFFTComplexSize);
					float a = 0.f;
					float s = 1.f / n * 2.f * M_PI;

					gxColor4f(1.f, 0.f, 0.f, 1.f);

					for (int i = 0; i < n; ++i, a += s)
					{
						float v = fftPowerValue(i) * 5.f + 200.f;
						float x = cosf(a) * v;
						float y = sinf(a) * v;
						gxVertex2f(x, y);
					}
				}
				glEnd();

				for (int i = 0; i < MAX_LEMMINGS; ++i)
					if (lemmings[i].isActive)
						lemmings[i].draw();

				player.draw();
				sun.draw();

				gxPopMatrix();
			}

			if (DEBUG_DRAW)
			{
				setFont("calibri.ttf");
				setColor(colorWhite);

				drawText(10, 10, 24, +1, +1, "Debug Draw!");
			}

			if (doOptions)
				optionsMenu.Draw(100, 100, 400, 600);

			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
