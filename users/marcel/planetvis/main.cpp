#include "Calc.h"
#include "framework.h"
#include "Timer.h"

#define GFX_SX 1024
#define GFX_SY 1024

#define DO_VOXELTRACE 0
#define DO_DISTANCE_TEXTURE 0
#define DO_SPARSE_TEXTURE 0
#define DO_QUADTREE 1

#define INVERTED_DISTANCE_METRIC 0

#if defined(DEBUG) || 1
	#define _DS_SX 15
	#define _DS_SY 15
	#define _DS_SZ 15
#else
	#define _DS_SX 255
	#define _DS_SY 255
	#define _DS_SZ 255
#endif

#if 1
	#define VS_SX 1
	#define VS_SY 1
	#define VS_SZ 1
#else
	#define VS_SX 4
	#define VS_SY 4
	#define VS_SZ 1
#endif

#define MAX_ITERATIONS 20

struct AABB
{
	float min[3];
	float max[3];

	bool intersect(const float px, const float py, const float pz, const float rdx, const float rdy, const float rdz, float & t) const
	{
		float tmin = std::numeric_limits<float>().min();
		float tmax = std::numeric_limits<float>().max();

		const float p[3] = { px, py, pz };
		const float rd[3] = { rdx, rdy, rdz };

		for (int i = 0; i < 3; ++i)
		{
			const float t1 = (min[i] - p[i]) * rd[i];
			const float t2 = (max[i] - p[i]) * rd[i];

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));
		}

		if (tmax >= tmin)
		{
			t = tmin;

			return true;
		}
		else
		{
			return false;
		}
	}
};

template <int sx, int sy, int sz>
struct DataSet
{
	bool isSolid[sx][sy][sz];
};

struct Voxel
{
	float distance;
};

template <int sx, int sy, int sz>
struct VoxelSet
{
	int getSx() const { return sx; }
	int getSy() const { return sy; }
	int getSz() const { return sz; }

	float sampleDistance(const int x, const int y, const int z1, const int z2, const float zt) const
	{
		const float v1 = voxels[x][y][z1].distance;
		const float v2 = voxels[x][y][z2].distance;

		const float t1 = 1.f - zt;
		const float t2 =       zt;

		return v1 * t1 + v2 * t2;
	}

	float sampleDistance(const int x, const int y1, const int y2, const float yt, const int z1, const int z2, const float zt) const
	{
		const float v1 = sampleDistance(x, y1, z1, z2, zt);
		const float v2 = sampleDistance(x, y2, z1, z2, zt);

		const float t1 = 1.f - yt;
		const float t2 =       yt;

		return v1 * t1 + v2 * t2;
	}

	float sampleDistance(const float x, const float y, const float z) const
	{
		const int ix = int(std::floor(x));
		const int iy = int(std::floor(y));
		const int iz = int(std::floor(z));

		const int x1 = Calc::Clamp(ix + 0, 0, sx - 1);
		const int x2 = Calc::Clamp(ix + 1, 0, sx - 1);

		const int y1 = Calc::Clamp(iy + 0, 0, sy - 1);
		const int y2 = Calc::Clamp(iy + 1, 0, sy - 1);

		const int z1 = Calc::Clamp(iz + 0, 0, sz - 1);
		const int z2 = Calc::Clamp(iz + 1, 0, sz - 1);

		const float xt = x - ix;
		const float yt = y - iy;
		const float zt = z - iz;

		const float v1 = sampleDistance(x1, y1, y2, yt, z1, z2, zt);
		const float v2 = sampleDistance(x2, y1, y2, yt, z1, z2, zt);

		const float t1 = 1.f - xt;
		const float t2 =       xt;

		return v1 * t1 + v2 * t2;
	}

	bool trace(
		const float px, const float py, const float pz,
		const float dx, const float dy, const float dz,
		float & ox,
		float & oy,
		float & oz) const
	{
		const float treshold = .1f;

		float tx = px;
		float ty = py;
		float tz = pz;

		for (int i = 0; i < MAX_ITERATIONS; ++i)
		{
			const float distance = sampleDistance(tx, ty, tz);

			if (distance < treshold)
			{
				ox = tx;
				oy = ty;
				oz = tz;

				return true;
			}

			tx += dx * distance;
			ty += dy * distance;
			tz += dz * distance;
		}

		return false;
	}

	AABB aabb;
	Voxel voxels[sx][sy][sz];
};

struct VoxelSpace
{
	VoxelSet<_DS_SX, _DS_SY, _DS_SZ> sets[VS_SX][VS_SY][VS_SZ];

	bool trace(
		const float px, const float py, const float pz,
		const float dx, const float dy, const float dz,
		const float rdx, const float rdy, const float rdz,
		float & ox,
		float & oy,
		float & oz) const
	{
		bool result = false;

		float minDistance = std::numeric_limits<float>().max();

		for (int x = 0; x < VS_SX; ++x)
		{
			for (int y = 0; y < VS_SY; ++y)
			{
				for (int z = 0; z < VS_SZ; ++z)
				{
					const VoxelSet<_DS_SX, _DS_SY, _DS_SZ> & voxelSet = sets[x][y][z];
					
					float t;

					if (!voxelSet.aabb.intersect(px, py, pz, rdx, rdy, rdz, t))
						continue;

#if 0
					const float lx = px - DS_SX * x;
					const float ly = py - DS_SY * y;
					const float lz = pz - DS_SZ * z;
#else
					const float gx = px + dx * t;
					const float gy = py + dy * t;
					const float gz = pz + dz * t;

					const float lx = gx - _DS_SX * x;
					const float ly = gy - _DS_SY * y;
					const float lz = gz - _DS_SZ * z;
#endif

#if 0
					ox = gx;
					oy = gy;
					oz = gz;

					return true;
#endif

					float lox;
					float loy;
					float loz;

					if (voxelSet.trace(lx, ly, lz, dx, dy, dz, lox, loy, loz))
					{
						const float gox = lox + _DS_SX * x;
						const float goy = loy + _DS_SY * y;
						const float goz = loz + _DS_SZ * z;
						const float gos = std::sqrt(gox * gox + goy * goy + goz * goz);

						if (gos < minDistance)
						{
							minDistance = gos;

							ox = gox;
							oy = goy;
							oz = goz;

							result = true;
						}
					}
				}
			}
		}

		return true;
	}
};

