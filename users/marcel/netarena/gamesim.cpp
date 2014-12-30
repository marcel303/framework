#include "bullet.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "main.h"
#include "netsprite.h"
#include "player.h"

OPTION_DECLARE(int, g_pickupTimeBase, 10);
OPTION_DEFINE(int, g_pickupTimeBase, "Pickup/Spawn Interval (Sec)");
OPTION_DECLARE(int, g_pickupTimeRandom, 5);
OPTION_DEFINE(int, g_pickupTimeRandom, "Pickup/Spawn Interval Random (Sec)");
OPTION_DECLARE(int, g_pickupMax, 5);
OPTION_DEFINE(int, g_pickupMax, "Pickup/Maximum Pickup Count");

//

GameSim * g_gameSim = 0;

//

static const char * s_pickupSprites[kPickupType_COUNT] =
{
	"pickup-ammo.png",
	"pickup-nade.png"
};

//

uint32_t GameSim::GameState::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	return m_randomSeed;
}

uint32_t GameSim::GameState::GetTick()
{
	return m_tick;
}

//

GameSim::GameSim(bool isAuthorative)
	: m_isAuthorative(isAuthorative)
#if !ENABLE_CLIENT_SIMULATION
	, m_arenaNetObject()
#endif
	, m_state()
	, m_bulletPool(0)
{
#if ENABLE_CLIENT_SIMULATION
	m_arena.init(0);
#else
	m_arena.init(&m_arenaNetObject);
#endif

	for (int i = 0; i < MAX_PLAYERS; ++i)
		m_players[i] = 0;

	m_bulletPool = new BulletPool(!isAuthorative);

	m_particlePool = new BulletPool(true);

#if !ENABLE_CLIENT_SIMULATION
	m_spriteManager = new NetSpriteManager();
#endif
}

GameSim::~GameSim()
{
#if !ENABLE_CLIENT_SIMULATION
	delete m_spriteManager;
	m_spriteManager = 0;
#endif

	delete m_particlePool;
	m_particlePool = 0;

	delete m_bulletPool;
	m_bulletPool = 0;
}

uint32_t GameSim::calcCRC() const
{
#if ENABLE_CLIENT_SIMULATION
	clearPlayerPtrs();

	uint32_t result = 0;

	const uint8_t * bytes = (uint8_t*)&m_state;
	const uint32_t numBytes = sizeof(m_state);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	setPlayerPtrs();

	return result;
#else
	return 0;
#endif
}

void GameSim::serialize(NetSerializationContext & context)
{
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i])
			m_players[i]->m_player->m_netObject = 0;

	context.SerializeBytes(&m_state, sizeof(m_state));

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i])
			m_players[i]->m_player->m_netObject = m_players[i];

	m_arena.serialize(context);

	if (context.IsRecv())
		Assert(crc == calcCRC());

	// todo : serialize player animation state, since it affects game play
}

void GameSim::clearPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i])
			m_players[i]->m_player->m_netObject = 0;
}

void GameSim::setPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i])
			m_players[i]->m_player->m_netObject = m_players[i];
}

void GameSim::setGameState(::GameState gameState)
{
	m_state.m_gameState = gameState;

	if (gameState == kGameState_NewGame)
	{
		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_players[i])
			{
				Player * player = m_players[i]->m_player;

				player->handleNewGame();
			}
		}
	}

	if (gameState == kGameState_Play)
	{
		// reset pickups

		for (int i = 0; i < MAX_PICKUPS; ++i)
			m_state.m_pickups[i].isAlive = false;

		// respawn players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_players[i])
			{
				Player * player = m_players[i]->m_player;

				player->handleNewRound();

				player->respawn();
			}
		}
	}
}

void GameSim::tick()
{
	switch (m_state.m_gameState)
	{
	case kGameState_Lobby:
		tickLobby();
		break;

	case kGameState_Play:
		tickPlay();
		break;

	case kGameState_RoundComplete:
		tickRoundComplete();
		break;
	}
}

void GameSim::tickLobby()
{
	// wait for the host to enter the next game state
}

