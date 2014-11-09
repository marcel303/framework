#ifndef RESTEXR_H
#define RESTEXR_H
#pragma once

#include "ResTexV.h"

enum TEXR_TARGET
{
	TEXR_COLOR,
	TEXR_DEPTH,
	TEXR_COLOR32F
};

class ResTexR : public ResTexV
{
public:
	ResTexR();
	ResTexR(int sx, int sy, TEXR_TARGET target = TEXR_COLOR);

	void SetTarget(TEXR_TARGET target);

	TEXR_TARGET m_target;
};

#endif
