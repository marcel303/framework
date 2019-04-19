#pragma once

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>
#include <typeindex>

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

struct ParameterVec2 : Parameter<Vec2, kParameterType_Vec2>
{
	bool hasLimits = false;
	Vec2 min;
	Vec2 max;
	
	float editingCurveExponential = 1.f;
	
	ParameterVec2(const char * name, const Vec2 & defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	ParameterVec2 & setLimits(const Vec2 & in_min, const Vec2 & in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	ParameterVec2 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
};

struct ParameterVec3 : Parameter<Vec3, kParameterType_Vec3>
{
	bool hasLimits = false;
	Vec3 min;
	Vec3 max;
	
	float editingCurveExponential = 1.f;
	
	ParameterVec3(const char * name, const Vec3 & defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	ParameterVec3 & setLimits(const Vec3 & in_min, const Vec3 & in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	ParameterVec3 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
};

struct ParameterVec4 : Parameter<Vec4, kParameterType_Vec4>
{
	bool hasLimits = false;
	Vec4 min;
	Vec4 max;
	
	float editingCurveExponential = 1.f;
	
	ParameterVec4(const char * name, const Vec4 & defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	ParameterVec4 & setLimits(const Vec4 & in_min, const Vec4 & in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	ParameterVec4 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
};

typedef Parameter<std::string, kParameterType_String> ParameterString;