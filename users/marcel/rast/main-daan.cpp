#include <assert.h>
#include <emmintrin.h>
#include <math.h>
#include <SDL/SDL.h>
#include <vector>
#include <xmmintrin.h>
#include "../../../libgg/SIMD.h"
#include "../../../libgg/SimdMat4x4.h"
#include "md3.h"

#if defined(WINDOWS)
#include <windows.h>
#endif

typedef unsigned int ptr_t;
typedef unsigned char u8;
typedef unsigned int u32;

#define _mm_set_xyzw_ps(x, y, z, w) _mm_set_ps(w, z, y, x)

#define USE_SOLID_COLOR 0
#define USE_BILINEAR 1
#define USE_MIPMAP 0
#define USE_TRILINEAR 0
#define USE_NEAREST 0
#define USE_SUBPIXEL 1
#define FORCE_WHITE_COLOR 0
#define ZPREPASS 0

//

#define SY 1280

static const __m128 kVec1   = _mm_set1_ps(1.0f);
static const __m128 kVec255 = _mm_set1_ps(255.0f);
static const __m128 kVec255Rcp = _mm_set1_ps(1.0f / 255.0f);

//

static void exception(const char* msg)
{
	printf("error: %s\n", msg);
	exit(-1);
}

//

struct g_Surface
{
	u32 sx;
	u32 sy;
	u32* pixels;
	u32 pitch;
	u32 shiftR;
	u32 shiftG;
	u32 shiftB;
} g_Surface;

//

class TextureLevel
{
public:
	TextureLevel()
	{
		m_Size = 0;
		m_SizeMask = 0;
		m_SizeShift = 0;
		m_Colors = 0;
	}

	void SetSize(unsigned int size)
	{
		_mm_free(m_Colors);
		m_Colors = 0;
		m_SizeMask = 0;
		m_SizeShift = 0;

		m_Size = size;

		if (m_Size > 0)
		{
			m_SizeMask = m_Size - 1;

			m_Colors = (__m128*)_mm_malloc(m_Size * m_Size * sizeof(__m128), 16);

			while (1 << m_SizeShift != m_Size)
				m_SizeShift++;
		}
	}

	void Load(const char* fileName)
	{
		SDL_Surface * surface = SDL_LoadBMP(fileName);

		if (surface == 0)
			exception("failed to load texture");
		if (surface->w != surface->h)
			exception("texture must be square");
		if (surface->format->BitsPerPixel != 32)
			exception("texture must be 32 bit per pixel");

		unsigned int size = surface->w;

		SetSize(size);

		if (SDL_LockSurface(surface) < 0)
			exception("failed to lock surface");

		__m128 scale = kVec255Rcp;

		for (unsigned int y = 0; y < size; ++y)
		{
			const u32 * __restrict srcLine = reinterpret_cast<u32 *>(surface->pixels) + ((surface->pitch * y) >> 2);
			__m128 * __restrict dstLine = &GetPixel(0, y);

			for (unsigned int x = 0; x < size; ++x)
			{
				const u32 rgb = srcLine[x];
				const u32 r = (rgb & surface->format->Rmask) >> surface->format->Rshift;
				const u32 g = (rgb & surface->format->Gmask) >> surface->format->Gshift;
				const u32 b = (rgb & surface->format->Bmask) >> surface->format->Bshift;

				__m128 temp = _mm_set_xyzw_ps(r, g, b, 255.0f);

				dstLine[x] = _mm_mul_ps(temp, scale);
			}
		}

		SDL_UnlockSurface(surface);
		SDL_FreeSurface(surface);
		surface = 0;
	}

	FORCEINLINE __m128 & GetPixel(unsigned int x, unsigned int y)
	{
		//return m_Colors[x + y * m_Size];
		return m_Colors[x + (y << m_SizeShift)];
	}

	FORCEINLINE const __m128 & GetPixel(unsigned int x, unsigned int y) const
	{
		//return m_Colors[x + y * m_Size];
		return m_Colors[x + (y << m_SizeShift)];
	}

	TextureLevel* DownScale()
	{
		TextureLevel* level = new TextureLevel();

		unsigned int size = m_Size >> 1;

		assert(size >= 1);

		level->SetSize(size);

		for (unsigned int y = 0; y < size; ++y)
		{
			const __m128 * __restrict srcLine0 = &GetPixel(0, y * 2 + 0);
			const __m128 * __restrict srcLine1 = &GetPixel(0, y * 2 + 1);

			for (unsigned int x = 0; x < size; ++x, srcLine0 += 2, srcLine1 += 2)
			{
				__m128 p1 = srcLine0[0];
				__m128 p2 = srcLine0[1];
				__m128 p3 = srcLine1[1];
				__m128 p4 = srcLine1[0];

				__m128 & dst = level->GetPixel(x, y);

				dst =
					_mm_mul_ps(
					_mm_set_ps1(0.25f),
					_mm_add_ps(p1,
					_mm_add_ps(p2,
					_mm_add_ps(p3, p4))));
			}
		}

		return level;
	}

