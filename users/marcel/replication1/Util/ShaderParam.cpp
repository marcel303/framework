#include "ShaderParam.h"

ShaderParam& ShaderParamList::operator[](const std::string& name)
{
	ParamCollItr i = m_parameters.find(name);

	if (i == m_parameters.end())
	{
		m_parameters[name] = ShaderParam();
		i = m_parameters.find(name);
	}

	return i->second;
}
