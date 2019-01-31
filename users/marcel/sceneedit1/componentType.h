#pragma once

#include "component.h"
#include "Vec2.h" // todo : remove ?
#include "Vec3.h" // todo : remove ?
#include "Vec4.h" // todo : remove ?
#include <functional>
#include <string>
#include <vector>

#include "json.hpp" // sadly cannot forward declare json class ..

struct ComponentPropertyBase;
struct ComponentTypeBase;

// component properties

enum ComponentPropertyType
{
	kComponentPropertyType_Bool,
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
};

struct ComponentPropertyBool : ComponentProperty<bool>
{
	ComponentPropertyBool(const char * name)
		: ComponentProperty<bool>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyInt : ComponentProperty<int>
{
	bool hasLimits = false;
	int min;
	int max;
	
	ComponentPropertyInt(const char * name)
		: ComponentProperty<int>(name)
	{
	}
	
	ComponentPropertyInt & setLimits(const int in_min, const int in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

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
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyVec2 : ComponentProperty<Vec2>
{
	ComponentPropertyVec2(const char * name)
		: ComponentProperty<Vec2>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyVec3 : ComponentProperty<Vec3>
{
	ComponentPropertyVec3(const char * name)
		: ComponentProperty<Vec3>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyVec4 : ComponentProperty<Vec4>
{
	ComponentPropertyVec4(const char * name)
		: ComponentProperty<Vec4>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyString : ComponentProperty<std::string>
{
	ComponentPropertyString(const char * name)
		: ComponentProperty<std::string>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

struct ComponentPropertyAngleAxis : ComponentProperty<AngleAxis>
{
	ComponentPropertyAngleAxis(const char * name)
		: ComponentProperty<AngleAxis>(name)
	{
	}
	
	virtual void to_json(ComponentBase * component, nlohmann::json & j) override final;
	virtual void from_json(ComponentBase * component, const nlohmann::json & j) override final;
};

// component types

enum ComponentPriority
{
	kComponentPriority_Transform = 10,
	kComponentPriority_Default = 100
};

struct ComponentTypeBase
{
	struct KeyValuePair
	{
		const char * key;
		const char * value;
	};
	
	typedef std::function<void(ComponentBase * component, const std::string&)> SetString;
	typedef std::function<std::string(ComponentBase * component)> GetString;
	
	std::string typeName;
	std::vector<ComponentPropertyBase*> properties;
	int tickPriority = kComponentPriority_Default;
	
	ComponentMgrBase * componentMgr = nullptr;
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	ComponentPropertyBool & in(const char * name, bool T::* member)
	{
		auto p = new ComponentPropertyBool(name);
		p->getter = [=](ComponentBase * comp) -> bool & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const bool & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
		return *p;
	}
	
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
