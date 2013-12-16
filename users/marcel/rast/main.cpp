#include <assert.h>
#include <emmintrin.h>
#include <exception>
#include <math.h>
#include <SDL/SDL.h>
#include <vector>
#include <xmmintrin.h>
#include "../../../libgg/SIMD.h"
#include "../../../libgg/SimdMat4x4.h"
#include "md3.h"

#if 0
	#define FTOI(v) _mm_cvtss_si32(_mm_set_ss(v))
	#define FTOI_PTR(v) _mm_cvtss_si32(_mm_load_ss(v))
#else
	#define FTOI(v) u32(v)
	#define FTOI_PTR(v) u32(*(v))
#endif

#if !defined(_MSC_VER)
	#define _MM_ACCESS_I32(v, i) (((int*)&(v))[i])
#else
	#define _MM_ACCESS_I32(v, i) ((v).m128i_i32[i])
#endif

#if !defined(_MSC_VER)
#include <sys/time.h>
class Timer
{
public:
	void Start()
	{
		gettimeofday(&t1, 0);
	}
	
	void Stop()
	{
		gettimeofday(&t2, 0);
	}
	
	unsigned int usec()
	{
		timeval d;
		timersub(&t2, &t1, &d);
		return d.tv_sec * 1000000 + d.tv_usec;
	}
	
	timeval t1;
	timeval t2;
};
#else
#include <windows.h>
class Timer
{
public:
	void Start()
	{
		t1 = timeGetTime();
	}

	void Stop()
	{
		t2 = timeGetTime();
	}

	unsigned int usec()
	{
		return (t2 - t1) * 1000;
	}

	DWORD t1;
	DWORD t2;
};
#endif

#define ASSERT assert
//#define ASSERT(expr) do { } while (false)

#if 0
class exception : public std::exception
{
public:
	exception(const char * msg) : m_msg(msg) { }
	virtual const char * what() const throw() { return m_msg; }
private:
	const char * m_msg;
};
#else
static void throw_exception(const char* msg)
{
	printf("%s", msg);
	exit(-1);
}
#endif

typedef unsigned int ptr_t;
typedef unsigned char u8;
typedef unsigned int u32;

typedef int TexCoordValue;
typedef int ScreenCoordValue;

#define _mm_set_xyzw_ps(x, y, z, w) _mm_set_ps(w, z, y, x)

#define USE_SOLID_COLOR 0
#define USE_BILINEAR 1
#define USE_NEAREST 0
#define USE_SUBPIXEL 1
#define FORCE_WHITE_COLOR 1
#define USE_SDL_SHIFTS 1

//

#define SY 1280

//static const vec128 kVec1   = _mm_set1_ps(1.0f);
static const vec128 kVec255 = _mm_set1_ps(255.0f);
static const vec128 kVec255Rcp = _mm_set1_ps(1.0f / 255.0f);

//

struct g_Surface
{
	int sx;
	int sy;
	int* pixels;
	int pitch;
	int shiftR;
	int shiftG;
	int shiftB;
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
		m_SizeVec = _mm_set1_ps(size);

