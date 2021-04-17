#pragma once

#include "component.h"
#include "parameter.h"
#include <vector>

//

struct TypeDB;

//

struct ParameterDefinition
{
	std::string type;
	std::string name;
	std::string defaultValue;
	std::string min;
	std::string max;
	
	static void reflect(TypeDB & typeDB);
};

struct ParameterComponent : Component<ParameterComponent>
{
	friend struct ParameterComponentType;
	
private:
	std::string prefix;
	
	std::vector<ParameterDefinition> parameterDefinitions;
	
	ParameterMgr parameterMgr;
	
public:
	virtual bool init() override final;
	virtual void tick(const float dt) override final;
	
	virtual void propertyChanged(void * address) override final;
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
	ParameterVec2 * addVec2(const char * name, const Vec2 & defaultValue);
	ParameterVec3 * addVec3(const char * name, const Vec3 & defaultValue);
	ParameterVec4 * addVec4(const char * name, const Vec4 & defaultValue);
	ParameterString * addString(const char * name, const char * defaultValue);
	
	ParameterMgr & access_parameterMgr()
	{
		return parameterMgr;
	}
};

//

struct ParameterComponentMgr : ComponentMgr<ParameterComponent>
{
};

extern ParameterComponentMgr g_parameterComponentMgr;


#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ParameterComponentType : ComponentType<ParameterComponent>
{
	ParameterComponentType()
		: ComponentType("ParameterComponent", &g_parameterComponentMgr)
	{
		tickPriority = kComponentPriority_Parameter;
		
		add("prefix", &ParameterComponent::prefix);
		add("parameters", &ParameterComponent::parameterDefinitions);
	}
};

#endif