void GameSim::tickPlay()
{
	g_gameSim = this;

#if ENABLE_CLIENT_SIMULATION
	if (g_devMode && ENABLE_GAMESTATE_CRC_LOGGING)
	{
		int numPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (m_players[i])
				numPlayers++;

		const uint32_t crc = calcCRC();
		LOG_DBG("gamesim %p: tick=%u, crc=%08x, numPlayers=%d", this, m_state.m_tick, crc, numPlayers);
	}
#endif

	const float dt = 1.f / TICKS_PER_SECOND;

	const uint32_t tick = m_state.GetTick();

#if ENABLE_CLIENT_SIMULATION
	// player update

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_players[i])
			m_players[i]->m_player->tick(dt);
	}
#endif

	// pickup spawning

	if (tick >= m_state.m_nextPickupSpawnTick)
	{
		int weights[kPickupType_COUNT] =
		{
			PICKUP_AMMO_WEIGHT,
			PICKUP_NADE_WEIGHT
		};

		int totalWeight = 0;

		for (int i = 0; i < kPickupType_COUNT; ++i)
		{
			totalWeight += weights[i];
			weights[i] = totalWeight;
		}

		if (DEBUG_RANDOM_CALLSITES)
			LOG_DBG("Random called from pre trySpawnPickup");
		int value = m_state.Random() % totalWeight;

		PickupType type = kPickupType_COUNT;

		for (int i = 0; type == kPickupType_COUNT; ++i)
			if (value < weights[i])
				type = (PickupType)i;

		trySpawnPickup(type);

		m_state.m_nextPickupSpawnTick = tick + (g_pickupTimeBase + (m_state.Random() % g_pickupTimeRandom)) * TICKS_PER_SECOND;
	}

#if ENABLE_CLIENT_SIMULATION
	anim(dt);

	m_bulletPool->tick(*this, dt);
#endif

	m_state.m_tick++;

	g_gameSim = 0;
}

void GameSim::tickRoundComplete()
{
	// wait for the host to enter the next game state
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

void GameSim::trySpawnPickup(PickupType type)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_state.m_pickups[i];

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
						if (self->m_state.m_pickups[i].blockX == x && self->m_state.m_pickups[i].blockY == y)
							return true;
					return false;
				}))
			{
				if (DEBUG_RANDOM_CALLSITES)
					LOG_DBG("Random called from trySpawnPickup");
				const int index = m_state.Random() % numLocations;
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
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	pickup.isAlive = true;
	pickup.type = type;
	pickup.blockX = blockX;
	pickup.blockY = blockY;
	pickup.x1 = blockX * BLOCK_SX + (BLOCK_SX - sprite.getWidth()) / 2;
	pickup.y1 = blockY * BLOCK_SY + BLOCK_SY - sprite.getHeight();
	pickup.x2 = pickup.x1 + sprite.getWidth();
	pickup.y2 = pickup.y1 + sprite.getHeight();
#if !ENABLE_CLIENT_SIMULATION
	pickup.spriteId = g_app->netAddSprite(filename, pickup.x1, pickup.y1);
#endif
}

Pickup * GameSim::grabPickup(int x1, int y1, int x2, int y2)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_state.m_pickups[i];

		if (pickup.isAlive)
		{
			if (x2 >= pickup.x1 && x1 < pickup.x2 &&
				y2 >= pickup.y1 && y1 < pickup.y2)
			{
				m_state.m_grabbedPickup = pickup;
				pickup.isAlive = false;

			#if !ENABLE_CLIENT_SIMULATION
				g_app->netRemoveSprite(m_state.m_grabbedPickup.spriteId);
			#endif

				g_app->netPlaySound("gun-pickup.ogg");

				return &m_state.m_grabbedPickup;
			}
		}
	}

	return 0;
}

void GameSim::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	for (int i = 0; i < spawnInfo.count; ++i)
	{
		uint16_t id = m_particlePool->alloc();

		if (id != INVALID_BULLET_ID)
		{
			Bullet & b = m_particlePool->m_bullets[id];

			initBullet(*this, b, spawnInfo);
		}
	}
}

void GameSim::addScreenShake(Vec2 delta, float stiffness, float life)
{
	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (!shake.isActive)
		{
			shake.isActive = true;
			shake.life = life;
			shake.stiffness = stiffness;

			shake.pos = delta;
			shake.vel.Set(0.f, 0.f);
			return;
		}
	}

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addScreenShake");
	m_screenShakes[m_state.Random() % MAX_SCREEN_SHAKES].isActive = false;
	addScreenShake(delta, stiffness, life);
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