		if (m_Size > 0)
		{
			m_SizeMask = m_Size - 1;

			m_Colors = (vec128*)_mm_malloc(m_Size * m_Size * sizeof(vec128), 16);

			while (1U << m_SizeShift != size)
				m_SizeShift++;
		}
	}

	void Load(const char* fileName)
	{
		SDL_Surface * surface = SDL_LoadBMP(fileName);

		if (surface == 0)
			throw_exception("failed to load texture");
		if (surface->w != surface->h)
			throw_exception("texture must be square");
		if (surface->format->BitsPerPixel != 32)
			throw_exception("texture must be 32 bit per pixel");

		unsigned int size = surface->w;

		SetSize(size);

		if (SDL_LockSurface(surface) < 0)
			throw_exception("failed to lock surface");

		vec128 scale = kVec255Rcp;

		for (unsigned int y = 0; y < size; ++y)
		{
			const u32 * __restrict srcLine = reinterpret_cast<u32 *>(surface->pixels) + ((surface->pitch * y) >> 2);
			vec128 * __restrict dstLine = &GetPixel(0, y);

			for (unsigned int x = 0; x < size; ++x)
			{
				const u32 rgb = srcLine[x];
				const u32 r = (rgb & surface->format->Rmask) >> surface->format->Rshift;
				const u32 g = (rgb & surface->format->Gmask) >> surface->format->Gshift;
				const u32 b = (rgb & surface->format->Bmask) >> surface->format->Bshift;

				vec128 temp = _mm_set_xyzw_ps(r, g, b, 255.0f);

				dstLine[x] = _mm_mul_ps(temp, scale);
			}
		}

		SDL_UnlockSurface(surface);
		SDL_FreeSurface(surface);
		surface = 0;
	}

	FORCEINLINE vec128 & GetPixel(TexCoordValue x, TexCoordValue y)
	{
		return m_Colors[x + (y << m_SizeShift)];
	}

	FORCEINLINE const vec128 & GetPixel(TexCoordValue x, TexCoordValue y) const
	{
		return m_Colors[x + (y << m_SizeShift)];
	}

	TextureLevel* DownScale()
	{
		TextureLevel* level = new TextureLevel();

		unsigned int size = m_Size >> 1;

		ASSERT(size >= 1);

		level->SetSize(size);

		for (unsigned int y = 0; y < size; ++y)
		{
			const vec128 * srcLine0 = &GetPixel(0, y * 2 + 0);
			const vec128 * srcLine1 = &GetPixel(0, y * 2 + 1);

			for (unsigned int x = 0; x < size; ++x, srcLine0 += 2, srcLine1 += 2)
			{
				const vec128 & p1 = srcLine0[0];
				const vec128 & p2 = srcLine0[1];
				const vec128 & p3 = srcLine1[1];
				const vec128 & p4 = srcLine1[0];

				vec128 & dst = level->GetPixel(x, y);

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

	TexCoordValue m_Size;
	TexCoordValue m_SizeMask;
	TexCoordValue m_SizeShift;
	vec128 m_SizeVec;
	vec128* m_Colors;

	void* operator new(size_t s)
	{
		return _mm_malloc(s, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
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

	FORCEINLINE float* GetDepth(unsigned int x, unsigned int y)
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
		vec128 zuvl;
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
		vec128 rgb0;
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
//	float x;
	VertInfo info;
};

// list of scan lines, 2 for each y, holding state at x1 and x2

typedef int ScanStateValue;

static ScanStateValue g_ScanState[SY];
static float g_ScanExtentsX[SY][2];
static ScanVert g_ScanVerts[SY][2];

// sample texture

static FORCEINLINE vec128 Sample_Base(TexCoordValue u, TexCoordValue v, unsigned int level) FORCEINLINE_BACK;
static FORCEINLINE vec128 Sample(float u, float v, unsigned int level) FORCEINLINE_BACK;
static FORCEINLINE vec128 SampleB(float u, float v, unsigned int level) FORCEINLINE_BACK;

static FORCEINLINE vec128 Sample_Base(TexCoordValue u, TexCoordValue v, const TextureLevel* __restrict texture)
{
	const TexCoordValue tu = u & texture->m_SizeMask;
	const TexCoordValue tv = v & texture->m_SizeMask;

	return texture->GetPixel(tu, tv);
}

static FORCEINLINE vec128 Sample_Base(TexCoordValue u, TexCoordValue v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	const TexCoordValue tu = u & texture->m_SizeMask;
	const TexCoordValue tv = v & texture->m_SizeMask;

	return texture->GetPixel(tu, tv);
}

static FORCEINLINE vec128 Sample(float u, float v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	const TexCoordValue tu = FTOI(u * texture->m_Size) & texture->m_SizeMask;
	const TexCoordValue tv = FTOI(v * texture->m_Size) & texture->m_SizeMask;

	return texture->GetPixel(tu, tv);
}

static FORCEINLINE vec128 Sample128(vec128 nuvn, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	vec128  nuvnF = _mm_mul_ps(nuvn, texture->m_SizeVec);
	__m128i nuvnI = _mm_cvttps_epi32(nuvnF);

	const TexCoordValue tu = _MM_ACCESS_I32(nuvnI, 1) & texture->m_SizeMask;
	const TexCoordValue tv = _MM_ACCESS_I32(nuvnI, 2) & texture->m_SizeMask;

	return texture->GetPixel(tu, tv);
}

// mix colours

static FORCEINLINE vec128 Mix2(
	const vec128 & c1,
	const vec128 & c2,
	const float * __restrict w)
{
	vec128 cw1 = _mm_mul_ps(c1, _mm_set_ps1(w[0]));
	vec128 cw2 = _mm_mul_ps(c2, _mm_set_ps1(w[1]));

	return _mm_add_ps(cw1, cw2);
}


// mix colours for bilinear texture sampling

static FORCEINLINE vec128 Mix4(
	const vec128 & c1,
	const vec128 & c2,
	const vec128 & c3,
	const vec128 & c4,
	const vec128 & w)
{
	vec128 cw1 = _mm_mul_ps(c1, _mm_shuffle_ps(w, w, _MM_SHUFFLE(0,0,0,0)));
	vec128 cw2 = _mm_mul_ps(c2, _mm_shuffle_ps(w, w, _MM_SHUFFLE(1,1,1,1)));
	vec128 cw3 = _mm_mul_ps(c3, _mm_shuffle_ps(w, w, _MM_SHUFFLE(2,2,2,2)));
	vec128 cw4 = _mm_mul_ps(c4, _mm_shuffle_ps(w, w, _MM_SHUFFLE(3,3,3,3)));

	return
		_mm_add_ps(cw1,
		_mm_add_ps(cw2,
		_mm_add_ps(cw3, cw4)));
}

// sample texture bilinearly

static FORCEINLINE vec128 SampleB(float u, float v, unsigned int level)
{
	const TextureLevel* __restrict texture = g_Texture->m_Levels[level];

	const float tuf = u * texture->m_Size;
	const float tvf = v * texture->m_Size;
	const TexCoordValue tu = FTOI(tuf);
	const TexCoordValue tv = FTOI(tvf);

	vec128 c1 = Sample_Base(tu + 0, tv + 0, texture);
	vec128 c2 = Sample_Base(tu + 1, tv + 0, texture);
	vec128 c3 = Sample_Base(tu + 1, tv + 1, texture);
	vec128 c4 = Sample_Base(tu + 0, tv + 1, texture);

	const float tu2 = tuf - tu;
	const float tv2 = tvf - tv;
	const float tu1 = 1.0f - tu2;
	const float tv1 = 1.0f - tv2;

	vec128 wu = _mm_set_xyzw_ps(tu1, tu2, tu2, tu1);
	vec128 wv = _mm_set_xyzw_ps(tv1, tv1, tv2, tv2);
	vec128 w = _mm_mul_ps(wu, wv);

	return Mix4(c1, c2, c3, c4, w);
}

static FORCEINLINE void Fill(
	vec128 zuvl,
	vec128 dzuvl_dx,
	int* __restrict line,
	float* __restrict depth,
	int x1)
{
	int x4 = x1 >> 2;
	x1 -= x4 << 2;

	if (x4 != 0)
	{
		vec128 dz4 = _mm_shuffle_ps(dzuvl_dx, dzuvl_dx, _MM_SHUFFLE(0,0,0,0));
		vec128 z4;
		float z0 = _MM_ACCESS(zuvl, 0);
		float dz = _MM_ACCESS(dzuvl_dx, 0);
		_MM_ACCESS(z4, 0) = z0;
		_MM_ACCESS(z4, 1) = z0 + dz;
		_MM_ACCESS(z4, 2) = z0 + dz + dz;
		_MM_ACCESS(z4, 3) = z0 + dz + dz + dz;
		for (int x = x4; x != 0; --x, depth += 4, line += 4)
		{
			vec128 depthPS = _mm_loadu_ps(depth);
			vec128 zPass = _mm_cmplt_ps(depthPS, z4);

			// flip bits where a != b && pass
			vec128 diff = _mm_xor_ps(depthPS, z4);
			vec128 pass = zPass;
			vec128 wwww = _mm_xor_ps(_mm_and_ps(diff, pass), z4);
			_mm_storeu_ps(depth, wwww);

			int mask = _mm_movemask_ps(zPass);
			if (mask)
			{
				if (mask & 1)
					line[0] = 0xffffffff;
				if (mask & 2)
					line[1] = 0xffffffff;
				if (mask & 4)
					line[2] = 0xffffffff;
				if (mask & 8)
					line[3] = 0xffffffff;
			}
			z4 = _mm_add_ps(z4, dz4);
		}
	}

	for (int x = x1; x != 0; --x, ++depth, ++line)
	{
		vec128 depthSS = _mm_load_ss(depth);

		if (_mm_movemask_ps(_mm_cmplt_ss(depthSS, zuvl)) & 1)
		{
			_mm_store_ss(depth, zuvl);

			*line = 0xffffffff;
		}

		zuvl = _mm_add_ps(zuvl, dzuvl_dx);
	}
}

// draw a single horizontal span from x1 to x2

static void Fill(ScreenCoordValue y)
{
	ASSERT(y < g_Surface.sy);
	
	float x1f = g_ScanExtentsX[y][0];
	float x2f = g_ScanExtentsX[y][1];
	
	if (x1f == x2f)
		return;

	ScanVert* __restrict v1 = &g_ScanVerts[y][0];
	ScanVert* __restrict v2 = &g_ScanVerts[y][1];
	
	if (x1f > x2f)
	{
		ScanVert* temp = v1;
		v1 = v2;
		v2 = temp;
		
		float temp2 = x1f;
		x1f = x2f;
		x2f = temp2;
	}

	const vec128 _dxVec = _mm_set_ss(x2f - x1f);
	const vec128 dxRcpSS = _mm_rcp_ss(_dxVec);
	const vec128 dxRcpVec = _mm_shuffle_ps(dxRcpSS, dxRcpSS, _MM_SHUFFLE(0,0,0,0));

	vec128 v1_zuvl = v1->info.zuvl;
	vec128 v2_zuvl = v2->info.zuvl;
#if FORCE_WHITE_COLOR == 0
	vec128 v1_rgb0 = _mm_mul_ps(v1->info.rgb0, kVec255);
	vec128 v2_rgb0 = _mm_mul_ps(v2->info.rgb0, kVec255);
#endif

	vec128 dzuvl_dx = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dxRcpVec);
#if FORCE_WHITE_COLOR == 0
	vec128 drgb0_dx = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dxRcpVec);
#endif

	vec128 v_zuvl = v1_zuvl;
#if FORCE_WHITE_COLOR == 0
	vec128 v_rgb0 = v1_rgb0;
#endif

	//vec128 dx = _mm_min_ps(_mm_setzero_ps(), _mm_set_ps1(x1f));
	//vec128 dx2 = _mm_max_ps(_mm_setzero_ps(), _mm_set_ps1(x1f));
	
	if (x1f < 0.0f)
	{
		vec128 dx = _mm_set_ps1(x1f);
		v_zuvl = _mm_sub_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, dx));
#if FORCE_WHITE_COLOR == 0
		v_rgb0 = _mm_sub_ps(v_rgb0, _mm_mul_ps(drgb0_dx, dx));
#endif

		x1f = 0.0f;
	}

	ScreenCoordValue x1 = FTOI(x1f);
	ScreenCoordValue x2 = FTOI(x2f);

	if (x2 > g_Surface.sx)
		x2 = g_Surface.sx;

	if (x2 <= x1)
		return;

	ASSERT(x1 <= 65535);
	ASSERT(x2 <= 65535);

#if USE_SUBPIXEL == 1
	const float xd = x1 - x1f + 1.0f;
	const vec128 xdVec = _mm_set_ps1(xd);

	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, xdVec));