template <int sx, int sy, int sz>
static void createDataSet(DataSet<sx, sy, sz> & dataSet)
{
	const float mx = (sx - 1) / 2.f;
	const float my = (sy - 1) / 2.f;
	const float mz = (sz - 1) / 2.f;

	const float md = std::min(mx, std::min(my, mz));
	const float radius = md * (4.f / 5.f);

	for (int x = 0; x < sx; ++x)
	{
		for (int y = 0; y < sy; ++y)
		{
			for (int z = 0; z < sz; ++z)
			{
				const float dx = x - mx;
				const float dy = y - mx;
				const float dz = z - mx;

				const float ds = std::sqrt(dx * dx + dy * dy + dz * dz);

				dataSet.isSolid[x][y][z] = (ds < radius);
			}
		}
	}
}

template <int sx, int sy, int sz>
static void createVoxelSet(VoxelSet<sx, sy, sz> & voxelSet, DataSet<sx, sy, sz> & dataSet)
{
	for (int vx = 0; vx < sx; ++vx)
	{
		for (int vy = 0; vy < sy; ++vy)
		{
			for (int vz = 0; vz < sz; ++vz)
			{
				int minDistance = INT_MAX;

				for (int dsX = 0; dsX < sx; ++dsX)
				{
					for (int dsY = 0; dsY < sy; ++dsY)
					{
						for (int dsZ = 0; dsZ < sz; ++dsZ)
						{
							if (dataSet.isSolid[dsX][dsY][dsZ])
							{
								const int dx = dsX - vx;
								const int dy = dsY - vy;
								const int dz = dsZ - vz;

								const int ds = dx * dx + dy * dy + dz * dz;

								if (ds < minDistance)
									minDistance = ds;
							}
						}
					}
				}

				voxelSet.voxels[vx][vy][vz].distance = std::sqrt(double(minDistance));
			}
		}
	}
}

template <typename T, int C>
struct BinaryHeap
{
	T nodes[C];
	int size;

	BinaryHeap()
		: size(0)
	{
	}

	void push(const T & t)
	{
		Assert(size < C);

		int index = size;

		size++;

		nodes[index] = t;

		for (;;)
		{
			if (index == 0)
				break;
			
			const int parent = (index - 1) >> 1;

			if (nodes[parent] < nodes[index])
			{
				std::swap(nodes[index], nodes[parent]);
				index = parent;
			}
			else
			{
				break;
			}
		}
	}

	void pop()
	{
		Assert(size > 0);

		int index = 0;

		nodes[index] = nodes[size - 1];

		size--;

		for (;;)
		{
			const int c1 = (index << 1) + 1;
			const int c2 = (index << 1) + 2;

			int bestIndex = index;

			if (c1 < size && nodes[bestIndex] < nodes[c1])
				bestIndex = c1;
			if (c2 < size && nodes[bestIndex] < nodes[c2])
				bestIndex = c2;

			if (bestIndex != index)
			{
				std::swap(nodes[bestIndex], nodes[index]);
				index = bestIndex;
			}
			else
			{
				break;
			}
		}
	}

	bool empty() const
	{
		return size == 0;
	}

	const T & top() const
	{
		return nodes[0];
	}
};

struct PqElem
{
	int srcX;
	int srcY;
	int srcZ;

	int x;
	int y;
	int z;

	int64_t distance;

	bool operator<(const PqElem & other) const
	{
#if INVERTED_DISTANCE_METRIC
		return distance < other.distance;
#else
		return distance > other.distance;
#endif
	}
};

#include <algorithm>
#include <vector>
#include <queue>

#if defined(DEBUG)
	#define USE_BINARY_HEAP 1
#else
	#define USE_BINARY_HEAP 0
#endif
#define USE_HEAP 1

template <int sx, int sy, int sz>
struct Pq
{
	bool visited[sx][sy][sz];

#if USE_BINARY_HEAP
	BinaryHeap<PqElem, sx * sy * sz> elems;
#elif USE_HEAP
	std::vector<PqElem> elems;
#else
	std::priority_queue<PqElem, std::vector<PqElem>> elems;
#endif

	Pq()
	{
#if USE_BINARY_HEAP
#elif USE_HEAP
		elems.reserve(sx * sy * sz);
#endif
	}

	void insert(VoxelSet<sx, sy, sz> & voxelSet, const int srcX, const int srcY, const int srcZ, const int x, const int y, const int z)
	{
		Assert(visited[x][y][z] == false);

		visited[x][y][z] = true;

		//

		const int dx = x - srcX;
		const int dy = y - srcY;
		const int dz = z - srcZ;
		const int64_t distanceSq = dx * dx + dy * dy + dz * dz;

		PqElem e;

		e.srcX = srcX;
		e.srcY = srcY;
		e.srcZ = srcZ;

		e.x = x;
		e.y = y;
		e.z = z;

		e.distance = distanceSq;

#if USE_BINARY_HEAP
		elems.push(e);
#elif USE_HEAP
		elems.push_back(e);

		std::push_heap(elems.begin(), elems.end());
#else
		elems.push(e);
#endif

		//

		//double dd = voxelSet.voxels[x][y][z].distance - distance;

		//logDebug("dd: %g", float(dd));

		voxelSet.voxels[x][y][z].distance = std::sqrt(double(distanceSq));
	}

