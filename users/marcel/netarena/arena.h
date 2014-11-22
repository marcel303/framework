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
	kBlockType_COUNT
};

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

	bool getRandomSpawnPoint(int & x, int & y);
	std::vector<Block*> getIntersectingBlocks(int x1, int y1, int x2, int y2);
	uint32_t getIntersectingBlocksMask(int x1, int y1, int x2, int y2);
};
