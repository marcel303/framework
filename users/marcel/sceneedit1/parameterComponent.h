#pragma once

#include "component.h"
#include <string>
#include <typeindex>
#include <vector>

enum ParameterType
{
	kParameterType_Bool,
	kParameterType_Int,
	kParameterType_Float,
	kParameterType_Vec2,
	kParameterType_Vec3,
	kParameterType_Vec4,
	kParameterType_String
};

struct ParameterBase
{
	ParameterType type;
	std::string name;
	
	ParameterBase(const ParameterType in_type, const std::string & in_name)
		: type(in_type)
		, name(in_name)
	{
	}
	
	virtual std::type_index typeIndex() const = 0;
};

template <typename T, ParameterType kType>
struct Parameter : ParameterBase
{
	T value;
	T defaultValue;
	
	Parameter(const char * name, const T & in_defaultValue)
		: ParameterBase(kType, name)
		, value(in_defaultValue)
		, defaultValue(in_defaultValue)
	{
	}
	
	virtual std::type_index typeIndex() const override final
	{
		return std::type_index(typeid(T));
	}
};

typedef Parameter<bool, kParameterType_Bool> ParameterBool;
typedef Parameter<bool, kParameterType_Int> ParameterInt;
typedef Parameter<bool, kParameterType_Float> ParameterFloat;
typedef Parameter<bool, kParameterType_Vec3> ParameterVec2;
typedef Parameter<bool, kParameterType_Vec3> ParameterVec3;
typedef Parameter<bool, kParameterType_Vec4> ParameterVec4;
typedef Parameter<bool, kParameterType_String> ParameterString;

struct ParameterComponent : Component<ParameterComponent>
{
	std::string prefix;
	
	std::vector<ParameterBase*> parameters;
	
	virtual void tick(const float dt) override;
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
};

struct ParameterComponentMgr : ComponentMgr<ParameterComponent>
{
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ParameterComponentType : ComponentType<ParameterComponent>
{
	ParameterComponentType()
	{
		typeName = "ParameterComponent";
		tickPriority = kComponentPriority_Parameter;
		
		in("prefix", &ParameterComponent::prefix);
	}
};

#endif
