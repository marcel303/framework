#ifndef RESTEXV_H
#define RESTEXV_H
#pragma once

#include "ResBaseTex.h"

class ResTexV : public ResBaseTex
{
public:
	ResTexV();

	void SetSize(int width, int height);

	virtual int GetW() const;
	virtual int GetH() const;

private:
	int m_w;
	int m_h;
};

#endif
