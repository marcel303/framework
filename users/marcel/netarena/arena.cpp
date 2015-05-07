#include <algorithm>
#include <tuple>
#include "arena.h"
#include "bullet.h"
#include "FileStream.h"
#include "framework.h"
#include "gamesim.h"
#include "host.h"
#include "image.h"
#include "main.h"
#include "Path.h"
#include "player.h"
#include "StreamReader.h"

#if USE_32X32_TILES
	#define BLOCK_SPRITE_SCALE 1.f
#else
	#define BLOCK_SPRITE_SCALE 1.f
#endif

static Sprite * s_sprites[kBlockType_COUNT] = { };

class BlockMask
{
public:
	uint8_t data[BLOCK_SY][(BLOCK_SX + 7) / 8];

	void set(int x, int y)
	{
		data[y][x >> 3] |= 1 << (x & 7);
	}

	bool test(int x, int y) const
	{
		const int idx = x >> 3;
		const int bit = x & 7;
		return (data[y][idx] & (1 << bit)) != 0;
	}
};

static BlockMask s_blockMasks[kBlockShape_COUNT];
static CollisionShape s_blockPolys[kBlockShape_COUNT];

static void createBlockMask(BlockShape shape, bool (*proc)(int x, int y))
{
	BlockMask & mask = s_blockMasks[shape];
	memset(&mask, 0, sizeof(mask));
	for (int y = 0; y < BLOCK_SY; ++y)
		for (int x = 0; x < BLOCK_SX; ++x)
			if (proc(x, y))
				mask.set(x, y);
}

static void initializeBlockMasks()
{
	static bool init = false;

	if (!init)
	{
		init = true;

		createBlockMask(kBlockShape_Empty,  [](int x, int y) { return false; });
		createBlockMask(kBlockShape_Opaque, [](int x, int y) { return true; });

		createBlockMask(kBlockShape_TL,     [](int x, int y) { return x + y < BLOCK_SX; });
		createBlockMask(kBlockShape_TR,     [](int x, int y) { return s_blockMasks[kBlockShape_TL].test(BLOCK_SX - 1 - x,                y); });
		createBlockMask(kBlockShape_BL,     [](int x, int y) { return s_blockMasks[kBlockShape_TL].test(               x, BLOCK_SY - 1 - y); });
		createBlockMask(kBlockShape_BR,     [](int x, int y) { return s_blockMasks[kBlockShape_TL].test(BLOCK_SX - 1 - x, BLOCK_SY - 1 - y); });

		createBlockMask(kBlockShape_TL2a,   [](int x, int y) { return x + y * 2 < BLOCK_SX * 2; });
		createBlockMask(kBlockShape_TL2b,   [](int x, int y) { return x + y * 2 < BLOCK_SX * 1; });
		createBlockMask(kBlockShape_TR2a,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2b].test(BLOCK_SX - 1 - x,                y); });
		createBlockMask(kBlockShape_TR2b,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2a].test(BLOCK_SX - 1 - x,                y); });
		createBlockMask(kBlockShape_BL2a,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2a].test(               x, BLOCK_SY - 1 - y); });
		createBlockMask(kBlockShape_BL2b,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2b].test(               x, BLOCK_SY - 1 - y); });
		createBlockMask(kBlockShape_BR2a,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2b].test(BLOCK_SX - 1 - x, BLOCK_SY - 1 - y); });
		createBlockMask(kBlockShape_BR2b,   [](int x, int y) { return s_blockMasks[kBlockShape_TL2a].test(BLOCK_SX - 1 - x, BLOCK_SY - 1 - y); });

		createBlockMask(kBlockShape_HT,     [](int x, int y) { return y < BLOCK_SY / 2; });
		createBlockMask(kBlockShape_HB,     [](int x, int y) { return s_blockMasks[kBlockShape_HT].test(x, BLOCK_SY - 1 - y); });

		// setup polygon shapes

		s_blockPolys[kBlockShape_Empty ].setEmpty();
		s_blockPolys[kBlockShape_Opaque].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f, BLOCK_SY    ));
		
		s_blockPolys[kBlockShape_TL    ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(0.f,      BLOCK_SY    )                         );
		s_blockPolys[kBlockShape_TR    ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY    )                         );
		s_blockPolys[kBlockShape_BL    ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f,      BLOCK_SY    )                         );
		s_blockPolys[kBlockShape_BR    ].set(Vec2(0.f, BLOCK_SY),     Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY    )                         );
		
		s_blockPolys[kBlockShape_TL2a  ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY/2.f), Vec2(0.f, BLOCK_SY    ));
		s_blockPolys[kBlockShape_TL2b  ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(0.f,      BLOCK_SY/2.f)                         );
		s_blockPolys[kBlockShape_TR2a  ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY/2.f)                         );
		s_blockPolys[kBlockShape_TR2b  ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f, BLOCK_SY/2.f));
		s_blockPolys[kBlockShape_BL2a  ].set(Vec2(0.f, 0.f),          Vec2(BLOCK_SX, BLOCK_SY/2.f), Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f, BLOCK_SY    ));
		s_blockPolys[kBlockShape_BL2b  ].set(Vec2(0.f, BLOCK_SY/2.f), Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f,      BLOCK_SY    )                         );
		s_blockPolys[kBlockShape_BR2a  ].set(Vec2(0.f, BLOCK_SY),     Vec2(BLOCK_SX, BLOCK_SY/2.f), Vec2(BLOCK_SX, BLOCK_SY    )                         );
		s_blockPolys[kBlockShape_BR2b  ].set(Vec2(0.f, BLOCK_SY/2.f), Vec2(BLOCK_SX, 0.f         ), Vec2(BLOCK_SX, BLOCK_SY    ), Vec2(0.f, BLOCK_SY    ));
		
		s_blockPolys[kBlockShape_HT    ].set(Vec2(0.f, 0.f), Vec2(BLOCK_SX, 0.f), Vec2(BLOCK_SX, BLOCK_SY/2.f), Vec2(0.f, BLOCK_SY/2.f));
		s_blockPolys[kBlockShape_HB    ].set(Vec2(0.f, BLOCK_SY/2.f), Vec2(BLOCK_SX, BLOCK_SY/2.f), Vec2(BLOCK_SX, BLOCK_SY), Vec2(0.f, BLOCK_SY));
	}
}