	void tryInsert(VoxelSet<sx, sy, sz> & voxelSet, const PqElem & e, const int ox, const int oy, const int oz)
	{
		const int x = e.x + ox;
		const int y = e.y + oy;
		const int z = e.z + oz;

		if (x >= 0 && x < sx &&
			y >= 0 && y < sy &&
			z >= 0 && z < sz)
		{
			if (!visited[x][y][z])
			{
				insert(voxelSet, e.srcX, e.srcY, e.srcZ, x, y, z);
			}
		}
	}

	void calculateDistances(DataSet<sx, sy, sz> & dataSet, VoxelSet<sx, sy, sz> & voxelSet)
	{
		memset(visited, 0, sizeof(visited));

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				for (int z = 0; z < sz; ++z)
				{
					if (dataSet.isSolid[x][y][z])
					{
						insert(voxelSet, x, y, z, x, y, z);
					}
				}
			}
		}

		while (!elems.empty())
		{
#if USE_BINARY_HEAP
			const PqElem e = elems.top();

			elems.pop();
#elif USE_HEAP
			std::pop_heap(elems.begin(), elems.end());

			const PqElem e = elems.back();

			elems.pop_back();
#else
			const PqElem e = elems.top();

			elems.pop();
#endif

			if (sx > 1)
			{
				tryInsert(voxelSet, e, -1, +0, +0);
				tryInsert(voxelSet, e, +1, +0, +0);
			}
			if (sy > 1)
			{
				tryInsert(voxelSet, e, +0, -1, +0);
				tryInsert(voxelSet, e, +0, +1, +0);
			}
			if (sz > 1)
			{
				tryInsert(voxelSet, e, +0, +0, -1);
				tryInsert(voxelSet, e, +0, +0, +1);
			}
		}

#if defined(DEBUG) && 0
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				for (int z = 0; z < sz; ++z)
				{
					Assert(visited[x][y][z]);
				}
			}
		}
#endif
	}
};

template <int sx, int sy, int sz>
static void createVoxelSetV2(VoxelSet<sx, sy, sz> & voxelSet, DataSet<sx, sy, sz> & dataSet)
{
	Pq<sx, sy, sz> * pq = new Pq<sx, sy, sz>();

	const uint64_t t1 = g_TimerRT.TimeUS_get();

	pq->calculateDistances(dataSet, voxelSet);

	const uint64_t t2 = g_TimerRT.TimeUS_get();

	printf("calculateDistances took %gms\n", float(t2 - t1) / 1000.f);

	delete pq;
	pq = nullptr;
}

#if DO_SPARSE_TEXTURE

struct SparseTextureObject
{
	static const int textureSx = 1024 * 16;
	static const int textureSy = 1024 * 16;

	struct PageInfo
	{
		int level;

		int px;
		int py;
		int sx;
		int sy;

		bool isResident;

		PageInfo()
			: level(0)
			, px(0)
			, py(0)
			, sx(0)
			, sy(0)
			, isResident(false)
		{
		}

		void init(const int _level, const int _px, const int _py, const int _sx, const int _sy)
		{
			level = _level;

			px = _px;
			py = _py;
			sx = _sx;
			sy = _sy;
		}

		void makeResident()
		{
			Assert(!isResident);

			isResident = true;

			//logDebug("makeResident: %d (%d, %d) x (%d, %d)", level, px, py, sx, sy);

			glTexPageCommitmentARB(GL_TEXTURE_2D, level,
				px, py, 0,
				sx, sy, 1,
				GL_TRUE);
			checkErrorGL();

			// upload pixel data

			const uint8_t c = 32 + ((px ^ py ^ level) % (256 - 32));
			//const uint8_t c = 32 + (rand() % (256 - 32));

			uint8_t * pixels = new uint8_t[sx * sy * 4];
			memset(pixels, c, sx * sy * 4);

			glTexSubImage2D(GL_TEXTURE_2D, level,
				px, py,
				sx, sy,
				GL_RGBA, GL_UNSIGNED_BYTE,
				pixels);
			checkErrorGL();

			delete[] pixels;
			pixels = nullptr;
		}

		void dealloc()
		{
			Assert(isResident);

			isResident = false;

			//logDebug("dealloc: %d (%d, %d) x (%d, %d)", level, px, py, sx, sy);

			glTexPageCommitmentARB(GL_TEXTURE_2D, 0,
				px, py, 0,
				sx, sy, 1,
				GL_FALSE);
			checkErrorGL();
		}

		int memUsage() const
		{
			if (isResident)
				return sx * sy * 4;
			else
				return 0;
		}
	};

	struct LayerInfo
	{
		PageInfo ** pages;
		int sx;
		int sy;

		LayerInfo()
			: pages(nullptr)
			, sx(0)
			, sy(0)
		{
		}

		~LayerInfo()
		{
			init(0, 0, 0, 0, 0);
		}

		void init(const int _level, const int _sx, const int _sy, const int pageSx, const int pageSy)
		{
			if (pages != nullptr)
			{
				for (int x = 0; x < sx; ++x)
				{
					delete[] pages[x];
					pages[x] = nullptr;
				}

				delete[] pages;
				pages = nullptr;
			}

			sx = 0;
			sy = 0;

			//

			sx = _sx;
			sy = _sy;

			if (sx > 0 && sy > 0)
			{
				pages = new PageInfo*[sy];

				for (int x = 0; x < sx; ++x)
				{
					pages[x] = new PageInfo[sy];

					for (int y = 0; y < sy; ++y)
					{
						const int px = x * pageSx;
						const int py = y * pageSy;
						const int sx = pageSx;
						const int sy = pageSy;

						pages[x][y].init(
							_level,
							px, py,
							sx, sy);
					}
				}
			}
		}

