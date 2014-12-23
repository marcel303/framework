#pragma once

#include "gamedefs.h"
#include "netobject.h"

enum BlockShape : unsigned char
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
	kBlockShape_COUNT
};

enum BlockType : unsigned char
{
	kBlockType_Empty,
	kBlockType_Destructible,
	kBlockType_Indestructible,
	kBlockType_Slide,
	kBlockType_Moving,
	kBlockType_Sticky,
	kBlockType_Spike,
	kBlockType_Spawn,
	kBlockType_Spring,
	kBlockType_Teleport,
	kBlockType_GravityReverse,
	kBlockType_GravityDisable,
	kBlockType_GravityStrong,
	kBlockType_GravityLeft,
	kBlockType_GravityRight,
	kBlockType_ConveyorBeltLeft,
	kBlockType_ConveyorBeltRight,
	kBlockType_Passthrough,
	kBlockType_COUNT
};

static const int kBlockMask_Solid =
	(1 << kBlockType_Destructible) |
	(1 << kBlockType_Indestructible) |
	(1 << kBlockType_Slide) |
	(1 << kBlockType_Sticky) |
	(1 << kBlockType_Spring) |
	(1 << kBlockType_ConveyorBeltLeft) |
	(1 << kBlockType_ConveyorBeltRight) |
	(1 << kBlockType_Passthrough);

static const int kBlockMask_Passthrough =
	(1 << kBlockType_Passthrough);

#pragma pack(push)
#pragma pack(1)

struct Block
{
	BlockType type;
	uint8_t clientData;

	BlockShape shape;
	uint16_t serverData;
};

#pragma pack(pop)

struct BlockAndDistance
{
	Block * block;
	uint8_t x;
	uint8_t y;
	int distanceSq;
};

class Arena : public NetObject
{
	class Arena_NS : public NetSerializable
	{
	public:
		Arena_NS(NetSerializableObject * owner);

		virtual void SerializeStruct();
	};

	Arena_NS m_serializer;

	Block m_blocks[ARENA_SX][ARENA_SY];

	void reset();

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Arena; }

public:
	// fixme .. should be part of.. well.. something else..
	class GameState_NS : public NetSerializable
	{
	public:
		GameState_NS(NetSerializableObject * owner)
			: NetSerializable(owner)
			, m_gameState(kGameState_Undefined)
		{
		}

		virtual void SerializeStruct()
		{
			uint8_t gameState = m_gameState;
			Serialize(gameState);
			m_gameState = static_cast<GameState>(gameState);
		}

		GameState m_gameState;
	};

	GameState_NS m_gameState;

	Arena();

	void generate();
	void load(const char * filename);

	void drawBlocks();

	bool getRandomSpawnPoint(int & out_x, int & out_y, int & io_lastSpawnIndex);
	bool getRandomPickupLocation(int & out_x, int & out_y);

	uint32_t getIntersectingBlocksMask(int x1, int y1, int x2, int y2);
	uint32_t getIntersectingBlocksMask(int x, int y);

	bool getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2);
	bool getBlocksFromPixels(int x, int y, int x1, int y1, int x2, int y2, bool wrap, BlockAndDistance * out_blocks, int & io_numBlocks);
	Block & getBlock(int x, int y) { return m_blocks[x][y]; }

	bool handleDamageRect(int x, int y, int x1, int y1, int x2, int y2, bool hitDestructible);

	// todo : optimized way for making small changes
	void setDirty()
	{
		m_serializer.SetDirty();
	}
};