	unsigned int m_Size;
	unsigned int m_SizeMask;
	unsigned int m_SizeShift;
	__m128* m_Colors;
};

class Texture
{
public:
	Texture()
	{
		m_Levels = 0;
		m_LevelCount = 0;
	}

	~Texture()
	{
		for (unsigned int i = 0; i < m_LevelCount; ++i)
			delete m_Levels[i];
		delete[] m_Levels;
		m_Levels = 0;
		m_LevelCount = 0;
	}

	void Load(const char* fileName)
	{
		TextureLevel* base = new TextureLevel();

		base->Load(fileName);

		GenerateMipMaps(base);
	}

	void GenerateMipMaps(TextureLevel* base)
	{
		unsigned int levelCount = 0;

		unsigned int size = base->m_Size;

		while (size > 1)
		{
			levelCount++;

			size >>= 1;
		}

		m_Levels = new TextureLevel*[levelCount];
		m_LevelCount = levelCount;

		m_Levels[0] = base;

		for (unsigned int i = 1; i < levelCount; ++i)
		{
			m_Levels[i] = m_Levels[i - 1]->DownScale();
		}
	}

	TextureLevel** m_Levels;
	unsigned int m_LevelCount;
};

Texture* g_Texture = 0;

class DepthBuffer
{
public:
	DepthBuffer()
	{
		m_Depth = 0;
	}

	~DepthBuffer()
	{
		_mm_free(m_Depth);
	}

	void Initialize(unsigned int sx, unsigned int sy)
	{
		_mm_free(m_Depth);

		m_Depth = 0;

		//

		m_Sx = sx;
		m_Sy = sy;

		//

		const unsigned int area = m_Sx * m_Sy;

		if (area > 0)
			m_Depth = (float*)_mm_malloc(sizeof(float) * area, 16);
	}

	void ClearZero()
	{
		const unsigned int area = m_Sx * m_Sy;

		memset(m_Depth, 0x00, sizeof(float) * area);
	}

	__forceinline float* GetDepth(unsigned int x, unsigned int y)
	{
		return m_Depth + x + y * m_Sx;
	}

	float* m_Depth;
	unsigned int m_Sx;
	unsigned int m_Sy;
};

static DepthBuffer* g_DepthBuffer = 0;

// struct which holds variables to be interpolated across surface
// note: we don't interpolate x/y in world space because they have no role in calculating the output colour

ALIGN_CLASS(16) VertInfo
{
public:
	union
	{
		struct
		{
			float z;
			float u;
			float v;
			float l; // mip level
		};
		__m128 zuvl;
	};

#if FORCE_WHITE_COLOR == 0
	union
	{
		struct
		{
			float r;
			float g;
			float b;
		};
		__m128 rgb0;
	};
#endif
};

// vertex

ALIGN_CLASS(16) Vert
{
public:
	inline Vert()
	{
	}

	Vert(float x, float y, float z, float u, float v, float r, float g, float b)
	{
		this->x = x;
		this->y = y;
		info.z = z;
		info.u = u;
		info.v = v;
#if FORCE_WHITE_COLOR == 0
		info.r = r;
		info.g = g;
		info.b = b;
#endif
	}

	float x;
	float y;
	VertInfo info;
};

// struct which holds a single scan line vertex

ALIGN_CLASS(16) ScanVert
{
public:
	inline ScanVert()
	{
		Reset();
	}

	__forceinline void Set()
	{
		set = 1;
	}

	__forceinline void Reset()
	{
		set = 0;
	}

	unsigned int set;
	float x;
	VertInfo info;
};

// list of scan lines, 2 for each y, holding state at x1 and x2

static ScanVert g_ScanVerts[SY][2];

// sample texture

static FORCEINLINE __m128 Sample_Base(unsigned int u, unsigned int v, const TextureLevel* __restrict texture)
{
	const unsigned int tu = u & texture->m_SizeMask;
	const unsigned int tv = v & texture->m_SizeMask;

	return texture->GetPixel(tu, tv);
}

