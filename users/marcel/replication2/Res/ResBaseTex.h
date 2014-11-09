#ifndef RESBASETEX_H
#define RESBASETEX_H
#pragma once

#include "Res.h"

class ResBaseTex : public Res
{
public:
	inline ResBaseTex()
		: Res()
	{
		SetType(RES_BASE_TEX);
	}

	virtual int GetW() const = 0;
	virtual int GetH() const = 0;
};

#endif
