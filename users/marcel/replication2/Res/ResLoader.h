#ifndef RELOADER_H
#define RELOADER_H
#pragma once

#include "Res.h"

class ResLoader
{
public:
	ResLoader();
	virtual ~ResLoader();

	virtual Res* Load(const std::string& name) = 0;
};

#endif
