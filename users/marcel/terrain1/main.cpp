#include "framework.h"

#include "gx_mesh.h"

#include "data/engine/ShaderCommon.txt"

namespace
{
	int Sign(int v)
	{
		int result = 0;

		if (v < 0)
			result = -1;
		if (v > 0)
			result = +1;

		return result;
	}

	class LodBuffers
	{
	public:
		typedef struct
		{
			int level;
			int size;
			GxIndexBuffer indexBuffer;
		} IB;

		class LodLevel
		{
		public:
			int m_level;
			IB m_buffers[16];

			static int GetBufferIndex(int dx1, int dy1, int dx2, int dy2)
			{
				dx1 = Sign(dx1);
				dy1 = Sign(dy1);
				dx2 = Sign(dx2);
				dy2 = Sign(dy2);

				if (dx1 < 0) dx1 = 0;
				if (dy1 < 0) dy1 = 0;
				if (dx2 < 0) dx2 = 0;
				if (dy2 < 0) dy2 = 0;

				const int index = (dx1 << 0) | (dy1 << 1) | (dx2 << 2) | (dy2 << 3);

				return index;
			}
		};

		LodBuffers()
		{
			Zero();
		}

		~LodBuffers()
		{
			Initialize(0, 0);
		}

		void Zero()
		{
			m_levels = 0;
			m_powah = 0;
			m_levelCount = 0;
			m_size = 0;
			m_radius = 0;
		}

		void Initialize(int powah, int levelCount)
		{
			if (m_levels)
				delete[] m_levels;

			Zero();

			if (levelCount == 0)
				return;

			m_powah = powah;
			m_levelCount = levelCount;

			m_size = 1;
			for (int i = 0; i < m_powah; ++i)
				m_size *= 2;

			m_vbSize = m_size + 1;

			m_radius = m_levelCount * 2 + 1;

			logDebug("Powah: %d.", m_powah);
			logDebug("LevelCount: %d.", m_levelCount);
			logDebug("Size: %dx%d.", m_size, m_size);
			logDebug("Radius: %d.", m_radius);

			AllocateLevels();
		}

		IB& GetIB(int level, int levelX1, int levelY1, int levelX2, int levelY2)
		{
			#define CLAMP_LEVEL(level) \
				if (level >= m_levelCount) level = m_levelCount - 1; else if (level < 0) level = 0

			CLAMP_LEVEL(level);
			CLAMP_LEVEL(levelX1);
			CLAMP_LEVEL(levelY1);
			CLAMP_LEVEL(levelX2);
			CLAMP_LEVEL(levelY2);

			const int dx1 = levelX1 - level;
			const int dy1 = levelY1 - level;
			const int dx2 = levelX2 - level;
			const int dy2 = levelY2 - level;

			const int index = LodLevel::GetBufferIndex(dx1, dy1, dx2, dy2);

			return m_levels[level].m_buffers[index];
		}

		//          L2
		//          L1
		// L3 L2 L1 CT L1 L2 L3
		//          L1
		//          L2

		int m_powah; // Power of tile size.
		int m_levelCount; // # LOD levels.
		int m_size; // # tiles
		int m_vbSize;
		int m_radius; // max H/V distance

		LodLevel* m_levels;

	private:
		void AllocateLevels()
		{
			m_levels = new LodLevel[m_levelCount];

			for (int level = 0; level < m_levelCount; ++level)
			{
				m_levels[level].m_level = 1;

				for (int dx1 = 0; dx1 < 2; ++dx1)
					for (int dy1 = 0; dy1 < 2; ++dy1)
						for (int dx2 = 0; dx2 < 2; ++dx2)
							for (int dy2 = 0; dy2 < 2; ++dy2)
							{
								const int index = LodLevel::GetBufferIndex(dx1, dy1, dx2, dy2);

								CreateIndexBuffer(
									m_levels[level].m_buffers[index],
									level,
									dx1, dy1,
									dx2, dy2);
							}
			}
		}

		void CreateIndexBuffer(IB& ib, int level, int dx1, int dy1, int dx2, int dy2)
		{
			ib.level = level;

			int size = m_size;

			for (int i = 0; i < level; ++i)
				size /= 2;

			ib.size = size;

			logDebug("Level: %d. Size: %d. DX1: %+d. DY1: %+d. DX2: %+d. D21: %+d.", ib.level, ib.size, dx1, dy1, dx2, dy2);

			int indexCount = ib.size * ib.size * 2 * 3;

			int16_t* indices = (int16_t*)alloca(indexCount * sizeof(int16_t));
			
			int vbSkip = m_size / ib.size;

			logDebug("IndexCount: %d. VBSkip: %d.", indexCount, vbSkip);

			#define VBINDEX(x, y) (x) * vbSkip + (y) * vbSkip * m_vbSize;
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
				// Stitch left side.
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
				// Stitch left side.
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
				// Stitch left side.
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
		void Initialize(int vbSize)
		{
			m_vbSize = vbSize;

			Vec3 * positions = (Vec3*)alloca(vbSize * vbSize * sizeof(Vec3));

			int sign = (rand() % 3) - 1;

			for (int x = 0; x < vbSize; ++x)
			{
				for (int y = 0; y < vbSize; ++y)
				{
					int index = x + y * vbSize;

					//float phase = (x - vbSize / 2 + y - vbSize / 2) * 0.1f;
					float phase = x / float(vbSize - 1);

					float height = sin(phase * 2.0f * M_PI) * 10.0f;
					//float height = 0.0f;

					float dx = fabs((x / float(vbSize - 1) - 0.5f) * 2.0f);
					float dy = fabs((y / float(vbSize - 1) - 0.5f) * 2.0f);

					float extra = 1.0f - (dx * dx + dy * dy);

					if (extra < 0.0f)
						extra = 0.0f;

					height += extra * 20.0f * sign;

					positions[index].Set(x * vbSize / (vbSize - 1.0f), y * vbSize / (vbSize - 1.0f), height);
				}
			}
			
			m_vb.alloc(positions, vbSize * vbSize * sizeof(Vec3));
		}

		int m_vbSize;
		GxVertexBuffer m_vb;
		int m_level;
	};

