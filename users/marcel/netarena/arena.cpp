#include "arena.h"
#include "framework.h"

Arena::Arena()
	: NetObject()
	, m_serializer(this)
{
}

void Arena::generate()
{
	// clear the arena

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			if (x == 0 || x == ARENA_SX - 1 || y == 0 || y == ARENA_SY - 1)
				m_blocks[x][y].type = kBlockType_Indestructible;
			else
				m_blocks[x][y].type = kBlockType_Empty;
		}
	}

	// add some random stuff

	for (int i = 0; i < 20; ++i)
	{
		const int x = rand() % ARENA_SX;
		const int y = rand() % ARENA_SY;

		m_blocks[x][y].type = (BlockType)(rand() % kBlockType_COUNT);
	}
}

void Arena::drawBlocks()
{
	const char * filenames[kBlockType_COUNT] =
	{
		"block-empty.png",
		"block-destructible.png",
		"block-indestructible.png",
		"block-moving.png",
		"block-sticky.png",
		"block-spike.png",
		"block-spawn.png",
		"block-spring.png"
	};

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			const Block & block = m_blocks[x][y];

			const char * filename = filenames[block.type];

			Sprite(filename).drawEx(x * 128, y * 128, 0.f, 4.f);
		}
	}
}

bool Arena::getRandomSpawnPoint(int & x, int & y)
{
	// todo : find a spawn point

	x = rand() % ARENA_SX;
	y = rand() % ARENA_SY;

	return true;
}

std::vector<Block*> Arena::getIntersectingBlocks(int x1, int y1, int x2, int y2)
{
	std::vector<Block*> result;

	return result;
}

uint32_t Arena::getIntersectingBlocksMask(int x1, int y1, int x2, int y2)
{
	uint32_t result = 0;

	return result;
}