#if FORCE_WHITE_COLOR == 0
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dx, xdVec));
#endif
#endif

	int* __restrict line = g_Surface.pixels + g_Surface.pitch * y;
	float* __restrict depth = g_DepthBuffer->GetDepth(0, y);

	int* __restrict line_scan;
	float* __restrict depth_scan;

	//

	vec128 v_zuvl_scan = v_zuvl;
#if FORCE_WHITE_COLOR == 0
	vec128 v_rgb0_scan = v_rgb0;
#endif

	line_scan = line + x1;
	depth_scan = depth + x1;

#if 0
	Fill(v_zuvl_scan, dzuvl_dx, line_scan, depth_scan, x2 - x1);
	return;
#endif

#if USE_SDL_SHIFTS == 1
	const int shiftR = g_Surface.shiftR;
	const int shiftG = g_Surface.shiftG;
	const int shiftB = g_Surface.shiftB;
#endif

	for (ScreenCoordValue _x = x2 - x1; _x != 0; --_x, ++depth_scan, ++line_scan)
	{
		vec128 depthSS = _mm_load_ss(depth_scan);
		vec128 zPass = _mm_cmplt_ss(depthSS, v_zuvl_scan);

		if (_mm_movemask_ps(zPass) & 1)
		{
			_mm_store_ss(depth_scan, v_zuvl_scan);

			vec128 zRcpSS = _mm_rcp_ss(v_zuvl_scan);
			vec128 zRcpVec = _mm_shuffle_ps(zRcpSS, zRcpSS, _MM_SHUFFLE(0,0,0,0));

#if USE_BILINEAR == 1
			vec128 zuvl = _mm_mul_ps(v_zuvl_scan, zRcpVec);
			const float tu = _MM_ACCESS(zuvl, 1);
			const float tv = _MM_ACCESS(zuvl, 2);
			vec128 texColor = SampleB(tu, tv, 0);
#endif
#if USE_NEAREST == 1
			vec128 zuvl = _mm_mul_ps(v_zuvl_scan, zRcpVec);
#if 0
			const float tu = _MM_ACCESS(zuvl, 1);
			const float tv = _MM_ACCESS(zuvl, 2);
			vec128 texColor = Sample(tu, tv, 0);
#else
			vec128 texColor = Sample128(zuvl, 0);
#endif
#endif

			vec128 color;

#if FORCE_WHITE_COLOR == 1
			color = kVec255;
#else
			color = _mm_mul_ps(v_rgb0_scan, zRcpVec);
#endif

#if USE_SOLID_COLOR == 0
			color = _mm_mul_ps(color, texColor);
#endif

			__m128i v = _mm_cvttps_epi32(color);

#if 1
			*line_scan =
				/* R */ _MM_ACCESS_I32(v, 0) << shiftR |
				/* G */ _MM_ACCESS_I32(v, 1) << shiftG |
				/* B */ _MM_ACCESS_I32(v, 2) << shiftB;
#else
			_mm_storeu_si128((__m128i*)line_scan, v);
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

static FORCEINLINE void Fill(int y1, int y2)
{
	ASSERT(y1 >= 0);
	ASSERT(y2 <= g_Surface.sy);

	if (y1 < 0)
		y1 = 0;
	if (y2 > g_Surface.sy)
		y2 = g_Surface.sy;
	
	for (int y = y1; y < y2; ++y)
	{
		ASSERT(g_ScanState[y] == 2);
		//if (g_ScanState[y] == 2)
		{
			Fill(y);
		}

		g_ScanState[y] = 0;
	}
}

static FORCEINLINE void Fill()
{
	if (g_ScanExtents[0] != EXT_UNDEFINED)
	{
		Fill(g_ScanExtents[0], g_ScanExtents[1]);
	}
}

// fill span structure across line

static FORCEINLINE void ScanBegin()
{
	g_ScanExtents[0] = EXT_UNDEFINED;
	g_ScanExtents[1] = EXT_UNDEFINED;
}

static void Scan(const Vert * __restrict v1, const Vert * __restrict v2)
{
	if (v1->y > v2->y)
		std::swap(v1, v2);

	int y1 = FTOI_PTR(&v1->y);
	int y2 = FTOI_PTR(&v2->y);
	//int y1 = (int)v1->y;
	//int y2 = (int)v2->y;

	if (y1 == y2)
		return;
	if (y1 >= g_Surface.sy || y2 <= 0)
		return;
	
	vec128 v1_zuvl = _mm_set_xyzw_ps(1.0f, v1->info.u, v1->info.v, v1->info.l);
	vec128 v2_zuvl = _mm_set_xyzw_ps(1.0f, v2->info.u, v2->info.v, v2->info.l);

#if FORCE_WHITE_COLOR == 0
	vec128 v1_rgb0 = v1->info.rgb0;
	vec128 v2_rgb0 = v2->info.rgb0;
#endif

	//

	const float dy = v2->y - v1->y;
	const float dyRcp = 1.0f / dy;
	const vec128 dyRcpVec = _mm_set_ps1(dyRcp);

	// move to 1/z space

	const vec128 v1_zRcp = _mm_rcp_ss(_mm_load_ss(&v1->info.z));
	const vec128 v2_zRcp = _mm_rcp_ss(_mm_load_ss(&v2->info.z));
	const vec128 v1_zRcpVec = _mm_shuffle_ps(v1_zRcp, v1_zRcp, _MM_SHUFFLE(0,0,0,0));
	const vec128 v2_zRcpVec = _mm_shuffle_ps(v2_zRcp, v2_zRcp, _MM_SHUFFLE(0,0,0,0));

	v1_zuvl = _mm_mul_ps(v1_zuvl, v1_zRcpVec);
	v2_zuvl = _mm_mul_ps(v2_zuvl, v2_zRcpVec);

#if FORCE_WHITE_COLOR == 0
	v1_rgb0 = _mm_mul_ps(v1_rgb0, v1_zRcpVec);
	v2_rgb0 = _mm_mul_ps(v2_rgb0, v2_zRcpVec);
#endif

	// calculate line segment deltas

	const float dx_dy = (v2->x - v1->x) * dyRcp;
	vec128 dzuvl_dy = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dyRcpVec);
#if FORCE_WHITE_COLOR == 0
	vec128 drgb0_dy = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dyRcpVec);
#endif

	// trace line from y1 to y2

	float x = v1->x;

	vec128 v_zuvl = v1_zuvl;
#if FORCE_WHITE_COLOR == 0
	vec128 v_rgb0 = v1_rgb0;
#endif

#if USE_SUBPIXEL == 1
	const float yd = y1 - v1->y + 1.0f;
	vec128 ydVec = _mm_set_ps1(yd);

	x += dx_dy * yd;

	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dy, ydVec));