static FORCEINLINE __m128 Sample_Base(unsigned int u, unsigned int v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	return Sample_Base(u, v, texture);
}

static FORCEINLINE __m128 Sample(float u, float v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	const unsigned int tu = static_cast<unsigned int>(u * texture->m_Size);
	const unsigned int tv = static_cast<unsigned int>(v * texture->m_Size);

	return Sample_Base(tu, tv, level);
}

// mix colours

static __forceinline __m128 Mix2(
	const __m128 & c1,
	const __m128 & c2,
	const float * __restrict w)
{
	__m128 cw1 = _mm_mul_ps(c1, _mm_set_ps1(w[0]));
	__m128 cw2 = _mm_mul_ps(c2, _mm_set_ps1(w[1]));

	return _mm_add_ps(cw1, cw2);
}


// mix colours for bilinear texture sampling

static __forceinline __m128 Mix4(
	const __m128 & c1,
	const __m128 & c2,
	const __m128 & c3,
	const __m128 & c4,
	const __m128 & w)
{
	__m128 cw1 = _mm_mul_ps(c1, _mm_shuffle_ps(w, w, _MM_SHUFFLE(0,0,0,0)));
	__m128 cw2 = _mm_mul_ps(c2, _mm_shuffle_ps(w, w, _MM_SHUFFLE(1,1,1,1)));
	__m128 cw3 = _mm_mul_ps(c3, _mm_shuffle_ps(w, w, _MM_SHUFFLE(2,2,2,2)));
	__m128 cw4 = _mm_mul_ps(c4, _mm_shuffle_ps(w, w, _MM_SHUFFLE(3,3,3,3)));

	return
		_mm_add_ps(cw1,
		_mm_add_ps(cw2,
		_mm_add_ps(cw3, cw4)));
}

// get power of 2

static __forceinline unsigned int PowerOf2(unsigned int v)
{
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return ++v;
}

// sample texture bilinearly

static FORCEINLINE __m128 SampleB(float u, float v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	const float tuf = u * texture->m_Size;
	const float tvf = v * texture->m_Size;
	const unsigned int tu = static_cast<unsigned int>(tuf);
	const unsigned int tv = static_cast<unsigned int>(tvf);

	__m128 c1 = Sample_Base(tu + 0, tv + 0, level);
	__m128 c2 = Sample_Base(tu + 1, tv + 0, level);
	__m128 c3 = Sample_Base(tu + 1, tv + 1, level);
	__m128 c4 = Sample_Base(tu + 0, tv + 1, level);

	const float tu2 = tuf - tu;
	const float tv2 = tvf - tv;
	const float tu1 = 1.0f - tu2;
	const float tv1 = 1.0f - tv2;

	__m128 wu = _mm_set_xyzw_ps(tu1, tu2, tu2, tu1);
	__m128 wv = _mm_set_xyzw_ps(tv1, tv1, tv2, tv2);
	__m128 w = _mm_mul_ps(wu, wv);

	return Mix4(c1, c2, c3, c4, w);
}

static void Fill(unsigned int y)
{
	ScanVert* __restrict v1 = &g_ScanVerts[y][0];
	ScanVert* __restrict v2 = &g_ScanVerts[y][1];

	v1->Reset();
	v2->Reset();

	if (v1->x == v2->x)
		return;

	if (v1->x > v2->x)
	{
		ScanVert* temp = v1;
		v1 = v2;
		v2 = temp;
	}

	const __m128 _dxVec = _mm_set_ss(v2->x - v1->x);
	const __m128 dxRcpSS = _mm_rcp_ss(_dxVec);
	const __m128 dxRcpVec = _mm_shuffle_ps(dxRcpSS, dxRcpSS, _MM_SHUFFLE(0,0,0,0));

	__m128 v1_zuvl = v1->info.zuvl;
	__m128 v2_zuvl = v2->info.zuvl;
#if FORCE_WHITE_COLOR == 0
	__m128 v1_rgb0 = _mm_mul_ps(v1->info.rgb0, kVec255);
	__m128 v2_rgb0 = _mm_mul_ps(v2->info.rgb0, kVec255);
#endif

	__m128 dzuvl_dx = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dxRcpVec);
#if FORCE_WHITE_COLOR == 0
	__m128 drgb0_dx = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dxRcpVec);
#endif

	__m128 v_zuvl = v1_zuvl;
#if FORCE_WHITE_COLOR == 0
	__m128 v_rgb0 = v1_rgb0;
#endif

	if (v1->x < 0.0f)
	{
		v_zuvl = _mm_sub_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, _mm_set_ps1(v1->x)));