		int memUsage() const
		{
			int result = 0;

			for (int x = 0; x < sx; ++x)
			{
				for (int y = 0; y < sy; ++y)
				{
					result += pages[x][y].memUsage();
				}
			}

			return result;
		}

		int numPages() const
		{
			return sx * sy;
		}

		int numActivePages() const
		{
			int result = 0;

			for (int x = 0; x < sx; ++x)
			{
				for (int y = 0; y < sy; ++y)
				{
					if (pages[x][y].isResident)
						result++;
				}
			}

			return result;
		}
	};

	GLuint texture;

	LayerInfo * layers;
	int numLayers;

	SparseTextureObject()
		: texture(0)
		, layers(nullptr)
		, numLayers(0)
	{
	}

	void init()
	{
		// create sparse texture

		glGenTextures(1, &texture);
		checkErrorGL();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		checkErrorGL();

		//

		numLayers = 1;

		int sx = pageSx;
		int sy = pageSy;

		while (sx * 2 <= textureSx && sy * 2 <= textureSy)
		{
			numLayers++;
			sx *= 2;
			sy *= 2;
		}

		layers = new LayerInfo[numLayers];

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLayers - 1);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		glTexStorage2D(GL_TEXTURE_2D, numLayers, GL_RGBA8, textureSx, textureSy);
		checkErrorGL();

		// setup administration

		int layerSx = textureSx;
		int layerSy = textureSy;

		for (int i = 0; i < numLayers; ++i, layerSx /= 2, layerSy /= 2)
		{
			const int numPagesX = (layerSx + pageSx - 1) / pageSx;
			const int numPagesY = (layerSy + pageSy - 1) / pageSy;

			LayerInfo & layer = layers[i];

			layer.init(i, numPagesX, numPagesY, pageSx, pageSy);
		}
	}

	void eval(LayerInfo & layer, const Mat4x4 & mat)
	{
		const float vsx = GFX_SX;
		const float vsy = GFX_SY;

		for (int x = 0; x < layer.sx; ++x)
		{
			for (int y = 0; y < layer.sy; ++y)
			{
				PageInfo & page = layer.pages[x][y];

				// determine AABB for transformed page texels

				Vec2 min;
				Vec2 max;

				bool first = true;

				for (int ox = 0; ox < 2; ++ox)
				{
					for (int oy = 0; oy < 2; ++oy)
					{
						const Vec2 p = mat * Vec2(page.px + page.sx * ox, page.py + page.sy * oy);

						if (first)
						{
							first = false;

							min = p;
							max = p;
						}
						else
						{
							min = min.Min(p);
							max = max.Max(p);
						}
					}
				}

				bool isWanted = true;

				if (isWanted)
				{
					// todo : we actually want an accurate intersection test with the viewport here

					if (min[0] > vsx ||
						min[1] > vsy ||
						max[0] < 0.f ||
						max[1] < 0.f)
					{
						isWanted = false;
					}
				}

				if (isWanted)
				{
					const Vec2 p1 = mat * Vec2(page.px + page.sx * 0, page.py + page.sy * 0);
					const Vec2 p2 = mat * Vec2(page.px + page.sx * 1, page.py + page.sy * 1);

					const Vec2 vd = p2 - p1;
					const float vds = vd.CalcSize();
					const Vec2 pd(page.sx, page.sy);
					const float pds = pd.CalcSize();

					const float samplingRate = vds / pds;

					if (samplingRate > 4.f || samplingRate < .5f)
					{
						isWanted = false;
					}
				}

				if (isWanted)
				{
					if (!page.isResident)
					{
						page.makeResident();
						break; // force only one update per layer per frame for demonstration purposes
					}
				}
				else
				{
					if (page.isResident)
					{
						page.dealloc();
						break; // force only one update per layer per frame for demonstration purposes
					}
				}
			}
		}
	}

	void eval(const Mat4x4 & mat)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();

		int scale = 1;

		for (int i = 0; i < numLayers; ++i, scale *= 2)
		{
			const Mat4x4 mat2 = mat.Scale(scale, scale, scale);

			LayerInfo & layer = layers[i];

			eval(layer, mat2);
		}
	}

	int memUsage() const
	{
		int result = 0;

		for (int i = 0; i < numLayers; ++i)
		{
			result += layers[i].memUsage();
		}

		return result;
	}
};

#endif

#if DO_QUADTREE

struct QuadNode;
struct QuadParams;
struct QuadTree;

struct QuadParams
{
	QuadParams()
		: tree(nullptr)
		, viewX(0)
		, viewY(0)
		, viewZ(0)
		, drawScale(1.f)
	{
	}

	QuadTree * tree;

	int viewX;
	int viewY;
	int viewZ;

	float drawScale;
};

struct QuadNode
{
	QuadNode * children;

	bool isResident;

	QuadNode()
		: children(nullptr)
		, isResident(false)
	{
	}

	~QuadNode()
	{
		delete[] children;
		children = nullptr;
	}

	void subdivide(const int size, const int maxSize)
	{
		Assert(size > maxSize);

		children = new QuadNode[4];

		const int childSize = size / 2;

		if (childSize > maxSize)
		{
			for (int i = 0; i < 4; ++i)
			{
				children[i].subdivide(childSize, maxSize);
			}
		}
	}

