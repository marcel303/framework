#ifndef RESTEXCR_H
#define RESTEXCR_H
#pragma once

#include "ResTexCF.h"
#include "ResTexR.h"
#include "ResTexV.h"

class ResTexCR : public ResTexV
{
public:
	ResTexCR();

	ShTexCF GetFace(CUBE_FACE face);

	void SetTarget(TEXR_TARGET target);

	TEXR_TARGET m_target;
	ShTexCF m_faces[6];
};

#endif
