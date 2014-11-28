#pragma once

#include "gamedefs.h"
#include "netobject.h"

enum BlockType : unsigned char
{
	kBlockType_Empty,
	kBlockType_Destructible,
	kBlockType_Indestructible,
	kBlockType_Moving,
	kBlockType_Sticky,
	kBlockType_Spike,
	kBlockType_Spawn,
	kBlockType_Spring,
	kBlockType_GravityReverse,
	kBlockType_GravityDisable,
	kBlockType_GravityStrong,
	kBlockType_ConveyorBeltLeft,
	kBlockType_ConveyorBeltRight,
	kBlockType_COUNT
};

static const int kBlockMask_Solid =
	(1 << kBlockType_Destructible) |
	(1 << kBlockType_Indestructible) |
	(1 << kBlockType_Sticky) |
	(1 << kBlockType_Spring) |
	(1 << kBlockType_ConveyorBeltLeft) |
	(1 << kBlockType_ConveyorBeltRight);

#pragma pack(push)
#pragma pack(1)

struct Block
{
	BlockType type : 4;
};

#pragma pack(pop)

class Arena : public NetObject
{
	class Arena_NS : public NetSerializable
	{
	public:
		Arena_NS(NetSerializableObject * owner)
			: NetSerializable(owner)
		{
		}

		virtual void SerializeStruct()
		{
			Arena * arena = static_cast<Arena*>(GetOwner());

			SerializeBytes(arena->m_blocks, sizeof(arena->m_blocks));
		}
	};

	Arena_NS m_serializer;

	Block m_blocks[ARENA_SX][ARENA_SY];

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Arena; }

public:
	Arena();

	void generate();

	void drawBlocks();

	bool getRandomSpawnPoint(int & out_x, int & out_y);
	std::vector<Block*> getIntersectingBlocks(int x1, int y1, int x2, int y2);
	uint32_t getIntersectingBlocksMask(int x1, int y1, int x2, int y2);

	bool getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2);
};