#if FORCE_WHITE_COLOR == 0
		v_rgb0 = _mm_sub_ps(v_rgb0, _mm_mul_ps(drgb0_dx, _mm_set_ps1(v1->x)));
#endif

		v1->x = 0.0f;
	}

	if (v2->x > g_Surface.sx)
	{
		v2->x = static_cast<float>(g_Surface.sx);
	}

	if (v2->x <= v1->x)
		return;

	const unsigned int x1 = (unsigned int)v1->x;
	const unsigned int x2 = (unsigned int)v2->x;

	if (x2 <= x1)
		return;

	assert(x1 <= 65535);
	assert(x2 <= 65535);

#if USE_SUBPIXEL == 1
	const float xd = x1 - v1->x + 1.0f;
	const __m128 xdVec = _mm_set_ps1(xd);

	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, xdVec));
#if FORCE_WHITE_COLOR == 0
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dx, xdVec));
#endif
#endif

	u32* __restrict line = g_Surface.pixels + g_Surface.pitch * y + x1;
	float* __restrict depth = g_DepthBuffer->GetDepth(0, y);

	u32* __restrict line_scan;
	float* __restrict depth_scan;

	//

	__m128 v_zuvl_scan = v_zuvl;
#if FORCE_WHITE_COLOR == 0
	__m128 v_rgb0_scan = v_rgb0;
#endif

	line_scan = line;
	depth_scan = depth + x1;

	for (unsigned int x = x1; x < x2; ++x, ++depth_scan, ++line_scan)
	{
		__m128 depthSS = _mm_load_ss(depth_scan);
		__m128 zPass = _mm_cmplt_ss(depthSS, v_zuvl_scan);

		if (_mm_movemask_ps(zPass) & 1)
		{
			_mm_store_ss(depth_scan, v_zuvl_scan);

			__m128 zRcpSS = _mm_rcp_ss(v_zuvl_scan);
			__m128 zRcpVec = _mm_shuffle_ps(zRcpSS, zRcpSS, _MM_SHUFFLE(0,0,0,0));

#if USE_BILINEAR == 1
			__m128 zuvl = _mm_mul_ps(v_zuvl_scan, zRcpVec);
			const float tu = _MM_ACCESS(zuvl, 1);
			const float tv = _MM_ACCESS(zuvl, 2);
			__m128 texColor = SampleB(tu, tv, 0);
#endif

#if USE_NEAREST == 1
			__m128 zuvl = _mm_mul_ps(v_zuvl_scan, zRcpVec);
			const float tu = _MM_ACCESS(zuvl, 1);
			const float tv = _MM_ACCESS(zuvl, 2);
			__m128 texColor = Sample(tu, tv, 0);
#endif

			__m128 color;

#if USE_SOLID_COLOR == 1
			color = kVec255;
#elif FORCE_WHITE_COLOR == 1
			color = _mm_mul_ps(texColor, kVec255);
#elif FORCE_WHITE_COLOR == 0
			color = _mm_mul_ps(v_rgb0_scan, zRcpVec);
			color = _mm_mul_ps(color, texColor);
#else
	#error
#endif

#if 0
			*line_scan =
				/* R */ _mm_cvtss_si32(color) << 16 |
				/* G */ _mm_cvtss_si32(_mm_shuffle_ps(color, color, _MM_SHUFFLE(1,1,1,1))) << 8 |
				/* B */ _mm_cvtss_si32(_mm_shuffle_ps(color, color, _MM_SHUFFLE(2,2,2,2)));
#else
			*line_scan =
				/* R */ _mm_cvtss_si32(color) << g_Surface.shiftR |
				/* G */ _mm_cvtss_si32(_mm_shuffle_ps(color, color, _MM_SHUFFLE(1,1,1,1))) << g_Surface.shiftG |
				/* B */ _mm_cvtss_si32(_mm_shuffle_ps(color, color, _MM_SHUFFLE(2,2,2,2))) << g_Surface.shiftB;
#endif
		}

		v_zuvl_scan = _mm_add_ps(v_zuvl_scan, dzuvl_dx);
#if FORCE_WHITE_COLOR == 0
		v_rgb0_scan = _mm_add_ps(v_rgb0_scan, drgb0_dx);
#endif
	}
}

//

#define EXT_UNDEFINED -10000

static int g_ScanExtents[2];

// draw horizontal spans