	class Map
	{
	public:
		Map()
		{
			Zero();
		}

		~Map()
		{
			Initialize(0, 0, 0);
		}

		void Zero()
		{
			m_tiles = 0;
			m_sizeX = 0;
			m_sizeY = 0;
			m_vbSize = 0;
			m_centerX = 0;
			m_centerY = 0;
		}

		void Initialize(int sizeX, int sizeY, int vbSize)
		{
			if (m_tiles)
				delete[] m_tiles;

			Zero();

			if (sizeX == 0 || sizeY == 0)
				return;

			m_sizeX = sizeX;
			m_sizeY = sizeY;
			m_vbSize = vbSize;

			m_tiles = new Tile[sizeX * sizeY];

			for (int i = 0; i < sizeX * sizeY; ++i)
				m_tiles[i].Initialize(vbSize);
		}

		void WorldToTile(float x, float z, int& out_x, int& out_y)
		{
			out_x = int(floor(x / float(m_vbSize)));
			out_y = int(floor(z / float(m_vbSize)));
		}

		void SetCenter(int x, int y)
		{
			m_centerX = x;
			m_centerY = y;

			CalculateLodLevels();
		}

		int GetLevel(int x, int y)
		{
			if (x < 0) x = 0;
			if (y < 0) y = 0;
			if (x >= m_sizeX) x = m_sizeX - 1;
			if (y >= m_sizeY) y = m_sizeY - 1;

			return m_tiles[x + y * m_sizeX].m_level;
		}

		void CalculateLodLevels()
		{
			for (int x = 0; x < m_sizeX; ++x)
				for (int y = 0; y < m_sizeY; ++y)
				{
					int dx = x - m_centerX;
					int dy = y - m_centerY;

					int level = 0;

					dx = abs(dx);
					dy = abs(dy);
					
					if (dx > dy)
						level = dx;
					else
						level = dy;

					//int level = abs(dx) + abs(dy);

					m_tiles[x + y * m_sizeX].m_level = level;
				}
		}

		Tile* m_tiles;
		int m_sizeX;
		int m_sizeY;
		int m_vbSize;
		int m_centerX;
		int m_centerY;
	};
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

	LodBuffers buffers;

	buffers.Initialize(powah, levelCount);

	Map map;
	map.Initialize(5, 5, vbSize);

	for (int x = 0; x < map.m_sizeX; ++x)
		for (int y = 0; y < map.m_sizeY; ++y)
			map.m_tiles[x + y * map.m_sizeX].m_level = (abs(x - (map.m_sizeX - 1) / 2) + abs(y - 2));

	float min[2] = { 1.0f, 1.0f };
	float max[2] = { map.m_sizeX * map.m_vbSize - 1.0f, map.m_sizeY * map.m_vbSize - 1.0f };
	float position[2] = { min[0], min[1] };
	float speed[2] = { 1.0f, 0.3f };

	Camera3d camera;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(100, 0, 0, 0);
		{
			projectPerspective3d(79.f, .01f, 100.f);
			camera.pushViewMatrix();
		
			float scale = 0.0075f;
			//float scale = 0.01f;

			Mat4x4 matScale;
			matScale.MakeScaling(Vec3(scale, scale, scale));

			Mat4x4 matRot;
			//matRot.MakeRotationY(timer.GetTime());
			//matRot.MakeRotationEuler(Vec3(sin(framework.time) * 0.3f, 0.0f, framework.time * 0.1f));
			matRot.MakeIdentity();

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

			map.WorldToTile(position[0], position[1], mapX, mapY);

			//logDebug("Position: (%d, %d).", mapX, mapY);

			map.SetCenter(mapX, mapY);

			for (int x = 0; x < map.m_sizeX; ++x)
			{
				for (int y = 0; y < map.m_sizeY; ++y)
				{
					const GxVertexInput vsInputs[1] =
					{
						{ VS_POSITION, 3, GX_ELEMENT_FLOAT32, false, 0, sizeof(Vec3) }
					};
					
					gxSetVertexBuffer(
						&map.m_tiles[x + y * map.m_sizeX].m_vb,
						vsInputs,
						sizeof(vsInputs) / sizeof(vsInputs[0]));

					const int offsetX = (map.m_sizeX - 1) / 2;
					const int offsetY = (map.m_sizeY - 1) / 2;

					Mat4x4 matMove;
					matMove.MakeTranslation(Vec3(((x - offsetX) - 0.5f) * vbSize, ((y - offsetY) - 0.5f) * vbSize, 0.0f));

					Mat4x4 mat = matRot * matScale * matMove;
					gxPushMatrix();
					gxMultMatrixf(mat.m_v);

					GxIndexBuffer* indexBuffer = &buffers.GetIB(
						map.GetLevel(x, y),
						map.GetLevel(x - 1, y),
						map.GetLevel(x, y - 1),
						map.GetLevel(x + 1, y),
						map.GetLevel(x, y + 1)).indexBuffer;

					gxDrawIndexedPrimitives(
						GX_TRIANGLES,
						0,
						indexBuffer->getNumIndices(),
						indexBuffer);
					
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
