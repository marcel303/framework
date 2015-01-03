#include "bullet.h"
#include "client.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "main.h"
#include "player.h"

OPTION_DECLARE(bool, g_noSound, false);
OPTION_DEFINE(bool, g_noSound, "Sound/Disable Sound Effects");
OPTION_ALIAS(g_noSound, "nosound");

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

#define TOKEN_SPRITE "token.png"

//

PhysicsActor::PhysicsActor()
{
	memset(this, 0, sizeof(*this));
}

void PhysicsActor::tick(GameSim & gameSim, float dt, PhysicsActorCBs & cbs)
{
	// physics

	if (!m_noGravity)
	{
		m_vel[1] += GRAVITY * dt;
	}

	const uint32_t oldBlockMask = getIntersectingBlockMask(gameSim, Vec2(m_pos[0], m_pos[1]));
	const bool isInPassthrough = (oldBlockMask & kBlockMask_Passthrough) != 0;
	const bool passthroughMode = isInPassthrough || (m_vel[1] < 0.f);
	const uint32_t blockExcludeMask = passthroughMode ? ~kBlockMask_Passthrough : ~0;

	if (m_friction != 0.f)
	{
		uint32_t blockMask = getIntersectingBlockMask(gameSim, Vec2(m_pos[0], m_pos[1] + 1.f)) & blockExcludeMask;

		if (blockMask & kBlockMask_Solid)
		{
			m_vel[0] *= std::pow(m_friction, dt);
		}
	}

	if (m_airFriction)
	{
		m_vel *= std::pow(m_airFriction, dt);
	}

	// collision

	Vec2 delta = m_vel * dt;
	float deltaLen = delta.CalcSize();

	int numSteps = std::max<int>(1, (int)std::ceil(deltaLen));

	Vec2 step = delta / numSteps;

	const float wrapSizes[2] = { ARENA_SX_PIXELS, ARENA_SY_PIXELS };

	for (int i = 0; i < numSteps; ++i)
	{
		Vec2 newPos = m_pos;

		for (int j = 0; j < 2; ++j)
		{
			bool collision = false;

			float oldPos = newPos[j];

			newPos[j] += step[j];

			if (newPos[j] < 0.f)
				newPos[j] = wrapSizes[j];
			else if (newPos[j] > wrapSizes[j])
				newPos[j] = 0.f;

			uint32_t blockMask = getIntersectingBlockMask(gameSim, newPos) & blockExcludeMask;

			if (j == 0)
				blockMask &= ~kBlockMask_Passthrough;

			if (blockMask & kBlockMask_Solid)
			{
				collision = true;
			}

			if (collision)
			{
				newPos[j] = oldPos;

				m_vel[j] *= m_bounciness;
				step[j] *= m_bounciness;

				if (cbs.onBounce)
					cbs.onBounce(cbs, *this);
			}
			else
			{
				m_pos[j] = newPos[j];
			}

			if (cbs.onBlockMask)
				cbs.onBlockMask(cbs, *this, blockMask);
		}
	}
}

void PhysicsActor::drawBB()
{
	Vec2 min = m_pos + m_bbMin;
	Vec2 max = m_pos + m_bbMax;

	setColor(0, 255, 0, 127);
	drawRect(min[0], min[1], max[0], max[1]);

	setColor(255, 255, 255);
}

