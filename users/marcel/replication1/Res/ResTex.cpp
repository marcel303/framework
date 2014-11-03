#include "ResTex.h"

ResTex::ResTex() : ResBaseTex()
{
	SetType(RES_TEX);

	m_w = 0;
	m_h = 0;
}

void ResTex::SetSize(int width, int height)
{
	m_w = width;
	m_h = height;

	m_pixels.SetSize(m_w * m_h);

	Invalidate();
}

int ResTex::GetW() const
{
	return m_w;
}

int ResTex::GetH() const
{
	return m_h;
}

Color ResTex::GetPixel(int x, int y) const
{
	return m_pixels[x + y * m_w];
}

const Color* ResTex::GetData() const
{
	return m_pixels.GetData();
}
