#pragma once

#include <map>
#include <string>
#include <vector>
#include "Parameter.h"

class ParameterList
{
public:
	void Add(const Parameter& parameter);
	void Add(ShParameter parameter);

	int Find(const std::string& name);

	Parameter* operator[](const std::string& name);

	std::vector<ShParameter> m_parameters;

private:
	std::map<std::string, int> m_nameToIndex;
};
