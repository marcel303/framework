#include "bullet.h"
#include "Calc.h"
#include "client.h"
#include "Debugging.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "host.h"
#include "main.h"
#include "Parse.h"
#include "Path.h"
#include "player.h"
#include "StreamReader.h"
#include "Timer.h"

#include "BinaryDiff.h" // fixme : remove

OPTION_DECLARE(bool, g_noSound, false);
OPTION_DEFINE(bool, g_noSound, "Sound/Disable Sound Effects");
OPTION_ALIAS(g_noSound, "nosound");

OPTION_DECLARE(int, g_gameModeNextRound, -1);
OPTION_DEFINE(int, g_gameModeNextRound, "Game State/Game Mode Next Round");
OPTION_STEP(g_gameModeNextRound, -1, kGameMode_COUNT - 1, 1);
OPTION_ALIAS(g_gameModeNextRound, "gamemode");
OPTION_VALUE_ALIAS(g_gameModeNextRound, deathmatch, kGameMode_DeathMatch);
OPTION_VALUE_ALIAS(g_gameModeNextRound, tokenhunt, kGameMode_TokenHunt);
OPTION_VALUE_ALIAS(g_gameModeNextRound, coincollector, kGameMode_CoinCollector);

OPTION_EXTERN(std::string, g_map);

extern std::vector<std::string> g_mapList;

//

void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

GameSim * g_gameSim = 0;

//

static const char * s_pickupSprites[kPickupType_COUNT] =
{
	"pickup-ammo.png",
	"pickup-nade.png",
	"pickup-shield.png",
	"pickup-ice.png",
	"pickup-bubble.png"
};

#define TOKEN_SPRITE "token.png"
#define COIN_SPRITE "coin.png"

//

uint32_t GameStateData::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	m_randomSeed = ~m_randomSeed;
	m_randomSeed -= 1;
	return m_randomSeed;
}

uint32_t GameStateData::GetTick() const
{
	return m_tick;
}

float GameStateData::getRoundTime() const
{
	return m_roundTime;
}

void GameStateData::addTimeDilationEffect(float multiplier1, float multiplier2, float duration)
{
	int minIndex = 0;

	for (int i = 1; i < MAX_TIMEDILATION_EFFECTS; ++i)
	{
		if (m_timeDilationEffects[i].ticksRemaining < m_timeDilationEffects[minIndex].ticksRemaining)
			minIndex = i;
	}

	m_timeDilationEffects[minIndex].multiplier1 = multiplier1;
	m_timeDilationEffects[minIndex].multiplier2 = multiplier2;
	m_timeDilationEffects[minIndex].ticks = duration * TICKS_PER_SECOND;
	m_timeDilationEffects[minIndex].ticksRemaining = m_timeDilationEffects[minIndex].ticks;
}

LevelEvent GameStateData::getRandomLevelEvent()
{
	return (LevelEvent)(Random() % kLevelEvent_COUNT);
}

//

void Pickup::setup(PickupType _type, int _blockX, int _blockY)
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	type = _type;
	blockX = _blockX;
	blockY = _blockY;

	//

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_pos.Set(
		blockX * BLOCK_SX + BLOCK_SX / 2.f,
		blockY * BLOCK_SY + BLOCK_SY - sprite.getHeight());
	m_vel.Set(0.f, 0.f);

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight());
	m_bbMax.Set(+sprite.getWidth() / 2.f, 0.f);

	m_doTeleport = true;
	m_bounciness = .25f;
	m_friction = .1f;
	m_airFriction = .5f;
}

void Pickup::tick(GameSim & gameSim, float dt)
{
	PhysicsActorCBs cbs;
	PhysicsActor::tick(gameSim, dt, cbs);
}

void Pickup::draw() const
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	sprite.drawEx(m_pos[0] + m_bbMin[0], m_pos[1] + m_bbMin[1]);

	if (true)
	{
		// fixme!

		setFont("calibri.ttf");
		drawText(m_pos[0], m_pos[1], 24.f, 0.f, 0.f, "type: %d", type);
	}
}

void Pickup::drawLight() const
{
	const Vec2 pos = m_pos + (m_bbMin + m_bbMax) / 2.f;

	Sprite("player-light.png").drawEx(pos[0], pos[1], 0.f, 1.f, 1.f, false, FILTER_LINEAR);
}

//