static struct Initializer
{
	Initializer()
	{
		initializeBlockMasks();
	}
} s_initializer;

OPTION_DECLARE(int, s_drawBlockMask, -1);
OPTION_DEFINE(int, s_drawBlockMask, "Arena/Debug/Draw Block Mask");

//

bool Block::handleDamage(GameSim & gameSim, int blockX, int blockY)
{
	bool result = false;

	if ((type == kBlockType_Destructible || (type == kBlockType_DestructibleRegen && shape != kBlockShape_Empty)))
	{
		if (type == kBlockType_DestructibleRegen)
		{
			shape = kBlockShape_Empty;
			RegenBlockData & data = (RegenBlockData&)param;
			data.isVisible = false;
			data.regenTime = BLOCKTYPE_REGEN_TIME * TICKS_PER_SECOND;
		}
		else
		{
			type = kBlockType_Empty;
		}

		gameSim.playSound("block-destroy.ogg");

		ParticleSpawnInfo spawnInfo(
			(blockX + .5f) * BLOCK_SX,
			(blockY + .5f) * BLOCK_SY,
			kBulletType_ParticleA, 10, 50, 200, 20);
		spawnInfo.color = 0xff8040ff;
		gameSim.spawnParticles(spawnInfo);

		result = true;
	}

	return result;
}

//

Arena::Arena()
	: m_sprites(0)
	, m_numSprites(0)
{
}

Arena::~Arena()
{
	reset();
}

void Arena::init()
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
			m_blocks[x][y].shape = kBlockShape_Opaque;
			m_blocks[x][y].type = kBlockType_Empty;
			m_blocks[x][y].artIndex[0] = -1;
			m_blocks[x][y].artIndex[1] = -1;
			m_blocks[x][y].param = 0;
		}
	}

	// clear sprites

	freeArt();
}

void Arena::generate()
{
	Assert(false);

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
}

