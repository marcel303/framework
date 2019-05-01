#include "Debugging.h"
#include "parameter.h"
#include <string.h>

//

int ParameterEnum::translateKeyToValue(const char * key) const
{
	for (auto & elem : elems)
		if (strcmp(elem.key, key) == 0)
			return elem.value;
	return -1;
}

//

void ParameterMgr::init(const char * in_prefix, const int in_index)
{
	prefix = in_prefix;
	index = in_index;
}

void ParameterMgr::tick()
{
	// todo : make a separate list for parameters that can trigger
	
	for (auto * parameter : parameters)
	{
		if (parameter->isDirty)
		{
			parameter->isDirty = false;
			parameter->hasChanged = true;
		}
		else if (parameter->hasChanged)
		{
			parameter->hasChanged = false;
		}
	}
}

void ParameterMgr::setPrefix(const char * in_prefix)
{
	prefix = in_prefix;
}

void ParameterMgr::add(ParameterBase * parameter)
{
	parameters.push_back(parameter);
}

ParameterBool * ParameterMgr::addBool(const char * name, const bool defaultValue)
{
	auto * parameter = new ParameterBool(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterInt * ParameterMgr::addInt(const char * name, const int defaultValue)
{
	auto * parameter = new ParameterInt(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterFloat * ParameterMgr::addFloat(const char * name, const float defaultValue)
{
	auto * parameter = new ParameterFloat(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec2 * ParameterMgr::addVec2(const char * name, const Vec2 & defaultValue)
{
	auto * parameter = new ParameterVec2(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec3 * ParameterMgr::addVec3(const char * name, const Vec3 & defaultValue)
{
	auto * parameter = new ParameterVec3(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec4 * ParameterMgr::addVec4(const char * name, const Vec4 & defaultValue)
{
	auto * parameter = new ParameterVec4(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterString * ParameterMgr::addString(const char * name, const char * defaultValue)
{
	auto * parameter = new ParameterString(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterEnum * ParameterMgr::addEnum(const char * name, const int defaultValue, const std::vector<ParameterEnum::Elem> & elems)
{
	auto * parameter = new ParameterEnum(name, defaultValue, elems);
	add(parameter);
	return parameter;
}

ParameterBase * ParameterMgr::find(const char * name) const
{
	for (auto * parameter : parameters)
		if (parameter->name == name)
			return parameter;
	
	return nullptr;
}
