#include "framework.h"

#include "gx_mesh.h"

#include "data/engine/ShaderCommon.txt"

class LodBuffers
{
public:
	class LodLevel;
	
	//          L2
	//          L1
	// L3 L2 L1 CT L1 L2 L3
	//          L1
	//          L2

	int m_powah;      // power of tile size
	int m_levelCount; // # LOD levels
	int m_size;       // # tiles
	int m_vbSize;     // vertex buffer size

	LodLevel * m_levels;
	
	class IB
	{
	public:
		int level;
		int size;
		GxIndexBuffer indexBuffer;
	};

	class LodLevel
	{
	public:
		int m_level;
		IB m_buffers[16]; // pre-computed index buffers, one for each combination of neighboring LOD levels

		static int sign(const int v)
		{
			return v < 0 ? -1 : v > 0 ? +1 : 0;
		}

		static int getBufferIndex(int dx1, int dy1, int dx2, int dy2)
		{
			dx1 = sign(dx1);
			dy1 = sign(dy1);
			dx2 = sign(dx2);
			dy2 = sign(dy2);

			if (dx1 < 0) dx1 = 0;
			if (dy1 < 0) dy1 = 0;
			if (dx2 < 0) dx2 = 0;
			if (dy2 < 0) dy2 = 0;

			const int index =
				(dx1 << 0) |
				(dy1 << 1) |
				(dx2 << 2) |
				(dy2 << 3);

			return index;
		}
	};

	LodBuffers()
	{
		setZero();
	}

	~LodBuffers()
	{
		free();
	}

	void setZero()
	{
		m_powah = 0;
		m_levelCount = 0;
		m_size = 0;
		m_vbSize = 0;
		
		m_levels = nullptr;
	}

	void alloc(int powah, int levelCount)
	{
		free();

		if (levelCount == 0)
			return;

		m_powah = powah;
		m_levelCount = levelCount;

		m_size = 1;
		for (int i = 0; i < m_powah; ++i)
			m_size *= 2;

		m_vbSize = m_size + 1;

		logDebug("Powah: %d.", m_powah);
		logDebug("LevelCount: %d.", m_levelCount);
		logDebug("Size: %dx%d.", m_size, m_size);

		allocLevels();
	}
	
	void free()
	{
		if (m_levels != nullptr)
		{
			delete [] m_levels;
			m_levels = nullptr;
		}

		setZero();
	}

	IB & getIB(int level, int levelX1, int levelY1, int levelX2, int levelY2)
	{
		#define CLAMP_LEVEL(level) \
			if (level >= m_levelCount) \
				level = m_levelCount - 1; \
			else if (level < 0) \
				level = 0

		CLAMP_LEVEL(level);
		CLAMP_LEVEL(levelX1);
		CLAMP_LEVEL(levelY1);
		CLAMP_LEVEL(levelX2);
		CLAMP_LEVEL(levelY2);
		
		#undef CLAMP_LEVEL

		const int dx1 = levelX1 - level;
		const int dy1 = levelY1 - level;
		const int dx2 = levelX2 - level;
		const int dy2 = levelY2 - level;

		const int index = LodLevel::getBufferIndex(dx1, dy1, dx2, dy2);

		return m_levels[level].m_buffers[index];
	}

private:
	void allocLevels()
	{
		m_levels = new LodLevel[m_levelCount];

		for (int level = 0; level < m_levelCount; ++level)
		{
			m_levels[level].m_level = 1;

			for (int dx1 = 0; dx1 < 2; ++dx1)
			{
				for (int dy1 = 0; dy1 < 2; ++dy1)
				{
					for (int dx2 = 0; dx2 < 2; ++dx2)
					{
						for (int dy2 = 0; dy2 < 2; ++dy2)
						{
							const int index = LodLevel::getBufferIndex(dx1, dy1, dx2, dy2);

							createIndexBuffer(
								m_levels[level].m_buffers[index],
								level,
								dx1, dy1,
								dx2, dy2);
						}
					}
				}
			}
		}
	}

