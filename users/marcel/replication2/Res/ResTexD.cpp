#include "ResTexD.h"

ResTexD::ResTexD()
	: ResTexR()
{
	SetType(RES_TEXD);
	SetTarget(TEXR_DEPTH);
}

ResTexD::ResTexD(int sx, int sy)
	: ResTexR()
{
	SetType(RES_TEXD);
	SetTarget(TEXR_DEPTH);
	SetSize(sx, sy);
}
