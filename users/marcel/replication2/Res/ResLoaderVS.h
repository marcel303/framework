#ifndef RESLOADERVS_H
#define RESLOADERVS_H
#pragma once

#include "ResLoader.h"

class ResLoaderVS : public ResLoader
{
public:
	virtual Res* Load(const std::string& name);
};

#endif