static void Fill(int y1, int y2)
{
	if (y1 < 0)
		y1 = 0;
	if (y2 > SY)
		y2 = SY;
	
	for (int y = y1; y < y2; ++y)
	{
		if (!g_ScanVerts[y][0].set)
			continue;

		assert(g_ScanVerts[y][0].set);
		assert(g_ScanVerts[y][1].set);

		Fill(y);
	}
}

static void Fill()
{
	Fill(g_ScanExtents[0], g_ScanExtents[1]);
}

// fill span structure across line

static void ScanBegin()
{
	g_ScanExtents[0] = EXT_UNDEFINED;
	g_ScanExtents[1] = EXT_UNDEFINED;
}

static void Scan(const Vert * __restrict v1, const Vert * __restrict v2)
{
	if (v1->y > v2->y)
		std::swap(v1, v2);

	const int y1 = (int)v1->y;
	const int y2 = (int)v2->y;

	if (y1 == y2)
		return;

	if (y1 < g_ScanExtents[0] || g_ScanExtents[0] == EXT_UNDEFINED)
		g_ScanExtents[0] = y1;
	if (y2 > g_ScanExtents[1] || g_ScanExtents[1] == EXT_UNDEFINED)
		g_ScanExtents[1] = y2;
	
	__m128 v1_zuvl = _mm_set_xyzw_ps(1.0f, v1->info.u, v1->info.v, v1->info.l);
	__m128 v2_zuvl = _mm_set_xyzw_ps(1.0f, v2->info.u, v2->info.v, v2->info.l);

#if FORCE_WHITE_COLOR == 0
	__m128 v1_rgb0 = v1->info.rgb0;
	__m128 v2_rgb0 = v2->info.rgb0;
#else
	// set color to white

	__m128 v1_rgb0 = _mm_set_ps1(1.0f);
	__m128 v2_rgb0 = _mm_set_ps1(1.0f);
#endif

	// go from normalized coordinates to coordinates we can directly use in RGB color calculations

#if 0
	v1_rgb0 = _mm_mul_ps(v1_rgb0, _mm_set_ps1(255.0f));
	v2_rgb0 = _mm_mul_ps(v2_rgb0, _mm_set_ps1(255.0f));
#endif

#if USE_MIPMAP == 1 || USE_TRILINEAR == 1
	// calculate mip levels

	{
	const float dx = v2->x * v2->info.z - v1->x * v1->info.z;
	const float dy = v2->y * v2->info.z - v1->y * v1->info.z;
	const float d = sqrtf(dx * dx + dy * dy);
	const float dtu = (v2->info.u - v1->info.u) * g_Texture->m_Levels[0]->m_Size;
	const float dtv = (v2->info.v - v1->info.v) * g_Texture->m_Levels[0]->m_Size;
	const float dt = sqrtf(dtu * dtu + dtv * dtv);

	const float d1 = dt / d / v1->info.z;
	const float d2 = dt / d / v2->info.z;

	//printf("d1: %g, d2: %g\n", d1, d2);

	float v1_l = 1.0f / d1 * 2.0f;
	float v2_l = 1.0f / d2 * 2.0f;

	_MM_ACCESS(v1_zuvl, 3) = v1_l;
	_MM_ACCESS(v2_zuvl, 3) = v2_l;

	//printf("l1: %g, l2: %g\n", v1->info.l, v2->info.l);
	}
#endif

#if 0
	const float lo = 0.1f;
	const float ls = 4.0f;

	v1->info.r = (v1->info.l + lo) / ls;
	v1->info.g = (v1->info.l + lo) / ls;
	v1->info.b = (v1->info.l + lo) / ls;

	v2->info.r = (v2->info.l + lo) / ls;
	v2->info.g = (v2->info.l + lo) / ls;
	v2->info.b = (v2->info.l + lo) / ls;
#endif

	//

	const float dy = v2->y - v1->y;
	const float dyRcp = 1.0f / dy;
	const __m128 dyRcpVec = _mm_set_ps1(dyRcp);

	// move to 1/z space

#if 0
	const float v1_zRcp = 1.0f / v1->info.z;
	const __m128 v1_zRcpVec = _mm_set_ps1(v1_zRcp);
#else
	const __m128 v1_zRcp = _mm_rcp_ss(_mm_set_ss(v1->info.z));
	const __m128 v1_zRcpVec = _mm_shuffle_ps(v1_zRcp, v1_zRcp, _MM_SHUFFLE(0,0,0,0));
#endif
	v1_zuvl = _mm_mul_ps(v1_zuvl, v1_zRcpVec);
	v1_rgb0 = _mm_mul_ps(v1_rgb0, v1_zRcpVec);

#if 0
	const float v2_zRcp = 1.0f / v2->info.z;
	const __m128 v2_zRcpVec = _mm_set_ps1(v2_zRcp);
#else
	const __m128 v2_zRcp = _mm_rcp_ss(_mm_set_ss(v2->info.z));
	const __m128 v2_zRcpVec = _mm_shuffle_ps(v2_zRcp, v2_zRcp, _MM_SHUFFLE(0,0,0,0));
#endif
	v2_zuvl = _mm_mul_ps(v2_zuvl, v2_zRcpVec);
	v2_rgb0 = _mm_mul_ps(v2_rgb0, v2_zRcpVec);

	// calculate line segment deltas

	const float dx_dy = (v2->x - v1->x) * dyRcp;
	__m128 dzuvl_dy = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dyRcpVec);
	__m128 drgb0_dy = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dyRcpVec);

	// trace line from y1 to y2

	float x = v1->x;
	VertInfo v = v1->info;
	__m128 v_zuvl = v1_zuvl;
	__m128 v_rgb0 = v1_rgb0;

