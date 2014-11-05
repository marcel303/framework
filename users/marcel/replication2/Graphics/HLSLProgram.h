#pragma once

#include "ShaderParam.h"

class HLSLProgram
{
public:
	bool Load(const std::string& filename);

	ShaderParamList p;
	std::string m_text;
};
