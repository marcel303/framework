#include "ResTexR.h"

ResTexR::ResTexR()
	: ResTexV()
{
	SetType(RES_TEXR);
	SetTarget(TEXR_COLOR);
}

ResTexR::ResTexR(int sx, int sy, TEXR_TARGET target)
	: ResTexV()
{
	SetType(RES_TEXR);
	SetSize(sx, sy);
	SetTarget(target);
}

void ResTexR::SetTarget(TEXR_TARGET target)
{
	m_target = target;

	Invalidate();
}
