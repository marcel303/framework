#include <tuple>
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

	for (int i = 0; i < 30; ++i)
	{
		const int x = rand() % ARENA_SX;
		const int y = rand() % ARENA_SY;

		m_blocks[x][y].type = (BlockType)(rand() % kBlockType_COUNT);

		if (x == 0 || x == ARENA_SX - 1)
			m_blocks[ARENA_SX - 1 - x][y].type = m_blocks[x][y].type;
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

			Sprite(filename).drawEx(x * 128, y * 128);
		}
	}
}

bool Arena::getRandomSpawnPoint(int & out_x, int & out_y)
{
	// find a spawn point

	std::vector< std::tuple<int, int> > blocks;

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			if (m_blocks[x][y].type == kBlockType_Spawn)
				blocks.push_back(std::tuple<int, int>(x, y));
		}
	}

	if (blocks.empty())
		return false;
	else
	{
		const int index = rand() % blocks.size();

		out_x = std::get<0>(blocks[index]) * BLOCK_SX;
		out_y = std::get<1>(blocks[index]) * BLOCK_SY;

		out_x += BLOCK_SX / 2;
		out_y += BLOCK_SY - 1;

		return true;
	}
}

std::vector<Block*> Arena::getIntersectingBlocks(int x1, int y1, int x2, int y2)
{
	std::vector<Block*> result;

	if (getBlockRectFromPixels(x1, y1, x2, y2, x1, y1, x2, y2))
	{
		int numBlocks = (x2 - x1 + 1) * (y2 - y1 + 1);

		result.reserve(numBlocks);

		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				result.push_back(&m_blocks[x][y]);
			}
		}

		Assert(result.size() == numBlock);
	}

	return result;
}

uint32_t Arena::getIntersectingBlocksMask(int x1, int y1, int x2, int y2)
{
	uint32_t result = 0;

	if (getBlockRectFromPixels(x1, y1, x2, y2, x1, y1, x2, y2))
	{
		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				result |= (1 << m_blocks[x][y].type);
			}
		}
	}

	return result;
}

static bool getBlockStripFromPixels(int v1, int v2, int & out_v1, int & out_v2, int stripSize, int blockSize)
{
	int temp;

	if (v1 > v2)
	{
		temp = v1;
		v1 = v2;
		v2 = temp;
	}

	v1 /= blockSize;
	v2 /= blockSize;

	if (v1 < 0)
		v1 = 0;
	if (v2 > stripSize - 1)
		v2 = stripSize - 1;

	if (v1 > v2)
		return false;

	out_v1 = v1;
	out_v2 = v2;

	return true;
}

bool Arena::getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2)
{
	return
		getBlockStripFromPixels(x1, x2, out_x1, out_x2, ARENA_SX, BLOCK_SX) &&
		getBlockStripFromPixels(y1, y2, out_y1, out_y2, ARENA_SY, BLOCK_SY);
}
