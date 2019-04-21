#pragma once

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
	kParameterType_String,
	kParameterType_Enum
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
protected:
	T value;
	T defaultValue;
	
public:
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
	
	const T & get() const
	{
		return value;
	}
	
	T & access_rw() // read-write access. be careful to invalidate the value when you change it!
	{
		return value;
	}
	
	void setDirty()
	{
	}
};

struct ParameterBool : Parameter<bool, kParameterType_Bool>
{
	ParameterBool(const char * name, const bool defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	void set(const bool in_value)
	{
		value = in_value;
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
	
	void set(const int in_value)
	{
		if (hasLimits)
		{
			value = in_value < min ? min : in_value > max ? max : in_value;
		}
		else
		{
			value = in_value;
		}
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
	
	void set(const float in_value)
	{
		if (hasLimits)
		{
			value = in_value < min ? min : in_value > max ? max : in_value;
		}
		else
		{
			value = in_value;
		}
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
	
	void set(Vec2Arg in_value)
	{
		value = in_value;
		
		// todo : apply limits
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
	
	void set(Vec3Arg in_value)
	{
		value = in_value;
		
		// todo : apply limits
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
	
	void set(Vec4Arg in_value)
	{
		value = in_value;
		
		// todo : apply limits
	}
};

struct ParameterString : Parameter<std::string, kParameterType_String>
{
	ParameterString(const char * name, const char * defaultValue)
		: Parameter(name, defaultValue)
	{
	}
	
	void set(const char * in_value)
	{
		value = in_value;
	}
};

struct ParameterEnum : ParameterBase
{
	struct Elem
	{
		const char * key;
		const int value;
	};
	
protected:
	int value;
	int defaultValue;
	
	std::vector<Elem> elems;
	
public:
	ParameterEnum(const char * name, const int in_defaultValue, const std::vector<Elem> & in_elems)
		: ParameterBase(kParameterType_Enum, name)
		, value(in_defaultValue)
		, defaultValue(in_defaultValue)
		, elems(in_elems)
	{
		// todo : assert the default value exists within the given enumeration elements
	}
	
	virtual std::type_index typeIndex() const override final
	{
		return std::type_index(typeid(ParameterEnum));
	}
	
	int get() const
	{
		return value;
	}
	
	void set(const int in_value)
	{
		value = in_value;
	}
	
	int & access_rw() // read-write access. be careful to invalidate the value when you change it!
	{
		return value;
	}
	
	const std::vector<Elem> & getElems() const
	{
		return elems;
	}
	
	void setDirty()
	{
	}
};

//

struct ParameterMgr
{
private:
	std::string prefix;
	
	std::vector<ParameterBase*> parameters;
	
public:
	void init(const char * prefix);
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
	ParameterVec2 * addVec2(const char * name, const Vec2 & defaultValue);
	ParameterVec3 * addVec3(const char * name, const Vec3 & defaultValue);
	ParameterVec4 * addVec4(const char * name, const Vec4 & defaultValue);
	ParameterString * addString(const char * name, const char * defaultValue);
	ParameterEnum * addEnum(const char * name, const int defaultValue, const std::vector<ParameterEnum::Elem> & elems);
	
	const std::string & access_prefix()
	{
		return prefix;
	}
	
	const std::vector<ParameterBase*> & access_parameters() const
	{
		return parameters;
	}
};