uint32_t PhysicsActor::getIntersectingBlockMask(GameSim & gameSim, Vec2 pos)
{
	Vec2 min = pos + m_bbMin;
	Vec2 max = pos + m_bbMax;

	const int x1 = (int(min[0]         )     + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int x2 = (int(max[0]         )     + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int y1 = (int(min[1]         )     + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y2 = (int(max[1]         )     + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y3 = (int(min[1] + max[1]) / 2 + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;

	const Arena & arena = gameSim.m_arena;

	uint32_t result = 0;

	result |= arena.getIntersectingBlocksMask(x1, y1);
	result |= arena.getIntersectingBlocksMask(x2, y1);
	result |= arena.getIntersectingBlocksMask(x2, y2);
	result |= arena.getIntersectingBlocksMask(x1, y2);

	result |= arena.getIntersectingBlocksMask(x1, y3);
	result |= arena.getIntersectingBlocksMask(x2, y3);

	return result;
}

void PhysicsActor::getCollisionInfo(CollisionInfo & collisionInfo)
{
	Vec2 min = m_pos + m_bbMin;
	Vec2 max = m_pos + m_bbMax;

	collisionInfo.x1 = min[0];
	collisionInfo.y1 = min[1];
	collisionInfo.x2 = max[0];
	collisionInfo.y2 = max[1];
}

//

uint32_t GameStateData::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	return m_randomSeed;
}

uint32_t GameStateData::GetTick()
{
	return m_tick;
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

	m_bounciness = .25f;
	m_friction = .1f;
	m_airFriction = .5f;
}

void Pickup::tick(GameSim & gameSim, float dt)
{
	PhysicsActorCBs cbs;
	PhysicsActor::tick(gameSim, dt, cbs);
}

void Pickup::draw()
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	sprite.drawEx(m_pos[0] + m_bbMin[0], m_pos[1] + m_bbMin[1]);
}

void Pickup::drawLight()
{
	const float x = m_pos[0];
	const float y = m_pos[1];

	Sprite("player-light.png").drawEx(x, y, 0.f, 1.f);
}

//

void Token::setPos(int blockX, int blockY)
{
	Sprite sprite(TOKEN_SPRITE);
	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f);
	m_bbMax.Set(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f);
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
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

void Token::draw()
{
	if (m_isDropped)
	{
		Sprite(TOKEN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}
}

void Token::drawLight()
{
	if (m_isDropped)
	{
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f);
	}
}

//

GameSim::GameSim()
	: GameStateData()
	, m_bulletPool(0)
{
	m_arena.init();

	for (int i = 0; i < MAX_PLAYERS; ++i)
		m_playerNetObjects[i] = 0;

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

uint32_t GameSim::calcCRC() const
{
	clearPlayerPtrs();

	uint32_t result = 0;

	const uint8_t * bytes = (const uint8_t*)static_cast<const GameStateData*>(this);
	const uint32_t numBytes = sizeof(GameStateData);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	setPlayerPtrs();

	return result;
}

void GameSim::serialize(NetSerializationContext & context)
{
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = 0;

	GameStateData * data = static_cast<GameStateData*>(this);

	context.SerializeBytes(data, sizeof(GameStateData));

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = m_playerNetObjects[i];

	// todo : serialize player animation state

	m_arena.serialize(context);

	m_bulletPool->serialize(context);

	if (context.IsRecv())
		Assert(crc == calcCRC());

	// todo : serialize player animation state, since it affects game play
}

void GameSim::clearPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = 0;
}

void GameSim::setPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = m_playerNetObjects[i];
}

void GameSim::setGameState(::GameState gameState)
{
	m_gameState = gameState;

	if (gameState == kGameState_NewGame)
	{
		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerNetObjects[i])
			{
				Player * player = m_playerNetObjects[i]->m_player;

				player->handleNewGame();
			}
		}
	}

	if (gameState == kGameState_Play)
	{
		playSound("round-begin.ogg");

		// reset pickups

		for (int i = 0; i < MAX_PICKUPS; ++i)
			m_pickups[i].isAlive = false;

		// reset bullets

		for (int i = 0; i < MAX_BULLETS; ++i)
		{
			if (m_bulletPool->m_bullets[i].isAlive)
				m_bulletPool->free(i);

			if (m_particlePool->m_bullets[i].isAlive)
				m_particlePool->free(i);
		}

		// respawn players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerNetObjects[i])
			{
				Player * player = m_playerNetObjects[i]->m_player;

				player->handleNewRound();

				player->respawn();
			}
		}

		if (m_gameMode == kGameMode_TokenHunt)
		{
			spawnToken();
		}
	}
}

void GameSim::setGameMode(GameMode gameMode)
{
	m_gameMode = gameMode;
}

void GameSim::tick()
{
	switch (m_gameState)
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

	if (g_devMode && ENABLE_GAMESTATE_CRC_LOGGING)
	{
		int numPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (m_playerNetObjects[i])
				numPlayers++;

		const uint32_t crc = calcCRC();
		LOG_DBG("gamesim %p: tick=%u, crc=%08x, numPlayers=%d", this, m_tick, crc, numPlayers);
	}

	const float dt = 1.f / TICKS_PER_SECOND;

	const uint32_t tick = GetTick();

	// arena update
	m_arena.tick(*this);

	// player update

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->tick(dt);
	}

	// picksup

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		if (m_pickups[i].isAlive)
			m_pickups[i].tick(*this, dt);
	}

	// pickup spawning

	if (tick >= m_nextPickupSpawnTick)
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
		int value = Random() % totalWeight;

		PickupType type = kPickupType_COUNT;

		for (int i = 0; type == kPickupType_COUNT; ++i)
			if (value < weights[i])
				type = (PickupType)i;

		trySpawnPickup(type);

		m_nextPickupSpawnTick = tick + (g_pickupTimeBase + (Random() % g_pickupTimeRandom)) * TICKS_PER_SECOND;
	}

	// token

	m_tokenHunt.m_token.tick(*this, dt);

	anim(dt);

	m_bulletPool->tick(*this, dt);

	m_tick++;

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

Pickup * GameSim::grabPickup(int x1, int y1, int x2, int y2)
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
				m_grabbedPickup = pickup;
				pickup.isAlive = false;

				playSound("gun-pickup.ogg");

				return &m_grabbedPickup;
			}
		}
	}

	return 0;
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

		token = Token();
		token.setPos(spawnX, spawnY);
		token.m_bounciness = -TOKEN_BOUNCINESS;
		token.m_dropTimer = 0.f;
		token.m_isDropped = true;
		token.m_noGravity = false;
		token.m_friction = 0.1f;
		token.m_airFriction = 0.9f;
		token.m_vel.Set(0.f, 0.f);
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
			token.m_isDropped = false;
			g_gameSim->playSound("token-pickup.ogg");

			return true;
		}
	}

	return false;
}

uint16_t GameSim::spawnBullet(int16_t x, int16_t y, uint8_t _angle, uint8_t type, uint8_t ownerPlayerId)
{
	const uint16_t id = m_bulletPool->alloc();

	if (id != INVALID_BULLET_ID)
	{
		Bullet & b = m_bulletPool->m_bullets[id];

		Assert(!b.isAlive);
		memset(&b, 0, sizeof(b));
		b.isAlive = true;
		b.type = static_cast<BulletType>(type);
		b.pos[0] = x;
		b.pos[1] = y;
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
			b.doGravity = true;
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
	m_screenShakes[Random() % MAX_SCREEN_SHAKES].isActive = false;
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