#if FORCE_WHITE_COLOR == 0
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dy, ydVec));
#endif
#endif

	if (y1 < 0)
	{
		vec128 ydVec = _mm_set_ps1(y1);
		x -= dx_dy * y1;
		v_zuvl = _mm_sub_ps(v_zuvl, _mm_mul_ps(dzuvl_dy, ydVec));
#if FORCE_WHITE_COLOR == 0
		v_rgb0 = _mm_sub_ps(v_rgb0, _mm_mul_ps(drgb0_dy, ydVec));
#endif
		y1 = 0;
	}
	if (y2 > g_Surface.sy)
		y2 = g_Surface.sy;

	ASSERT(y1 < y2);

	if (y1 < g_ScanExtents[0] || g_ScanExtents[0] == EXT_UNDEFINED)
		g_ScanExtents[0] = y1;
	if (y2 > g_ScanExtents[1] || g_ScanExtents[1] == EXT_UNDEFINED)
		g_ScanExtents[1] = y2;

	for (int y = y1; y < y2; ++y)
	{
		// skip lines above or below the destination bitmap

		ScanStateValue state = g_ScanState[y];
		
		ASSERT(state <= 1);

		//if (state <= 1)
		{
			g_ScanExtentsX[y][state] = x;
			
			ScanVert* __restrict sv = g_ScanVerts[y] + state;
			
			sv->info.zuvl = v_zuvl;
#if FORCE_WHITE_COLOR == 0
			sv->info.rgb0 = v_rgb0;
#endif
		
			g_ScanState[y] = state + 1;
		}

		// advance

		x += dx_dy;
		v_zuvl = _mm_add_ps(v_zuvl, dzuvl_dy);
#if FORCE_WHITE_COLOR == 0
		v_rgb0 = _mm_add_ps(v_rgb0, drgb0_dy);
#endif
	}
}