#if USE_SUBPIXEL == 1
	const float yd = y1 - v1->y + 1.0f;

	x += dx_dy * yd;
	__m128 ydVec = _mm_set_ps1(yd);
	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dy, ydVec));
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dy, ydVec));
#endif

	for (int y = y1; y < y2; ++y)
	{
		// skip lines above or below the destination bitmap

		if (y >= 0 && y < (int)g_Surface.sy)
		{
			ScanVert* __restrict sv = &g_ScanVerts[y][0];

			if (sv->set)
				sv = &g_ScanVerts[y][1];

			assert(!sv->set);

			sv->x = x;
			sv->info = v;
			sv->info.zuvl = v_zuvl;
#if FORCE_WHITE_COLOR == 0
			sv->info.rgb0 = v_rgb0;
#endif
			sv->Set();
		}

		// advance

		x += dx_dy;
		v_zuvl = _mm_add_ps(v_zuvl, dzuvl_dy);
		v_rgb0 = _mm_add_ps(v_rgb0, drgb0_dy);
	}
}

// fill span structure for triangle

static void Scan(const Vert * __restrict v)
{
	Scan(v + 0, v + 1);
	Scan(v + 1, v + 2);
	Scan(v + 2, v + 0);
}

static void Scan(const Vert * __restrict v, u32 n)
{
	for (u32 prev = n - 1, curr = 0; curr < n; prev = curr, curr++)
	{
		Scan(v + prev, v + curr);
	}
}

#if 1
static void RenderMd3(const SimdMat4x4 & mat, const Md3 & md3)
{
	const float midX = g_Surface.sx / 2.0f;
	const float midY = g_Surface.sy / 2.0f;

	Vert tri[3];

	for (unsigned int i = 0; i < md3.SurfCount; ++i)
	{
		const DataSurf * __restrict s = &md3.SurfList[i];

		const unsigned int * __restrict indexPtr = s->IndexList;

		for (unsigned int j = s->IndexCount / 3; j != 0; --j)
		{
			Vert* __restrict triPtr = tri;

			for (unsigned int k = 0; k < 3; ++k, ++triPtr, ++indexPtr)
			{
				const unsigned int index = *indexPtr;

				const DataVert * __restrict dv = s->VertList + index;

				SimdVec v = mat.Mul4x4(dv->p);
				
				triPtr->x = v.X();
				triPtr->y = v.Y();
				triPtr->info.z = v.Z();
				triPtr->info.z += 40.0f;

				assert(triPtr->info.z > 0.0f);
				triPtr->x /= triPtr->info.z;
				triPtr->y /= triPtr->info.z;
				triPtr->x *= 400.0f;
				triPtr->y *= 400.0f;
				triPtr->x += midX;
				triPtr->y += midY;
				
#if FORCE_WHITE_COLOR == 0
				triPtr->info.r = 1.0f;
				triPtr->info.g = 1.0f;
				triPtr->info.b = 1.0f;
#endif
				triPtr->info.u = dv->uv[0];
				triPtr->info.v = dv->uv[1];
			}

			ScanBegin();
			Scan(tri);
			Fill();
		}
	}
}
#endif

class Damper
{
public:
	Damper(double falloffPerSec)
		: mValue(0.0)
		, mDesiredValue(0.0)
		, mFalloff(falloffPerSec)
	{
	}

	void Update(double dt)
	{
		const double t1 = pow(mFalloff, dt);
		const double t2 = 1.0 - t1;
		mValue = mDesiredValue * t2 + mValue * t1;
	}

