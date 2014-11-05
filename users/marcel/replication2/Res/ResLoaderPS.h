#ifndef RESLOADERPS_H
#define RESLOADERPS_H
#pragma once

#include "ResLoader.h"

class ResLoaderPS : public ResLoader
{
public:
	virtual Res* Load(const std::string& name);
};

#endif