void Arena::load(const char * name)
{
	reset();

	//

#if NEW_LEVEL_FORMAT
	m_name = name;

	loadArt(name);

	const std::string baseName = std::string("levels/") + name + "/";

	const std::string mecFilename = baseName + "Mec.txt";
	const std::string colFilename = baseName + "Col.txt";
	const std::string objFilename = baseName + "Obj.txt";
	const std::string artFilenameBG = baseName + "Art.txt";
	const std::string artFilenameFG = baseName + "ArtFG.txt";
#else
	std::string baseName = Path::GetBaseName(name);

	std::string techFilename = baseName + "Mec.txt";
	std::string maskFilename = baseName + "Col.txt";
	std::string artFilename = baseName + "Art.txt";
#endif

	// load block type layer

	try
	{
		FileStream stream;
		stream.Open(mecFilename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
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
				r = kBlockType_DestructibleRegen,
				x = kBlockType_Indestructible,
				v = kBlockType_Slide,
				    kBlockType_Moving,
				s = kBlockType_Sticky,
				q   kBlockType_Spike,
				p = kBlockType_Spawn,
				j = kBlockType_Spring,
				t = kBlockType_Teleport,
				u = kBlockType_GravityReverse,
				- = kBlockType_GravityDisable,
				d = kBlockType_GravityStrong,
				[ = kBlockType_GravityLeft,
				] = kBlockType_GravityRight,
				< = kBlockType_ConveyorBeltLeft,
				> = kBlockType_ConveyorBeltRight,
				i = kBlockType_Passthrough,
				a = kBlockType_Appear,
				*/

				BlockType type = kBlockType_Empty;

				switch (line[x])
				{
				case ' ': type = kBlockType_Empty; break;
				case 'c': type = kBlockType_Destructible; break;
				case 'r': type = kBlockType_DestructibleRegen; break;
				case 'x': type = kBlockType_Indestructible; break;
				case 'v': type = kBlockType_Slide; break;
				case 's': type = kBlockType_Sticky; break;
				case 'q': type = kBlockType_Spike; break;
				case 'p': type = kBlockType_Spawn; break;
				case 'j': type = kBlockType_Spring; break;
				case 't': type = kBlockType_Teleport; break;
				case 'u': type = kBlockType_GravityReverse; break;
				case '-': type = kBlockType_GravityDisable; break;
				case 'd': type = kBlockType_GravityStrong; break;
				case '[': type = kBlockType_GravityLeft; break;
				case ']': type = kBlockType_GravityRight; break;
				case '<': type = kBlockType_ConveyorBeltLeft; break;
				case '>': type = kBlockType_ConveyorBeltRight; break;
				case 'i': type = kBlockType_Passthrough; break;
				case 'a': type = kBlockType_Appear; break;
				default:
					LOG_WRN("invalid block type: '%c'", line[x]);
					Assert(false);
					break;
				}

				m_blocks[x][y].type = type;

				if (type == kBlockType_Appear)
				{
					AppearBlockData & blockData = (AppearBlockData&)m_blocks[x][y].param;
					blockData.isVisible = true;
					blockData.switchTime = 255;
				}

				if (type == kBlockType_DestructibleRegen)
				{
					RegenBlockData & blockData = (RegenBlockData&)m_blocks[x][y].param;
					blockData.isVisible = true;
					blockData.regenTime = 0;
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to open %s: %s", mecFilename.c_str(), e.what());
	}

	// load collision shape layer

	try
	{
		FileStream stream;
		stream.Open(colFilename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
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
					x = kBlockShape_Empty,
					  = kBlockShape_Opaque,
					q = kBlockShape_TL,
					w = kBlockShape_TR,
					a = kBlockShape_BL,
					s = kBlockShape_BR,
					e = kBlockShape_TL2a,
					r = kBlockShape_TL2b,
					t = kBlockShape_TR2a,
					y = kBlockShape_TR2b,
					d = kBlockShape_BL2a,
					f = kBlockShape_BL2b,
					g = kBlockShape_BR2a,
					h = kBlockShape_BR2b,
					c = kBlockShape_HT,
					v = kBlockShape_HB,
				*/

				BlockShape shape = kBlockShape_Opaque;

				switch (line[x])
				{
				case 'x': shape = kBlockShape_Empty; break;
				case ' ': shape = kBlockShape_Opaque; break;
				case 'q': shape = kBlockShape_TL; break;
				case 'w': shape = kBlockShape_TR; break;
				case 'a': shape = kBlockShape_BL; break;
				case 's': shape = kBlockShape_BR; break;
				case 'e': shape = kBlockShape_TL2a; break;
				case 'r': shape = kBlockShape_TL2b; break;
				case 't': shape = kBlockShape_TR2a; break;
				case 'y': shape = kBlockShape_TR2b; break;
				case 'd': shape = kBlockShape_BL2a; break;
				case 'f': shape = kBlockShape_BL2b; break;
				case 'g': shape = kBlockShape_BR2a; break;
				case 'h': shape = kBlockShape_BR2b; break;
				case 'c': shape = kBlockShape_HT; break;
				case 'v': shape = kBlockShape_HB; break;

				case '.':
					break;
				default:
					LOG_WRN("invalid shape type: '%c'", line[x]);
					Assert(false);
					break;
				}

				m_blocks[x][y].shape = shape;
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to open %s: %s", colFilename.c_str(), e.what());
	}

#if NEW_LEVEL_FORMAT
	// load art layer

	loadArtIndices(artFilenameBG.c_str(), 0);
	loadArtIndices(artFilenameFG.c_str(), 1);
#endif
}

void Arena::loadArt(const char * name)
{
	freeArt();

	const std::string baseName = std::string("levels/") + name + "/";
	const std::string artFileName = baseName + "ArtIndex.txt";
	const std::string spriteBaseName = std::string("levels/") + name + "/";

	try
	{
		FileStream stream(artFileName.c_str(), OpenMode_Read);
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();

		m_sprites = new Sprite*[lines.size()];
		m_numSprites = (int)lines.size();

		for (size_t i = 0; i < lines.size(); ++i)
		{
			const std::string spriteFilename = spriteBaseName + lines[i];
			m_sprites[i] = new Sprite(spriteFilename.c_str());
		}
	}
	catch (std::exception & e)
	{
		logError("failed to load art index for %s: %s", name, e.what());
	}
}

void Arena::freeArt()
{
	for (int i = 0; i < m_numSprites; ++i)
	{
		delete m_sprites[i];
		m_sprites[i] = 0;
	}

	delete [] m_sprites;
	m_sprites = 0;
	m_numSprites = 0;
}

void Arena::loadArtIndices(const char * filename, int layer)
{
	try
	{
		FileStream stream;
		stream.Open(filename, OpenMode_Read);
		StreamReader reader(&stream, false);
		
		const int sx = reader.ReadInt32();
		const int sy = reader.ReadInt32();

		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				const int index = reader.ReadInt32();
				Assert(index == -1 || (index >= 0 && index < m_numSprites));

				if (x < ARENA_SX && y < ARENA_SY)
				{
					if (index == -1 || (index >= 0 && index < m_numSprites))
					{
						m_blocks[x][y].artIndex[layer] = index;
					}
					else
					{
						logDebug("art tile at (%d, %d) is out of range. layer=%d, index=%d", x, y, layer, index);
					}
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to open %s: %s", filename, e.what());
	}
}

void Arena::serialize(NetSerializationContext & context)
{
	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			Block & block = m_blocks[x][y];

			uint8_t type = block.type;
			uint8_t shape = block.shape;

			context.SerializeBits(type, 5);
			context.SerializeBits(shape, 5);
			context.Serialize(block.artIndex[0]);
			context.Serialize(block.artIndex[1]);
			context.Serialize(block.param);

			block.type = (BlockType)type;
			block.shape = (BlockShape)shape;
		}
	}

	if (context.IsSend())
	{
		std::string name = m_name.c_str();
		context.Serialize(name);
	}
	else
	{
		std::string name;
		context.Serialize(name);
		m_name = name.c_str();

		loadArt(m_name.c_str());
	}
}

#if ENABLE_GAMESTATE_DESYNC_DETECTION
uint32_t Arena::calcCRC() const
{
	uint32_t result = 0;

	const uint8_t * bytes = (const uint8_t*)m_blocks;
	const uint32_t numBytes = sizeof(m_blocks);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	return result;
}
#endif

void Arena::drawBlocks(int layer) const
{
	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			const Block & block = m_blocks[x][y];

			if (layer == 0 && block.type == kBlockType_Appear)
			{
				const AppearBlockData& data = (AppearBlockData&)block.param;

				if (data.isVisible && data.switchTime < 20)
					setColor(255, 255, 255, 255 - (255 - data.switchTime * 12.75));
				else if (!data.isVisible)
				{
					if (data.switchTime < 20)
						setColor(255, 255, 255, 2.55 * (100 - data.switchTime * 5));
					else
						continue;
				}
			}
			else if (layer == 0 && block.type == kBlockType_DestructibleRegen)
			{
				const RegenBlockData & data = (RegenBlockData&)block.param;
				if (!data.isVisible)
					continue;
				setColor(colorWhite);
			}
			else
			{
				setColor(255, 255, 255, 255);
			}

		#if NEW_LEVEL_FORMAT
			if (block.artIndex[layer] != (uint16_t)-1)
				m_sprites[block.artIndex[layer]]->drawEx(x * BLOCK_SX, y * BLOCK_SY, 0.f, BLOCK_SPRITE_SCALE);
		#else
			s_sprites[block.type]->drawEx(x * BLOCK_SX, y * BLOCK_SY, 0.f, BLOCK_SPRITE_SCALE);
		#endif
		}
	}

	if (layer == 0 && s_drawBlockMask >= 0 && s_drawBlockMask < kBlockShape_COUNT)
	{
		const BlockMask & mask = s_blockMasks[s_drawBlockMask];
		const int scale = 8;

		setColor(colorWhite);
		for (int x = 0; x < BLOCK_SX; ++x)
		{
			for (int y = 0; y < BLOCK_SY; ++y)
			{
				if (mask.test(x, y))
				{
					drawRect(
						(x + 0) * scale,
						(y + 0) * scale,
						(x + 1) * scale,
						(y + 1) * scale);
				}
			}
		}

		const CollisionShape & shape = s_blockPolys[s_drawBlockMask];
		setColor(colorRed);
		gxPushMatrix();
		gxScalef(scale, scale, 1.f);
		shape.debugDraw();
		gxPopMatrix();
		setColor(colorWhite);
	}
}

