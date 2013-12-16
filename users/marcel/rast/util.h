#pragma once

#include "../../../libgg/SIMD.h"

typedef unsigned int ptr_t;
typedef unsigned char u8;
typedef unsigned int u32;

typedef int TexCoordValue;
typedef int ScreenCoordValue;

//static const vec128 kVec1   = _mm_set1_ps(1.0f);
static const vec128 kVec4      = _mm_set1_ps(4.0f);
static const vec128 kVec255    = _mm_set1_ps(255.0f);
static const vec128 kVec255Rcp = _mm_set1_ps(1.0f / 255.0f);

#define PREFETCH(x) _mm_prefetch((char*)(x), _MM_HINT_T0)

#if 1
	#define FTOI(v)     _mm_cvtss_si32(_mm_set_ss(v))
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

static void throw_exception(const char* msg)
{
	printf("%s\n", msg);
	exit(-1);
}

#define _mm_set_xyzw_ps(x, y, z, w) _mm_set_ps(w, z, y, x)

class TextureLevel
{
public:
	TextureLevel()
	{
		m_Size = 0;
		m_SizeMask = 0;
		m_SizeShift = 0;
		m_Colors_U32 = 0;
	}

	void SetSize(unsigned int size)
	{
		delete[] m_Colors_U32;
		m_Colors_U32 = 0;
		m_SizeMask = 0;
		m_SizeShift = 0;

		m_Size = size;
		m_SizeVec = _mm_set1_ps(float(size));

		if (m_Size > 0)
		{
			m_SizeMask = m_Size - 1;

			m_Colors_U32 = new u32[m_Size * m_Size];

			while (1U << m_SizeShift != size)
				m_SizeShift++;
		}
	}

	void Load(
		const char* fileName,
		u32 shiftR,
		u32 shiftG,
		u32 shiftB)
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
			u32    * __restrict dstLine = &GetPixel_U32(0, y);

			for (unsigned int x = 0; x < size; ++x)
			{
				const u32 rgb = srcLine[x];
				const u32 r = (rgb & surface->format->Rmask) >> surface->format->Rshift;
				const u32 g = (rgb & surface->format->Gmask) >> surface->format->Gshift;
				const u32 b = (rgb & surface->format->Bmask) >> surface->format->Bshift;

				vec128 temp = _mm_set_xyzw_ps(
					float(r),
					float(g),
					float(b),
					255.0f);

				dstLine[x] =
					r << shiftR |
					g << shiftG |
					b << shiftB;
			}
		}

		SDL_UnlockSurface(surface);
		SDL_FreeSurface(surface);
		surface = 0;
	}

	FORCEINLINE u32 & GetPixel_U32(TexCoordValue x, TexCoordValue y)
	{
		return m_Colors_U32[x + (y << m_SizeShift)];
	}

	FORCEINLINE const u32 & GetPixel_U32(TexCoordValue x, TexCoordValue y) const
	{
		return m_Colors_U32[x + (y << m_SizeShift)];
	}

	TexCoordValue m_Size;
	TexCoordValue m_SizeMask;
	TexCoordValue m_SizeShift;
	vec128 m_SizeVec;
	u32* m_Colors_U32;

	void* operator new(size_t s)
	{
		return _mm_malloc(s, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
};

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
