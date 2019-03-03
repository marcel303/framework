#include "Log.h"
#include "parameterComponent.h"
#include "Parse.h"

void ParameterComponent::add(ParameterBase * parameter)
{
	parameters.push_back(parameter);
}

ParameterBool * ParameterComponent::addBool(const char * name, const bool defaultValue)
{
	auto * parameter = new ParameterBool(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterInt * ParameterComponent::addInt(const char * name, const int defaultValue)
{
	auto * parameter = new ParameterInt(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterFloat * ParameterComponent::addFloat(const char * name, const float defaultValue)
{
	auto * parameter = new ParameterFloat(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec2 * ParameterComponent::addVec2(const char * name, const Vec2 & defaultValue)
{
	auto * parameter = new ParameterVec2(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec3 * ParameterComponent::addVec3(const char * name, const Vec3 & defaultValue)
{
	auto * parameter = new ParameterVec3(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterVec4 * ParameterComponent::addVec4(const char * name, const Vec4 & defaultValue)
{
	auto * parameter = new ParameterVec4(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterString * ParameterComponent::addString(const char * name, const char * defaultValue)
{
	auto * parameter = new ParameterString(name, defaultValue);
	add(parameter);
	return parameter;
}

bool ParameterComponent::init()
{
	bool result = true;
	
	for (auto & definition : parameterDefinitions)
	{
		if (definition.type == "bool")
		{
			const bool defaultValue = Parse::Bool(definition.defaultValue);
			
			addBool(definition.name.c_str(), defaultValue);
		}
		else if (definition.type == "int")
		{
			const int defaultValue = Parse::Int32(definition.defaultValue);
			
			auto * param = addInt(definition.name.c_str(), defaultValue);
			
			if (!definition.min.empty() && !definition.max.empty())
			{
				const int min = Parse::Int32(definition.min);
				const int max = Parse::Int32(definition.max);
				
				param->setLimits(min, max);
			}
		}
		else if (definition.type == "float")
		{
			const float defaultValue = Parse::Float(definition.defaultValue);
			
			auto * param = addFloat(definition.name.c_str(), defaultValue);
			
			if (!definition.min.empty() && !definition.max.empty())
			{
				const float min = Parse::Float(definition.min);
				const float max = Parse::Float(definition.max);
				
				param->setLimits(min, max);
			}
		}
		else if (definition.type == "vec2")
		{
			const float defaultValue = Parse::Float(definition.defaultValue);
			
			auto * param = addVec2(definition.name.c_str(), Vec2(defaultValue, defaultValue));
			
			if (!definition.min.empty() && !definition.max.empty())
			{
				const float min = Parse::Float(definition.min);
				const float max = Parse::Float(definition.max);
				
				param->setLimits(Vec2(min, min), Vec2(max, max));
			}
		}
		else if (definition.type == "vec3")
		{
			const float defaultValue = Parse::Float(definition.defaultValue);
			
			auto * param = addVec3(definition.name.c_str(), Vec3(defaultValue, defaultValue, defaultValue));
			
			if (!definition.min.empty() && !definition.max.empty())
			{
				const float min = Parse::Float(definition.min);
				const float max = Parse::Float(definition.max);
				
				param->setLimits(Vec3(min, min, min), Vec3(max, max, max));
			}
		}
		else if (definition.type == "vec4")
		{
			const float defaultValue = Parse::Float(definition.defaultValue);
			
			auto * param = addVec4(definition.name.c_str(), Vec4(defaultValue, defaultValue, defaultValue, defaultValue));
			
			if (!definition.min.empty() && !definition.max.empty())
			{
				const float min = Parse::Float(definition.min);
				const float max = Parse::Float(definition.max);
				
				param->setLimits(Vec4(min, min, min, min), Vec4(max, max, max, max));
			}
		}
		else if (definition.type == "string")
		{
			const char * defaultValue = definition.defaultValue.c_str();
			
			addString(definition.name.c_str(), defaultValue);
		}
		else
		{
			LOG_ERR("unknown parameter type in definition: %s", definition.type.c_str());
			result = false;
		}
	}
	
	return result;
}

void ParameterComponent::tick(const float dt)
{
}