void Arena::tick(GameSim & gameSim)
{
	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			Block & block = m_blocks[x][y];

			if (block.type == kBlockType_Appear)
			{
				AppearBlockData& data = (AppearBlockData&)block.param;

				if (data.switchTime == 0)
				{
					if (data.isVisible)
					{
						data.isVisible = 0;
						block.shape = kBlockShape_Empty;
					}
					else
					{
						data.isVisible = 1;
						block.shape = kBlockShape_Opaque;
					}

					data.switchTime = 255;
				}
				else
					data.switchTime--;
			}
			else if (block.type == kBlockType_DestructibleRegen)
			{
				RegenBlockData & data = (RegenBlockData&)block.param;

				if (!data.isVisible)
				{
					if (data.regenTime != 0)
					{
						data.regenTime--;

						if (data.regenTime == 0)
						{
							data.isVisible = true;
							block.shape = kBlockShape_Opaque;
							block.type = kBlockType_COUNT;

							CollisionInfo blockCollision;
							blockCollision.min = Vec2((x + 0) * BLOCK_SX, (y + 0) * BLOCK_SY);
							blockCollision.max = Vec2((x + 1) * BLOCK_SX, (y + 1) * BLOCK_SY);

							for (int i = 0; i < MAX_PLAYERS; ++i)
							{
								const Player & player = gameSim.m_players[i];

								if (!player.m_isUsed || !player.m_isAlive)
									continue;

								CollisionInfo playerCollision;
								if (player.getPlayerCollision(playerCollision))
								{
									if (blockCollision.intersects(playerCollision))
									{
										data.regenTime = 1;
										data.isVisible = false;
										block.shape = kBlockShape_Empty;
										break;
									}
								}
							}

							if (data.regenTime == 0)
							{
								gameSim.playSound("block-regen.ogg");
							}

							block.type = kBlockType_DestructibleRegen;
						}
					}
				}
			}
		}
	}
}

