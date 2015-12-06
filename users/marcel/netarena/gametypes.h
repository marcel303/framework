#pragma once

#include <stdint.h>
#include <string>
#include <string.h>
#include "config.h"
#include "Debugging.h"
#include "gamedefs.h"
#include "Vec2.h"

#pragma pack(push)
#pragma pack(1)

enum NetAction
{
	kPlayerInputAction_CycleGameMode,
	kNetAction_PlayerInputAction,
	kNetAction_TextChat
};

enum NetPlayerAction
{
	kPlayerInputAction_PrevChar,
	kPlayerInputAction_NextChar,
	kPlayerInputAction_ReadyUp
};

enum GameState
{
	kGameState_Initial,
	kGameState_Connecting,
	kGameState_OnlineMenus,
	kGameState_NewGame,
	kGameState_RoundBegin,
	kGameState_Play,
	kGameState_RoundComplete,
	kGameState_COUNT
};

extern const char * g_gameStateNames[kGameState_COUNT];

enum GameMode
{
	kGameMode_Lobby,
	kGameMode_FootBrawl,
	kGameMode_DeathMatch,
	kGameMode_TokenHunt,
	kGameMode_CoinCollector,
	kGameMode_COUNT
};

enum ObjectType
{
	kObjectType_Undefined,
	kObjectType_Axe,
	kObjectType_FootBall,
	kObjectType_Bullet,
	kObjectType_Coin,
	kObjectType_Pickup,
	kObjectType_PipeBomb,
	kObjectType_Player,
	kObjectType_Token,
	kObjectType_COUNT
};

enum DrawLayer
{
	kDrawLayer_Game,
	kDrawLayer_PlayerHUD,
	kDrawLayer_HUD,
};

enum TransitionType
{
	kTransitionType_None,
	kTransitionType_SlideFromLeft,
	kTransitionType_SlideFromRight,
	kTransitionType_SlideFromTop,
	kTransitionType_SlideFromBottom
};

Vec2 getTransitionOffset(TransitionType type, int sxPixels, int syPixels, float transitionAmount);

struct TransitionInfo
{
	TransitionInfo()
	{
		memset(this, 0, sizeof(TransitionInfo));
	}

	TransitionType m_type;
	int m_sxPixels;
	int m_syPixels;
	float m_time;
	float m_curve;

	void setup(TransitionType type, int sxPixels, int syPixels, float time, float curve);
	void parse(const class Dictionary & d, int sxPixels, int syPixels);
	bool isActiveAtTime(float time) const { return time >= 0.f && time <= m_time; }
	Vec2 eval(float transitionProgress) const;
};

extern const char * g_gameModeNames[kGameMode_COUNT];

enum PlayerAnim
{
	kPlayerAnim_NULL,
	kPlayerAnim_Idle,
	kPlayerAnim_InAir,
	kPlayerAnim_EmoteCheer,
	kPlayerAnim_EmoteTaunt,
	kPlayerAnim_EmoteDance,
	kPlayerAnim_EmoteFacepalm,
	kPlayerAnim_Jump,
	kPlayerAnim_DoubleJump,
	kPlayerAnim_WallSlide,
	kPlayerAnim_Walk,
	kPlayerAnim_Attack,
	kPlayerAnim_AttackUp,
	kPlayerAnim_AttackDown,
	kPlayerAnim_Fire,
	kPlayerAnim_RocketPunch_Charge,
	kPlayerAnim_RocketPunch_Attack,
	kPlayerAnim_Zweihander_Charge,
	kPlayerAnim_Zweihander_Attack,
	kPlayerAnim_Zweihander_AttackDown,
	kPlayerAnim_Zweihander_Stunned,
	kPlayerAnim_Pipebomb_Deploy,
	kPlayerAnim_AirDash,
	kPlayerAnim_Spawn,
	kPlayerAnim_Die,
	kPlayerAnim_COUNT
};

enum PlayerWeapon
{
	kPlayerWeapon_None,
	kPlayerWeapon_Gun,
	kPlayerWeapon_Ice,
	kPlayerWeapon_Bubble,
	kPlayerWeapon_Grenade,
	kPlayerWeapon_TimeDilation,
	kPlayerWeapon_COUNT
};

// special ability that is bound to the Y button
enum PlayerSpecial
{
	kPlayerSpecial_None,
	kPlayerSpecial_RocketPunch,
	kPlayerSpecial_DoubleSidedMelee,
	kPlayerSpecial_DownAttack,
	kPlayerSpecial_Shield,
	kPlayerSpecial_Invisibility,
	kPlayerSpecial_Jetpack,
	kPlayerSpecial_Zweihander,
	kPlayerSpecial_AxeThrow,
	kPlayerSpecial_Pipebomb,
	kPlayerSpecial_Grapple,
	kPlayerSpecial_COUNT
};