	void makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool _isResident);

	void traverse(const int level, const int x, const int y, const int size, const QuadParams & params)
	{
		const bool doTraverse = traverseImpl(x, y, size, params);

		if (doTraverse || children == nullptr)
		{
			if (!isResident)
				makeResident(level, x, y, size, params, true);
		}
		else
		{
			if (isResident)
				makeResident(level, x, y, size, params, false);
		}

		if (children == nullptr || doTraverse == false)
		{
			//logDebug("child (%d, %d) - (%d, %d)",
			//	x,
			//	y,
			//	x + size,
			//	y + size);

			const int ds = calculateDistanceSq(params.viewX, params.viewY, params.viewZ, x, y, size);
			const float d = std::sqrt(float(ds));

			const float drawScale = params.drawScale;

			setColorf(1.f, 1.f, 1.f, 1.f / (d / 200.f + 1.f));

			drawRectLine(x * drawScale, y * drawScale, (x + size) * drawScale, (y + size) * drawScale);

			//drawRectLine(x, y, x + size, y + size);

			drawText((x + size/2) * drawScale, (y + size/2) * drawScale, 20, 0, 0, "%d", int(d));
		}
		
		if (children != nullptr && doTraverse)
		{
			const int childSize = size / 2;

			const int x1 = x;
			const int x2 = x + childSize;
			const int y1 = y;
			const int y2 = y + childSize;
			
			children[0].traverse(level + 1, x1, y1, childSize, params);
			children[1].traverse(level + 1, x2, y1, childSize, params);
			children[2].traverse(level + 1, x2, y2, childSize, params);
			children[3].traverse(level + 1, x1, y2, childSize, params);
		}
	}

	void traverseDrawResidency(const int level, const int x, const int y, const int size, const QuadParams & params)
	{
		if (isResident)
		{
			const float drawScale = params.drawScale;

			setColor(255, 0, 0, 63);
			drawRect(x * drawScale, y * drawScale, (x + size) * drawScale, (y + size) * drawScale);
		}

		if (children != nullptr)
		{
			const int childSize = size / 2;

			const int x1 = x;
			const int x2 = x + childSize;
			const int y1 = y;
			const int y2 = y + childSize;

			children[0].traverseDrawResidency(level + 1, x1, y1, childSize, params);
			children[1].traverseDrawResidency(level + 1, x2, y1, childSize, params);
			children[2].traverseDrawResidency(level + 1, x2, y2, childSize, params);
			children[3].traverseDrawResidency(level + 1, x1, y2, childSize, params);
		}
	}

	//

	static int calculateDistanceSq(const int viewX, const int viewY, const int viewZ, const int x, const int y, const int size)
	{
		const int x1 = x;
		const int x2 = x + size;
		const int y1 = y;
		const int y2 = y + size;

		const int dx = viewX < x1 ? x1 - viewX : viewX > x2 ? viewX - x2 : 0;
		const int dy = viewY < y1 ? y1 - viewY : viewY > y2 ? viewY - y2 : 0;
		const int dz = viewZ;

		const int ds = dx * dx + dy * dy + dz * dz;

		return ds;
	}

	bool traverseImpl(const int x, const int y, const int size, const QuadParams & params)
	{
		const int ds = calculateDistanceSq(params.viewX, params.viewY, params.viewZ, x, y, size);

		if (ds * 4 < size * size)
			return true;
		else
			return false;
	}
};

struct QuadTree
{
	GLuint texture;

	QuadNode root;

	int initSize;
	int pageSize;

	int numLevels;
	int lastLevel;
	int numAllocatedLevels;

	QuadTree()
		: texture(0)
		, root()
		, initSize(0)
		, pageSize(0)
		, numLevels(0)
		, lastLevel(0)
		, numAllocatedLevels(0)
	{
	}

	void init(const int size)
	{
		initSize = size;

		// create sparse texture

		glGenTextures(1, &texture);
		checkErrorGL();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		checkErrorGL();

		//

		pageSize = std::max(pageSx, pageSy);

		lastLevel = 0;

		while (initSize / (1 << lastLevel) > pageSize)
		{
			lastLevel++;
		}

		numLevels = 1;

		while ((1 << (numLevels - 1)) < initSize)
		{
			numLevels++;
		}

		numAllocatedLevels = lastLevel + 1;

		Assert((1 << (numLevels - 1)) == initSize);
		Assert((1 << lastLevel) == (initSize / pageSize));
		Assert((1 << (numLevels - numAllocatedLevels)) == pageSize);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numAllocatedLevels - 1);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		glTexStorage2D(GL_TEXTURE_2D, numAllocatedLevels, GL_RGBA8, initSize, initSize);
		checkErrorGL();

		//

		root.subdivide(size, pageSize);
	}
};

void QuadNode::makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool _isResident)
{
	isResident = _isResident;

	const int levelSize = params.tree->pageSize << level;
	const int mipIndex = params.tree->numAllocatedLevels - level - 1;
	const int scale = params.tree->initSize / levelSize;

	const int px = x / scale;
	const int py = y / scale;
	const int sx = params.tree->pageSize;
	const int sy = params.tree->pageSize;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, params.tree->texture);
	checkErrorGL();

	glTexPageCommitmentARB(
		GL_TEXTURE_2D,
		mipIndex,
		px, py, 0,
		sx, sy, 1,
		isResident);
	checkErrorGL();

	if (isResident)
	{
		uint8_t * pixels = new uint8_t[sx * sy * 4];
		uint8_t * pixelPtr = pixels;

		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				*pixelPtr++ = x >> 0;
				*pixelPtr++ = y >> 1;
				*pixelPtr++ = (x + y) >> 2;
				*pixelPtr++ = 255;
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D,
			mipIndex,
			px, py,
			sx, sy,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);
		checkErrorGL();

		delete[] pixels;
		pixels = nullptr;
	}

	if (children != nullptr && isResident == false)
	{
		const int childSize = size / 2;

		const int x1 = x;
		const int x2 = x + childSize;
		const int y1 = y;
		const int y2 = y + childSize;

		if (children[0].isResident)
			children[0].makeResident(level + 1, x1, y1, childSize, params, false);
		if (children[1].isResident)
			children[1].makeResident(level + 1, x2, y1, childSize, params, false);
		if (children[2].isResident)
			children[2].makeResident(level + 1, x2, y2, childSize, params, false);
		if (children[3].isResident)
			children[3].makeResident(level + 1, x1, y2, childSize, params, false);
	}
}

