#pragma once

#include "component.h"
#include "parameter.h"
#include <vector>

//

struct ParameterDefinition
{
	std::string type;
	std::string name;
	std::string defaultValue;
	std::string min;
	std::string max;
};

struct ParameterComponent : Component<ParameterComponent>
{
	std::string prefix;
	
	std::vector<ParameterBase*> parameters;
	
	std::vector<ParameterDefinition> parameterDefinitions;
	
	virtual bool init() override final;
	virtual void tick(const float dt) override final;
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
	ParameterVec2 * addVec2(const char * name, const Vec2 & defaultValue);
	ParameterVec3 * addVec3(const char * name, const Vec3 & defaultValue);
	ParameterVec4 * addVec4(const char * name, const Vec4 & defaultValue);
	ParameterString * addString(const char * name, const char * defaultValue);
};

//

struct ParameterComponentMgr : ComponentMgr<ParameterComponent>
{
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ParameterComponentType : ComponentType<ParameterComponent>
{
	ParameterComponentType()
		: ComponentType("ParameterComponent")
	{
		tickPriority = kComponentPriority_Parameter;
		
		add("prefix", &ParameterComponent::prefix);
		add("parameters", &ParameterComponent::parameterDefinitions);
	}
};

#endif
