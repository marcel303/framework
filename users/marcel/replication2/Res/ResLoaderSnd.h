#ifndef RESLOADERSND_H
#define RESLOADERSND_H
#pragma once

#include "ResLoader.h"

class ResLoaderSnd : public ResLoader
{
public:
	virtual Res* Load(const std::string& name);
};

#endif