#endif

int main(int argc, char * argv[])
{
	changeDirectory("data");

	framework.enableDepthBuffer = true;

	framework.enableRealTimeEditing = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
	#if DO_VOXELTRACE
		DataSet<_DS_SX, _DS_SY, _DS_SZ> * dataSet = new DataSet<_DS_SX, _DS_SY, _DS_SZ>();

		createDataSet<_DS_SX, _DS_SY, _DS_SZ>(*dataSet);

		VoxelSet<_DS_SX, _DS_SY, _DS_SZ> * voxelSet = new VoxelSet<_DS_SX, _DS_SY, _DS_SZ>();

		createVoxelSetV2<_DS_SX, _DS_SY, _DS_SZ>(*voxelSet, *dataSet);
	#endif

		//

	#if DO_VOXELTRACE
		VoxelSpace * voxelSpace = new VoxelSpace();

		for (int x = 0; x < VS_SX; ++x)
		{
			for (int y = 0; y < VS_SY; ++y)
			{
				for (int z = 0; z < VS_SZ; ++z)
				{
					voxelSpace->sets[x][y][z] = *voxelSet;

					AABB & aabb = voxelSpace->sets[x][y][z].aabb;

					aabb.min[0] = (x + 0) * _DS_SX;
					aabb.min[1] = (y + 0) * _DS_SY;
					aabb.min[2] = (z + 0) * _DS_SZ;
					aabb.max[0] = (x + 1) * _DS_SX;
					aabb.max[1] = (y + 1) * _DS_SY;
					aabb.max[2] = (z + 1) * _DS_SZ;
				}
			}
		}
	#endif

		//

	#if DO_DISTANCE_TEXTURE
		GLuint texture = 0;

		{
			const int textureSx = GFX_SX;
			const int textureSy = GFX_SY;

			typedef DataSet<textureSx, textureSy, 1> TxDataSet;
			typedef VoxelSet<textureSx, textureSy, 1> TxVoxelSet;
			typedef Pq<textureSx, textureSy, 1> TxPq;

			printf("TxDataSet: %u bytes\n", sizeof(TxDataSet));
			printf("TxVoxelSet: %u bytes\n", sizeof(TxVoxelSet));

			TxDataSet * dataSet = new TxDataSet();

			for (int x = 0; x < textureSx; ++x)
			{
				for (int y = 0; y < textureSy; ++y)
				{
					dataSet->isSolid[x][y][0] = (rand() % 1000) == 0;
				}
			}

			TxVoxelSet * voxelSet = new TxVoxelSet();

			TxPq * pq = new TxPq();

			const uint64_t t1 = g_TimerRT.TimeUS_get();

			pq->calculateDistances(*dataSet, *voxelSet);

			const uint64_t t2 = g_TimerRT.TimeUS_get();

			delete pq;
			pq = nullptr;

			printf("calculateDistances took %gms\n", float(t2 - t1) / 1000.f);

		#if 0
			BinaryHeap<int, 1000> heap;

			for (int i = 0; i < 1000; ++i)
				heap.push(rand());

			while (!heap.empty())
			{
				printf("%d ", heap.top());
				heap.pop();
			}
		#endif

		#if 1
			uint8_t * pixels = new uint8_t[textureSx * textureSy * 4];
			uint8_t * pixelPtr = pixels;

			for (int y = 0; y < textureSy; ++y)
			{
				for (int x = 0; x < textureSx; ++x)
				{
					const float distance = voxelSet->voxels[x][y][0].distance;
					const float c = 1.f / (distance / 5.f + 1.f);
					const uint8_t v = Calc::Clamp(int(c * 255.f), 0, 255);

					pixelPtr[0] = v;
					pixelPtr[1] = v;
					pixelPtr[2] = v;
					pixelPtr[3] = 1.f;

					pixelPtr += 4;
				}
			}

			texture = createTextureFromRGBA8(pixels, textureSx, textureSy, true, true);

			delete[] pixels;
			pixels = nullptr;
		#endif

			delete voxelSet;
			voxelSet = nullptr;

			delete dataSet;
			dataSet = nullptr;
		}
	#endif

		//

	#if DO_SPARSE_TEXTURE
		SparseTextureObject sparseTextureObject;

		sparseTextureObject.init();
	#endif

		//

	#if DO_QUADTREE
		QuadTree quadTree;

		quadTree.init(4 * 1024);
	#endif

		//

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

		#if DO_VOXELTRACE
			if (keyboard.wentDown(SDLK_c))
			{
				createVoxelSet(*voxelSet, *dataSet);

				//createVoxelSetV2(*voxelSet, *dataSet);
			}
		#endif

			framework.beginDraw(0, 0, 0, 0);
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				setBlend(BLEND_OPAQUE);

			#if DO_VOXELTRACE
				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				{
					Mat4x4 matP;
					Mat4x4 matV;

					matP.MakePerspectiveLH(Calc::DegToRad(90.f), GFX_SY / float(GFX_SX), .001f, 1000.f);
					matV = Mat4x4(true).Translate(0.f, 0.f, -20.f * mouse.y / float(GFX_SY)).Invert();

					Mat4x4 mat = matP * matV;

					gxLoadMatrixf(mat.m_v);

					//

					gxMatrixMode(GL_MODELVIEW);
					gxPushMatrix();
					{
						gxLoadIdentity();

						const float mx = (_DS_SX - 1) / 2.f;
						const float my = (_DS_SY - 1) / 2.f;
						const float mz = (_DS_SZ - 1) / 2.f;

						gxRotatef(framework.time * 20.f, 1.f, 1.f, 1.f);
						gxScalef(1.f / mx, 1.f / my, 1.f / mz);
						gxTranslatef(-mx, -my, -mz);

						gxBegin(GL_POINTS);
						{
							const float step = .4f;

						#if 0
							for (float x = 0.f; x < voxelSet->getSx(); x += step)
							{
								for (float y = 0.f; y < voxelSet->getSy(); y += step)
								{
									for (float z = 0.f; z < voxelSet->getSz(); z += step)
									{
										//const float distance = voxelSet->voxels[x][y][z].distance;
										const float distance = voxelSet->sampleDistance(x, y, z);
										const float c = 1.f / (distance + 1.f);
										//const float c = Calc::Max(0.f, 1.f - distance * .5f);
										//const float c = 1.f;

										gxColor4f(c, c, c, 1.f);
										gxVertex3f(x, y, z);
									}
								}
							}
						#endif

							const int num = 50000;

							const float offset = std::sqrt(mx * mx + my * my + mz * mz) * .5f;

						#define SIMPLE_SPHERE 1

							for (int i = 0; i < num; ++i)
							{
							#if SIMPLE_SPHERE
								const float rx = random(-1.f, +1.f);
								const float ry = random(-1.f, +1.f);
								const float rz = random(-1.f, +1.f);
							#else
								const float rx = random(-1.f, +1.f);
								const float ry = random(-1.f, +1.f);
								const float rz = 1.f;
							#endif

								const float rs = std::sqrt(rx * rx + ry * ry + rz * rz);

								if (rs > 0.f)
								{
									const float dx = rx / rs;
									const float dy = ry / rs;
									const float dz = rz / rs;

								#if SIMPLE_SPHERE
									const float px = mx - dx * offset;
									const float py = my - dy * offset;
									const float pz = mz - dz * offset;
								#else
									const float px = random(0, _DS_SX * VS_SX);
									const float py = random(0, _DS_SY * VS_SY);
									const float pz = 0.f;

									const float rdx = 1.f / dx;
									const float rdy = 1.f / dy;
									const float rdz = 1.f / dz;
								#endif

									float ox;
									float oy;
									float oz;

								#if SIMPLE_SPHERE
									if (voxelSet->trace(px, py, pz, dx, dy, dz, ox, oy, oz))
								#else
									if (voxelSpace->trace(px, py, pz, dx, dy, dz, rdx, rdy, rdz, ox, oy, oz))
								#endif
									{
									#if 0
										const float no = .01f;

										const float dx = voxelSet->sampleDistance(ox + no, oy,      oz     ) - voxelSet->sampleDistance(ox - no, oy,      oz     );
										const float dy = voxelSet->sampleDistance(ox,      oy + no, oz     ) - voxelSet->sampleDistance(ox,      oy - no, oz     );
										const float dz = voxelSet->sampleDistance(ox,      oy,      oz + no) - voxelSet->sampleDistance(ox,      oy,      oz - no);
										const float ds = std::sqrt(dx * dx + dy * dy + dz * dz);

										const float nx = dx / ds;
										const float ny = dy / ds;
										const float nz = dz / ds;

										const float cr = (nx + 1.f) * .5f;
										const float cg = (ny + 1.f) * .5f;
										const float cb = (nz + 1.f) * .5f;

										gxColor4f(cr, cg, cb, 1.f);
									#else
										gxColor4f(1.f, 1.f, 0.f, 1.f);
									#endif
										//gxVertex3f(px, py, pz);
										gxVertex3f(ox, oy, oz);
									}
								}
							}

						#if 0
							for (float x = 0.f; x < voxelSet->getSx(); x += step)
							{
								for (float y = 0.f; y < voxelSet->getSy(); y += step)
								{
									float px = x;
									float py = y;
									float pz = 0.f;
									float dx = 0.f;
									float dy = 0.f;
									float dz = 1.f;
									float ox;
									float oy;
									float oz;

									if (traceVoxelSet(*voxelSet, px, py, pz, dx, dy, dz, ox, oy, oz))
									{
										gxColor4f(1.f, 1.f, 0.f, 1.f);
										gxVertex3f(ox, oy, oz);
									}
								}
							}
						#endif
						}
						gxEnd();
					}
					gxMatrixMode(GL_MODELVIEW);
					gxPopMatrix();
				}
				gxMatrixMode(GL_PROJECTION);
				gxPopMatrix();

				gxMatrixMode(GL_MODELVIEW);
			#endif

			#if DO_DISTANCE_TEXTURE
				if (texture != 0)
				{
					gxSetTexture(texture);
					{
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					gxSetTexture(0);
				}
			#endif

			#if DO_SPARSE_TEXTURE
				if (sparseTextureObject.texture != 0)
				{
					const float x = std::cos(framework.time / 1.234f * .1f) * sparseTextureObject.textureSx * .2f;
					const float y = std::cos(framework.time / 2.345f * .1f) * sparseTextureObject.textureSy * .2f;

				#if 0
					const float scale = 1.f;
					const float angle = Calc::DegToRad(45);
				#else
					const float scale = (1.f - std::sin(framework.time * .1f)) / 2.f;
					const float angle = framework.time * .1f;
				#endif

					const Mat4x4 mat = Mat4x4(true)
						.Translate(GFX_SX/2, GFX_SY/2, 0.f)
						.Scale(scale, scale, 1.f)
						.Translate(x, y, 0.f)
						.RotateZ(angle)
						.Translate(-sparseTextureObject.textureSy/2, -sparseTextureObject.textureSy/2, 0.f);

					sparseTextureObject.eval(mat);

					//

					glDisable(GL_DEPTH_TEST);
					setBlend(BLEND_OPAQUE);

					gxPushMatrix();
					{
						gxMultMatrixf(mat.m_v);

						Shader shader("VirtualTexture");
						shader.setTexture("texture", 0, sparseTextureObject.texture);
						setShader(shader);
						{
							gxBegin(GL_QUADS);
							{
								gxTexCoord2f(0.f, 0.f); gxVertex2f(0, 0);
								gxTexCoord2f(1.f, 0.f); gxVertex2f(sparseTextureObject.textureSx, 0);
								gxTexCoord2f(1.f, 1.f); gxVertex2f(sparseTextureObject.textureSx, sparseTextureObject.textureSy);
								gxTexCoord2f(0.f, 1.f); gxVertex2f(0, sparseTextureObject.textureSy);
							}
							gxEnd();
						}
						clearShader();
					}
					gxPopMatrix();

					setBlend(BLEND_ALPHA);

					gxPushMatrix();
					{
						gxScalef(
							GFX_SX / float(sparseTextureObject.textureSx),
							GFX_SY / float(sparseTextureObject.textureSy),
							1.f);

						setBlend(BLEND_ALPHA);

						int scale = 1;

						for (int i = 0; i < sparseTextureObject.numLayers; ++i, scale *= 2)
						{
							const SparseTextureObject::LayerInfo & layer = sparseTextureObject.layers[i];

							for (int x = 0; x < layer.sx; ++x)
							{
								for (int y = 0; y < layer.sy; ++y)
								{
									const SparseTextureObject::PageInfo & page = layer.pages[x][y];

									if (page.isResident)
									{
										const int x1 = (page.px + page.sx * 0) * scale;
										const int y1 = (page.py + page.sy * 0) * scale;
										const int x2 = (page.px + page.sx * 1) * scale;
										const int y2 = (page.py + page.sy * 1) * scale;

										setColor(255, 0, 0, 31);
										drawRect(x1, y1, x2, y2);

										setColor(0, 0, 0, 255);
										drawRectLine(x1, y1, x2, y2);
									}
								}
							}
						}
					}
					gxPopMatrix();

					gxPushMatrix();
					{
						int x = 20;
						int y = 20;
						int fontSize = 24;
						int spacing = fontSize + 5;

						setFont("calibri.ttf");
						setColor(colorWhite);

						const int memUsage = sparseTextureObject.memUsage();

						drawText(x, y, fontSize, +1, +1, "memUsage: %.2fmb", memUsage / 1024.f / 1024.f);
						y += spacing;

						for (int i = 0; i < sparseTextureObject.numLayers; ++i)
						{
							const int numPages = sparseTextureObject.layers[i].numPages();
							const int numActivePages = sparseTextureObject.layers[i].numActivePages();

							drawText(x, y, fontSize, +1, +1, "mipLevel=%d, activePages=%d / %d", i, numActivePages, numPages);
							y += spacing;
						}
					}
					gxPopMatrix();

					//
					
					setBlend(BLEND_OPAQUE);
					glEnable(GL_DEPTH_TEST);
				}
			#endif

			#if DO_QUADTREE
				gxPushMatrix();
				{
					const float scale = quadTree.initSize / float(GFX_SX);

					glDisable(GL_DEPTH_TEST);
					setBlend(BLEND_ALPHA);

					gxPushMatrix();
					{
						gxScalef(1.f / scale, 1.f / scale, 1.f);

						Shader shader("TexLod");
						setShader(shader);
						shader.setTexture("texture", 0, quadTree.texture);
						shader.setImmediate("lod", mouse.isDown(BUTTON_RIGHT) ? (mouse.x / float(GFX_SX) * quadTree.numAllocatedLevels) : -1);
						shader.setImmediate("numLods", quadTree.numAllocatedLevels);
						{
							gxBegin(GL_QUADS);
							{
								gxTexCoord2f(0.f, 0.f); gxVertex2f(quadTree.initSize * 0, quadTree.initSize * 0);
								gxTexCoord2f(1.f, 0.f); gxVertex2f(quadTree.initSize * 1, quadTree.initSize * 0);
								gxTexCoord2f(1.f, 1.f); gxVertex2f(quadTree.initSize * 1, quadTree.initSize * 1);
								gxTexCoord2f(0.f, 1.f); gxVertex2f(quadTree.initSize * 0, quadTree.initSize * 1);
							}
							gxEnd();
						}
						clearShader();
					}
					gxPopMatrix();

					static bool highAltitude = false;
					static int altitude = 0;

					if (mouse.wentDown(BUTTON_LEFT))
						highAltitude = !highAltitude;

					altitude = Calc::Clamp(altitude + (highAltitude ? +1 : -1), 0, 500);

					QuadParams params;
					params.tree = &quadTree;
					params.viewX = mouse.x * scale;
					params.viewY = mouse.y * scale;
					params.viewZ = altitude;
					params.drawScale = 1.f / scale;

					setFont("calibri.ttf");

					//quadTree.root.traverseDrawResidency(0, 0, 0, quadTree.initSize, params);
					quadTree.root.traverse(0, 0, 0, quadTree.initSize, params);

					setBlend(BLEND_OPAQUE);
					glEnable(GL_DEPTH_TEST);
				}
				gxPopMatrix();
			#endif
			}
			framework.endDraw();
		}

	#if DO_VOXELTRACE
		delete voxelSet;
		voxelSet = nullptr;

		delete dataSet;
		dataSet = nullptr;
	#endif

		framework.shutdown();
	}

	return 0;
}