bool Arena::getRandomSpawnPoint(GameSim & gameSim, int & out_x, int & out_y, int & io_lastSpawnIndex, Player * playerToIgnore) const
{
	// find a spawn point

	const int kMaxCandidates = 8;
	struct Candidate
	{
		int blockX;
		int blockY;
		float minDistanceToPlayer;
	} candidates[kMaxCandidates];
	int numCandidates = 0;

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			if (m_blocks[x][y].type == kBlockType_Spawn)
			{
				candidates[numCandidates].blockX = x;
				candidates[numCandidates].blockY = y;
				candidates[numCandidates].minDistanceToPlayer = 1000000.f;

				for (int p = 0; p < MAX_PLAYERS; ++p)
				{
					if (gameSim.m_playerInstanceDatas[p])
					{
						Player * player = gameSim.m_playerInstanceDatas[p]->m_player;

						if (player != playerToIgnore)
						{
							const float px = (x + .5f) * BLOCK_SX;
							const float py = (y + .5f) * BLOCK_SY;
							const float dx = px - player->m_pos[0];
							const float dy = py - player->m_pos[1];
							const float d = std::sqrt(dx * dx + dy * dy);
							if (d < candidates[numCandidates].minDistanceToPlayer)
								candidates[numCandidates].minDistanceToPlayer = d;
						}
					}
				}

				numCandidates++;
			}
		}
	}

	if (numCandidates == 0)
		return false;
	else
	{
		std::sort(candidates, candidates + numCandidates, [](const Candidate & c1, const Candidate & c2)
		{
			if (c1.minDistanceToPlayer != c2.minDistanceToPlayer)
				return c1.minDistanceToPlayer > c2.minDistanceToPlayer;
			if (c1.blockX != c2.blockX)
				return c1.blockX < c2.blockX;
			return c1.blockY > c2.blockY;
		});

		for (int index = 0; index < numCandidates; ++index)
		{
			bool accept = (io_lastSpawnIndex == -1 || numCandidates == 1 || index != io_lastSpawnIndex);

			if (accept)
			{
				out_x = candidates[index].blockX * BLOCK_SX;
				out_y = candidates[index].blockY * BLOCK_SY;

				out_x += BLOCK_SX / 2;
				out_y += BLOCK_SY - 1;

				io_lastSpawnIndex = index;

				return true;
			}
		}
	}

	return false;
}