void Token::setup(int blockX, int blockY)
{
	Sprite sprite(TOKEN_SPRITE);

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f);
	m_bbMax.Set(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f);
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = -TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Token::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-TOKEN_FLEE_SPEED, +TOKEN_FLEE_SPEED), -TOKEN_FLEE_SPEED);
					g_gameSim->playSound("token-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[1]) >= TOKEN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("token-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Token::draw() const
{
	if (m_isDropped)
	{
		Sprite(TOKEN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}
}

void Token::drawLight() const
{
	if (m_isDropped)
	{
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_LINEAR);
	}
}

//

void Coin::setup(int blockX, int blockY)
{
	Sprite sprite(TOKEN_SPRITE); // fixme : COIN_SPRITE

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f);
	m_bbMax.Set(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f);
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = -TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Coin::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-COIN_FLEE_SPEED, +COIN_FLEE_SPEED), -COIN_FLEE_SPEED);
					g_gameSim->playSound("token-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[1]) >= COIN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("token-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Coin::draw() const
{
	if (m_isDropped)
	{
		Sprite(COIN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}
}

void Coin::drawLight() const
{
	if (m_isDropped)
	{
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_LINEAR);
	}
}

//

void Mover::setSprite(const char * filename)
{
	strcpy_s(m_sprite, sizeof(m_sprite), filename);

	Sprite sprite(m_sprite);
	m_sx = sprite.getWidth();
	m_sy = sprite.getHeight();
}

void Mover::tick(GameSim & gameSim, float dt)
{
	if (m_moveMultiplier == 0.f)
	{
		const int dx = m_x2 - m_x1;
		const int dy = m_y2 - m_y1;

		m_moveMultiplier = m_speed / std::sqrtf(dx * dx + dy * dy) / 2.f;
	}

	m_moveAmount = std::fmodf(m_moveAmount + m_moveMultiplier * dt, 1.f);

	CollisionInfo collision;
	getCollisionInfo(collision);

	const Vec2 pos = getPosition();
	const Vec2 speed = getSpeed();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = gameSim.m_players[i];

		if (player.m_isUsed && player.m_isAlive)
		{
			bool grounded = false;

			CollisionInfo playerCollision;

			if (player.m_isGrounded && player.m_vel[1] >= 0.f)
			{
				player.getPlayerCollision(playerCollision);
				playerCollision.y1 += 2.f;
				playerCollision.y2 += 2.f;

				if (playerCollision.intersects(collision))
				{
					player.m_pos += speed * dt;

					grounded = true;
				}
			}

			player.getPlayerCollision(playerCollision);

			if (playerCollision.intersects(collision))
			{
			#if 0
				const float d[4] =
				{
					+ playerCollision.x2 - collision.x1,
					- playerCollision.x1 + collision.x2,
					+ playerCollision.y2 - collision.y1,
					- playerCollision.y1 + collision.y2
				};

				int lowest = 0;

				for (int j = 1; j < 4; ++j)
				{
					if (d[j] >= 0.f && d[j] < d[lowest])
						lowest = j;
				}

				if (lowest == 0)
					player.m_pos[0] -= d[0] + 1.f;
				if (lowest == 1)
					player.m_pos[0] += d[1] + 1.f;
				if (lowest == 2)
					player.m_pos[1] -= d[2] + 1.f;
				if (lowest == 3)
					player.m_pos[1] += d[3] + 1.f;
			#else
				const float d = + playerCollision.y2 - collision.y1;

				if (d >= 0.f && grounded)
				{
					player.m_pos[1] -= d + 1.f;
				}
			#endif
			}

			if (grounded)
			{
				for (int offset = 1; offset < 4; ++offset)
				{
					if (player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1] + offset) & kBlockMask_Solid)
					{
						if (offset != 0)
						{
							player.m_pos[1] += offset - 1.f;
							Assert((player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1] + 1.f) & kBlockMask_Solid) != 0);
						}
						break;
					}
				}
			}

			//Assert((player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1]) & kBlockMask_Solid) == 0);
		}
	}
}

void Mover::draw() const
{
	CollisionInfo collision;
	getCollisionInfo(collision);

	setColor(colorWhite);
	drawRect(collision.x1, collision.y1, collision.x2, collision.y2);
}

void Mover::drawLight() const
{
}

Vec2 Mover::getPosition() const
{
	const float t = m_moveAmount < .5f ? m_moveAmount * 2.f : (1.f - m_moveAmount) * 2.f;
	const float x = Calc::Lerp(m_x1, m_x2, t);
	const float y = Calc::Lerp(m_y1, m_y2, t);

	return Vec2(x, y);
}

Vec2 Mover::getSpeed() const
{
	const float t = m_moveAmount < .5f ? +2.f : -2.f;
	const float dx = m_x2 - m_x1;
	const float dy = m_y2 - m_y1;

	return Vec2(
		dx * t * m_moveMultiplier,
		dy * t * m_moveMultiplier);
}

void Mover::getCollisionInfo(CollisionInfo & collisionInfo) const
{
	const Vec2 pos = getPosition();

	collisionInfo.x1 = pos[0] - m_sx / 2.f;
	collisionInfo.y1 = pos[1] - m_sy / 2.f;
	collisionInfo.x2 = pos[0] + m_sx / 2.f;
	collisionInfo.y2 = pos[1] + m_sy / 2.f;
}

bool Mover::intersects(CollisionInfo & collisionInfo) const
{
	CollisionInfo myCollision;
	getCollisionInfo(myCollision);

	return myCollision.intersects(collisionInfo);
}

//

void Torch::setup(float x, float y, const Color & color)
{
	m_isAlive = true;
	m_pos.Set(x, y);
	m_color[0] = color.r;
	m_color[1] = color.g;
	m_color[2] = color.b;
	m_color[3] = color.a;
}

void Torch::tick(GameSim & gameSim, float dt)
{
}

void Torch::draw() const
{
	Sprite("torch.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.f);
}

void Torch::drawLight() const
{
	float a = 0.f;
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * TORCH_FLICKER_FREQ_A);
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * TORCH_FLICKER_FREQ_B);
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * TORCH_FLICKER_FREQ_C);
	a = (a + 3.f) / 6.f;
	a = Calc::Lerp(1.f, a, TORCH_FLICKER_STRENGTH);

	Color color;
	color.r = m_color[0];
	color.g = m_color[1];
	color.b = m_color[2];
	color.a = a;
	setColor(color);

	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1] + TORCH_FLICKER_Y_OFFSET, 0.f, 1.5f, 1.5f, false, FILTER_LINEAR);

	setColor(colorWhite);
}

//

void ScreenShake::tick(float dt)
{
	Vec2 force = pos * (-stiffness);
	vel += force * dt;
	pos += vel * dt;

	life -= dt;

	if (life <= 0.f)
	{
		*this = ScreenShake();
	}
}

//