	void createIndexBuffer(IB & ib, int level, int dx1, int dy1, int dx2, int dy2) const
	{
		ib.level = level;

		int size = m_size;

		for (int i = 0; i < level; ++i)
			size /= 2;

		ib.size = size;

		logDebug("Level: %d. Size: %d. DX1: %+d. DY1: %+d. DX2: %+d. D21: %+d.",
			ib.level,
			ib.size,
			dx1, dy1,
			dx2, dy2);

		const int indexCount = ib.size * ib.size * 2 * 3;

		int16_t * indices = (int16_t*)alloca(indexCount * sizeof(int16_t));
		
		const int vbSkip = m_size / ib.size;

		logDebug("IndexCount: %d. VBSkip: %d.", indexCount, vbSkip);

		#define VBINDEX(x, y) ((x) * vbSkip + (y) * vbSkip * m_vbSize)
		#define IBINDEX(x, y, tri, vert) (((x) + (y) * ib.size) * 2 * 3 + (tri) * 3 + (vert))

		for (int x = 0; x < ib.size; ++x)
		{
			for (int y = 0; y < ib.size; ++y)
			{
				const int index1 = IBINDEX(x, y, 0, 0);
				const int index2 = IBINDEX(x, y, 0, 1);
				const int index3 = IBINDEX(x, y, 0, 2);

				const int index4 = IBINDEX(x, y, 1, 0);
				const int index5 = IBINDEX(x, y, 1, 1);
				const int index6 = IBINDEX(x, y, 1, 2);

				indices[index1] = VBINDEX(x + 0, y + 0);
				indices[index2] = VBINDEX(x + 1, y + 0);
				indices[index3] = VBINDEX(x + 1, y + 1);

				indices[index4] = VBINDEX(x + 1, y + 1);
				indices[index5] = VBINDEX(x + 0, y + 1);
				indices[index6] = VBINDEX(x + 0, y + 0);

				//logDebug("Index: %d.", index);
			}
		}

		// Do stitching.
		if (dx1 != 0)
		{
			// Stitch left side.
			for (int y = 0; y + 1 < ib.size; y += 2)
			{
				const int vbIndex = VBINDEX(0, y + 2);
				int ibIndex;

				ibIndex = IBINDEX(0, y + 0, 1, 1);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(0, y + 1, 0, 0);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(0, y + 1, 1, 2);
				indices[ibIndex] = vbIndex;
			}
		}
		// Do stitching.
		if (dy1 != 0)
		{
			// Stitch top side.
			for (int x = 0; x + 1 < ib.size; x += 2)
			{
				const int vbIndex = VBINDEX(x + 2, 0);
				int ibIndex;

				ibIndex = IBINDEX(x + 0, 0, 0, 1);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(x + 1, 0, 0, 0);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(x + 1, 0, 1, 2);
				indices[ibIndex] = vbIndex;
			}
		}
		// Do stitching.
		if (dx2 != 0)
		{
			// Stitch right side.
			for (int y = 0; y + 1 < ib.size; y += 2)
			{
				const int vbIndex = VBINDEX(ib.size, y + 0);
				int ibIndex;

				ibIndex = IBINDEX(ib.size - 1, y + 0, 0, 2);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(ib.size - 1, y + 0, 1, 0);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(ib.size - 1, y + 1, 0, 1);
				indices[ibIndex] = vbIndex;
			}
		}
		// Do stitching.
		if (dy2 != 0)
		{
			// Stitch bottom side.
			for (int x = 0; x + 1 < ib.size; x += 2)
			{
				const int vbIndex = VBINDEX(x + 0, ib.size);
				int ibIndex;

				ibIndex = IBINDEX(x + 0, ib.size - 1, 0, 2);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(x + 0, ib.size - 1, 1, 0);
				indices[ibIndex] = vbIndex;
				ibIndex = IBINDEX(x + 1, ib.size - 1, 1, 1);
				indices[ibIndex] = vbIndex;
			}
		}
		
		ib.indexBuffer.alloc(indices, indexCount, GX_INDEX_16);

		#undef VBINDEX
		#undef IBINDEX
	}
};

