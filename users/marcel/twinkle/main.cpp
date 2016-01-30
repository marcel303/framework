#include "Calc.h"
#include "framework.h"

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
	Sun()
	{
		angle = 0.0f;
		distance = 300.0f;
		speed = 0.05f;
		sun_state = 0;
		color = colorYellow;
	}

	void spawn()
	{
		spriteState.startAnim(SUN_SPRITE, 0);
	}

	void tick(float dt)
	{
		angle += dt * speed;
		spriteState.updateAnim(SUN_SPRITE, dt);
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteState.scale = 0.5f;
		setColor(colorYellow);
		SUN_SPRITE.draw(spriteState);
	}

	SpriterState spriteState;
	float angle;
	float distance;
	float speed;
	int sun_state; // -1 not happy, 0 it's okey, 1, too happy
	Color color;
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
	}

	void spawn()
	{
		spriteState.startAnim(LEMMING_SPRITE, state);
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
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.5f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		LEMMING_SPRITE.draw(spriteState);
	}

	void doPray()
	{
		pray = (pray ? false : true);
		state = (pray ? 0 : 3);
		spriteState.startAnim(LEMMING_SPRITE, state);
	}

	bool isActive;
	SpriterState spriteState;
	float angle;
	bool pray;
	int state;
};

Lemming lemmings[MAX_LEMMINGS];

class Player
{
public:
	SpriterState spriteState;
	float angle;
	float max_speed;
	float speed;


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

		speed += dt * currentAccel;

		if (speed < -max_speed)
			speed = -max_speed;
		if (speed > max_speed)
			speed = max_speed;

		if (currentAccel == 0)
			speed = speed * powf(0.01f, dt);

		angle += speed * dt;

		spriteState.updateAnim(PLAYER_SPRITE, dt);

		// lemmings

	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 1.0f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		PLAYER_SPRITE.draw(spriteState);
	}
};

Lemming	*check_collision(Player player, Lemming lemmings[], int nbr_lem)
{
	int dx;
	int dy;
	for (int i = 0; i < nbr_lem; ++i)
	{
		if (!lemmings[i].isActive)
			continue;

		dx = player.spriteState.x - lemmings[i].spriteState.x;
		dy = player.spriteState.y - lemmings[i].spriteState.y;
		if (sqrtf(dx * dx + dy * dy) <= 100)
			return (&lemmings[i]);
	}
	return (0);
}

Player player;
Sun sun;

int main(int argc, char * argv[])
{
	framework.fullscreen = true;
	//framework.minification = 2;

	if (framework.init(0, 0, SX, SY))
	{
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

		int activeAudioSet = 0;

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
			// process

			framework.process();

			asc.mTime = framework.time;
			ao.Update(&asc);
			fftProcess(framework.time);

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


			const float dt = 1.f / 60.f;

			heartState.updateAnim(HEART_SPRITE, dt);

			player.tick(dt);
			sun.tick(dt);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
				if (lemmings[i].isActive)
					lemmings[i].tick(dt);
			Lemming *le = check_collision(player, lemmings, MAX_LEMMINGS);
			if (le)
			{
				if (keyboard.wentDown(SDLK_UP))
					le->doPray();
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
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
