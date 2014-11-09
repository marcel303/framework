#include "ResTexCR.h"

ResTexCR::ResTexCR()
	: ResTexV()
{
	SetType(RES_TEXCR);

	SetTarget(TEXR_COLOR);
	//SetTarget(TEXR_DEPTH);

	for (int i = 0; i < 6; ++i)
		m_faces[i] = ShTexCF(new ResTexCF(this, (CUBE_FACE)i));
}

ShTexCF ResTexCR::GetFace(CUBE_FACE face)
{
	return m_faces[face];
}

void ResTexCR::SetTarget(TEXR_TARGET target)
{
	m_target = target;

	Invalidate();
}