void FloorEffect::tick(GameSim & gameSim, float dt)
{
	for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
	{
		if (m_tiles[i].time)
		{
			if (m_tiles[i].damageSize > 0)
			{
				CollisionInfo collisionInfo;
				collisionInfo.x1 = m_tiles[i].x - 4;
				collisionInfo.x2 = m_tiles[i].x + 4;
				collisionInfo.y1 = m_tiles[i].y - 16;
				collisionInfo.y2 = m_tiles[i].y;

				for (int p = 0; p < MAX_PLAYERS; ++p)
				{
					if (p != m_tiles[i].playerId)
					{
						Player & player = gameSim.m_players[p];

						if (!player.m_isUsed)
							continue;
						if (!player.m_isAlive)
							continue;

						CollisionInfo playerCollision;
						if (player.getPlayerCollision(playerCollision))
						{
							if (collisionInfo.intersects(playerCollision))
							{
								player.handleDamage(1.f, Vec2(m_tiles[i].dx, -1.f), &gameSim.m_players[m_tiles[i].playerId]);
							}
						}
					}
				}
			}

			m_tiles[i].time--;

			if (m_tiles[i].time == 0 && m_tiles[i].size != 0)
			{
				const int playerId = m_tiles[i].playerId;
				const int x = m_tiles[i].x + m_tiles[i].dx;
				const int y = m_tiles[i].y;
				const int dx = m_tiles[i].dx;
				const int size = m_tiles[i].size - 1;
				const int damageSize = m_tiles[i].damageSize > 0 ? m_tiles[i].damageSize - 1 : 0;
				memset(&m_tiles[i], 0, sizeof(m_tiles[i]));

				trySpawnAt(gameSim, playerId, x, y, dx, size, damageSize);
			}
		}
	}
}

void FloorEffect::trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize)
{
	x = (x + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;

	const Arena & arena = gameSim.m_arena;

	// try to move up

	bool foundEmpty = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y - dy);

		if (!(blockMask & kBlockMask_Solid))
		{
			y = y - dy;
			foundEmpty = true;
			break;
		}
	}

	if (!foundEmpty)
		return;

	// try to move down

	bool hitGround = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y + dy + 1);

		if (blockMask & kBlockMask_Solid)
		{
			y = y + dy;
			hitGround = true;
			break;
		}
	}

	if (hitGround)
	{
		for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
		{
			if (m_tiles[i].time == 0)
			{
				m_tiles[i].playerId = playerId;
				m_tiles[i].x = x;
				m_tiles[i].y = y;
				m_tiles[i].dx = dx;
				m_tiles[i].size = size;
				m_tiles[i].damageSize = damageSize;
				m_tiles[i].time = TICKS_PER_SECOND / 12; // fixme : gamedef

				ParticleSpawnInfo spawnInfo(
					x,
					y,
					kBulletType_ParticleA, 10,
					50.f, 100.f, 50.f);
				spawnInfo.color = damageSize > 0 ? 0xff8000a0 : 0x0000ffa0;
				g_gameSim->spawnParticles(spawnInfo);

				gameSim.playSound("grenade-frag.ogg");

				break;
			}
		}
	}
}

//

GameSim::GameSim()
	: GameStateData()
	, m_bulletPool(0)
{
	m_arena.init();

	for (int i = 0; i < MAX_PLAYERS; ++i)
		m_playerInstanceDatas[i] = 0;

	m_bulletPool = new BulletPool(false);

	m_particlePool = new BulletPool(true);
}

GameSim::~GameSim()
{
	delete m_particlePool;
	m_particlePool = 0;

	delete m_bulletPool;
	m_bulletPool = 0;
}

#if ENABLE_GAMESTATE_DESYNC_DETECTION
uint32_t GameSim::calcCRC() const
{
	clearPlayerPtrs();

	uint32_t result = 0;

	const uint8_t * bytes = (const uint8_t*)static_cast<const GameStateData*>(this);
	const uint32_t numBytes = sizeof(GameStateData);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	result = result * 13 + m_arena.calcCRC();

	result = result * 13 + m_bulletPool->calcCRC();

	// todo : add screen shakes, particle pool, bullet pool

	setPlayerPtrs();

	return result;
}
#endif

void GameSim::serialize(NetSerializationContext & context)
{
#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);
#endif

	clearPlayerPtrs();
	{
		GameStateData * data = static_cast<GameStateData*>(this);

		context.SerializeBytes(data, sizeof(GameStateData));

		//unsigned char temp[sizeof(GameStateData)];
		//memset(temp, 0, sizeof(temp));
		//BinaryDiffResult diff = BinaryDiff(data, temp, sizeof(GameStateData), 4);
	}
	setPlayerPtrs();

	// todo : serialize player animation state

	m_arena.serialize(context);

	m_bulletPool->serialize(context);

	// todo : serialize player animation state, since it affects game play

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	if (context.IsRecv())
		Assert(crc == calcCRC());
#endif
}

void GameSim::clearPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerInstanceDatas[i])
			m_playerInstanceDatas[i]->m_player->m_instanceData = 0;
}

void GameSim::setPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerInstanceDatas[i])
			m_playerInstanceDatas[i]->m_player->m_instanceData = m_playerInstanceDatas[i];
}

void GameSim::setGameState(::GameState gameState)
{
	m_gameState = gameState;

	switch (gameState)
	{
	case kGameState_MainMenus:
		resetGameSim();
		break;

	case kGameState_Connecting:
		break;

	case kGameState_OnlineMenus:
		resetGameSim();
		break;

	case kGameState_NewGame:
		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerInstanceDatas[i])
			{
				Player * player = m_playerInstanceDatas[i]->m_player;

				player->handleNewGame();
			}
		}
		break;

	case kGameState_Play:
		{
			playSound("round-begin.ogg");

			// respawn players

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (m_playerInstanceDatas[i])
				{
					Player * player = m_playerInstanceDatas[i]->m_player;

					player->handleNewRound();

					player->respawn();
				}
			}

			// level events

			m_timeUntilNextLevelEvent = 1.f;

			// game modes

			if (m_gameMode == kGameMode_TokenHunt)
			{
				spawnToken();
			}

			if (m_gameMode == kGameMode_CoinCollector)
			{
				for (int i = 0; i < COINCOLLECTOR_INITIAL_COIN_DROP; ++i)
					spawnCoin();
			}
		}
		break;

	case kGameState_RoundComplete:
		break;

	default:
		Assert(false);
		break;
	}
}

