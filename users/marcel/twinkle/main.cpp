#include "Calc.h"
#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"

#include "audio.h"
#include "audiostream/AudioOutput.h"

#include <Windows.h>

#define SX 1920
#define SY 1080

#define WORLD_RADIUS 400
#define WORLD_X 0
#define WORLD_Y 0
#define MAX_LEMMINGS 100

#define SPAWN_DEATH_CLAMP 5.f

#define LEMMING_SPRITE Spriter("Art Assets/Animation/Shoon/Shoon_anim_000.scml")
#define HEART_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define HEART_FACE_SPRITE Spriter("Art Assets/Animation/Faces_Earth/Earth_Faces_000.scml")
#define PLAYER_SPRITE Spriter("Art Assets/Characters/Player/Player_animations.scml")
#define PLAYER2_SPRITE Spriter("Art Assets/Characters/Him/Him_animations.scml")
#define SUN_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define PARTICLE_SPRITE Spriter("Art Assets/Animation/Particules/Particules.scml")
#define DIAMOND_SPRITE Spriter("Art Assets/Animation/Diamonds/Diamond.scml")
#define EXPLOSION_SPRITE Spriter("Art Assets/Animation/Explosion/Explosion.scml")

int activeAudioSet = 0;

OPTION_DECLARE(bool, DEBUG_DRAW, true);
OPTION_DEFINE(bool, DEBUG_DRAW, "Debug Draw");

OPTION_DECLARE(int, SUN_DISTANCE, 700);
OPTION_DEFINE(int, SUN_DISTANCE, "Sun/Distance");
OPTION_STEP(SUN_DISTANCE, 0, 1000, 50);

OPTION_DECLARE(float, SUN_LOSS_PER_SEC, 1.f);
OPTION_DEFINE(float, SUN_LOSS_PER_SEC, "Sun/Loss Per Sec");
OPTION_STEP(SUN_LOSS_PER_SEC, 0, 0, 0.01f);

OPTION_DECLARE(float, EARTH_XFER_PER_SEC, .05f);
OPTION_DEFINE(float, EARTH_XFER_PER_SEC, "Earth/Xfer Per Sec");
OPTION_STEP(EARTH_XFER_PER_SEC, 0, 0, 0.01f);

OPTION_DECLARE(float, H, 1.0f);
OPTION_DEFINE(float, H, "H");
OPTION_STEP(H, 0.0f, 1.0f, 0.01f);

OPTION_DECLARE(float, S, 1.0f);
OPTION_DEFINE(float, S, "S");
OPTION_STEP(S, 0.0f, 1.0f, 0.01f);

OPTION_DECLARE(float, L, 1.0f);
OPTION_DEFINE(float, L, "L");
OPTION_STEP(L, 0.0f, 1.0f, 0.01f);

OPTION_DECLARE(float, SHOON_SPAWN_CHANCE, 0.02f);
OPTION_DEFINE(float, SHOON_SPAWN_CHANCE, "Shoon Spawn Chance");
OPTION_STEP(SHOON_SPAWN_CHANCE, 0.01f, 20.0f, 0.01f);

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
	// worshipper
	addSound("pray", "Sound Assets/SFX/SFX_Chief_Convert_to Prayer.ogg", 1);
	addSound("pray_complete", "Sound Assets/SFX/SFX_Worshipper_Prayer_Complete.ogg", 1);
	addSound("pray_pickup", "Sound Assets/SFX/SFX_Prayer_PickUp.ogg", 1);
	addSound("stop_prayer_hit", "Sound Assets/SFX/SFX_Chief_Stop_Prayer_Hit_%02d.ogg", 4);

	addSound("pickup", "Sound Assets/SFX/SFX_Worshiper_Picked_Up_%02d.ogg", 4);
	addSound("throw", "Sound Assets/SFX/SFX_Worshipper_Throw_%02d.ogg", 5);
	addSound("land", "Sound Assets/SFX/SFX_Worshiper_Landing_%02d.ogg", 4);
	addSound("death", "Sound Assets/SFX/SFX_Worshiper_Death_%02d.ogg", 3);

	// earth
	addSound("earth_transition_frozen", "Sound Assets/SFX/SFX_Earth_Transition_to_Frozen.ogg", 1);
	addSound("earth_transition_cold", "Sound Assets/SFX/SFX_Earth_Transition_to_Cold.ogg", 1);
	addSound("earth_transition_warm", "Sound Assets/SFX/SFX_Earth_Transition_to_Warm.ogg", 1);
	addSound("earth_transition_hot", "Sound Assets/SFX/SFX_Earth_Transition_to_Hot.ogg", 1);
	addSound("earth_explode", "Sound Assets/SFX/SFX_Earth_Explodes.ogg", 1);

	// sun
	addSound("sun_transition_depressed", "Sound Assets/SFX/SFX_Sun_Depressed.ogg", 1);
	addSound("sun_transition_unhappy", "Sound Assets/SFX/SFX_Sun_Unhappy.ogg", 1);
	addSound("sun_transition_happy", "Sound Assets/SFX/SFX_Sun_Happy_New.ogg", 1);
	addSound("sun_transition_manichappy", "Sound Assets/SFX/SFX_Sun_Maniac_Happy.ogg", 1);
}

