#ifndef RESVS_H
#define RESVS_H
#pragma once

#include <string>
#include "HLSLProgram.h"
#include "ShaderParam.h"
#include "Res.h"

class ResVS : public Res, public HLSLProgram
{
public:
	ResVS();
};

#endif