void GameSim::setGameMode(GameMode gameMode)
{
	m_gameMode = gameMode;
}

static Color parseColor(const char * str)
{
	const uint32_t hex = std::stoul(str, 0, 16);
	const float r = ((hex >> 24) & 0xff) / 255.f;
	const float g = ((hex >> 16) & 0xff) / 255.f;
	const float b = ((hex >>  8) & 0xff) / 255.f;
	const float a = ((hex >>  0) & 0xff) / 255.f;
	return Color(r, g, b, a);
}

void GameSim::newGame()
{
	setGameState(kGameState_NewGame);

	newRound(0);

	framework.blinkTaskbarIcon(3); // todo : option
}

void GameSim::newRound(const char * mapOverride)
{
	// load arena

	std::string map;

	if (mapOverride)
		map = mapOverride;
	else if (!((std::string)g_map).empty())
		map = g_map;
	else if (!g_mapList.empty())
		map = g_mapList[m_nextRoundNumber % g_mapList.size()];
	else
		map = "arena.txt";

	load(map.c_str());

	// set game mode

	if (g_gameModeNextRound >= 0 && g_gameModeNextRound < kGameMode_COUNT)
	{
		setGameMode((GameMode)(int)g_gameModeNextRound);
	}

	// and start playing!

	setGameState(kGameState_Play);

	m_nextRoundNumber++;
}

void GameSim::endRound()
{
	setGameState(kGameState_RoundComplete);

	m_roundCompleteTicks = TICKS_PER_SECOND * GAMESTATE_COMPLETE_TIMER;
	m_roundCompleteTimeDilationTicks = TICKS_PER_SECOND * GAMESTATE_COMPLETE_TIME_DILATION_TIMER;
}

void GameSim::load(const char * filename)
{
	resetGameWorld();

	// load arena

	m_arena.load(filename);

	// load objects

	// fixme
#if 0
	{ Mover & mover = m_movers[0]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2-200, GFX_SX*3/4, GFX_SY/2-300, 100); }
	{ Mover & mover = m_movers[1]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2,     GFX_SX*3/4, GFX_SY/2-100, 111); }
	{ Mover & mover = m_movers[2]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2+200, GFX_SX*3/4, GFX_SY/2+100, 121); }
#endif

	std::string baseName = Path::GetBaseName(filename);

	std::string objectsFilename = baseName + "-objects.txt";

	try
	{
		FileStream stream;
		stream.Open(objectsFilename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();

		enum ObjectType
		{
			kObjectType_Undefined,
			kObjectType_Mover,
			kObjectType_Torch,
		};

		ObjectType objectType = kObjectType_Undefined;
		Mover * mover = 0;
		Torch * torch = 0;

		for (size_t i = 0; i < lines.size(); ++i)
		{
			if (lines[i].empty() || lines[i][0] == '#')
				continue;

			std::vector<std::string> fields;
			splitString(lines[i], fields, ':');

			if (fields.size() != 2)
			{
				LOG_WRN("syntax error: %s", lines[i].c_str());
			}
			else
			{
				if (fields[0] == "object")
				{
					objectType = kObjectType_Undefined;
					mover = 0;
					torch = 0;

					if (fields[1] == "mover")
					{
						objectType = kObjectType_Mover;

						for (int i = 0; i < MAX_TORCHES; ++i)
						{
							if (!m_movers[i].m_isActive)
							{
								mover = &m_movers[i];
								break;
							}
						}

						if (mover == 0)
							LOG_ERR("too many movers!");
						else
							mover->m_isActive = true;
					}
					else if (fields[1] == "torch")
					{
						objectType = kObjectType_Torch;

						for (int i = 0; i < MAX_TORCHES; ++i)
						{
							if (!m_torches[i].m_isAlive)
							{
								torch = &m_torches[i];
								break;
							}
						}

						if (torch == 0)
							LOG_ERR("too many torches!");
						else
							torch->m_isAlive = true;
					}
				}
				else
				{
					switch (objectType)
					{
					case kObjectType_Undefined:
						LOG_ERR("properties begin before object type is set!");
						break;

					case kObjectType_Mover:
						if (mover)
						{
							if (fields[0] == "sprite")
								mover->setSprite(fields[1].c_str());
							if (fields[0] == "x1")
								mover->m_x1 = Parse::Int32(fields[1]);
							if (fields[0] == "y1")
								mover->m_y1 = Parse::Int32(fields[1]);
							if (fields[0] == "x2")
								mover->m_x2 = Parse::Int32(fields[1]);
							if (fields[0] == "y2")
								mover->m_y2 = Parse::Int32(fields[1]);
							if (fields[0] == "speed")
								mover->m_speed = Parse::Int32(fields[1]);
						}
						break;

					case kObjectType_Torch:
						if (torch)
						{
							if (fields[0] == "x")
								torch->m_pos[0] = Parse::Int32(fields[1]);
							if (fields[0] == "y")
								torch->m_pos[1] = Parse::Int32(fields[1]);
							if (fields[0] == "color")
							{
								if (fields[1].length() != 8)
									LOG_ERR("invalid color format: %s", fields[1].c_str());
								else
								{
									const Color color = parseColor(fields[1].c_str());
									torch->m_color[0] = color.r;
									torch->m_color[1] = color.g;
									torch->m_color[2] = color.b;
									torch->m_color[3] = color.a;
								}
							}
						}
						break;

					default:
						Assert(false);
						break;
					}
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR(e.what());
	}
}

void GameSim::resetGameWorld()
{
	// reset round stuff

	m_roundTime = 0.f;

	// reset map

	m_arena.reset();

	// reset pickups

	for (int i = 0; i < MAX_PICKUPS; ++i)
		m_pickups[i] = Pickup();

	m_nextPickupSpawnTick = 0;

	// reset movers

	for (int i = 0; i < MAX_MOVERS; ++i)
		m_movers[i] = Mover();

	// reset floor effect

	m_floorEffect = FloorEffect();

	// reset torches

	for (int i = 0; i < MAX_TORCHES; ++i)
		m_torches[i] = Torch();

	// reset bullets

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (m_bulletPool->m_bullets[i].isAlive)
			m_bulletPool->free(i);

		if (m_particlePool->m_bullets[i].isAlive)
			m_particlePool->free(i);
	}

	// reset screen shakes

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
		m_screenShakes[i] = ScreenShake();

	// reset level events

	m_levelEvents = LevelEvents();
	m_timeUntilNextLevelEvent = 0.f;

	// reset token hunt game mode

	m_tokenHunt = TokenHunt();

	// reset coin collector game mode

	m_coinCollector = CoinCollector();
}

void GameSim::resetPlayers()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i] = Player();
	}
}