static void playSound(const char * name)
{
	log("playSound: %s", name);

	if (s_sounds.count(name) == 0)
	{
		logError("sound not found: %s", name);
		return;
	}

	SoundInfo & s = s_sounds[name];
	char temp[256];
	sprintf(temp, s.fmt.c_str(), 1 + (rand() % s.count));
	Sound(temp).play(s.volume * 100);
}

//

class Sun
{
public:
	Spriter * faces[4];

	enum Emotion
	{
		kEmotion_Depressed,
		kEmotion_Sad,
		kEmotion_Happy,
		kEmotion_Lucid
	};

	Sun()
	{
		angle = 0.0f;
		distance = 700.0f;
		speed = 0.1f;
		emotion = kEmotion_Happy;
		face = 0;
		actual_face = 0;
		happiness = 7.f;
	}

	void spawn()
	{
		spriteState.startAnim(SUN_SPRITE, 0);
		spriteFaceState.startAnim(*faces[emotion], actual_face);

		color = colorYellow;
		targetColor = color;

		faces[kEmotion_Depressed] = new Spriter("Art Assets/Sun/Sun_animations_depressed.scml");
		faces[kEmotion_Sad] = new Spriter("Art Assets/Sun/Sun_animations_unhappy.scml");
		faces[kEmotion_Happy] = new Spriter("Art Assets/Sun/Sun_animations_happy.scml");
		faces[kEmotion_Lucid] = new Spriter("Art Assets/Sun/Sun_animations_insane.scml");
	}

	void tickEmotion()
	{
		static Emotion lastEmotion = kEmotion_Happy;

		if (emotion != lastEmotion)
		{
			lastEmotion = emotion;

		#if !defined(DEBUG) || 1
			if (emotion == kEmotion_Depressed)
				playSound("sun_transition_depressed");
			if (emotion == kEmotion_Sad)
				playSound("sun_transition_unhappy");
			if (emotion == kEmotion_Happy)
				playSound("sun_transition_happy");
			if (emotion == kEmotion_Lucid)
				playSound("sun_transition_manichappy");
		#endif

			if (emotion == kEmotion_Lucid)
			{
				targetColor = colorRed;
				activeAudioSet = 3;
				face = rand() % 3;
			}
			else if (emotion == kEmotion_Happy)
			{
				targetColor = colorYellow;
				activeAudioSet = 2;
				face = rand() % 3;
			}
			else if (emotion == kEmotion_Sad)
			{
				targetColor = colorBlue;
				activeAudioSet = 1;
				face = rand() % 3;
			}
			else if (emotion == kEmotion_Depressed)
			{
				targetColor = colorBlue;
				activeAudioSet = 1;
				face = rand() % 3;
			}

			if (face != actual_face)
			{
				actual_face = face;
				spriteFaceState.startAnim(*faces[emotion], actual_face);
			}
		}

		color = color.interp(targetColor, 0.01f);
	}

	void tick(float dt)
	{
		distance = SUN_DISTANCE;

		angle += dt * speed;
		spriteState.updateAnim(SUN_SPRITE, dt);
		const bool isDone = spriteFaceState.updateAnim(*faces[emotion], dt);

		if (isDone)
		{
			face = actual_face = rand() % 3;
			spriteFaceState.startAnim(*faces[emotion], actual_face);
		}

		if (happiness > -50 && happiness < -25)
			emotion = kEmotion_Depressed;
		if (happiness > -25 && happiness < 0)
			emotion = kEmotion_Sad;
		if (happiness > 0 && happiness < 25)
			emotion = kEmotion_Happy;
		if (happiness > 25 && happiness < 50)
			emotion = kEmotion_Lucid;

		tickEmotion();
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteState.scale = 1.5f;
		setColor(color);
		SUN_SPRITE.draw(spriteState);
		spriteFaceState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteFaceState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteFaceState.scale = 0.4f;
		setColor(color);
		faces[emotion]->draw(spriteFaceState);
	}

