#pragma once

#include "gamedefs.h"
#include "gametypes.h"
#include "NetSerializable.h"
#include "physobj.h"

class Arena;
class BitStream;
class GameSim;
struct Player;
class Sprite;

#define USE_TEXTURE_ATLAS 1

enum BlockShape
{
	kBlockShape_Empty,
	kBlockShape_Opaque,
	kBlockShape_TL,
	kBlockShape_TR,
	kBlockShape_BL,
	kBlockShape_BR,
	kBlockShape_TL2a,
	kBlockShape_TL2b,
	kBlockShape_TR2a,
	kBlockShape_TR2b,
	kBlockShape_BL2a,
	kBlockShape_BL2b,
	kBlockShape_BR2a,
	kBlockShape_BR2b,
	kBlockShape_HT,
	kBlockShape_HB,
	kBlockShape_COUNT
};

enum BlockType
{
	kBlockType_Empty,
	kBlockType_Destructible,
	kBlockType_DestructibleRegen,
	kBlockType_Indestructible,
	kBlockType_Slide,
	kBlockType_Moving,
	kBlockType_Sticky,
	kBlockType_Spike,
	kBlockType_Spawn,
	kBlockType_Spring,
	kBlockType_GravityReverse,
	kBlockType_GravityDisable,
	kBlockType_GravityStrong,
	kBlockType_GravityLeft,
	kBlockType_GravityRight,
	kBlockType_ConveyorBeltLeft,
	kBlockType_ConveyorBeltRight,
	kBlockType_Passthrough,
	kBlockType_Appear,
	kBlockType_COUNT
};

static const int kBlockMask_Solid =
	(1 << kBlockType_Destructible) |
	(1 << kBlockType_DestructibleRegen) |
	(1 << kBlockType_Indestructible) |
	(1 << kBlockType_Slide) |
	(1 << kBlockType_Sticky) |
	(1 << kBlockType_Spring) |
	(1 << kBlockType_ConveyorBeltLeft) |
	(1 << kBlockType_ConveyorBeltRight) |
	(1 << kBlockType_Appear) |
	(1 << kBlockType_Passthrough);

static const int kBlockMask_Passthrough =
	(1 << kBlockType_Appear) |
	(1 << kBlockType_Passthrough);

static const int kBlockMask_Destructible =
	(1 << kBlockType_Destructible) |
	(1 << kBlockType_DestructibleRegen);

static const int kBlockMask_Spike =
	(1 << kBlockType_Spike);

#pragma pack(push)
#pragma pack(1)

struct Block
{
	BlockType type;
	BlockShape shape;
	uint16_t artIndex[3];
	uint16_t param;

	bool handleDamage(GameSim & gameSim, int blockX, int blockY);
};

#pragma pack(pop)

struct BlockAndDistance
{
	Block * block;
	uint8_t x;
	uint8_t y;
	int distanceSq;
};

struct AppearBlockData
{
	uint16_t isVisible : 1;
	uint16_t switchTime : 8;
	uint16_t padding : 7;
};

struct RegenBlockData
{
	uint16_t isVisible : 1;
	uint16_t regenTime : 15;
};

class Arena
{
public:
	Block m_blocks[ARENA_SX][ARENA_SY];

	FixedString<64> m_name;
#if USE_TEXTURE_ATLAS
	uint64_t m_texture;
	int m_textureSx;
	int m_textureSy;
#else
	Sprite ** m_sprites;
	int m_numSprites;
#endif

public:
	Arena();
	~Arena();

	void init();

	void reset();
	void generate();
	void load(const char * name);
	void loadArt(const char * name);
	void freeArt();
	void loadArtIndices(const char * name, int layer);
	void serialize(NetSerializationContext & context);

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t calcCRC() const;
#endif

	void drawBlocks(int layer) const;

	void tick(GameSim & gameSim);

	bool getRandomSpawnPoint(GameSim & gameSim, int & out_x, int & out_y, int & io_lastSpawnIndex, Player * playerToIgnore) const;
	bool isValidPickupLocation(int x, int y, bool grounded) const;
	bool getRandomPickupLocations(int * out_x, int * out_y, int & numLocations, void * obj, bool (*reject)(void * obj, int x, int y)) const;

	uint32_t getIntersectingBlocksMask(int x, int y) const;

	bool getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2) const;
	bool getBlocksFromPixels(int x, int y, int x1, int y1, int x2, int y2, bool wrap, BlockAndDistance * out_blocks, int & io_numBlocks);
	Block & getBlock(int x, int y) { return m_blocks[x][y]; }

	bool handleDamageRect(GameSim & gameSim, int x, int y, int x1, int y1, int x2, int y2, bool hitDestructible, bool hitSingleDestructible = true);
	bool handleDamageShape(GameSim & gameSim, int x, int y, const CollisionShape & shape, bool hitDestructible, bool hitSingleDestructible = true);

	static const CollisionShape & getBlockCollision(BlockShape shape);
	static void getBlockCollision(BlockShape shape, CollisionShape & collisionShape, int blockX, int blockY);
	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);

	bool testCollisionLine(float x1, float y1, float x2, float y2, uint32_t blockMask);
};

void initArenaData();
void shutArenaData();