void GameSim::resetGameSim()
{
	resetGameWorld();

	resetPlayers();
}

void GameSim::tick()
{
	Assert(!g_gameSim);
	g_gameSim = this;

#if ENABLE_GAMESTATE_CRC_LOGGING
	const uint32_t oldCRC = g_logCRCs ? calcCRC() : 0;
#endif

	switch (m_gameState)
	{
	case kGameState_MainMenus:
		Assert(false);
		break;

	case kGameState_OnlineMenus:
		tickMenus();
		break;

	case kGameState_Play:
		tickPlay();
		break;

	case kGameState_RoundComplete:
		tickPlay();
		tickRoundComplete();
		break;

	default:
		Assert(false);
		break;
	}

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerInstanceDatas[i])
		{
			m_playerInstanceDatas[i]->m_player->m_input.next();
		}
	}

#if ENABLE_GAMESTATE_CRC_LOGGING
	if (g_logCRCs)
	{
		int numPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (m_playerInstanceDatas[i])
				numPlayers++;

		const uint32_t newCRC = calcCRC();

		LOG_DBG("gamesim %p: tick=%u, oldCRC=%08x, newCRC=%08x, numPlayers=%d [%s]", this, m_tick, oldCRC, newCRC, numPlayers, g_host && &g_host->m_gameSim == this ?  "server" : "client");
		/*
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerInstanceDatas[i])
			{
				LOG_DBG("\tplayer %d: (%04x,%02d,%02d) - (%04x,%02d,%02d)", i,
					(int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.buttons, (int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.analogX, (int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.analogY,
					(int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.buttons, (int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.analogX, (int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.analogY);
			}
		}
		*/
	}
#endif

	g_gameSim = 0;
}

void GameSim::tickMenus()
{
	const int MAX_CHARACTERS = 4;

	// wait for the host to enter the next game state

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_players[i];

		if (player.m_isUsed)
		{
			PlayerInstanceData * playerInstanceData = m_playerInstanceDatas[i];
			Assert(playerInstanceData);
			if (playerInstanceData)
			{
				// character select

				if (!player.m_isReadyUpped)
				{
					int step = 0;

					if (player.m_input.wentDown(INPUT_BUTTON_LEFT) || (player.m_input.m_actions & (1 << kPlayerInputAction_PrevChar)))
						step = -1;
					if (player.m_input.wentDown(INPUT_BUTTON_RIGHT) || (player.m_input.m_actions & (1 << kPlayerInputAction_NextChar)))
						step += 1;

					if (step != 0)
					{
						const int characterIndex = (player.m_characterIndex + MAX_CHARACTERS + step) % MAX_CHARACTERS;
						playerInstanceData->setCharacterIndex(characterIndex);
					}
				}

				// ready up

				if (player.m_input.wentDown(INPUT_BUTTON_A) || (player.m_input.m_actions & (1 << kPlayerInputAction_ReadyUp)))
				{
					player.m_isReadyUpped = !player.m_isReadyUpped;
				}
			}
		}
	}

	bool allReady = true;

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i].m_isUsed)
			allReady &= m_players[i].m_isReadyUpped;

	if (allReady)
	{
		if (m_gameStartTicks == 0)
		{
			m_gameStartTicks = TICKS_PER_SECOND * (g_devMode ? 1 : 4);
		}
		else
		{
			m_gameStartTicks--;

			if (m_gameStartTicks == 0)
			{
				newGame();
			}
		}
	}
	else
	{
		m_gameStartTicks = 0;
	}
}

