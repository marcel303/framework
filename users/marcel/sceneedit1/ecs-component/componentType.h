#pragma once

#include "component.h"
#include "reflection.h" // StructuredType
#include <type_traits>

// component types

enum ComponentPriority
{
	kComponentPriority_Parameter = 100,
	kComponentPriority_Transform = 200,
	kComponentPriority_Camera = 400,
	kComponentPriority_Light = 500,
	kComponentPriority_Default = 1000
};

struct ComponentTypeBase : StructuredType
{
	int tickPriority = kComponentPriority_Default;
	
	ComponentMgrBase * componentMgr = nullptr;
	
	ComponentTypeBase(const char * in_typeName, ComponentMgrBase * in_componentMgr);
};

struct ComponentMemberFlag_Hidden : MemberFlag<ComponentMemberFlag_Hidden>
{
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

struct ComponentMemberFlag_EditorType_ColorSrgb : MemberFlag<ComponentMemberFlag_EditorType_ColorSrgb>
{
};

struct ComponentMemberFlag_EditorType_ColorLinear : MemberFlag<ComponentMemberFlag_EditorType_ColorLinear>
{
};

struct ComponentMemberFlag_EditorType_AngleDegrees : MemberFlag<ComponentMemberFlag_EditorType_AngleDegrees>
{
};

struct ComponentMemberFlag_EditorType_OrientationVector : MemberFlag<ComponentMemberFlag_EditorType_OrientationVector>
{
};

struct ComponentMemberFlag_EditorType_FilePath : MemberFlag<ComponentMemberFlag_EditorType_FilePath>
{
};

struct ComponentMemberAdder_Int
{
	Member * member;
	
	ComponentMemberAdder_Int(Member * in_member);
	
	ComponentMemberAdder_Int & limits(const int min, const int max);
};

struct ComponentMemberAdder_Float
{
	Member * member;
	
	ComponentMemberAdder_Float(Member * in_member);
	
	ComponentMemberAdder_Float & limits(const float min, const float max);
	ComponentMemberAdder_Float & editingCurveExponential(const float value);
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	ComponentType(const char * in_typeName, ComponentMgrBase * in_componentMgr)
		: ComponentTypeBase(in_typeName, in_componentMgr)
	{
	}
	
	// note : we duplicate some adders inherited from StructuredType, so they are
	//        treated with equal priority. if we don't, only our own specific
	//        versions would be considered by the compiler as valid overloads,
	//        causing compile errors
	
	template <typename C, typename MT>
	StructuredType & add(const char * name, std::vector<MT> C::* C_member)
	{
		return ComponentTypeBase::add(name, C_member);
	}
	
	template <typename C, typename MT>
	StructuredType & add(const char * name, MT C::* C_member)
	{
		return ComponentTypeBase::add(name, C_member);
	}
	
	ComponentMemberAdder_Int add(const char * name, int T::* member)
	{
		ComponentTypeBase::add(name, member);
		
		return ComponentMemberAdder_Int(members_tail);
	}
	
	ComponentMemberAdder_Float add(const char * name, float T::* member)
	{
		ComponentTypeBase::add(name, member);
		
		return ComponentMemberAdder_Float(members_tail);
	}
};
