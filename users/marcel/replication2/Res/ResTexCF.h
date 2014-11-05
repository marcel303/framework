#ifndef RESTEXCF_H
#define RESTEXCF_H
#pragma once

#include "ResTexR.h"

class ResTexCR;

enum CUBE_FACE
{
	CUBE_X_POS = 0,
	CUBE_Y_POS = 1,
	CUBE_Z_POS = 2,
	CUBE_X_NEG = 3,
	CUBE_Y_NEG = 4,
	CUBE_Z_NEG = 5
};

class ResTexCF : public ResTexR
{
public:
	ResTexCF(ResTexCR* cube, CUBE_FACE face);

	ResTexCR* m_cube;
	CUBE_FACE m_face;
};

#endif