void GameSim::tickPlay()
{
	float timeDilation = 1.f;

	if (m_gameState == kGameState_Play)
	{
		for (int i = 0; i < MAX_TIMEDILATION_EFFECTS; ++i)
		{
			if (m_timeDilationEffects[i].ticksRemaining != 0)
			{
				const float t = 1.f - (m_timeDilationEffects[i].ticksRemaining / float(m_timeDilationEffects[i].ticks));
				const float multiplier = Calc::Lerp(m_timeDilationEffects[i].multiplier1, m_timeDilationEffects[i].multiplier2, t);
				if (multiplier < timeDilation)
					timeDilation = multiplier;
				m_timeDilationEffects[i].ticksRemaining--;
			}
		}
	}
	else if (m_gameState == kGameState_RoundComplete)
	{
		const float t = 1.f - m_roundCompleteTimeDilationTicks / float(TICKS_PER_SECOND * GAMESTATE_COMPLETE_TIME_DILATION_TIMER);
		timeDilation = Calc::Lerp(GAMESTATE_COMPLETE_TIME_DILATION_BEGIN, GAMESTATE_COMPLETE_TIME_DILATION_END, t);
	}

	const float dt =
		(1.f / TICKS_PER_SECOND) *
		GAME_SPEED_MULTIPLIER *
		timeDilation;

	const uint32_t tick = GetTick();

	// arena update
	m_arena.tick(*this);

	// player update

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerInstanceDatas[i])
			m_playerInstanceDatas[i]->m_player->tick(dt);
	}

	// pickups

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		if (m_pickups[i].isAlive)
			m_pickups[i].tick(*this, dt);
	}

	// movers

	for (int i = 0; i < MAX_MOVERS; ++i)
	{
		if (m_movers[i].m_isActive)
			m_movers[i].tick(*this, dt);
	}

	// pickup spawning

	if (tick >= m_nextPickupSpawnTick)
	{
		int numPickups = 0;
		for (int i = 0; i < MAX_PICKUPS; ++i)
			if (m_pickups[i].isAlive)
				numPickups++;

		if (numPickups < MAX_PICKUP_COUNT)
		{
			int weights[kPickupType_COUNT] =
			{
				PICKUP_AMMO_WEIGHT,
				PICKUP_NADE_WEIGHT,
				PICKUP_SHIELD_WEIGHT,
				PICKUP_ICE_WEIGHT,
				PICKUP_BUBBLE_WEIGHT
			};

			int totalWeight = 0;

			for (int i = 0; i < kPickupType_COUNT; ++i)
			{
				totalWeight += weights[i];
				weights[i] = totalWeight;
			}

			if (DEBUG_RANDOM_CALLSITES)
				LOG_DBG("Random called from pre trySpawnPickup");
			int value = Random() % totalWeight;

			PickupType type = kPickupType_COUNT;

			for (int i = 0; type == kPickupType_COUNT; ++i)
				if (value < weights[i])
					type = (PickupType)i;

			trySpawnPickup(type);
		}

		m_nextPickupSpawnTick = tick + (PICKUP_INTERVAL + (Random() % PICKUP_INTERVAL_VARIANCE)) * TICKS_PER_SECOND;
	}

	// level events

	if (m_timeUntilNextLevelEvent > 0.f && PROTO_ENABLE_LEVEL_EVENTS)
	{
		m_timeUntilNextLevelEvent -= dt;

		if (m_timeUntilNextLevelEvent <= 0.f)
		{
			m_timeUntilNextLevelEvent = 30.f;

			//const LevelEvent e = getRandomLevelEvent();
			const LevelEvent e = kLevelEvent_GravityWell;
			//const LevelEvent e = kLevelEvent_EarthQuake;

			switch (e)
			{
			case kLevelEvent_EarthQuake:
				memset(&m_levelEvents.quake, 0, sizeof(m_levelEvents.quake));
				m_levelEvents.quake.endTimer = 3.f;
				m_levelEvents.quake.quakeTimer = 1.f; // fixme
				break;

			case kLevelEvent_GravityWell:
				memset(&m_levelEvents.gravityWell, 0, sizeof(m_levelEvents.gravityWell));
				m_levelEvents.gravityWell.endTimer = EVENT_GRAVITYWELL_DURATION;
				m_levelEvents.gravityWell.m_x = GFX_SX / 2;
				m_levelEvents.gravityWell.m_y = GFX_SY / 2;
				break;

			case kLevelEvent_DestroyBlocks:
				memset(&m_levelEvents.destroyBlocks, 0, sizeof(m_levelEvents.destroyBlocks));
				m_levelEvents.destroyBlocks.m_remainingBlockCount = 0;
				break;

			case kLevelEvent_TimeDilation:
				memset(&m_levelEvents.timeDilation, 0, sizeof(m_levelEvents.timeDilation));
				m_levelEvents.timeDilation.endTimer = 3.f;
				addTimeDilationEffect(.5f, .25f, 3.f);
				break;

			case kLevelEvent_SpikeWalls:
				memset(&m_levelEvents.spikeWalls, 0, sizeof(m_levelEvents.spikeWalls));
				m_levelEvents.spikeWalls.endTimer = 3.f;
				m_levelEvents.spikeWalls.m_left = true;
				m_levelEvents.spikeWalls.m_right = true;
				break;

			case kLevelEvent_Wind:
				memset(&m_levelEvents.wind, 0, sizeof(m_levelEvents.wind));
				m_levelEvents.wind.endTimer = 3.f;
				break;

			case kLevelEvent_BarrelDrop:
				memset(&m_levelEvents.barrelDrop, 0, sizeof(m_levelEvents.barrelDrop));
				m_levelEvents.barrelDrop.endTimer = 3.f;
				m_levelEvents.barrelDrop.spawnTimer = 1.f;
				break;

			case kLevelEvent_NightDayCycle:
				memset(&m_levelEvents.nightDayCycle, 0, sizeof(m_levelEvents.nightDayCycle));
				m_levelEvents.nightDayCycle.endTimer = 3.f;
				break;
			}
		}
	}

	if (m_levelEvents.quake.endTimer.tickActive(dt))
	{
		if (m_levelEvents.quake.quakeTimer.tickComplete(dt))
		{
			m_levelEvents.quake.quakeTimer = 1.f; // fixme

			// trigger quake

			addScreenShake(0.f, 25.f, 1000.f, .3f);

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = m_players[i];

				if (!player.m_isUsed && player.m_isAlive)
					continue;

				if (player.m_isGrounded)
					player.m_vel[1] = -Calc::Sign(player.m_facing[1]) * 350.f;
			}
		}
	}

	if (m_levelEvents.gravityWell.endTimer.tickActive(dt))
	{
		// affect player speeds

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player & player = m_players[i];

			if (!player.m_isUsed && player.m_isAlive)
				continue;

			const float dx = m_levelEvents.gravityWell.m_x - player.m_pos[0];
			const float dy = m_levelEvents.gravityWell.m_y - player.m_pos[1];
			const float d = std::sqrtf(dx * dx + dy * dy);
			if (d != 0.f)
			{
				const float ax = dx / d;
				const float ay = dy / d;

				const float t = m_levelEvents.gravityWell.endTimer.getProgress();
				const float s = Calc::Lerp(EVENT_GRAVITYWELL_STRENGTH_BEGIN, EVENT_GRAVITYWELL_STRENGTH_END, t);
				const float strength = s * 10000.f / (d + 1.f);

				player.m_vel[0] += ax * strength * dt;
				player.m_vel[1] += ay * strength * dt;
			}
		}
	}

	if (m_levelEvents.destroyBlocks.m_remainingBlockCount > 0)
	{
		if (m_levelEvents.destroyBlocks.destructionTimer.tickComplete(dt))
		{
			// todo : destroy a random block

			m_levelEvents.destroyBlocks.destructionTimer = 0.f;
		}
	}

	if (m_levelEvents.timeDilation.endTimer.tickActive(dt))
	{
		//
	}

	if (m_levelEvents.spikeWalls.endTimer.tickActive(dt))
	{
		// todo : collision versus players. kill on hit
	}

	if (m_levelEvents.wind.endTimer.tickActive(dt))
	{
		// affect player speeds

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player & player = m_players[i];

			if (!player.m_isUsed && player.m_isAlive)
				continue;

			player.m_vel[0] += 300.f * dt;
		}
	}

	if (m_levelEvents.barrelDrop.endTimer.tickActive(dt))
	{
		if (m_levelEvents.barrelDrop.spawnTimer.tickComplete(dt))
		{
			// todo : add barrel

			// todo : next tick

			m_levelEvents.barrelDrop.spawnTimer = 1.f;
		}
	}

	if (m_levelEvents.nightDayCycle.endTimer.tickActive(dt))
	{
		//
	}

	// token

	if (m_gameMode == kGameMode_TokenHunt)
	{
		m_tokenHunt.m_token.tick(*this, dt);
	}

	// coins

	if (m_gameMode == kGameMode_CoinCollector)
	{
		for (int i = 0; i < MAX_COINS; ++i)
		{
			m_coinCollector.m_coins[i].tick(*this, dt);
		}

		if (tick >= m_coinCollector.m_nextSpawnTick)
		{
			int numCoins = 0;

			for (int i = 0; i < MAX_COINS; ++i)
				if (m_coinCollector.m_coins[i].m_isDropped)
					numCoins++;

			for (int i = 0; i < MAX_PLAYERS; ++i)
				if (m_players[i].m_isAlive)
					numCoins += m_players[i].m_score;

			if (numCoins < COINCOLLECTOR_COIN_LIMIT)
			{
				spawnCoin();
			}

			m_coinCollector.m_nextSpawnTick = tick + (COIN_SPAWN_INTERVAL + (Random() % COIN_SPAWN_INTERVAL_VARIANCE)) * TICKS_PER_SECOND;
		}
	}

	// floor effect

	m_floorEffect.tick(*this, dt);

	// torches

	for (int i = 0; i < MAX_TORCHES; ++i)
	{
		if (m_torches[i].m_isAlive)
			m_torches[i].tick(*this, dt);
	}

	anim(dt);

	m_bulletPool->tick(*this, dt);

	m_tick++;

	m_roundTime += dt;

	if (m_gameState == kGameState_Play)
	{
		// check if the round has ended

		bool roundComplete = false;

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player & player = m_players[i];
			if (!player.m_isUsed)
				continue;

			bool hasWon = false;

			if (m_gameMode == kGameMode_DeathMatch)
			{
				if (player.m_score >= DEATHMATCH_SCORE_LIMIT)
				{
					hasWon = true;
				}
			}
			else if (m_gameMode == kGameMode_TokenHunt)
			{
				if (player.m_score >= TOKENHUNT_SCORE_LIMIT)
				{
					hasWon = true;
				}
			}
			else if (m_gameMode == kGameMode_CoinCollector)
			{
				if (player.m_score >= COINCOLLECTOR_SCORE_LIMIT)
				{
					hasWon = true;
				}
			}
			else
			{
				Assert(false);
			}

			if (hasWon)
			{
				roundComplete = true;
			}
		}

		if (roundComplete)
		{
			endRound();
		}
	}
}

