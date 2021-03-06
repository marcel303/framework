#include "Debugging.h"
#include "parameter.h"
#include <string.h>

ParameterInt & ParameterInt::setLimits(const int in_min, const int in_max)
{
	Assert(defaultValue >= in_min && defaultValue <= in_max);
	
	hasLimits = true;
	min = in_min;
	max = in_max;
	
	return *this;
}

//

ParameterFloat & ParameterFloat::setLimits(const float in_min, const float in_max)
{
	Assert(defaultValue >= in_min && defaultValue <= in_max);
	
	hasLimits = true;
	min = in_min;
	max = in_max;
	
	return *this;
}

//

ParameterVec2 & ParameterVec2::setLimits(const Vec2 & in_min, const Vec2 & in_max)
{
	Assert(defaultValue[0] >= in_min[0] && defaultValue[0] <= in_max[0]);
	Assert(defaultValue[1] >= in_min[1] && defaultValue[1] <= in_max[1]);
	
	hasLimits = true;
	min = in_min;
	max = in_max;
	
	return *this;
}

ParameterVec2 & ParameterVec2::setLimits(const float in_min, const float in_max)
{
	Assert(defaultValue[0] >= in_min && defaultValue[0] <= in_max);
	Assert(defaultValue[1] >= in_min && defaultValue[1] <= in_max);
	
	hasLimits = true;
	min = Vec2(in_min, in_min);
	max = Vec2(in_max, in_max);
	
	return *this;
}

//

ParameterVec3 & ParameterVec3::setLimits(const Vec3 & in_min, const Vec3 & in_max)
{
	Assert(defaultValue[0] >= in_min[0] && defaultValue[0] <= in_max[0]);
	Assert(defaultValue[1] >= in_min[1] && defaultValue[1] <= in_max[1]);
	Assert(defaultValue[2] >= in_min[2] && defaultValue[2] <= in_max[2]);
	
	hasLimits = true;
	min = in_min;
	max = in_max;
	
	return *this;
}

//

ParameterVec4 & ParameterVec4::setLimits(const Vec4 & in_min, const Vec4 & in_max)
{
	Assert(defaultValue[0] >= in_min[0] && defaultValue[0] <= in_max[0]);
	Assert(defaultValue[1] >= in_min[1] && defaultValue[1] <= in_max[1]);
	Assert(defaultValue[2] >= in_min[2] && defaultValue[2] <= in_max[2]);
	Assert(defaultValue[3] >= in_min[3] && defaultValue[3] <= in_max[3]);
	
	hasLimits = true;
	min = in_min;
	max = in_max;
	
	return *this;
}

//

int ParameterEnum::translateKeyToValue(const char * key) const
{
	for (auto & elem : elems)
		if (strcmp(elem.key.c_str(), key) == 0)
			return elem.value;
	return -1;
}

const char * ParameterEnum::translateValueToKey(const int value) const
{
	for (auto & elem : elems)
		if (elem.value == value)
			return elem.key.c_str();
	return nullptr;
}

//

ParameterMgr::~ParameterMgr()
{
	for (auto *& parameter : parameters)
	{
		delete parameter;
		parameter = nullptr;
	}
	
	parameters.clear();
}

void ParameterMgr::init(const char * in_prefix)
{
	prefix = in_prefix;
}

void ParameterMgr::tick()
{
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

ParameterBase * ParameterMgr::find(const char * name, const ParameterType type) const
{
	for (auto * parameter : parameters)
		if (parameter->name == name && parameter->type == type)
			return parameter;
	
	return nullptr;
}

static ParameterBase * findParameter(const ParameterMgr & paramMgr, const char * path, const char pathSeparator)
{
	Assert(path[0] == pathSeparator);
	
	if (path[0] != pathSeparator)
	{
		// invalid path
		
		return nullptr;
	}
	else
	{
		const char * name = path + 1;
		const char * nameSeparator = strchr(name, pathSeparator);
		
		if (nameSeparator == nullptr || paramMgr.getStrictStructuringEnabled() == false)
		{
			// the path specifies a parameter. look it up and return it
			
			ParameterBase * parameter = paramMgr.find(name);
			
			if (parameter == nullptr)
			{
				if (paramMgr.getStrictStructuringEnabled())
				{
					return nullptr;
				}
			}
			else
			{
				return parameter;
			}
		}
		
		if (nameSeparator != nullptr)
		{
			// the path specifies a parameter within a child. find the child and let it handle the OSC message
			
			ParameterMgr * child = nullptr;
			
			const char * nextName = nullptr;
			const char * nextNameSeparator = nullptr;
			
			bool foundIndexed = false;
			
			//
			
			const_cast<char*>(nameSeparator)[0] = 0;
			{
				// see if there's a next name and see if the next name is a number. if so, it may be an OSC address of the format "/items/0/value". where /0 specifies the index into an array
				
				nextName = nameSeparator + 1;
				nextNameSeparator = strchr(nextName, pathSeparator);
				
				bool nextNameIsNumber = false;
				int nextNumberValue = -1;
				
				if (nextNameSeparator != nullptr)
				{
					nextNameIsNumber = true;
					
					for (const char * c = nextName; c < nextNameSeparator; ++c)
						nextNameIsNumber &= c[0] >= '0' && c[0] <= '9';
					
					if (nextNameIsNumber)
					{
						const_cast<char*>(nextNameSeparator)[0] = 0;
						{
							nextNumberValue = atoi(nextName);
						}
						const_cast<char*>(nextNameSeparator)[0] = pathSeparator;
					}
				}
			
				for (auto * child_itr : paramMgr.access_children())
				{
					const std::string & child_name = child_itr->access_prefix();
					const int child_index = child_itr->access_index();
					
					if (child_name == name)
					{
						if (child_index == -1)
						{
							child = child_itr;
							break;
						}
						else if (nextNameIsNumber && child_index == nextNumberValue)
						{
							child = child_itr;
							foundIndexed = true;
							break;
						}
					}
				}
			}
			const_cast<char*>(nameSeparator)[0] = pathSeparator;
			
			if (child == nullptr)
			{
				return nullptr;
			}
			else
			{
				return findParameter(
					*child,
					foundIndexed
					? nextNameSeparator
					: nameSeparator,
					pathSeparator);
			}
		}
	}
	
	return nullptr;
}

ParameterBase * ParameterMgr::findRecursively(const char * path, const char pathSeparator) const
{
	return findParameter(*this, path, pathSeparator);
}

void ParameterMgr::setToDefault(const bool recurse)
{
	for (auto * parameter : parameters)
		parameter->setToDefault();
	
	if (recurse)
	{
		for (auto * child : children)
			child->setToDefault(recurse);
	}
}
