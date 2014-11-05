#ifndef RESTEX_H
#define RESTEX_H
#pragma once

#include "Array.h"
#include "ResBaseTex.h"
#include "Types.h"

class Color
{
public:
	inline Color()
	{
	}
	inline Color(uint32_t c)
	{
		m_c = c;
	}
	inline Color(int r, int g, int b, int a)
	{
		m_c = (r << 0) | (g << 8) | (b << 16) | (a << 24);
	}
	inline Color(float r, float g, float b, float a)
	{
		m_rgba[0] = uint8_t(r * 255.0f);
		m_rgba[1] = uint8_t(g * 255.0f);
		m_rgba[2] = uint8_t(b * 255.0f);
		m_rgba[3] = uint8_t(a * 255.0f);
	}

	union
	{
		uint8_t m_rgba[4];
		uint32_t m_c;
	};
};

class ResTex : public ResBaseTex
{
public:
	ResTex();

	void SetSize(int width, int height);
	inline void SetPixel(int x, int y, const Color& color)
	{
		m_pixels[x + y * m_w] = color;
	}

	virtual int GetW() const;
	virtual int GetH() const;
	virtual Color GetPixel(int x, int y) const;
	const Color* GetData() const;

private:
	int m_w;
	int m_h;

	Array<Color> m_pixels;
};

#endif
