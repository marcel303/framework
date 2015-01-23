#pragma once

#include <stdint.h>

#ifdef DEBUG
	// the interface here is only meant to enforce a common interface for each BitmapGraphics implementation. it's not to be
	// used as an interface in other code!
	class IBitmapGraphics
	{
	public:
		IBitmapGraphics(int clipX, int clipY, int clipSx, int clipSy);
		virtual ~IBitmapGraphics();

		virtual uint32_t ColorFromInt(int ri, int gi, int bi) = 0;
		virtual uint32_t ColorFromFloat(float r, float g, float b) = 0;
		virtual void Clear(uint32_t c) = 0;
		virtual void RectFill(int x, int y, int sx, int sy, uint32_t c) = 0;
		virtual void HLine(int x, int y, int sx, uint32_t c) = 0;
		virtual uint32_t * GetLine(int y) = 0;
	};
#else
	class IBitmapGraphics
	{
	public:
		IBitmapGraphics(int clipX, int clipY, int clipSx, int clipSy) { }
	};
#endif

#ifdef WIN32

#include <SDL/SDL.h>
#include <xmmintrin.h>

class BitmapGraphics : public IBitmapGraphics
{
public:
	BitmapGraphics(int clipX, int clipY, int clipSx, int clipSy)
		: IBitmapGraphics(clipX, clipY, clipSx, clipSy)
		, m_surface(0)
		, m_clipX1(clipX)
		, m_clipY1(clipY)
		, m_clipX2(clipX + clipSx - 1)
		, m_clipY2(clipY + clipSy - 1)
		, m_sx(clipSx)
		, m_sy(clipSy)
	{
	}

	void SetSurface(SDL_Surface * surface)
	{
		m_surface = surface;
	}

	uint32_t ColorFromInt(int ri, int gi, int bi)
	{
		return
			(ri << m_surface->format->Rshift) |
			(gi << m_surface->format->Gshift) |
			(bi << m_surface->format->Bshift);
	}

	uint32_t ColorFromFloat(float r, float g, float b)
	{
		__m128 m000 = _mm_setzero_ps();
		__m128 m255 = _mm_set_ps1(255.0f);

		__m128 rgb;
		
		rgb = _mm_set_ps(0.0f, b, g, r);
		rgb = _mm_mul_ps(rgb, m255);
		rgb = _mm_min_ps(rgb, m255);
		rgb = _mm_max_ps(rgb, m000);
		
		__m128 rm = rgb;
		__m128 gm = _mm_shuffle_ps(rgb, rgb, _MM_SHUFFLE(1,1,1,1));
		__m128 bm = _mm_shuffle_ps(rgb, rgb, _MM_SHUFFLE(2,2,2,2));

		int ri = _mm_cvtss_si32(rm);
		int gi = _mm_cvtss_si32(gm);
		int bi = _mm_cvtss_si32(bm);

		return
			(ri << m_surface->format->Rshift) |
			(gi << m_surface->format->Gshift) |
			(bi << m_surface->format->Bshift);
	}

	void Clear(uint32_t c)
	{
		RectFill(0, 0, m_sx, m_sy, c);
	}

	void RectFill(int x, int y, int sx, int sy, uint32_t c)
	{
		for (int i = 0; i < sy; ++i)
		{
			HLine(x, y + i, sx, c);
		}
	}

	void HLine(int x, int y, int sx, uint32_t c)
	{
		x += m_clipX1;
		y += m_clipY1;
		if (y < m_clipY1 || y > m_clipY2 || y < 0 || y > m_surface->h - 1)
			return;
		int x1 = x;
		int x2 = x + sx - 1;
		x1 = x1 < m_clipX1 ? m_clipX1 : x1;
		x2 = x2 > m_clipX2 ? m_clipX2 : x2;
		x1 = x1 < 0 ? 0 : x1;
		x2 = x2 > m_surface->w - 1 ? m_surface->w - 1 : x2;
		uint32_t * line = static_cast<uint32_t *>(m_surface->pixels) + ((m_surface->pitch * y) >> 2);
		for (int x = x1; x <= x2; ++x)
		{
			line[x] = c;
		}
	}

	uint32_t * GetLine(int y)
	{
		return static_cast<uint32_t *>(m_surface->pixels) + ((m_surface->pitch * y) >> 2);
	}

private:
	SDL_Surface * m_surface;
	int m_clipX1;
	int m_clipY1;
	int m_clipX2;
	int m_clipY2;
	int m_sx;
	int m_sy;
};

#endif
