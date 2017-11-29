#pragma once

#include "../libgg/Image.h"
#include <xmmintrin.h>

struct ImageMem
{
	int sx;
	int sy;
	uint32_t * pixels;
	
	ImageMem(int _sx, int _sy)
		: sx(_sx)
		, sy(_sy)
		, pixels(nullptr)
	{
		pixels = (uint32_t*)_mm_malloc(sx * sy * 4, 16);
	}
	
	~ImageMem()
	{
		_mm_free(pixels);
		pixels = nullptr;
	}
};

struct ImageCtx
{
	ImageCtx(ImageMem & image, int clipX, int clipY, int clipSx, int clipSy)
		: m_image(image)
		, m_clipX1(clipX)
		, m_clipY1(clipY)
		, m_clipX2(clipX + clipSx - 1)
		, m_clipY2(clipY + clipSy - 1)
		, m_sx(clipSx)
		, m_sy(clipSy)
	{
	}

	uint32_t Color(float r, float g, float b)
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
			(ri <<  0) |
			(gi <<  8) |
			(bi << 16);
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
		if (y < m_clipY1 || y > m_clipY2 || y < 0 || y > m_image.sx - 1)
			return;
		int x1 = x;
		int x2 = x + sx - 1;
		x1 = x1 < m_clipX1 ? m_clipX1 : x1;
		x2 = x2 > m_clipX2 ? m_clipX2 : x2;
		x1 = x1 < 0 ? 0 : x1;
		x2 = x2 > m_image.sx - 1 ? m_image.sx - 1 : x2;
		uint32_t * line = m_image.pixels + y * m_image.sx;
		for (int x = x1; x <= x2; ++x)
		{
			line[x] = c;
		}
	}
	
	ImageMem & m_image;
	int m_clipX1;
	int m_clipY1;
	int m_clipX2;
	int m_clipY2;
	int m_sx;
	int m_sy;
};
