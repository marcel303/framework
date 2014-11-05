#ifndef RESLOADERTEX_H
#define RESLOADERTEX_H
#pragma once

#include "ResLoader.h"

class ResLoaderTex : public ResLoader
{
public:
	virtual Res* Load(const std::string& name);
};

#endif