void GameSim::tickRoundComplete()
{
	// wait for the host to enter the next game state

	Assert(m_roundCompleteTicks > 0);
	if (m_roundCompleteTicks > 0)
	{
		m_roundCompleteTicks--;

		if (m_roundCompleteTicks == 0)
		{
			newRound(0);
		}
	}

	if (m_roundCompleteTimeDilationTicks > 0)
	{
		m_roundCompleteTimeDilationTicks--;
	}
}

void GameSim::anim(float dt)
{
	// screen shakes

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (shake.isActive)
			shake.tick(dt);
	}

	m_particlePool->tick(*this, dt);
}

void GameSim::playSound(const char * filename, int volume)
{
	if (g_noSound || !g_app->getSelectedClient() || g_app->getSelectedClient()->m_gameSim != this)
		return;

	Sound(filename).play(volume);
}

void GameSim::trySpawnPickup(PickupType type)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_pickups[i];

		if (!pickup.isAlive)
		{
			const int kMaxLocations = ARENA_SX * ARENA_SY;
			int numLocations = kMaxLocations;
			int x[kMaxLocations];
			int y[kMaxLocations];

			if (m_arena.getRandomPickupLocations(x, y, numLocations, this,
				[](void * obj, int x, int y) 
				{
					GameSim * self = (GameSim*)obj;
					for (int i = 0; i < MAX_PICKUPS; ++i)
						if (self->m_pickups[i].blockX == x && self->m_pickups[i].blockY == y)
							return true;
					return false;
				}))
			{
				if (DEBUG_RANDOM_CALLSITES)
					LOG_DBG("Random called from trySpawnPickup");
				const int index = Random() % numLocations;
				const int spawnX = x[index];
				const int spawnY = y[index];

				spawnPickup(pickup, type, spawnX, spawnY);
			}

			break;
		}
	}
}

void GameSim::spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY)
{
	pickup.isAlive = true;
	pickup.setup(type, blockX, blockY);
}

