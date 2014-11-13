#include "ShaderParam.h"

ShaderParam& ShaderParamList::operator[](const char * name)
{
	for (ParamCollItr i = m_parameters.begin(); i != m_parameters.end(); ++i)
	{
		NamedParam & namedParam = *i;

		if (!strcmp(namedParam.name.c_str(), name))
			return namedParam.param;
	}

	NamedParam namedParam;
	namedParam.name = name;
	m_parameters.push_back(namedParam);
	return m_parameters.back().param;
}
