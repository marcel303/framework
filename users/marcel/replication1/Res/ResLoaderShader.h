#ifndef RESLOADERSHADER_H
#define RESLOADERSHADER_H
#pragma once

#include "ResLoader.h"

class ResLoaderShader : public ResLoader
{
public:
	ResLoaderShader();

	virtual Res* Load(const std::string& name);
};

#endif
