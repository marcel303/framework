#include "ResTexV.h"

ResTexV::ResTexV() : ResBaseTex()
{
	m_w = 0;
	m_h = 0;
}

void ResTexV::SetSize(int width, int height)
{
	if (width == m_w && height == m_h)
		return;

	m_w = width;
	m_h = height;

	Invalidate();
}

int ResTexV::GetW() const
{
	return m_w;
}

int ResTexV::GetH() const
{
	return m_h;
}