	SpriterState spriteState;
	SpriterState spriteFaceState;
	float angle;
	float distance;
	float speed;
	Emotion emotion;
	Color color;
	Color targetColor;
	int face;
	int actual_face;
	float happiness;
};


class Heart_Faces
{
public:
	Heart_Faces()
	{
		animation = 0;
	}

	void update_animation(int anim)
	{
		if (animation != anim)
			spriteState.startAnim(HEART_FACE_SPRITE, anim);
		animation = anim;
	}

	void tick(float dt)
	{
		spriteState.updateAnim(HEART_FACE_SPRITE, dt);
	}

	void draw(Color color)
	{
		spriteState.x = WORLD_X;
		spriteState.y = WORLD_Y;
		spriteState.scale = 1.0f;
		setColor(color);
		HEART_FACE_SPRITE.draw(spriteState);
	}

	SpriterState spriteState;
	int animation;
};

class Lemming
{
public:
	Lemming()
	{
		isActive = false;
		pray = false;
		angle = 0.0f;
		speed = 0.0f;
		state = 2;
		sunHit = false;
		sunHitProcessed = false;
		distance = 0.0f;
		real_distance = 0.0f;
		diamond_distance = 100.0f;
		diamond = true;
	}

	void spawn()
	{
		pray = false;
		angle = 0.0f;
		speed = 0.0f;
		state = 2;
		sunHit = false;
		sunHitProcessed = false;
		distance = 0.0f;
		real_distance = 0.0f;
		diamond_distance = 100.0f;
		diamond = true;
		scale = 1.f;
		spriteState.startAnim(LEMMING_SPRITE, state);
		spriteStateDiamond.startAnim(DIAMOND_SPRITE, 0);
		spriteStateParticles.startAnim(PARTICLE_SPRITE, 0);
	}

	void tick(float dt)
	{
		const int oldState = state;

		if (real_distance != distance)
		{
			const float dropSpeed = 100.f;
			const float liftSpeed = 100.f;
			if (real_distance < distance)
				real_distance = Calc::Min(real_distance + liftSpeed * dt, distance);
			if (real_distance > distance)
				real_distance = Calc::Max(real_distance - dropSpeed * dt, distance);
			if (real_distance == distance)
			{
				if (distance == 0.f)
					playSound("land");
			}
		}
		bool animIsDone = spriteState.updateAnim(LEMMING_SPRITE, dt);
		if (animIsDone)
		{
			if (state == 0)
				state = 1;
			if (state == 3)
				isActive = false;
			spriteState.startAnim(LEMMING_SPRITE, state);
		}
		if (state == 1)
		{
			if (diamond)
				spriteStateDiamond.updateAnim(DIAMOND_SPRITE, dt);
			spriteStateParticles.updateAnim(PARTICLE_SPRITE, dt);
			if (sunHit)
				diamond_distance += 200.0f * dt;
			if (!sunHit && diamond_distance > 150.0f && diamond)
				diamond_distance -= 400.0f * dt;
			if (diamond_distance > 450.0f)
			{
				spriteStateDiamond.stopAnim(DIAMOND_SPRITE);
				diamond = false;

				//doPray(false);
			}
		}

		if (state != oldState)
		{
			if (state == 1)
				playSound("pray_complete");
		}

		angle += speed * dt;
		speed = speed * powf(0.01f, dt);
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance);
		spriteState.scale = 0.4f * scale;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		LEMMING_SPRITE.draw(spriteState);
		if (state == 1)
		{
			spriteStateParticles.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance + 50);
			spriteStateParticles.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance + 50);
			spriteStateParticles.scale = 0.4f;
			spriteStateParticles.angle = angle * Calc::rad2deg + 90;
			setColor(colorWhite);
			PARTICLE_SPRITE.draw(spriteStateParticles);
			if (diamond)
			{
				spriteStateDiamond.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance + diamond_distance);
				spriteStateDiamond.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance + diamond_distance);
				spriteStateDiamond.scale = 0.4f;
				spriteStateDiamond.angle = angle * Calc::rad2deg + 90;
				setColor(colorWhite);
				DIAMOND_SPRITE.draw(spriteStateDiamond);
			}
		}

		if (DEBUG_DRAW)
		{
			drawCircle(spriteState.x, spriteState.y, 10, 10);
		}
	}

	void doPray(bool v)
	{
		pray = v;
		state = (pray ? 0 : 2);
		spriteState.startAnim(LEMMING_SPRITE, state);

		if (pray)
		{
			sunHit = false;
			sunHitProcessed = false;
			diamond_distance = 100.0f;
			diamond = true;
		}
	}

	void doPray()
	{
		doPray(!pray);
	}

	void doDie()
	{
		state = 3;
		spriteState.startAnim(LEMMING_SPRITE, state);
	}

	bool isActive;
	SpriterState spriteState;
	SpriterState spriteStateParticles;
	SpriterState spriteStateDiamond;
	float angle;
	float speed;
	bool pray;
	int state;
	bool sunHit;
	bool sunHitProcessed;
	float diamond_distance;
	bool diamond;
	float distance;
	float real_distance;
	float scale;
};