class Tile
{
public:
	int m_vbSize;
	GxVertexBuffer m_vb;
	int m_level;
	
	Tile()
	{
		setZero();
	}
	
	~Tile()
	{
		free();
	}
	
	void free()
	{
		m_vb.free();
		
		setZero();
	}
	
	void setZero()
	{
		m_vbSize = 0;
		m_level = 0;
	}
	
	void initialize(int vbSize, int tileX, int tileY)
	{
		free();
		
		m_vbSize = vbSize;

		Vec3 * positions = (Vec3*)alloca(vbSize * vbSize * sizeof(Vec3));

		const int sign = (rand() % 3) - 1;

		for (int x = 0; x < vbSize; ++x)
		{
			for (int y = 0; y < vbSize; ++y)
			{
				const float tX = x / (vbSize - 1.0f);
				const float tY = y / (vbSize - 1.0f);
				
				const float worldX = (tX + tileX) * vbSize;
				const float worldY = (tY + tileY) * vbSize;

				// -- calculate terrain height
				
				float phase = ((worldX - vbSize / 2.0f) + (worldY - vbSize / 2.0f)) / vbSize;
				//float phase = worldX / vbSize * 1.3f;
				//float phase = worldY / vbSize * 1.3f;

				float height = sinf(phase * 2.0f * M_PI) * 10.0f;
				//float height = 0.0f;

				// -- add some additional humps along the surface
				
				float dx = fabsf((tX - 0.5f) * 2.0f);
				float dy = fabsf((tY - 0.5f) * 2.0f);

				float extra = fmaxf(0.f, 1.0f - (dx * dx + dy * dy));
				//float extra = 0.0f;
				
				height += extra * 20.0f * sign;
				
				// -- store position
				
				const int index = x + y * vbSize;

				positions[index].Set(
					tX * vbSize,
					tY * vbSize,
					height);
			}
		}
		
		m_vb.alloc(positions, vbSize * vbSize * sizeof(Vec3));
	}
};

class Map
{
public:
	Tile * m_tiles;
	int m_sizeX;
	int m_sizeY;
	int m_vbSize;
	int m_centerX;
	int m_centerY;
	
	Map()
	{
		setZero();
	}

	~Map()
	{
		free();
	}

	void setZero()
	{
		m_tiles = nullptr;
		m_sizeX = 0;
		m_sizeY = 0;
		m_vbSize = 0;
		m_centerX = 0;
		m_centerY = 0;
	}

	void alloc(int sizeX, int sizeY, int vbSize)
	{
		free();

		if (sizeX == 0 || sizeY == 0)
			return;

		m_sizeX = sizeX;
		m_sizeY = sizeY;
		m_vbSize = vbSize;

		m_tiles = new Tile[sizeX * sizeY];

		for (int x = 0; x < sizeX; ++x)
			for (int y = 0; y < sizeY; ++y)
				m_tiles[x + y * sizeX].initialize(vbSize, x, y);
	}
	
	void free()
	{
		if (m_tiles != nullptr)
		{
			delete [] m_tiles;
			m_tiles = nullptr;
		}

		setZero();
	}

	void worldToTile(float x, float y, int & out_x, int & out_y) const
	{
		out_x = int(floorf(x / float(m_vbSize)));
		out_y = int(floorf(y / float(m_vbSize)));
	}

	void setCenter(int x, int y)
	{
		m_centerX = x;
		m_centerY = y;

		calculateLodLevels();
	}

	int getLevel(int x, int y) const
	{
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= m_sizeX) x = m_sizeX - 1;
		if (y >= m_sizeY) y = m_sizeY - 1;