// fill span structure for triangle

static FORCEINLINE void Scan(const Vert * __restrict v)
{
	Scan(v + 0, v + 1);
	Scan(v + 1, v + 2);
	Scan(v + 2, v + 0);
}

static FORCEINLINE void Scan(const Vert * __restrict v, u32 n)
{
	for (u32 prev = n - 1, curr = 0; curr < n; prev = curr++)
	{		
		Scan(v + prev, v + curr);
	}
}

#if 1
static SimdVec screenSize(240.0f, 240.0f);
static SimdVec mid;

static FORCEINLINE void WriteVert(Vert * __restrict rv, const DataVert * __restrict dv, SimdVecArg v, SimdVecArg p) FORCEINLINE_BACK;

static FORCEINLINE void WriteVert(Vert * __restrict rv, const DataVert * __restrict dv, SimdVecArg v, SimdVecArg p)
{
	ASSERT(v.Z() > 0.0f);
#if 0
	SimdVec p2 = p.Mul(screenSize).Add(mid);
	rv->x = p2.X();
	rv->y = p2.Y();
#elif 1
	//rv->x = p.X() * screenSize.X() + mid.X();
	//rv->y = p.Y() * screenSize.Y() + mid.Y();
	rv->x = v.X() / v.Z() * screenSize.X() + mid.X();
	rv->y = v.Y() / v.Z() * screenSize.Y() + mid.Y();
#else
	rv->x = v.X() / v.Z() * 4.0f * screenSize.X() + mid.X();
	rv->y = v.Y() / v.Z() * 4.0f * screenSize.Y() + mid.Y();
	//rv->x = p.X() * 4.0f * screenSize.X() + mid.X();
	//rv->y = p.Y() * 4.0f * screenSize.Y() + mid.Y();
#endif
	rv->info.z = v.Z();

#if USE_SOLID_COLOR == 0
	rv->info.u = dv->uv[0];
	rv->info.v = dv->uv[1];
#endif

#if FORCE_WHITE_COLOR == 0
#if 1
	rv->info.r = 1.0f;
	rv->info.g = 1.0f;
	rv->info.b = 1.0f;
#else
	SimdVec n = v.UnitVec3() ;
	rv->info.r = (n.X() + 1.0f) * 0.5f;
	rv->info.g = (n.Y() + 1.0f) * 0.5f;
	rv->info.b = (n.Z() + 1.0f) * 0.5f;
#endif
#endif
}

