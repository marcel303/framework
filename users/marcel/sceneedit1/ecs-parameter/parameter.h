#pragma once

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>
#include <typeindex>
#include <vector>

// forward declarations

struct ParameterBase;

struct ParameterBool;
struct ParameterEnum;
struct ParameterFloat;
struct ParameterInt;
struct ParameterString;
struct ParameterVec2;
struct ParameterVec3;
struct ParameterVec4;

struct ParameterMgr;

//

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

//

struct ParameterBase
{
	ParameterType type;
	std::string name;
	bool isDirty = false;
	bool hasChanged = false;
	
	explicit ParameterBase(const ParameterType in_type, const std::string & in_name)
		: type(in_type)
		, name(in_name)
	{
	}
	
	virtual ~ParameterBase()
	{
	}
	
	virtual std::type_index typeIndex() const = 0;
	
	virtual bool isSetToDefault() const = 0;
	virtual void setToDefault() = 0;
	
	void setDirty()
	{
		isDirty = true;
	}
};

//

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
	
	inline bool equals(const bool value1, const bool value2) const
	{
		return value1 == value2;
	}
	
	inline bool equals(const int value1, const int value2) const
	{
		return value1 == value2;
	}
	
	inline bool equals(const float value1, const float value2) const
	{
		return value1 == value2;
	}
	
	inline bool equals(Vec2Arg value1, Vec2Arg value2) const
	{
		return
			value1[0] == value2[0] &&
			value1[1] == value2[1];
	}
	
	inline bool equals(Vec3Arg value1, Vec3Arg value2) const
	{
		return
			value1[0] == value2[0] &&
			value1[1] == value2[1] &&
			value1[2] == value2[2];
	}
	
	inline bool equals(Vec4Arg value1, Vec4Arg value2) const
	{
		return
			value1[0] == value2[0] &&
			value1[1] == value2[1] &&
			value1[2] == value2[2] &&
			value1[3] == value2[3];
	}
	
	inline bool equals(const std::string & value1, const std::string & value2) const
	{
		return value1 == value2;
	}
	
	virtual bool isSetToDefault() const override final
	{
		return equals(value, defaultValue);
	}
	
	const T & get() const
	{
		return value;
	}
	
	T & access_rw() // read-write access. be careful to invalidate the value when you change it!
	{
		return value;
	}
	
	const T & getDefaultValue() const
	{
		return defaultValue;
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
		if (value != in_value)
		{
			value = in_value;
			setDirty();
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
	
	ParameterInt & setLimits(const int in_min, const int in_max);
	
	void set(const int in_value)
	{
		if (hasLimits)
		{
			const int new_value = in_value < min ? min : in_value > max ? max : in_value;
			
			if (value != new_value)
			{
				value = new_value;
				setDirty();
			}
		}
		else
		{
			if (value != in_value)
			{
				value = in_value;
				setDirty();
			}
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
	
	ParameterFloat & setLimits(const float in_min, const float in_max);
	
	ParameterFloat & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
	
	void set(const float in_value)
	{
		if (hasLimits)
		{
			const float new_value = in_value < min ? min : in_value > max ? max : in_value;
			
			if (value != new_value)
			{
				value = new_value;
				setDirty();
			}
		}
		else
		{
			if (value != in_value)
			{
				value = in_value;
				setDirty();
			}
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
	
	ParameterVec2 & setLimits(const Vec2 & in_min, const Vec2 & in_max);
	ParameterVec2 & setLimits(const float in_min, const float in_max);
	
	ParameterVec2 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
	
	void set(Vec2Arg in_value)
	{
		if (hasLimits)
		{
			for (int i = 0; i < 2; ++i)
			{
				const float new_value = in_value[i] < min[i] ? min[i] : in_value[i] > max[i] ? max[i] : in_value[i];
				
				if (value[i] != new_value)
				{
					value[i] = new_value;
					setDirty();
				}
			}
		}
		else
		{
			if (value != in_value)
			{
				value = in_value;
				setDirty();
			}
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
	
	ParameterVec3 & setLimits(const Vec3 & in_min, const Vec3 & in_max);
	
	ParameterVec3 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
	
	void set(Vec3Arg in_value)
	{
		if (hasLimits)
		{
			for (int i = 0; i < 3; ++i)
			{
				const float new_value = in_value[i] < min[i] ? min[i] : in_value[i] > max[i] ? max[i] : in_value[i];
				
				if (value[i] != new_value)
				{
					value[i] = new_value;
					setDirty();
				}
			}
		}
		else
		{
			if (value != in_value)
			{
				value = in_value;
				setDirty();
			}
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
	
	ParameterVec4 & setLimits(const Vec4 & in_min, const Vec4 & in_max);
	
	ParameterVec4 & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
	
	void set(Vec4Arg in_value)
	{
		if (hasLimits)
		{
			for (int i = 0; i < 4; ++i)
			{
				const float new_value = in_value[i] < min[i] ? min[i] : in_value[i] > max[i] ? max[i] : in_value[i];
				
				if (value[i] != new_value)
				{
					value[i] = new_value;
					setDirty();
				}
			}
		}
		else
		{
			if (value != in_value)
			{
				value = in_value;
				setDirty();
			}
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
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
		if (value != in_value)
		{
			value = in_value;
			setDirty();
		}
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue.c_str());
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
	
	virtual bool isSetToDefault() const override final
	{
		return value == defaultValue;
	}
	
	virtual void setToDefault() override final
	{
		set(defaultValue);
	}
	
	int get() const
	{
		return value;
	}
	
	bool set(const int in_value)
	{
		if (value != in_value)
		{
			bool valueExists = false;
			for (auto & elem : elems)
				if (elem.value == in_value)
					valueExists = true;
			
			if (valueExists)
			{
				value = in_value;
				setDirty();
				
				return true;
			}
			else
			{
				return false;
			}
		}
		
		return true;
	}
	
	int translateKeyToValue(const char * key) const;
	const char * translateValueToKey(const int value) const;
	
	int & access_rw() // read-write access. be careful to invalidate the value when you change it!
	{
		return value;
	}
	
	const std::vector<Elem> & getElems() const
	{
		return elems;
	}
	
	const int getDefaultValue() const
	{
		return defaultValue;
	}
	
	void setDefaultValue(const int value)
	{
		defaultValue = value;
	}
};

//

struct ParameterMgr
{
private:
	std::string prefix;
	int index = -1;
	
	std::vector<ParameterBase*> parameters;
	
	std::vector<ParameterMgr*> children;
	
	bool strictStructuringEnabled = false;
	
	bool isHiddenFromUi = false;
	
public:
	void init(const char * prefix);
	void tick();
	
	void setPrefix(const char * prefix);
	
	void add(ParameterBase * parameter);
	
	ParameterBool * addBool(const char * name, const bool defaultValue);
	ParameterInt * addInt(const char * name, const int defaultValue);
	ParameterFloat * addFloat(const char * name, const float defaultValue);
	ParameterVec2 * addVec2(const char * name, const Vec2 & defaultValue);
	ParameterVec3 * addVec3(const char * name, const Vec3 & defaultValue);
	ParameterVec4 * addVec4(const char * name, const Vec4 & defaultValue);
	ParameterString * addString(const char * name, const char * defaultValue);
	ParameterEnum * addEnum(const char * name, const int defaultValue, const std::vector<ParameterEnum::Elem> & elems);
	
	ParameterBase * find(const char * name) const;
	ParameterBase * find(const char * name, const ParameterType type) const;
	ParameterBase * findRecursively(const char * path, const char pathSeparator) const;
	
	template <typename T> T * find(const char * name, const ParameterType type) const
	{
		return static_cast<T*>(find(name, type));
	}
	
	void setToDefault(const bool recurse);
	
	void addChild(ParameterMgr * child, const int childIndex = -1)
	{
		child->index = childIndex;
		
		children.push_back(child);
	}
	
	bool getStrictStructuringEnabled() const
	{
		return strictStructuringEnabled;
	}
	
	void setStrictStructuringEnabled(const bool enabled)
	{
		strictStructuringEnabled = enabled;
	}
	
	bool getIsHiddenFromUi() const
	{
		return isHiddenFromUi;
	}
	
	void setIsHiddenFromUi(const bool isHidden)
	{
		isHiddenFromUi = isHidden;
	}
	
	const std::string & access_prefix() const
	{
		return prefix;
	}
	
	const int access_index() const
	{
		return index;
	}
	
	const std::vector<ParameterBase*> & access_parameters() const
	{
		return parameters;
	}
	
	const std::vector<ParameterMgr*> & access_children() const
	{
		return children;
	}
};