		return m_tiles[x + y * m_sizeX].m_level;
	}

	void calculateLodLevels()
	{
		for (int x = 0; x < m_sizeX; ++x)
		{
			for (int y = 0; y < m_sizeY; ++y)
			{
				int dx = x - m_centerX;
				int dy = y - m_centerY;

			#if 1
				int level = 0;

				dx = abs(dx);
				dy = abs(dy);
				
				if (dx > dy)
					level = dx;
				else
					level = dy;
			#elif 1
				int level = sqrt(dx * dx + dy * dy);
			#else
				int level = abs(dx) + abs(dy);
			#endif

				m_tiles[x + y * m_sizeX].m_level = level;
			}
		}
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.init(800, 600);

	const int size = 64;
	const int vbSize = size + 1;

	int powah = 0;
	int temp = 1;

	while (temp != size)
	{
		powah += 1;
		temp *= 2;
	}

	const int levelCount = 3;

	logDebug("Size: %d. VB size: %d. Powah: %d. LevelCount: %d", size, vbSize, powah, levelCount);

// todo : change LOD index buffer so they don't need duplicate vertices
//        inside tiles. this will require usage of a single vertex buffer
//        for the entire terrain (instead of per-tile) and LodBuffers will
//        need to know this width of the terrain so it can use the correct
//        stride

	LodBuffers buffers;

	buffers.alloc(powah, levelCount);

	Map map;
	map.alloc(5, 5, vbSize);

	for (int x = 0; x < map.m_sizeX; ++x)
		for (int y = 0; y < map.m_sizeY; ++y)
			map.m_tiles[x + y * map.m_sizeX].m_level = (abs(x - (map.m_sizeX - 1) / 2) + abs(y - 2));

	const float min[2] = { 1.0f, 1.0f };
	const float max[2] = { map.m_sizeX * map.m_vbSize - 1.0f, map.m_sizeY * map.m_vbSize - 1.0f };
	
	float position[2] = { min[0], min[1] };
	float speed[2] = { 0.041f, 0.013f };

	Camera3d camera;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(100, 100, 100, 0);
		{
			projectPerspective3d(79.f, .01f, 100.f);
			camera.pushViewMatrix();

			pushWireframe(true);
			
			for (int i = 0; i < 2; ++i)
			{
				float newPosition = position[i] + speed[i] * 2.0f;

				if (newPosition < min[i] || newPosition > max[i])
					speed[i] *= -1;
				else
					position[i] = newPosition;
			}

			int mapX;
			int mapY;

			map.worldToTile(position[0], position[1], mapX, mapY);

			//logDebug("Position: (%d, %d).", mapX, mapY);

			map.setCenter(mapX, mapY);

			for (int x = 0; x < map.m_sizeX; ++x)
			{
				for (int y = 0; y < map.m_sizeY; ++y)
				{
					gxPushMatrix();
					{
						gxScalef(
							1.0f / vbSize,
							1.0f / vbSize,
							1.0f / vbSize);


						const float offsetX = map.m_sizeX / 2.0f;
						const float offsetY = map.m_sizeY / 2.0f;
						
						gxTranslatef(
							(x - offsetX) * vbSize,
							(y - offsetY) * vbSize,
							0.0f);

						const GxVertexInput vsInputs[1] =
						{
							{ VS_POSITION, 3, GX_ELEMENT_FLOAT32, false, 0, sizeof(Vec3) }
						};
						
						gxSetVertexBuffer(
							&map.m_tiles[x + y * map.m_sizeX].m_vb,
							vsInputs,
							sizeof(vsInputs) / sizeof(vsInputs[0]));

						const GxIndexBuffer * indexBuffer = &buffers.getIB(
							map.getLevel(x    , y    ),
							map.getLevel(x - 1, y    ),
							map.getLevel(x    , y - 1),
							map.getLevel(x + 1, y    ),
							map.getLevel(x    , y + 1)).indexBuffer;

					// todo : use vertex offset and use a single vertex buffer
						gxDrawIndexedPrimitives(
							GX_TRIANGLES,
							0,
							indexBuffer->getNumIndices(),
							indexBuffer);
					}
					gxPopMatrix();
				}
			}
			
			popWireframe();
			
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