	float GetDesiredValue() const
	{
		return static_cast<float>(mDesiredValue);
	}

	void SetDesiredValue(float value, bool force = false)
	{
		mDesiredValue = value;
		if (force)
			mValue = mDesiredValue;
	}

	float GetValue() const
	{
		return static_cast<float>(mValue);
	}

private:
	double mValue;
	double mDesiredValue;
	double mFalloff;
};

static void Execute()
{
#if defined(WINDOWS)
	HANDLE thread = GetCurrentThread();
	if (!SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL))
		exception("failed to set thread priority");
	if (!SetThreadPriorityBoost(thread, FALSE))
		exception("failed to set thread priority boost");
	if (!SetThreadAffinityMask(thread, 2))
		exception("failed to set thread affinity");
#endif

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0)
		exception("failed to initialize SDL");

	//SDL_Surface* surface = SDL_SetVideoMode(320, 240, 32, 0);
	SDL_Surface* surface = SDL_SetVideoMode(640, 480, 32, 0);
	//SDL_Surface* surface = SDL_SetVideoMode(1920, 1080, 32, SDL_FULLSCREEN);
	//SDL_Surface* surface = SDL_SetVideoMode(640, 480, 32, SDL_FULLSCREEN);

	if (surface == 0)
		exception("failed to set video mode");

	g_Surface.sx = surface->w;
	g_Surface.sy = surface->h;
	g_Surface.shiftR = surface->format->Rshift;
	g_Surface.shiftG = surface->format->Gshift;
	g_Surface.shiftB = surface->format->Bshift;

	g_DepthBuffer = new DepthBuffer();
	g_DepthBuffer->Initialize(g_Surface.sx, g_Surface.sy);

	Texture* texture1 = new Texture();
	Texture* texture2 = new Texture();
	texture1->Load("texture.bmp");
	texture2->Load("shotgun.bmp");
	g_Texture = texture2;

	Md3 md3;

	md3.Load("shotgun.md3");
	u32 surfCount = md3.SurfCount;
	u32 vertexCount = 0;
	u32 indexCount = 0;
	for (u32 i = 0; i < surfCount; ++i)
	{
		vertexCount += md3.SurfList[i].VertCount;
		indexCount += md3.SurfList[i].IndexCount;
	}
	printf("MD3: %d surfaces, %d total vertices, %d total indices\n", surfCount, vertexCount, indexCount);

	Vert backQuad[4] =
	{
		Vert(-1.0f, -1.0f, 100, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f),
		Vert(+1.0f, -1.0f, 100, 1.0f, 0.0f, 0.5f, 1.0f, 0.5f),
		Vert(+1.0f, +1.0f, 100, 1.0f, 1.0f, 0.5f, 0.5f, 1.0f),
		Vert(-1.0f, +1.0f, 100, 0.0f, 1.0f, 0.5f, 0.5f, 1.0f),
	};

	Vert quad[4];

	Damper mrx(0.00001f);
	Damper mry(0.00001f);

	u32 frame = 0;

	bool stop = false;
	bool flat = false;
	u32 tn = 1;
	u32 time = 0;

	while (stop == false)
	{
		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q)
					stop = true;
				if (e.key.keysym.sym == SDLK_LSHIFT)
					g_Texture = g_Texture == texture1 ? texture2 : texture1;
				if (e.key.keysym.sym == SDLK_SPACE)
					flat = true;
				if (e.key.keysym.sym == SDLK_t)
					tn = (tn + 1) % 5;
			}
			if (e.type == SDL_KEYUP)
			{
				if (e.key.keysym.sym == SDLK_SPACE)
					flat = false;
			}
			if (e.type == SDL_MOUSEMOTION)
			{
				mrx.SetDesiredValue(mrx.GetDesiredValue() + e.motion.xrel / 20.0f);
				mry.SetDesiredValue(mry.GetDesiredValue() + e.motion.yrel / 20.0f);
			}
			if (e.type == SDL_QUIT)
			{
				stop = true;
			}
		}

		const float dt = 1.0f / 60.0f;

		mrx.Update(dt);
		mry.Update(dt);

		if (SDL_LockSurface(surface) < 0)
			exception("failed to lock surface");

		g_Surface.pixels = reinterpret_cast<u32*>(surface->pixels);
		g_Surface.pitch = surface->pitch >> 2;

		g_DepthBuffer->ClearZero();

		time -= SDL_GetTicks();;

		SimdMat4x4 backR;
		//backR.MakeRotationZ(frame / 1000.0f);
		backR.MakeIdentity();