bool Arena::isValidPickupLocation(int x, int y, bool grounded) const
{
	const int sx = (PICKUP_BLOCK_SX - 1) / 2;
	const int sy = (PICKUP_BLOCK_SY - 1) / 2;

	bool result = true;

	if (x < sx || y < sy || x >= ARENA_SX - sx || y >= ARENA_SY - sy - 1)
		result = false;
	else
	{
		for (int ox = -sx; ox <= +sx; ++ox)
			for (int oy = -sy; oy <= +sy; ++oy)
				result &= (m_blocks[x + ox][y + oy].type == kBlockType_Empty);

		if (grounded)
		{
			for (int ox = -sx; ox <= +sx; ++ox)
				result &= ((1 << m_blocks[x + ox][y + sy + 1].type) & kBlockMask_Solid) != 0;
		}
	}

	return result;
}

bool Arena::getRandomPickupLocations(int * out_x, int * out_y, int & numLocations, void * obj, bool (*reject)(void * obj, int x, int y)) const
{
	int numCandidates = 0;

	for (int x = 0; x < ARENA_SX; ++x)
	{
		for (int y = 0; y < ARENA_SY; ++y)
		{
			if (isValidPickupLocation(x, y, true))
			{
				if (!reject || !reject(obj, x, y))
				{
					if (numCandidates < numLocations)
					{
						out_x[numCandidates] = x;
						out_y[numCandidates] = y;
						numCandidates++;
					}
				}
			}
		}
	}

	if (numCandidates == 0)
		return false;

	numLocations = numCandidates;

	return true;
}

bool Arena::getTeleportDestination(GameSim & gameSim, int startX, int startY, int & out_x, int & out_y) const
{
	// find a teleport destination

	std::vector< std::pair<int, int> > destinations;

	for (int x = 0; x < ARENA_SX; ++x)
		for (int y = 0; y < ARENA_SY; ++y)
			if (x != startX && y != startY && m_blocks[x][y].type == kBlockType_Teleport)
				destinations.push_back(std::make_pair(x, y));

	if (destinations.empty())
	{
		LOG_WRN("unable to find teleport destination");

		return false;
	}
	else
	{
		if (DEBUG_RANDOM_CALLSITES)
			LOG_DBG("Random called from getTeleportDestination");
		const int idx = gameSim.Random() % destinations.size();

		out_x = std::get<0>(destinations[idx]);
		out_y = std::get<1>(destinations[idx]);

		return true;
	}
}

