#pragma once

#include "component.h"
#include "reflection.h"
#include <functional> // todo : use a more simple way to add a getter/setter and remove this dependency
#include <string>
#include <vector>

class Vec2;
class Vec3;
class Vec4;

#if ENABLE_COMPONENT_JSON
	struct ComponentJson;
#endif

struct ComponentPropertyBase;
struct ComponentTypeBase;

struct ResourcePtr;

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
	kComponentPropertyType_AngleAxis,
	kComponentPropertyType_ResourcePtr
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
	
	virtual void setToDefault(ComponentBase * component) = 0;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) = 0;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) = 0;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) = 0;
	virtual bool from_text(ComponentBase * component, const char * text) = 0;
};

template <typename T>
struct ComponentProperty : ComponentPropertyBase
{
	typedef std::function<void(ComponentBase * component, const T&)> Setter;
	typedef std::function<T&(ComponentBase * component)> Getter;
	
	Getter getter;
	Setter setter;
	
	ComponentProperty(const char * name, const ComponentPropertyType type)
		: ComponentPropertyBase(name, type)
	{
	}
};

struct ComponentPropertyBool : ComponentProperty<bool>
{
	ComponentPropertyBool(const char * name)
		: ComponentProperty<bool>(name, kComponentPropertyType_Bool)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyInt : ComponentProperty<int>
{
	bool hasLimits = false;
	int min;
	int max;
	
	ComponentPropertyInt(const char * name)
		: ComponentProperty<int>(name, kComponentPropertyType_Int32)
	{
	}
	
	ComponentPropertyInt & setLimits(const int in_min, const int in_max)
	{
		hasLimits = true;
		min = in_min;
		max = in_max;
		
		return *this;
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyFloat : ComponentProperty<float>
{
	bool hasLimits = false;
	float min;
	float max;
	
	float editingCurveExponential = 1.f;
	
	ComponentPropertyFloat(const char * name)
		: ComponentProperty<float>(name, kComponentPropertyType_Float)
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
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyVec2 : ComponentProperty<Vec2>
{
	ComponentPropertyVec2(const char * name)
		: ComponentProperty<Vec2>(name, kComponentPropertyType_Vec2)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyVec3 : ComponentProperty<Vec3>
{
	ComponentPropertyVec3(const char * name)
		: ComponentProperty<Vec3>(name, kComponentPropertyType_Vec3)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyVec4 : ComponentProperty<Vec4>
{
	ComponentPropertyVec4(const char * name)
		: ComponentProperty<Vec4>(name, kComponentPropertyType_Vec4)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyString : ComponentProperty<std::string>
{
	ComponentPropertyString(const char * name)
		: ComponentProperty<std::string>(name, kComponentPropertyType_String)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyAngleAxis : ComponentProperty<AngleAxis>
{
	ComponentPropertyAngleAxis(const char * name)
		: ComponentProperty<AngleAxis>(name, kComponentPropertyType_AngleAxis)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

struct ComponentPropertyResourcePtr : ComponentProperty<ResourcePtr>
{
	ComponentPropertyResourcePtr(const char * name)
		: ComponentProperty<ResourcePtr>(name, kComponentPropertyType_ResourcePtr)
	{
	}
	
	virtual void setToDefault(ComponentBase * component) override final;
	
#if ENABLE_COMPONENT_JSON
	virtual void to_json(ComponentBase * component, ComponentJson & j) override final;
	virtual void from_json(ComponentBase * component, const ComponentJson & j) override final;
#endif

	virtual void to_text(ComponentBase * component, std::string & text) override final;
	virtual bool from_text(ComponentBase * component, const char * text) override final;
};

// component types

enum ComponentPriority
{
	kComponentPriority_Parameter = 100,
	kComponentPriority_Transform = 200,
	kComponentPriority_Camera = 400,
	kComponentPriority_Default = 1000
};

struct ComponentTypeBase : StructuredType
{
	// todo : remove properties array, and property types defined above. we should use the type system!
	std::vector<ComponentPropertyBase*> _properties;
	int tickPriority = kComponentPriority_Default;
	
	ComponentMgrBase * componentMgr = nullptr;
	
	ComponentTypeBase(const char * in_typeName)
		: StructuredType(in_typeName)
	{
	}
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	ComponentType(const char * in_typeName)
		: ComponentTypeBase(in_typeName)
	{
	}
	
	ComponentPropertyBool & in(const char * name, bool T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyBool(name);
		p->getter = [=](ComponentBase * comp) -> bool & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const bool & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyInt & in(const char * name, int T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyInt(name);
		p->getter = [=](ComponentBase * comp) -> int & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const int & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyFloat & in(const char * name, float T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyFloat(name);
		p->getter = [=](ComponentBase * comp) -> float & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const float & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyVec2 & in(const char * name, Vec2 T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyVec2(name);
		p->getter = [=](ComponentBase * comp) -> Vec2 & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const Vec2 & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyVec3 & in(const char * name, Vec3 T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyVec3(name);
		p->getter = [=](ComponentBase * comp) -> Vec3 & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const Vec3 & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyVec4 & in(const char * name, Vec4 T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyVec4(name);
		p->getter = [=](ComponentBase * comp) -> Vec4 & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const Vec4 & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyString & in(const char * name, std::string T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyString(name);
		p->getter = [=](ComponentBase * comp) -> std::string & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const std::string & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyAngleAxis & in(const char * name, AngleAxis T::* member)
	{
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyAngleAxis(name);
		p->getter = [=](ComponentBase * comp) -> AngleAxis & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const AngleAxis & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
	
	ComponentPropertyResourcePtr & in(const char * name, ResourcePtr T::* member)
	{
		// todo : add reflection type for ResourcePtr
		
		add(name, member);
		
		//
		
		auto p = new ComponentPropertyResourcePtr(name);
		p->getter = [=](ComponentBase * comp) -> ResourcePtr & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const ResourcePtr & s) { static_cast<T*>(comp)->*member = s; };
		
		_properties.push_back(p);
		
		return *p;
	}
};
