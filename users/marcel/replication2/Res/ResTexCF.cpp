#include "ResTexCF.h"

ResTexCF::ResTexCF(ResTexCR* cube, CUBE_FACE face) : ResTexR()
{
	SetType(RES_TEXCF);

	m_cube = cube;
	m_face = face;
}