#if 0
		for (u32 i = 0; i < 4; ++i)
		{
			SimdVec vert(backQuad[i].x, backQuad[i].y, backQuad[i].info.z);
			vert = backR.Mul4x4(vert);

			const float s = g_Surface.sx / 2.0f;

			quad[i] = backQuad[i];
			quad[i].x = vert.X();
			quad[i].y = vert.Y();
			quad[i].info.z = 1000.0f;

			quad[i].x *= s;
			quad[i].y *= s;

			quad[i].x += g_Surface.sx / 2.0f;
			quad[i].y += g_Surface.sy / 2.0f;
		}

		ScanBegin();
		Scan(quad, 4);
		Fill();
#endif

		//const u32 tn = 1;
		//const u32 tn = 5;
		//const u32 tn = 3;
		//const u32 tn = 0;
		const u32 qn = 3;

		for (u32 t = 0; t < tn; ++t)
		{
			for (u32 i = 0; i < qn; ++i)
			{
				const float r1 = t * 2.0f * (float)M_PI / tn + frame / 100.0f;
				const float r = t - i * 2.0f * (float)M_PI / qn + frame * ((t + 1) / (float)tn) / 10.0f;

				//const float size1 = 300.0f;
				const float size1 = 256.0f;
				//const float size = 512.0f;
				//const float size = 256.0f;
				//const float size = 128.0f;
				const float size = 64.0f;
				//const float size = 16.0f;

				const float x = sinf(r1) * size1;
				const float y = cosf(r1) * size1;

				quad[i] = backQuad[i];

				quad[i].x = x + sinf(r) * size;
				quad[i].y = y + cosf(r) * size;

				if (flat == false)
					quad[i].info.z = i == 0 ? 2.0f : 1.0f;
				else
					quad[i].info.z = 1.0f;

				quad[i].x /= quad[i].info.z;
				quad[i].y /= quad[i].info.z;

				quad[i].x += g_Surface.sx / 2.0f;
				quad[i].y += g_Surface.sy / 2.0f;

#if FORCE_WHITE_COLOR == 0
				quad[i].info.r = (cosf(r * 10.0f) + 1.0f) / 2.0f;
				quad[i].info.g = (cosf(r * 7.5f) + 1.0f) / 2.0f;
				quad[i].info.b = (cosf(r * 5.5f) + 1.0f) / 2.0f;
#endif
			}

			ScanBegin();
			Scan(quad, qn);
			Fill();
		}

#if 1
		{
		const int mn = 25;
		static float rx[mn];
		static float ry[mn];
		static float rz[mn];
		for (int i = 0; i < mn; ++i)
		{
			float speed = i / 10.0f;
			SimdMat4x4 matR;
			SimdMat4x4 matT;
			SimdMat4x4 matS;
			rx[i] += 0.01f * speed;
			ry[i] += 0.02f * speed;
			rz[i] += 0.03f * speed;
			SimdMat4x4 matRx;
			SimdMat4x4 matRy;
			SimdMat4x4 matRz;
			matRx.MakeRotationX(mrx.GetValue() + rx[i]);
			matRy.MakeRotationY(                 ry[i]);
			matRz.MakeRotationZ(mry.GetValue() + rz[i]);
			matR = matRz * matRy * matRx;
			int is = 5;
			int ix = i / is;
			int iy = i - ix * is;
			ix -= (is - 1) / 2;
			iy -= (is - 1) / 2;
			matT.MakeTranslation(ix * 5.0f, iy * 5.0f, 0.0f);
			//float scale = 4.0f;
			//float scale = 2.0f;
			//float scale = 1.0f;
			//float scale = 0.8f;
			float scale = 0.5f;
			matS.MakeScaling3f(scale, scale, scale);
			SimdMat4x4 mat = matT * matR * matS;

			RenderMd3(mat, md3);
		}
		}
#endif

		time += SDL_GetTicks();

#if 1
		if (frame % 100 == 0)
		{
			printf("time: %gms\n", time / 100.0f);
			time = 0;
		}
#endif

		SDL_UnlockSurface(surface);

		if (SDL_Flip(surface) < 0)
			exception("failed to flip back buffer");

		++frame;
	}

	g_Texture = 0;

	delete texture1;
	texture1 = 0;
	delete texture2;
	texture2 = 0;

	delete g_DepthBuffer;
	g_DepthBuffer = 0;

	SDL_FreeSurface(surface);

	SDL_Quit();
}

int main(int argc, char* argv[])
{
	try
	{
		Execute();

		return 0;
	}
	catch (std::exception & e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
}
