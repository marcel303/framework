#include "Debug.h"
#include "ParameterList.h"

void ParameterList::Add(const Parameter& parameter)
{
	Parameter* temp = new Parameter();

	*temp = parameter;

	Add(ShParameter(temp));
}

void ParameterList::Add(ShParameter parameter)
{
	m_nameToIndex[parameter->m_name] = (int)m_parameters.size();

	m_parameters.push_back(parameter);
}

int ParameterList::Find(const std::string& name)
{
	std::map<std::string, int>::iterator i = m_nameToIndex.find(name);

	if (i != m_nameToIndex.end())
		return i->second;

	return -1;
}

Parameter* ParameterList::operator[](const std::string& name)
{
	int index = Find(name);

	if (index == -1)
	{
		FASSERT(0);
		return 0;
	}

	return m_parameters[index].get();
}
