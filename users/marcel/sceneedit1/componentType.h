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

struct ComponentTypeBase;

struct ResourcePtr;

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
	int tickPriority = kComponentPriority_Default;
	
	ComponentMgrBase * componentMgr = nullptr;
	
	ComponentTypeBase(const char * in_typeName)
		: StructuredType(in_typeName)
	{
	}
};

struct ComponentMemberFlag_IntLimits : MemberFlag<ComponentMemberFlag_IntLimits>
{
	int min;
	int max;
};

struct ComponentMemberFlag_FloatLimits : MemberFlag<ComponentMemberFlag_FloatLimits>
{
	float min;
	float max;
};

struct ComponentMemberFlag_FloatEditorCurveExponential : MemberFlag<ComponentMemberFlag_FloatEditorCurveExponential>
{
	float exponential;
};

template <typename T>
struct ComponentMemberAdder
{
	Member * member;
	
	ComponentMemberAdder(Member * in_member)
		: member(in_member)
	{
	}
	
	ComponentMemberAdder & setLimits(const T min, const T max)
	{
		if (std::is_same<T, int>())
		{
			ComponentMemberFlag_IntLimits * limits = new ComponentMemberFlag_IntLimits();
			limits->min = min;
			limits->max = max;
		
			member->addFlag(limits);
		}
		else if (std::is_same<T, float>())
		{
			ComponentMemberFlag_FloatLimits * limits = new ComponentMemberFlag_FloatLimits();
			limits->min = min;
			limits->max = max;
		
			member->addFlag(limits);
		}
		
		return *this;
	}
	
	ComponentMemberAdder & setEditingCurveExponential(const T value)
	{
		if (std::is_same<T, float>())
		{
			ComponentMemberFlag_FloatEditorCurveExponential * curveExponential = new ComponentMemberFlag_FloatEditorCurveExponential();
			curveExponential->exponential = value;
		
			member->addFlag(curveExponential);
		}
		
		return *this;
	}
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	ComponentType(const char * in_typeName)
		: ComponentTypeBase(in_typeName)
	{
	}
	
	void in(const char * name, bool T::* member)
	{
		add(name, member);
	}
	
	ComponentMemberAdder<int> in(const char * name, int T::* member)
	{
		add(name, member);
		
		return ComponentMemberAdder<int>(members_tail);
	}
	
	ComponentMemberAdder<float> in(const char * name, float T::* member)
	{
		add(name, member);
		
		return ComponentMemberAdder<float>(members_tail);
	}
	
	void in(const char * name, Vec2 T::* member)
	{
		add(name, member);
	}
	
	void in(const char * name, Vec3 T::* member)
	{
		add(name, member);
	}
	
	void in(const char * name, Vec4 T::* member)
	{
		add(name, member);
	}
	
	void in(const char * name, std::string T::* member)
	{
		add(name, member);
	}
	
	void in(const char * name, AngleAxis T::* member)
	{
		add(name, member);
	}
	
	void in(const char * name, ResourcePtr T::* member)
	{
		// todo : add reflection type for ResourcePtr
		
		add(name, member);
	}
};
