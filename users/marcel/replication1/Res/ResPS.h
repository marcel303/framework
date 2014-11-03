#ifndef RESPS_H
#define RESPS_H
#pragma once

#include <string>
#include "HLSLProgram.h"
#include "Res.h"
#include "ShaderParam.h"

class ResPS : public Res, public HLSLProgram
{
public:
	ResPS();
};

#endif