extern const char * g_playerSpecialNames[kPlayerSpecial_COUNT];

enum PlayerTrait
{
	kPlayerTrait_StickyWalk = 1 << 0,
	kPlayerTrait_DoubleJump = 1 << 1,
	kPlayerTrait_AirDash = 1 << 2,
	kPlayerTrait_NinjaDash = 1 << 3
};

enum PlayerEvent
{
	kPlayerEvent_Spawn,
	kPlayerEvent_Respawn,
	kPlayerEvent_Die,
	kPlayerEvent_Jump,
	kPlayerEvent_WallJump,
	kPlayerEvent_LandOnGround,
	kPlayerEvent_StickyAttach,
	kPlayerEvent_StickyRelease,
	kPlayerEvent_StickyJump,
	kPlayerEvent_SpringJump,
	kPlayerEvent_SpikeHit,
	kPlayerEvent_ArenaWrap,
	kPlayerEvent_DashAir,
	kPlayerEvent_DestructibleDestroy
};

struct PlayerInput
{
	const static int AnalogRange = 10000;

	PlayerInput()
		: buttons(0)
		, analogX(0)
		, analogY(0)
	{
	}

	bool operator!=(const PlayerInput & other)
	{
		return
			buttons != other.buttons ||
			analogX != other.analogX ||
			analogY != other.analogY;
	}

	void gather(bool useKeyboard, int gamepadIndex, bool monkeyMode);

	Vec2 getAnalogDirection() const
	{
		return Vec2(analogX / float(AnalogRange), analogY / float(AnalogRange));
	}

	uint16_t buttons;
	int16_t analogX;
	int16_t analogY;
};

enum PickupType
{
	kPickupType_Gun,
	kPickupType_Nade,
	kPickupType_Shield,
	kPickupType_Ice,
	kPickupType_Bubble,
	kPickupType_TimeDilation,
	kPickupType_COUNT
};

bool parsePickupType(char c, PickupType & type);
bool parsePickupType(const char * s, PickupType & type);

enum BulletType
{
	kBulletType_Gun,
	kBulletType_Ice,
	kBulletType_Grenade,
	kBulletType_GrenadeA,
	kBulletType_ParticleA,
	kBulletType_Bubble,
	kBulletType_BloodParticle,
	kBulletType_COUNT
};

enum BulletEffect
{
	kBulletEffect_None,
	kBulletEffect_Damage,
	kBulletEffect_Ice,
	kBulletEffect_Bubble
};

enum CollisionType
{
	kCollisionType_Block = 1 << 0,
	kCollisionType_Player = 1 << 1,
	kCollisionType_PhysObj = 1 << 2
};

struct CollisionBox
{
	Vec2 min;
	Vec2 max;

	bool intersects(const CollisionBox & other) const
	{
		return
			max[0] >= other.min[0] &&
			max[1] >= other.min[1] &&
			min[0] <= other.max[0] &&
			min[1] <= other.max[1];
	}

	bool intersects(float x, float y) const
	{
		return
			max[0] >= x &&
			max[1] >= y &&
			min[0] <= x &&
			min[1] <= y;
	}
};

struct CollisionInfo : CollisionBox
{
	CollisionInfo getTranslated(Vec2 offset) const
	{
		CollisionInfo result;
		result.min = min + offset;
		result.max = max + offset;
		return result;
	}
};

struct CollisionShape
{
	enum Type
	{
		kType_Poly,
		kType_Circle
	};

	const static int kMaxPoints = 16;

	Type type;
	Vec2 points[kMaxPoints];
	int numPoints;
	float radius;

	//

	CollisionShape()
	{
		//memset(this, 0, sizeof(*this));
	}

	CollisionShape(const CollisionShape & shape, Vec2Arg offset)
	{
		type = shape.type;
		numPoints = shape.numPoints;

		switch (shape.type)
		{
		case kType_Poly:
			for (int i = 0; i < shape.numPoints; ++i)
				points[i] = shape.points[i] + offset;
			break;

		case kType_Circle:
			points[0] = shape.points[0] + offset;
			radius = shape.radius;
			break;
			
		default:
			Assert(false);
			break;
		}
	}

	CollisionShape(const CollisionBox & box)
	{
		//memset(this, 0, sizeof(*this));

		*this = box;
	}

	CollisionShape(Vec2Arg min, Vec2Arg max)
	{
		set(
			Vec2(min[0], min[1]),
			Vec2(max[0], min[1]),
			Vec2(max[0], max[1]),
			Vec2(min[0], max[1]));
	}

	void setEmpty()
	{
		numPoints = 0;
	}

