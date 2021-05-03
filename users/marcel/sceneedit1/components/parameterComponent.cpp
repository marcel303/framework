#include "parameterComponent.h"

#include "reflection.h"

#include "Log.h"
#include "Parse.h"

ParameterComponentMgr g_parameterComponentMgr;

//

void ParameterDefinition::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<ParameterDefinition>("parameter")
		.add("type", &ParameterDefinition::type)
		.add("name", &ParameterDefinition::name)
		.add("defaultValue", &ParameterDefinition::defaultValue)
		.add("min", &ParameterDefinition::min)
		.add("max", &ParameterDefinition::max);
}

//

void ParameterComponent::add(ParameterBase * parameter)
{
	parameterMgr.add(parameter);
}

ParameterBool * ParameterComponent::addBool(const char * name, const bool defaultValue)
{
	return parameterMgr.addBool(name, defaultValue);
}

ParameterInt * ParameterComponent::addInt(const char * name, const int defaultValue)
{
	return parameterMgr.addInt(name, defaultValue);
}

ParameterFloat * ParameterComponent::addFloat(const char * name, const float defaultValue)
{
	return parameterMgr.addFloat(name, defaultValue);
}

ParameterVec2 * ParameterComponent::addVec2(const char * name, const Vec2 & defaultValue)
{
	return parameterMgr.addVec2(name, defaultValue);
}

ParameterVec3 * ParameterComponent::addVec3(const char * name, const Vec3 & defaultValue)
{
	return parameterMgr.addVec3(name, defaultValue);
}

ParameterVec4 * ParameterComponent::addVec4(const char * name, const Vec4 & defaultValue)
{
	return parameterMgr.addVec4(name, defaultValue);
}

ParameterString * ParameterComponent::addString(const char * name, const char * defaultValue)
{
	return parameterMgr.addString(name, defaultValue);
}

bool ParameterComponent::init()
{
	bool result = true;
	
	parameterMgr.init(prefix.c_str());
	
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
	
	addInt("int-test", 2);
	addFloat("float-test", 3.f);
	addVec2("vec2-test", Vec2(4.f, 5.f));
	
	return result;
}

void ParameterComponent::propertyChanged(void * address)
{
	if (address == &prefix)
		parameterMgr.setPrefix(prefix.c_str());
}
