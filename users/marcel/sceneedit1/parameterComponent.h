#pragma once

#include "component.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
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
	
	explicit ParameterBase(const ParameterType in_type, const std::string & in_name)
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
	
	explicit Parameter(const char * name, const T & in_defaultValue)
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

struct ParameterBool : Parameter<bool, kParameterType_Bool>
{
	ParameterBool(const char * name, const bool defaultValue)
		: Parameter(name, defaultValue)
	{
	}
};

struct ParameterInt : Parameter<int, kParameterType_Int>
{
	bool hasLimits = false;
	int min = 0;
	int max = 0;
	
	ParameterInt(const char * name, const int defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	ParameterInt & setLimits(const int in_min, const int in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
};

struct ParameterFloat : Parameter<float, kParameterType_Float>
{
	bool hasLimits = false;
	float min = 0.f;
	float max = 0.f;
	
	float editingCurveExponential = 1.f;
	
	ParameterFloat(const char * name, const float defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	ParameterFloat & setLimits(const float in_min, const float in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	ParameterFloat & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
};

typedef Parameter<Vec2, kParameterType_Vec2> ParameterVec2;
typedef Parameter<Vec3, kParameterType_Vec3> ParameterVec3;
typedef Parameter<Vec4, kParameterType_Vec4> ParameterVec4;
typedef Parameter<std::string, kParameterType_String> ParameterString;

//

struct ParameterComponent : Component<ParameterComponent>
{
	std::string prefix;
	
	std::vector<ParameterBase*> parameters;
	
	virtual void tick(const float dt) override final;
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
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
	}
};

#endif