uint32_t Arena::getIntersectingBlocksMask(int x, int y) const
{
	uint32_t result = 0;

	if (x >= 0 && y >= 0)
	{
		const int blockX = x / BLOCK_SX;
		const int blockY = y / BLOCK_SY;

		if (blockX < ARENA_SX && blockY < ARENA_SY)
		{
			const int maskX = x % BLOCK_SX;
			const int maskY = y % BLOCK_SY;

			const Block & block = m_blocks[blockX][blockY];
			const BlockMask & mask = s_blockMasks[block.shape];

			if (mask.test(maskX, maskY))
			{
				result |= 1 << block.type;
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

bool Arena::getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2) const
{
	return
		getBlockStripFromPixels(x1, x2, out_x1, out_x2, ARENA_SX, BLOCK_SX) &&
		getBlockStripFromPixels(y1, y2, out_y1, out_y2, ARENA_SY, BLOCK_SY);
}

bool Arena::getBlocksFromPixels(int baseX, int baseY, int x1, int y1, int x2, int y2, bool wrap, BlockAndDistance * out_blocks, int & io_numBlocks)
{
	if (io_numBlocks == 0)
		return false;

	int result = 0;

	if (wrap)
	{
		for (int dx = -1; dx <= +1; ++dx)
		{
			for (int dy = -1; dy <= +1; ++dy)
			{
				int numBlocks = io_numBlocks - result;

				getBlocksFromPixels(
					baseX + dx * ARENA_SX_PIXELS,
					baseY + dy * ARENA_SY_PIXELS,
					x1 + dx * ARENA_SX_PIXELS,
					y1 + dy * ARENA_SY_PIXELS,
					x2 + dx * ARENA_SX_PIXELS,
					y2 + dy * ARENA_SY_PIXELS,
					false,
					out_blocks + result,
					numBlocks);

				result += numBlocks;
			}
		}
	}
	else
	{
		if (getBlockRectFromPixels(x1, y1, x2, y2, x1, y1, x2, y2))
		{
			for (int x = x1; x <= x2; ++x)
			{
				for (int y = y1; y <= y2; ++y)
				{
					if (result < io_numBlocks)
					{
						const float blockX = (x + .5f) * BLOCK_SX;
						const float blockY = (y + .5f) * BLOCK_SY;

						out_blocks[result].block = &getBlock(x, y);
						out_blocks[result].x = x;
						out_blocks[result].y = y;
						out_blocks[result].distanceSq = (baseX - blockX) * (baseX - blockX) + (baseY - blockY) * (baseY - blockY);

						result++;
					}
				}
			}
		}
	}

	io_numBlocks = result;

	return result != 0;
}

bool Arena::handleDamageRect(GameSim & gameSim, int baseX, int baseY, int x1, int y1, int x2, int y2, bool hitDestructible, bool hitSingleDestructible)
{
	bool result = false;

	const int kMaxBlocks = 64;

	BlockAndDistance blocks[kMaxBlocks];

	int numBlocks = kMaxBlocks;

	if (getBlocksFromPixels(
		baseX, baseY,
		x1, y1, x2, y2,
		true,
		blocks, numBlocks))
	{
		std::sort(blocks, blocks + numBlocks, [] (BlockAndDistance & block1, BlockAndDistance & block2) { return block1.distanceSq < block2.distanceSq; });

		for (int i = 0; i < numBlocks; ++i)
		{
			BlockAndDistance & blockInfo = blocks[i];

			Block & block = *blockInfo.block;

			if (!hitDestructible && (block.type == kBlockType_Destructible || block.type == kBlockType_DestructibleRegen))
				continue;

			if (block.handleDamage(gameSim, blockInfo.x, blockInfo.y))
			{
				if (hitSingleDestructible)
					hitDestructible = false;

				result = true;
			}
		}
	}

	return result;
}

bool Arena::handleDamageShape(GameSim & gameSim, int baseX, int baseY, const CollisionShape & shape, bool hitDestructible, bool hitSingleDestructible)
{
	bool result = false;

	Vec2 min;
	Vec2 max;
	shape.getMinMax(min, max);

	const int x1 = (int)std::floor(min[0]);
	const int y1 = (int)std::floor(min[1]);
	const int x2 = (int)std::ceil(max[0]);
	const int y2 = (int)std::ceil(max[1]);

	const int kMaxBlocks = 64;

	BlockAndDistance blocks[kMaxBlocks];

	int numBlocks = kMaxBlocks;

	if (getBlocksFromPixels(
		baseX, baseY,
		x1, y1, x2, y2,
		true,
		blocks, numBlocks))
	{
		std::sort(blocks, blocks + numBlocks, [] (BlockAndDistance & block1, BlockAndDistance & block2) { return block1.distanceSq < block2.distanceSq; });

		for (int i = 0; i < numBlocks; ++i)
		{
			BlockAndDistance & blockInfo = blocks[i];

			Block & block = *blockInfo.block;

			if (block.shape == kBlockShape_Empty)
				continue;

			CollisionShape blockCollision;
			getBlockCollision(block.shape, blockCollision, blockInfo.x, blockInfo.y);

			if (!blockCollision.intersects(shape))
				continue;

			if (!hitDestructible && (block.type == kBlockType_Destructible || block.type == kBlockType_DestructibleRegen))
				continue;

			if (block.handleDamage(gameSim, blockInfo.x, blockInfo.y))
			{
				if (hitSingleDestructible)
					hitDestructible = false;

				result = true;
			}
		}
	}

	return result;
}

const CollisionShape & Arena::getBlockCollision(BlockShape shape)
{
	return s_blockPolys[shape];
}

void Arena::getBlockCollision(BlockShape shape, CollisionShape & collisionShape, int blockX, int blockY)
{
	collisionShape = getBlockCollision(shape);
	collisionShape.translate(blockX * BLOCK_SX, blockY * BLOCK_SY);
}

void Arena::testCollision(const CollisionShape & shape, void * arg, CollisionCB cb)
{
	const int kMaxBlocks = 64;
	BlockAndDistance blocks[kMaxBlocks];
	int numBlocks = kMaxBlocks;

	Vec2 min;
	Vec2 max;
	shape.getMinMax(min, max);

	if (getBlocksFromPixels(
		0, 0,
		(int)min[0],
		(int)min[1],
		(int)max[0],
		(int)max[1],
		false,
		blocks,
		numBlocks))
	{
		for (int i = 0; i < numBlocks; ++i)
		{
			if (blocks[i].block->type == kBlockType_Empty)
				continue;
			if (blocks[i].block->shape == kBlockShape_Empty)
				continue;

			CollisionShape blockShape;
			Arena::getBlockCollision(blocks[i].block->shape, blockShape, blocks[i].x, blocks[i].y);

			if (shape.intersects(blockShape))
			{
				cb(shape, arg, 0, &blocks[i], 0);
			}
		}
	}
}

//

static const char * filenames[kBlockType_COUNT] =
{
	"block-empty.png",
	"block-destructible.png",
	"block-destructible.png",
	"block-indestructible.png",
	"block-slide.png",
	"block-moving.png",
	"block-sticky.png",
	"block-spike.png",
	"block-spawn.png",
	"block-spring.png",
	"block-teleport.png",
	"block-gravity-reverse.png",
	"block-gravity-disable.png",
	"block-gravity-strong.png",
	"block-gravity-left.png",
	"block-gravity-right.png",
	"block-conveyorbelt-left.png",
	"block-conveyorbelt-right.png",
	"block-passthrough.png",
	"block-appear.png"
};

void initArenaData()
{
	for (int i = 0; i < kBlockType_COUNT; ++i)
	{
		const char * filename = filenames[i];

		s_sprites[i] = new Sprite(filename);
	}
}

void shutArenaData()
{
	for (int i = 0; i < kBlockType_COUNT; ++i)
	{
		delete s_sprites[i];
		s_sprites[i] = 0;
	}
}
