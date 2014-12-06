#include <tuple>
#include "arena.h"
#include "FileStream.h"
#include "framework.h"
#include "StreamReader.h"

Arena::Arena()
	: NetObject()
	, m_serializer(this)
{
	reset();
}

void Arena::reset()
{
	// clear the arena

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			m_blocks[x][y].type = kBlockType_Empty;
		}
	}
}

void Arena::generate()
{
	// clear the arena

	reset();

	// generate a border

	for (int x = 0; x < ARENA_SX; ++x)
		for (int y = 0; y < ARENA_SY; ++y)
			if (x == 0 || x == ARENA_SX - 1 || y == 0 || y == ARENA_SY - 1)
				m_blocks[x][y].type = kBlockType_Indestructible;

	// add some random stuff

	for (int i = 0; i < 100; ++i)
	{
		const int x = rand() % ARENA_SX;
		const int y = rand() % ARENA_SY;

		if ((rand() % 3) != 0)
			m_blocks[x][y].type = kBlockType_Indestructible;
		else
			m_blocks[x][y].type = (BlockType)(rand() % kBlockType_COUNT);

		if (x == 0 || x == ARENA_SX - 1)
			m_blocks[ARENA_SX - 1 - x][y].type = m_blocks[x][y].type;

		if (true && (y == 0 || y == ARENA_SY - 1))
			m_blocks[x][ARENA_SY - 1 - y].type = m_blocks[x][y].type;
	}

	m_serializer.SetDirty();
}

void Arena::load(const char * filename)
{
	reset();

	try
	{
		FileStream stream;
		stream.Open(filename, (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();

		const int sy = lines.size() < ARENA_SY ? lines.size() : ARENA_SY;

		for (int y = 0; y < sy; ++y)
		{
			const std::string & line = lines[y];

			const int sx = line.size() < ARENA_SX ? line.size() : ARENA_SX;

			for (int x = 0; x < sx; ++x)
			{
				/*
				  = kBlockType_Empty,
				c = kBlockType_Destructible,
				x = kBlockType_Indestructible,
				    kBlockType_Moving,
				s = kBlockType_Sticky,
				    kBlockType_Spike,
				p = kBlockType_Spawn,
				j = kBlockType_Spring,
				u = kBlockType_GravityReverse,
				- = kBlockType_GravityDisable,
				d = kBlockType_GravityStrong,
				< = kBlockType_ConveyorBeltLeft,
				> = kBlockType_ConveyorBeltRight,
				*/

				BlockType type = kBlockType_Empty;

				switch (line[x])
				{
				case ' ': type = kBlockType_Empty; break;
				case 'c': type = kBlockType_Destructible; break;
				case 'x': type = kBlockType_Indestructible; break;
				case 's': type = kBlockType_Sticky; break;
				case 'p': type = kBlockType_Spawn; break;
				case 'j': type = kBlockType_Spring; break;
				case 'u': type = kBlockType_GravityReverse; break;
				case '-': type = kBlockType_GravityDisable; break;
				case 'd': type = kBlockType_GravityStrong; break;
				case '<': type = kBlockType_ConveyorBeltLeft; break;
				case '>': type = kBlockType_ConveyorBeltRight; break;
				default:
					LOG_WRN("invalid block type: '%c'", line[x]);
				}

				m_blocks[x][y].type = type;
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to open %s: %s", filename, e.what());
	}

	m_serializer.SetDirty();
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
		"block-spring.png",
		"block-gravity-reverse.png",
		"block-gravity-disable.png",
		"block-gravity-strong.png",
		"block-conveyorbelt-left.png",
		"block-conveyorbelt-right.png"
	};

	static Sprite * sprites[kBlockType_COUNT] = { };

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			const Block & block = m_blocks[x][y];

			if (!sprites[block.type])
			{
				const char * filename = filenames[block.type];

				sprites[block.type] = new Sprite(filename);
			}

			sprites[block.type]->drawEx(x * BLOCK_SX, y * BLOCK_SY);
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

		Assert(result.size() == numBlocks);
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