bool GameSim::grabPickup(int x1, int y1, int x2, int y2, Pickup & grabbedPickup)
{
	CollisionInfo collisionInfo;
	collisionInfo.x1 = x1;
	collisionInfo.y1 = y1;
	collisionInfo.x2 = x2;
	collisionInfo.y2 = y2;

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_pickups[i];

		if (pickup.isAlive)
		{
			CollisionInfo pickupCollision;
			pickup.getCollisionInfo(pickupCollision);

			if (collisionInfo.intersects(pickupCollision))
			{
				grabbedPickup = pickup;

				pickup = Pickup();

				playSound("gun-pickup.ogg");

				return true;
			}
		}
	}

	return false;
}

void GameSim::spawnToken()
{
	Token & token = m_tokenHunt.m_token;

	const int kMaxLocations = ARENA_SX * ARENA_SY;
	int numLocations = kMaxLocations;
	int x[kMaxLocations];
	int y[kMaxLocations];

	if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
	{
		const int index = Random() % numLocations;
		const int spawnX = x[index];
		const int spawnY = y[index];

		token.setup(spawnX, spawnY);
	}
}

bool GameSim::pickupToken(const CollisionInfo & collisionInfo)
{
	Token & token = m_tokenHunt.m_token;

	if (token.m_isDropped && token.m_dropTimer <= 0.f)
	{
		CollisionInfo tokenCollision;
		token.getCollisionInfo(tokenCollision);

		if (tokenCollision.intersects(collisionInfo))
		{
			token = Token();
			g_gameSim->playSound("token-pickup.ogg");

			return true;
		}
	}

	return false;
}

//

Coin * GameSim::allocCoin()
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		if (!m_coinCollector.m_coins[i].m_isDropped)
		{
			return &m_coinCollector.m_coins[i];
		}
	}

	return 0;
}

void GameSim::spawnCoin()
{
	Coin * coin = allocCoin();

	if (coin)
	{
		const int kMaxLocations = ARENA_SX * ARENA_SY;
		int numLocations = kMaxLocations;
		int x[kMaxLocations];
		int y[kMaxLocations];

		if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
		{
			const int index = Random() % numLocations;
			const int spawnX = x[index];
			const int spawnY = y[index];

			coin->setup(spawnX, spawnY);
		}
	}
}

bool GameSim::pickupCoin(const CollisionInfo & collisionInfo)
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		Coin & coin = m_coinCollector.m_coins[i];

		if (coin.m_isDropped && coin.m_dropTimer <= 0.f)
		{
			CollisionInfo coinCollision;
			coin.getCollisionInfo(coinCollision);

			if (coinCollision.intersects(collisionInfo))
			{
				coin = Coin();
				g_gameSim->playSound("token-pickup.ogg");

				return true;
			}
		}
	}

	return false;
}

//

uint16_t GameSim::spawnBullet(int16_t x, int16_t y, uint8_t _angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId)
{
	uint16_t id;
	m_bulletPool->alloc(&id, 1);

	if (id != INVALID_BULLET_ID)
	{
		Bullet & b = m_bulletPool->m_bullets[id];

		Assert(!b.isAlive);
		b = Bullet();
		b.isAlive = true;
		b.type = static_cast<BulletType>(type);
		b.effect = static_cast<BulletEffect>(effect);
		b.m_pos[0] = x;
		b.m_pos[1] = y;
		b.color = 0xffffffff;

		float angle = _angle / 128.f * float(M_PI);
		float velocity = 0.f;

		switch (type)
		{
		case kBulletType_A:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_B:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_Grenade:
			velocity = BULLET_GRENADE_NADE_SPEED;
			b.maxWrapCount = 100;
			b.m_noGravity = false;
			b.doBounce = true;
			b.bounceAmount = BULLET_GRENADE_NADE_BOUNCE_AMOUNT;
			b.noDamageMap = true;
			b.life = BULLET_GRENADE_NADE_LIFE;
			break;
		case kBulletType_GrenadeA:
			velocity = RandomFloat(BULLET_GRENADE_FRAG_SPEED_MIN, BULLET_GRENADE_FRAG_SPEED_MAX);
			b.maxWrapCount = 1;
			b.maxReflectCount = 0;
			b.maxDistanceTravelled = RandomFloat(BULLET_GRENADE_FRAG_RADIUS_MIN, BULLET_GRENADE_FRAG_RADIUS_MAX);
			b.maxDestroyedBlocks = 1;
			b.doDamageOwner = true;
			break;

		default:
			Assert(false);
			break;
		}

		b.setVel(angle, velocity);

		b.ownerPlayerId = ownerPlayerId;
	}

	return id;
}

void GameSim::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	uint16_t ids[MAX_BULLETS];

	const uint16_t count = m_particlePool->alloc(ids, spawnInfo.count);

	for (int i = 0; i < count; ++i)
	{
		uint16_t id = ids[i];

		Bullet & b = m_particlePool->m_bullets[id];

		initBullet(*this, b, spawnInfo);

		b.doAgeAlpha = true;
	}
}

void GameSim::addScreenShake(float dx, float dy, float stiffness, float life)
{
	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (!shake.isActive)
		{
			shake.isActive = true;
			shake.life = life;
			shake.stiffness = stiffness;

			shake.pos.Set(dx, dy);
			shake.vel.Set(0.f, 0.f);
			return;
		}
	}

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addScreenShake");
	m_screenShakes[Random() % MAX_SCREEN_SHAKES] = ScreenShake();
	addScreenShake(dx, dy, stiffness, life);
}

Vec2 GameSim::getScreenShake() const
{
	Vec2 result;

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		const ScreenShake & shake = m_screenShakes[i];
		if (shake.isActive)
			result += shake.pos;
	}

	return result;
}

void GameSim::addFloorEffect(int playerId, int x, int y, int size, int damageSize)
{
	m_floorEffect.trySpawnAt(*this, playerId, x, y, -BLOCK_SX, size, damageSize);
	m_floorEffect.trySpawnAt(*this, playerId, x, y, +BLOCK_SX, size, damageSize);
}