class Player
{
public:
	SpriterState spriteState;
	float angle;
	float max_speed;
	float speed;
	bool direction_right;
	Lemming *selected_lemming;
	float scale;

	Player()
	{
		max_speed = 1.f;
		speed = 0.0;
		angle = 0.0;
		direction_right = true;
		scale = 1.f;
	}

	void spawn()
	{
		spriteState.startAnim(PLAYER_SPRITE, 0);
	}

	void tick(float dt)
	{
		// input

		float accel = 2.5f;
		float currentAccel = 0;
		if (!keyboard.isDown(SDLK_RIGHT) && !keyboard.isDown(SDLK_LEFT))
			spriteState.startAnim(PLAYER_SPRITE, 0);
		if (keyboard.wentDown(SDLK_RIGHT))
		{
			if (selected_lemming)
				spriteState.startAnim(PLAYER_SPRITE, 3);
			else
				spriteState.startAnim(PLAYER_SPRITE, 4);
		}
		if (keyboard.wentDown(SDLK_LEFT))
		{
			if (selected_lemming)
				spriteState.startAnim(PLAYER_SPRITE, 3);
			else
				spriteState.startAnim(PLAYER_SPRITE, 4);
		}
		if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].getAnalog(0, ANALOG_X) > +.5f)
		{
			currentAccel += accel;
			direction_right = true;
		}
		if (keyboard.isDown(SDLK_LEFT) || gamepad[0].getAnalog(0, ANALOG_X) < -.5f)
		{
			currentAccel -= accel;
			direction_right = false;
		}
		if (keyboard.wentUp(SDLK_SPACE) || gamepad[0].wentUp(GAMEPAD_A))
		{
			if (selected_lemming)
			{
				selected_lemming->distance = 0.0f;
				selected_lemming->speed = speed * 6.0f;

				playSound("throw");
			}
			selected_lemming = 0;
		}

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
			selected_lemming->distance = 50.0f;
		}
		spriteState.updateAnim(PLAYER_SPRITE, dt);

		// lemmings

	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.2f * scale;
		spriteState.angle = angle * Calc::rad2deg + 90;
		if (!direction_right)
			spriteState.flipX = true;
		else
			spriteState.flipX = false;
		setColor(colorWhite);
		PLAYER_SPRITE.draw(spriteState);
		if (DEBUG_DRAW)
		{
			drawCircle(spriteState.x, spriteState.y, 10, 10);
		}
	}

};

class Player2
{
public:
	SpriterState spriteState;
	float angle;
	float max_speed;
	float speed;
	Lemming *selected_lemming;
	bool direction_right;
	float scale;

	Player2()
	{
		max_speed = 0.6;
		speed = 0.0;
		angle = 0.0;
		scale = 1.f;
		direction_right = true;
	}

	void spawn()
	{
		spriteState.startAnim(PLAYER2_SPRITE, 0);
	}

	void tick(float dt)
	{
		// input

		float accel = 2;
		float currentAccel = 0;
		if (!keyboard.isDown(SDLK_d) && !keyboard.isDown(SDLK_q))
			spriteState.startAnim(PLAYER2_SPRITE, 0);
		if (keyboard.wentDown(SDLK_d))
		{
			if (selected_lemming)
				spriteState.startAnim(PLAYER2_SPRITE, 3);
			else
				spriteState.startAnim(PLAYER2_SPRITE, 4);
		}
		if (keyboard.wentDown(SDLK_q))
		{
			if (selected_lemming)
				spriteState.startAnim(PLAYER2_SPRITE, 3);
			else
				spriteState.startAnim(PLAYER2_SPRITE, 4);
		}
		if (keyboard.isDown(SDLK_d) || gamepad[1].getAnalog(0, ANALOG_X) > +.5f)
		{
			currentAccel += accel;
			direction_right = true;
		}
		if (keyboard.isDown(SDLK_q) || gamepad[1].getAnalog(0, ANALOG_X) < -.5f)
		{
			currentAccel -= accel;
			direction_right = false;
		}
		if (keyboard.wentUp(SDLK_l) || gamepad[1].wentUp(GAMEPAD_A))
		{
			if (selected_lemming)
			{
				selected_lemming->distance = 0.0f;
				selected_lemming->speed = speed * 7.5f;

				playSound("throw");
			}
			selected_lemming = 0;
		}

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
			selected_lemming->distance = 50.0f;
		}
		spriteState.updateAnim(PLAYER2_SPRITE, dt);

