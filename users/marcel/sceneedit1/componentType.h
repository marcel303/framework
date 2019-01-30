#pragma once

#include "component.h"
#include "Vec2.h" // todo : remove ?
#include "Vec3.h" // todo : remove ?
#include "Vec4.h" // todo : remove ?
#include <functional>
#include <string>
#include <vector>

#include "json.hpp" // for assigment from templated class ..

struct ComponentPropertyBase;
struct ComponentTypeBase;

// component properties

enum ComponentPropertyType
{
	kComponentPropertyType_Int32,
	kComponentPropertyType_Float,
	kComponentPropertyType_Vec2,
	kComponentPropertyType_Vec3,
	kComponentPropertyType_Vec4,
	kComponentPropertyType_String,
	kComponentPropertyType_AngleAxis
};

struct ComponentPropertyBase
{
	std::string name;
	ComponentPropertyType type;
	
	ComponentPropertyBase(const char * in_name, const ComponentPropertyType in_type)
		: name(in_name)
		, type(in_type)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) = 0;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) = 0;
};

template <typename T> ComponentPropertyType getComponentPropertyType();

template <> inline ComponentPropertyType getComponentPropertyType<int>()
{
	return kComponentPropertyType_Int32;
}

template <> inline ComponentPropertyType getComponentPropertyType<float>()
{
	return kComponentPropertyType_Float;
}

template <> inline ComponentPropertyType getComponentPropertyType<Vec2>()
{
	return kComponentPropertyType_Vec2;
}

template <> inline ComponentPropertyType getComponentPropertyType<Vec3>()
{
	return kComponentPropertyType_Vec3;
}

template <> inline ComponentPropertyType getComponentPropertyType<Vec4>()
{
	return kComponentPropertyType_Vec4;
}

template <> inline ComponentPropertyType getComponentPropertyType<std::string>()
{
	return kComponentPropertyType_String;
}

template <> inline ComponentPropertyType getComponentPropertyType<AngleAxis>()
{
	return kComponentPropertyType_AngleAxis;
}

void to_json(nlohmann::json & j, const Vec2 & v);
void to_json(nlohmann::json & j, const Vec3 & v);
void to_json(nlohmann::json & j, const Vec4 & v);
void to_json(nlohmann::json & j, const AngleAxis & v);

void from_json(const nlohmann::json & j, Vec2 & v);
void from_json(const nlohmann::json & j, Vec3 & v);
void from_json(const nlohmann::json & j, Vec4 & v);
void from_json(const nlohmann::json & j, AngleAxis & v);

template <typename T> struct ComponentProperty : ComponentPropertyBase
{
	typedef std::function<void(ComponentBase * component, const T&)> Setter;
	typedef std::function<T&(ComponentBase * component)> Getter;
	
	Getter getter;
	Setter setter;
	
	ComponentProperty(const char * name)
		: ComponentPropertyBase(name, getComponentPropertyType<T>())
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override
	{
		j = getter(component);
	}
	
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override
	{
		setter(component, j.value(name, T()));
	}
};

typedef ComponentProperty<int> ComponentPropertyInt;

struct ComponentPropertyFloat : ComponentProperty<float>
{
	bool hasLimits = false;
	float min;
	float max;
	
	float editingCurveExponential = 1.f;
	
	ComponentPropertyFloat(const char * name)
		: ComponentProperty<float>(name)
	{
	}
	
	ComponentPropertyFloat & setLimits(const float in_min, const float in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	ComponentPropertyFloat & setEditingCurveExponential(const float in_exponential)
	{
		editingCurveExponential = in_exponential;
		
		return *this;
	}
};

typedef ComponentProperty<Vec2> ComponentPropertyVec2;
typedef ComponentProperty<Vec3> ComponentPropertyVec3;
typedef ComponentProperty<Vec4> ComponentPropertyVec4;
typedef ComponentProperty<std::string> ComponentPropertyString;
typedef ComponentProperty<AngleAxis> ComponentPropertyAngleAxis;

// component types

enum ComponentPriority
{
	kComponentPriority_Transform = 10,
	kComponentPriority_Default = 100
};

struct ComponentTypeBase
{
	typedef std::function<void(ComponentBase * component, const std::string&)> SetString;
	typedef std::function<std::string(ComponentBase * component)> GetString;
	
	std::string typeName;
	std::vector<ComponentPropertyBase*> properties;
	int tickPriority = kComponentPriority_Default;
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	ComponentPropertyInt & in(const char * name, int T::* member)
	{
		auto p = new ComponentPropertyInt(name);
		p->getter = [=](ComponentBase * comp) -> int & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const int & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyFloat & in(const char * name, float T::* member)
	{
		auto p = new ComponentPropertyFloat(name);
		p->getter = [=](ComponentBase * comp) -> float & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const float & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyVec3 & in(const char * name, Vec3 T::* member)
	{
		auto p = new ComponentPropertyVec3(name);
		p->getter = [=](ComponentBase * comp) -> Vec3 & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const Vec3 & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyString & in(const char * name, std::string T::* member)
	{
		auto p = new ComponentPropertyString(name);
		p->getter = [=](ComponentBase * comp) -> std::string & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const std::string & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyAngleAxis & in(const char * name, AngleAxis T::* member)
	{
		auto p = new ComponentPropertyAngleAxis(name);
		p->getter = [=](ComponentBase * comp) -> AngleAxis & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const AngleAxis & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
};