	void set(Vec2 p1, Vec2 p2, Vec2 p3)
	{
		type = kType_Poly;

		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		numPoints = 3;

		fixup();
	}

	void set(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4)
	{
		type = kType_Poly;

		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		points[3] = p4;
		numPoints = 4;

		fixup();
	}

	void setCircle(Vec2Arg p, float radius)
	{
	#if 1
		type = kType_Poly;

		for (int i = 0; i < kMaxPoints; ++i)
		{
			const float angle = 2.f * M_PI * float(i) / float(kMaxPoints);
			points[i][0] = std::cosf(angle) * radius;
			points[i][1] = std::sinf(angle) * radius;
		}
		numPoints = kMaxPoints;

		fixup();
	#else
		type = kType_Circle;

		points[0][0] = p[0];
		points[0][1] = p[1];
		radius = radius;
	#endif
	}

	const CollisionShape & operator=(const CollisionBox & box);

	void fixup();
	void translate(float x, float y);

	void getMinMax(Vec2 & min, Vec2 & max) const;
	float projectedMax(Vec2Arg n) const;
	Vec2 getSegmentNormal(int idx) const;
	bool intersects(const CollisionShape & other) const;
	bool checkCollision(const CollisionShape & other, Vec2Arg delta, float & contactDistance, Vec2 & contactNormal) const;

	void debugDraw(bool drawNormals = true) const;
};

struct Curve
{
	float value[32];

	Curve();
	Curve(float min, float max);

	void makeLinear(float v1, float v2);
	float eval(float t) const;
};

template <int SIZE>
struct FixedString
{
	FixedString()
	{
		memset(m_data, 0, sizeof(m_data));
	}

	size_t length() const
	{
		return strlen(m_data);
	}

	const char * c_str() const
	{
		return m_data;
	}

	bool empty() const
	{
		return m_data[0] == 0;
	}

	void operator=(const char * str)
	{
		const size_t len = strlen(str);
		Assert(len <= SIZE);
		const size_t copySize = (len > SIZE) ? SIZE : len;
		for (size_t i = 0; i < copySize; ++i)
			m_data[i] = str[i];
		for (size_t i = copySize; i < SIZE + 1; ++i)
			m_data[i] = 0;
	}

	bool operator==(const char * str) const
	{
		return strcmp(m_data, str) == 0;
	}

	bool operator!=(const char * str) const
	{
		return strcmp(m_data, str) != 0;
	}

	bool operator<(const FixedString & other) const
	{
		return strcmp(m_data, other.m_data) < 0;
	}

	char m_data[SIZE + 1];
};

#pragma pack(pop)

struct UserSettings
{
	struct Audio
	{
		Audio()
			: musicEnabled(true)
			, musicVolume(1.f)
			, soundEnabled(true)
			, soundVolume(1.f)
			, announcerEnabled(true)
			, announcerVolume(1.f)
		{
		}

		bool musicEnabled;
		float musicVolume;

		bool soundEnabled;
		float soundVolume;

		bool announcerEnabled;
		float announcerVolume;
	} audio;

	struct Display
	{
		Display()
			: fullscreen(true)
			, exclusiveFullscreen(false)
			, exclusiveFullscreenVsync(true)
			, exclusiveFullscreenHz(0)
			, fullscreenSx(1920)
			, fullscreenSy(1080)
			, windowedSx(1920/2)
			, windowedSy(1080/2)
		{
		}

		bool fullscreen;
		bool exclusiveFullscreen;
		bool exclusiveFullscreenVsync;
		int exclusiveFullscreenHz;
		int fullscreenSx;
		int fullscreenSy;
		int windowedSx;
		int windowedSy;
	} display;

	struct Graphics
	{
		Graphics()
			: brightness(1.f)
		{
		}

		float brightness;
	} graphics;

	struct Language
	{
		Language()
		{
			locale = "en";
		}

		std::string locale;
	} language;

	struct Effects
	{
		Effects()
			: screenShakeStrength(1.f)
		{
		}

		float screenShakeStrength;
	} effects;

	struct Char
	{
		Char()
			: emblem(0)
			, skin(0)
		{
		}

		int emblem;
		int skin;
	} chars[MAX_CHARACTERS];

	void save(class StreamWriter & writer);
	void load(class StreamReader & reader);
};

extern Curve defaultCurve;
extern Curve pipebombBlastCurve;
extern Curve grenadeBlastCurve;
extern Curve jetpackAnalogCurveX;
extern Curve jetpackAnalogCurveY;
extern Curve gravityWellFalloffCurve;

#if ITCHIO_BUILD
static const int g_validCharacterIndices[] = { 1, 2, 4 };
#else
static const int g_validCharacterIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
#endif