		// lemmings

	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.2f * scale;
		spriteState.angle = angle * Calc::rad2deg + 90;
		if (!direction_right)
			spriteState.flipX = true;
		else
			spriteState.flipX = false;
		setColor(colorWhite);
		PLAYER2_SPRITE.draw(spriteState);
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

Lemming	*check_collision_player2(Player2 player, Lemming lemmings[], int nbr_lem)
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
Player2 player2;
Lemming lemmings[MAX_LEMMINGS];
Sun sun;
Heart_Faces heart_faces;
float earth_happiness = 0;
//float earth_happiness = -45;
enum EarthState
{
	kEarthState_Frozen,
	kEarthState_Cold,
	kEarthState_Warm,
	kEarthState_Hot
};

float spawn_timer = 0.0;
float pray_timer = 0.0;

static bool getSunDead(float warmth)
{
	return warmth == -50.f || warmth == +50.f;
}

static bool getEarthDead(float warmth)
{
	return warmth == -50.f || warmth == +50.f;
}

static EarthState getEarthState(float warmth)
{
	fassert(warmth >= -50 && warmth <= +50);

	if (warmth < -25.0)
		return kEarthState_Frozen;
	else if (warmth < 0)
		return kEarthState_Cold;
	else if (warmth < 25)
		return kEarthState_Warm;
	else if (warmth <= 50)
		return kEarthState_Hot;
	else
	{
		fassert(false);
		return kEarthState_Hot;
	}
}

static Color getEarthColor(EarthState state)
{
	if (state == kEarthState_Frozen)
		return Color::fromHSL(0.520f, 1.000f, 0.620f);
	else if (state == kEarthState_Cold)
		return Color::fromHSL(0.520f, 1.000f, 0.620f);
	else if (state == kEarthState_Warm)
		return Color::fromHSL(0.320f, 1.000f, 0.710f);
	else if (state == kEarthState_Hot)
		return Color::fromHSL(1.000f, 0.920f, 0.480f);
	else
	{
		fassert(false);
		return colorWhite;
	}
}

static void doTitleScreen()
{
#define CINE 1
	Music music("Sound Assets/Music/Twinkle_Menu_Theme_Loop.ogg");
	music.play();

	Sprite background("Art Assets/Wallpaper.jpg");
	background.drawEx(0, 0);

#if CINE
	Spriter spriter("Cinematic/Cinematic.scml");
#else
	Spriter spriter("Art Assets/Sun/Sun_Animations.scml");
#endif
	
	SpriterState spriterState;
	spriterState.startAnim(spriter, 0);
#if CINE
	spriterState.x = SX/2;
	spriterState.y = SY/2;
	spriterState.scale = .14f;
#else
	spriterState.x = SX/2;
	spriterState.y = SY/2;
	spriterState.scale = .5f;
#endif
	
	bool stop = false;
	float stopTime = 0.f;
	bool wasInside = false;

	while (!stop)
	{
		framework.process();

		const float dt = framework.timeStep;

	#if CINE
		bool inside = true;
	#else
		int dx = mouse.x - SX/2;
		int dy = mouse.y - SY/2;
		int d = sqrtf(dx * dx + dy * dy);
		bool inside = d <= 400;

		if (inside && !wasInside)
			spriterState.startAnim(spriter, rand() % spriter.getAnimCount());
		if (!inside)
			spriterState.stopAnim(spriter);
	#endif

		wasInside = inside;

		if (stopTime == 0.f && ((mouse.wentDown(BUTTON_LEFT) && inside) || gamepad[0].wentDown(GAMEPAD_A)))
		{
			stopTime = 1.f;

			spriterState.animSpeed = 1.5f;
		}

		if (stopTime > 0.f)
		{
			stopTime = Calc::Max(0.f, stopTime - dt);

			if (stopTime == 0.f)
				stop = true;

			music.setVolume(100 * stopTime);
		}

		spriterState.updateAnim(spriter, dt);

		framework.beginDraw(0, 0, 0, 0);
		{
			setColorMode(COLOR_ADD);
			{
			#if CINE
				setColor(colorBlack);
			#else
				setColor(inside ? Color(63, 31, 15) : colorBlack);
			#endif
				spriter.draw(spriterState);
			}
			setColorMode(COLOR_MUL);

			if (stopTime > 0.f || stop)
			{
				setColorf(0.f, 0.f, 0.f, 1.f - stopTime);
				drawRect(0, 0, SX, SY);
			}
		}
		framework.endDraw();
	}

	music.stop();
}

Lemming * getRandomLemming()
{
	int numActive = 0;
	for (int i = 0; i < MAX_LEMMINGS; ++i)
		if (lemmings[i].isActive)
			numActive++;
	if (numActive > 1)
	{
		const int c = rand() % numActive;
		for (int i = 0, n = 0; i < MAX_LEMMINGS; ++i)
		{
			if (lemmings[i].isActive)
			{
				if (n == c)
					return &lemmings[i];
				n++;
			}
		}
	}
	return 0;
}

int main(int argc, char * argv[])
{
#ifdef DEBUG
	if (1 == 2)
	{
		framework.fullscreen = false;
		framework.minification = 2;
	}
	else
#endif
	{
		framework.fullscreen = true;
	}

	srand(GetTickCount());

	if (framework.init(0, 0, SX, SY))
	{
	#if !defined(DEBUG)
		framework.fillCachesWithPath("Art Assets", true);
		framework.fillCachesWithPath("Sound Assets", true);
	#endif

	#ifndef DEBUG
		doTitleScreen();
	#endif

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
		SpriterState explosion;
		explosion.x = WORLD_X;
		explosion.y = WORLD_Y;
		explosion.animSpeed = 1.f;
		heartState.x = WORLD_X;
		heartState.y = WORLD_Y;
		heartState.animSpeed = 0.1f;
		heartState.startAnim(HEART_SPRITE, 0);

		for (int i = 0; i < 8; ++i)
		{
			lemmings[i].isActive = true;
			lemmings[i].spawn();
			lemmings[i].angle = random(0.f, 1.f) * 2.f * M_PI;
			if (rand() % 2)
				lemmings[i].doPray();
		}

		sun.spawn();
		sun.angle = random(0.f, 1.f) * 2.f * M_PI;
		player.spawn();
		player.angle = random(0.f, 1.f) * 2.f * M_PI;
		player2.spawn();
		player2.angle = random(0.f, 1.f) * 2.f * M_PI;

		bool stop = false;

		int frameStart = 2;

		while (!stop)
		{
			static int dtMul = 1;
			if (keyboard.wentDown(SDLK_k))
				dtMul++;
			if (keyboard.wentDown(SDLK_l) && dtMul > 1)
				dtMul--;

			const float dt = dtMul / 60.f;

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

		#ifdef DEBUG
			if (keyboard.wentDown(SDLK_q))
				activeAudioSet = 0;
			if (keyboard.wentDown(SDLK_w))
				activeAudioSet = 1;
			if (keyboard.wentDown(SDLK_e))
				activeAudioSet = 2;
			if (keyboard.wentDown(SDLK_r))
				activeAudioSet = 3;
		#endif

			const float masVolumeMultiplier = 1.f / 4.f;
			for (int c = 0; c < 8; ++c)
				mas.targetVolume[c] = audioSets[activeAudioSet].volume[c] * masVolumeMultiplier;

		#ifdef DEBUG
			if (keyboard.wentDown(SDLK_s))
				playSound("pickup");
		#endif

			// logic

			heartState.updateAnim(HEART_SPRITE, dt);
			if (sun.emotion == sun.kEmotion_Lucid)
				heart_faces.update_animation(3);
			else if (sun.emotion == sun.kEmotion_Happy)
				heart_faces.update_animation(1);
			else if (sun.emotion == sun.kEmotion_Sad)
				heart_faces.update_animation(2);
			heart_faces.tick(dt);
			player.tick(dt);
			player2.tick(dt);
			sun.tick(dt);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
				if (lemmings[i].isActive)
					lemmings[i].tick(dt);
			Lemming *le = check_collision_player(player, lemmings, MAX_LEMMINGS);
			if (le)
			{
				if (keyboard.wentDown(SDLK_SPACE) || gamepad[0].wentDown(GAMEPAD_A))
				{
					player.selected_lemming = le;
					playSound("pickup");
				}
				if (keyboard.wentDown(SDLK_UP) || gamepad[0].wentDown(GAMEPAD_B))
				{
					le->doPray();
					if (le->pray)
						playSound("pray");
					else
						playSound("stop_prayer_hit");
				}
			}
			le = check_collision_player2(player2, lemmings, MAX_LEMMINGS);
			if (le)
			{
				if (keyboard.wentDown(SDLK_l) || gamepad[1].wentDown(GAMEPAD_A))
				{
					player2.selected_lemming = le;
					playSound("pickup");
				}
				if (keyboard.wentDown(SDLK_z) || gamepad[1].wentDown(GAMEPAD_B))
				{
					le->doPray();
					if (le->pray)
						playSound("pray");
					else
						playSound("stop_prayer_hit");
				}
			}

			check_collision_sun(sun, lemmings, MAX_LEMMINGS);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
			{
				if (!lemmings[i].isActive)
					continue;
				if (!lemmings[i].sunHitProcessed && lemmings[i].sunHit && lemmings[i].diamond_distance >= 450.0f)
				{
					sun.happiness += 5.f;
					lemmings[i].sunHitProcessed = true;
					lemmings[i].doPray(false);

					playSound("pray_pickup");
				}
			}

			//

			const float oldSunHappiness = sun.happiness;

			bool earth_dead = getEarthDead(earth_happiness);

			if (!earth_dead)
			{
				earth_happiness += sun.happiness * EARTH_XFER_PER_SEC * dt;
				earth_happiness = Calc::Clamp(earth_happiness, -50.f, +50.f);

				// did we die?
				earth_dead = getEarthDead(earth_happiness);
				if (earth_dead)
				{
					explosion.startAnim(EXPLOSION_SPRITE, 0);
					playSound("earth_explode");
				}
			}

			if (earth_dead)
				explosion.updateAnim(EXPLOSION_SPRITE, dt);

			bool sun_dead = getSunDead(sun.happiness);

			if (!sun_dead)
			{
				sun.happiness -= SUN_LOSS_PER_SEC * dt;
				sun.happiness = Calc::Clamp(sun.happiness, -50.f, +50.f);

				// did we die?
				sun_dead = getSunDead(sun.happiness);
				if (sun_dead)
				{
					// todo
				}

			#ifdef DEBUG
				if (keyboard.wentDown(SDLK_o))
					sun.happiness += 10.f;
				if (keyboard.wentDown(SDLK_p))
					sun.happiness -= 10.f;
			#endif
			}

			// randomly spawn

			spawn_timer += dt;

			if (spawn_timer >= 5.0f)
			{
				spawn_timer = 0.f;
				const float diceValue = random(0.f, 1.f);
				if((fabsf(earth_happiness) * SHOON_SPAWN_CHANCE) >= diceValue)
				{
					if (earth_happiness >= +SPAWN_DEATH_CLAMP)
					{
						for (int i = 0; i < MAX_LEMMINGS; ++i)
						{
							if (!lemmings[i].isActive)
							{
								lemmings[i].isActive = true;
								lemmings[i].spawn();
								lemmings[i].angle = random(0.f, 1.f) * 2.f * M_PI;
								break;
							}
						}
					}
					else if (earth_happiness <= -SPAWN_DEATH_CLAMP)
					{
						Lemming * le = getRandomLemming();
						if (le)
							le->doDie();
					}
				}
			}

			//

			const float newSunHappiness = sun.happiness;
			const float sunChange = newSunHappiness - oldSunHappiness;

			static volatile float sunRate = 0.f;
			sunRate += sunChange;
			sunRate = sunRate * powf(.5f, dt);

			const float sunVec1 = Calc::Clamp(sun.happiness / 25.f, -1.f, 1.f);
			const float sunVec2 = Calc::Clamp(sunRate, -1.f, 1.f);
			const float choirValue = Calc::Clamp(sunVec1 * sunVec2, 0.f, 1.f);

			for (int i = 8; i < 12; ++i)
				mas.targetVolume[i] = choirValue * audioSets[activeAudioSet].volume[i] * masVolumeMultiplier;

			// randomly activate pray

			pray_timer += dt;
			if (pray_timer >= 5.f)
			{
				pray_timer = 0.f;

				Lemming * le = getRandomLemming();
				if (le && le->state == 2)
					le->doPray();
			}
			// draw

			framework.beginDraw(0, 0, 0, 0);
			{
				setColor(colorWhite);

				gxPushMatrix();

				static float targetZoom = 1.f;
				static float zoom = -1.f;

				const int PLAYER_SIZE = 250;
				const float px1 = Calc::Min(player.spriteState.x, player2.spriteState.x) - PLAYER_SIZE;
				const float py1 = Calc::Min(player.spriteState.y, player2.spriteState.y) - PLAYER_SIZE;
				const float px2 = Calc::Max(player.spriteState.x, player2.spriteState.x) + PLAYER_SIZE;
				const float py2 = Calc::Max(player.spriteState.y, player2.spriteState.y) + PLAYER_SIZE;

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

				frameStart--;

				if (frameStart > 0)
					zoom = targetZoom;
				else
				{
					const float t = powf(.05f, dt);
					zoom = t * zoom + (1.f - t) * targetZoom;
				}

				gxPushMatrix();
				{
					const float zoomMin = .5f;
					const float zoomMax = 1.2f;
					const float zoomAmount = .25f;
					const float backgroundZoom = Calc::Clamp(zoom / zoomMin * zoomAmount + (1.f - zoomAmount), 1.f, 2.f);

					gxTranslatef(SX/2, SY/2, 0.f);
					gxScalef(backgroundZoom, backgroundZoom, 1.f);
					gxTranslatef(-SX/2, -SY/2, 0.f);
					Sprite background("Art Assets/Wallpaper.jpg");
					background.filter = FILTER_LINEAR;
					background.draw();
				}
				gxPopMatrix();

				gxTranslatef(SX/2, SY/2, 0);
				gxScalef(zoom, zoom, 1);
				gxTranslatef(-midX, -midY, 0.f);

				const EarthState earth_state = getEarthState(earth_happiness);
				static EarthState oldEarthState = earth_state;

				const Color earth_target_color = getEarthColor(earth_state);
				static Color earth_color = earth_target_color;

				if (earth_state != oldEarthState)
				{
					oldEarthState = earth_state;

					if (earth_state == kEarthState_Frozen)
						playSound("earth_transition_frozen");
					else if (earth_state == kEarthState_Cold)
						playSound("earth_transition_cold");
					else if (earth_state == kEarthState_Warm)
						playSound("earth_transition_warm");
					else if (earth_state == kEarthState_Hot)
						playSound("earth_transition_hot");
				}
				Sprite earth("Art Assets/planete.png");
				earth_color = earth_color.interp(earth_target_color, 0.004f);
				setColor(earth_color);
				if(!earth_dead)
				{
					earth.drawEx(-earth.getWidth() / 2, -earth.getHeight() / 2, 0.0, 1.0, 1.0, true, FILTER_LINEAR);
					setColor(earth_color);
					HEART_SPRITE.draw(heartState);
					heart_faces.draw(earth_color);
				}
				else
				{
					setColor(earth_color);
					EXPLOSION_SPRITE.draw(explosion);
				}

				/*
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
				*/

				if (earth_dead)
				{
					for (int i = 0; i < MAX_LEMMINGS; ++i)
					{
						if (lemmings[i].isActive)
						{
							lemmings[i].distance += 100.f * dt;
							lemmings[i].scale = Calc::Max(0.f, lemmings[i].scale - dt / 3.f);
						}
					}
					player.scale = Calc::Max(0.f, player.scale - dt / 3.f);
					player2.scale = Calc::Max(0.f, player2.scale - dt / 3.f);
				}

				//if (!earth_dead)
				{
					for (int i = 0; i < MAX_LEMMINGS; ++i)
						if (lemmings[i].isActive)
							lemmings[i].draw();

					player.draw();
					player2.draw();
				}
				sun.draw();

				gxPopMatrix();
			}

			if (frameStart > 0)
			{
				setColor(colorBlack);
				drawRect(0, 0, SX, SY);
			}

		#ifdef DEBUG
			if (DEBUG_DRAW)
			{
				setFont("calibri.ttf");

				int y = 10;

				setColor(colorWhite);
				drawText(10, y += 30, 24, +1, +1, "Debug Draw!");

				for (int i = 0; i < 12; ++i)
				{
					setColorf(1.f, 1.f, 1.f, .25f + 4.f * mas.volume[i]);
					drawText(10, y += 30, 24, +1, +1, mas.source[i].FileName_get());
				}

				setColor(colorWhite);
				drawText(10, y += 30, 24, +1, +1, "earth happiness %f", earth_happiness);
				drawText(10, y += 30, 24, +1, +1, "sun happiness %f", sun.happiness);

				drawText(10, y += 30, 24, +1, +1, "sun vec1 %f", sunVec1);
				drawText(10, y += 30, 24, +1, +1, "sun vec2 %f / %f", sunVec2, sunRate);
				drawText(10, y += 30, 24, +1, +1, "sun choir %f", choirValue);

				if (getSunDead(sun.happiness))
					drawText(10, y += 60, 48, +1, +1, "SUN DEAD");
				if (getEarthDead(earth_happiness))
					drawText(10, y += 60, 48, +1, +1, "EARTH DEAD");
			}
		#endif

			if (doOptions)
				optionsMenu.Draw(100, 100, 400, 600);

			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