static void RenderMd3(const SimdMat4x4 & mat, const Md3 & md3)
{
	mid = SimdVec(g_Surface.sx / 2.0f, g_Surface.sy / 2.0f, 0.0f, 0.0f);

#if 1
	SimdMat4x4 temp = mat.CalcTranspose();
#endif

	Vert tri[3];

	for (unsigned int i = 0; i < md3.SurfCount; ++i)
	{
		const DataSurf * __restrict s = &md3.SurfList[i];
		const DataVert * __restrict dv = s->VertList;

		const unsigned int * __restrict indexPtr = s->IndexList;

		for (unsigned int j = s->IndexCount / 3; j != 0; --j, indexPtr += 3)
		{
			const unsigned int index1 = indexPtr[0];
			const unsigned int index2 = indexPtr[1];
			const unsigned int index3 = indexPtr[2];
			
			const DataVert * __restrict dv1 = dv + index1;
			const DataVert * __restrict dv2 = dv + index2;
			const DataVert * __restrict dv3 = dv + index3;
			
			SimdVec v1 = mat.Mul4x4(dv1->p);
			SimdVec v2 = mat.Mul4x4(dv2->p);
			SimdVec v3 = mat.Mul4x4(dv3->p);

			//printf("%g, %g, %g\n", v1(0), v1(1), v1(2));
			
#if 1
			SimdVec p1 = v1.Div(v1.ReplicateZ());
			SimdVec p2 = v2.Div(v2.ReplicateZ());
			SimdVec p3 = v3.Div(v3.ReplicateZ());

			SimdVec d1 = p2.Sub(p1);
			SimdVec d2 = p3.Sub(p2);
			SimdVec dc = d1.Cross3(d2);
			
#if 1
			SimdVec zzzz = dc.ReplicateZ();
			if (zzzz.ANY_LE4(VEC_ZERO))
				continue;
#else
			if (dc.Z() <= 0.0f)
				continue;
#endif
#endif

			WriteVert(tri + 0, dv1, v1, p1);
			WriteVert(tri + 1, dv2, v2, p2);
			WriteVert(tri + 2, dv3, v3, p3);
			
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

#ifdef WINDOWS
int mswindows_handle_hardware_exceptions (DWORD code)
{
	printf("Handling exception\n");
	if (code == STATUS_DATATYPE_MISALIGNMENT)
	{
		printf("misalignment fault!\n");
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void Execute()
{
#if defined(WINDOWS)
	HANDLE process = GetCurrentProcess();
	SetPriorityClass(process, HIGH_PRIORITY_CLASS);
	HANDLE thread = GetCurrentThread();
	if (!SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST))
		throw_exception("failed to set thread priority");
	if (!SetThreadPriorityBoost(thread, FALSE))
		throw_exception("failed to set thread priority boost");
	//if (!SetThreadAffinityMask(thread, 2))
	//	throw_exception("failed to set thread affinity");
#endif

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0)
		throw_exception("failed to initialize SDL");

	SDL_Surface* surface = SDL_SetVideoMode(640, 480, 32, 0);
	//SDL_Surface* surface = SDL_SetVideoMode(1024, 768, 32, 0);
	//SDL_Surface* surface = SDL_SetVideoMode(320, 240, 32, 0);
	//SDL_Surface* surface = SDL_SetVideoMode(1920, 1080, 32, SDL_FULLSCREEN);
	//SDL_Surface* surface = SDL_SetVideoMode(1280, 1024, 32, SDL_FULLSCREEN);

	if (surface == 0)
		throw_exception("failed to set video mode");

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
	//texture2->Load("shotgun.bmp");
	texture2->Load("briareos.bmp");
	g_Texture = texture2;

	Md3 md3;

	md3.Load("shotgun.md3");
	//md3.Load("briareos-upper.md3");
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
	bool clear = false;
	bool flat = false;
	bool back = false;

	while (stop == false)
	{
#ifdef WINDOWS
		__try
		{
#endif

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
				if (e.key.keysym.sym == SDLK_b)
					back = true;
				if (e.key.keysym.sym == SDLK_c)
					clear = true;
			}
			if (e.type == SDL_KEYUP)
			{
				if (e.key.keysym.sym == SDLK_SPACE)
					flat = false;
				if (e.key.keysym.sym == SDLK_b)
					back = false;
				if (e.key.keysym.sym == SDLK_c)
					clear = false;
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
			throw_exception("failed to lock surface");

		g_Surface.pixels = reinterpret_cast<int*>(surface->pixels);
		g_Surface.pitch = surface->pitch >> 2;
		
		if (clear)
		{
			for (int i = 0; i < g_Surface.sy; ++i)
			{
				int* line = g_Surface.pixels + g_Surface.pitch * i;
				memset(line, 0, sizeof(u32) * g_Surface.sx);
			}
		}

		g_DepthBuffer->ClearZero();

		SimdMat4x4 backR;
		//backR.MakeRotationZ(frame / 1000.0f);
		backR.MakeIdentity();

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

#if 1
		if (back)
		{
			ScanBegin();
			Scan(quad, 4);
			Fill();
		}
#endif

		//const u32 tn = 1;
		//const u32 tn = 5;
		//const u32 tn = 3;
		const u32 tn = 0;
		const u32 qn = 3;

		for (u32 t = 0; t < tn; ++t)
		{
			for (u32 i = 0; i < qn; ++i)
			{
				const float r1 = t * 2.0f * (float)M_PI / (tn + 1e-10f) + frame / 100.0f;
				const float r = t - i * 2.0f * (float)M_PI / qn + frame * ((t + 1) / (float)(tn+1e-10f)) / 10.0f;

				//const float size1 = 300.0f;
				const float size1 = 256.0f;
				//const float size = 512.0f;
				//const float size = 256.0f;
				//const float size = 128.0f;
				const float size = 64.0f;

				const float x = sinf(r1) * size1;
				const float y = cosf(r1) * size1;

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
		
		Timer time;
		
		time.Start();

#if 1
		{
		//const int mn = 1;
		//const int mn = 25;
		const int mn = 49;
		//const int mn = 9;
		static float rx[mn];
		static float ry[mn];
		static float rz[mn];
		for (int i = 0; i < mn; ++i)
		{
			SimdMat4x4 matV;
			SimdMat4x4 matR;
			SimdMat4x4 matT;
			SimdMat4x4 matS;
			float speed = i / 1000.0f;
#if 1
			rx[i] += 0.01f * speed;
			ry[i] += 0.02f * speed;
			rz[i] += 0.03f * speed;
#else
			rx[i] = i * 100.0f;
			ry[i] = i * 100.0f;
			rz[i] = i * 100.0f;
#endif
			SimdMat4x4 matRx;
			SimdMat4x4 matRy;
			SimdMat4x4 matRz;
			matRx.MakeRotationX(mrx.GetValue() + rx[i]);
			matRy.MakeRotationY(                 ry[i]);
			matRz.MakeRotationZ(mry.GetValue() + rz[i]);
			matR = matRz * matRy * matRx;
			int is = FTOI(sqrtf((float)mn));
			int ix = i / is;
			int iy = i - ix * is;
			ix -= (is - 1) / 2;
			iy -= (is - 1) / 2;
			//matT.MakeTranslation(ix * 5.0f, iy * 5.0f, 0.0f);
			matT.MakeTranslation(ix * 2.0f, iy * 2.0f, 10.0f);
			//matT.MakeTranslation(ix * 1.0f, iy * 1.0f, 10.0f);
			//float scale = 4.0f;
			//float scale = 2.0f;
			//float scale = 1.0f;
			//float scale = 0.8f;
			//float scale = 0.25f;
			float scale = 0.1f;
			//float scale = 0.05f;
			//float scale = 0.03f;
			matS.MakeScaling3f(scale, scale, scale);
			matV.MakeIdentity();
			SimdMat4x4 mat = matV * matT * matR * matS;

			RenderMd3(mat, md3);
		}
		}
#endif

		time.Stop();

#if 1
		//if (frame % 100 == 0)
		{
			//printf("time: %gms\n", time / 100.0f);
			printf("time: %gms\n", time.usec() / 1000.0f);
			//time = 0;
		}
#endif

		SDL_UnlockSurface(surface);

		//if ((frame % 10) == 0)
		if (SDL_Flip(surface) < 0)
			throw_exception("failed to flip back buffer");

		++frame;

#ifdef WINDOWS
		}
		__except(mswindows_handle_hardware_exceptions (GetExceptionCode ()))
		{
		}
#endif
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
