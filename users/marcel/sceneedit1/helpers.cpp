#include "componentType.h"
#include "helpers.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "transformComponent.h"
#include "vfxgraphComponent.h"

#include <algorithm>

// todo : remove component mgr globals
CameraComponentMgr s_cameraComponentMgr;
extern TransformComponentMgr s_transformComponentMgr;
RotateTransformComponentMgr s_rotateTransformComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;
ParameterComponentMgr s_parameterComponentMgr;
VfxgraphComponentMgr s_vfxgraphComponentMgr;

TypeDB g_typeDB;

std::vector<ComponentTypeBase*> g_componentTypes;

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr)
{
	Assert(componentType->typeName != nullptr);
	Assert(componentType->componentMgr == nullptr);
	componentType->componentMgr = componentMgr;

	g_componentTypes.push_back(componentType);
	
	std::sort(
		g_componentTypes.begin(),
		g_componentTypes.end(),
		[](const ComponentTypeBase * r1, const ComponentTypeBase * r2)
		{
			return r1->tickPriority < r2->tickPriority;
		}
	);
}

void registerBuiltinTypes()
{
	g_typeDB.addPlain<bool>("bool", kDataType_Bool);
	g_typeDB.addPlain<int>("int", kDataType_Int);
	g_typeDB.addPlain<float>("float", kDataType_Float);
	g_typeDB.addPlain<Vec2>("vec2", kDataType_Vec2);
	g_typeDB.addPlain<Vec3>("vec3", kDataType_Vec3);
	g_typeDB.addPlain<Vec4>("vec4", kDataType_Vec4);
	g_typeDB.addPlain<std::string>("string", kDataType_String);
	g_typeDB.addPlain<AngleAxis>("AngleAxis", kDataType_Other);
}

void registerComponentTypes()
{
	registerComponentType(new CameraComponentType(), &s_cameraComponentMgr);
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	registerComponentType(new ParameterComponentType(), &s_parameterComponentMgr);
	registerComponentType(new VfxgraphComponentType(), &s_vfxgraphComponentMgr);
}

static ComponentTypeBase * findComponentType(const std::vector<ComponentTypeBase*> & componentTypes, const char * typeName)
{
	for (auto * componentType : componentTypes)
		if (strcmp(componentType->typeName, typeName) == 0)
			return componentType;
	
	return nullptr;
}

static ComponentTypeBase * findComponentType(const std::vector<ComponentTypeBase*> & componentTypes, const std::type_index & typeIndex)
{
	for (auto * componentType : componentTypes)
		if (componentType->componentMgr->typeIndex() == typeIndex)
			return componentType;
	
	return nullptr;
}

ComponentTypeBase * findComponentType(const char * typeName)
{
	return findComponentType(g_componentTypes, typeName);
}

ComponentTypeBase * findComponentType(const std::type_index & typeIndex)
{
	return findComponentType(g_componentTypes, typeIndex);
}

void freeComponentsInComponentSet(ComponentSet & componentSet)
{
	ComponentBase * next;

	for (auto * component = componentSet.head; component != nullptr; component = next)
	{
		// the component will be removed and next_in_set will become invalid, so we need to fetch it now
		
		next = component->next_in_set;
		
		auto * componentType = findComponentType(component->typeIndex());
		Assert(componentType != nullptr);
		
		auto * componentMgr = componentType->componentMgr;

		componentMgr->destroyComponent(component);
		Assert(component == nullptr);
	}

	componentSet.head = nullptr;
}

//

#include "Parse.h"

void splitString(const std::string & str, std::vector<std::string> & result);

bool member_fromtext(const TypeDB & typeDB, const Member * member, void * object, const char * text)
{
	if (member->isVector) // todo : add support for deserialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type->isStructured) // todo : add support for deserialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);
	
// todo : I definitely need better parse function. ones which return a success code

	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		plain_type->access<bool>(member_object) = Parse::Bool(text);
		break;
		
	case kDataType_Int:
		plain_type->access<int>(member_object) = Parse::Int32(text);
		break;
		
	case kDataType_Float:
		plain_type->access<float>(member_object) = Parse::Float(text);
		break;
		
	case kDataType_Vec2:
		{
			std::vector<std::string> parts;
			splitString(text, parts);
			
			if (parts.size() != 2)
				return false;
			
			plain_type->access<Vec2>(member_object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]));
		}
		break;
		
	case kDataType_Vec3:
		{
			std::vector<std::string> parts;
			splitString(text, parts);
			
			if (parts.size() != 3)
				return false;
			
			plain_type->access<Vec3>(member_object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]),
				Parse::Float(parts[2]));
		}
		break;
		
	case kDataType_Vec4:
		{
			std::vector<std::string> parts;
			splitString(text, parts);
			
			if (parts.size() != 4)
				return false;
			
			plain_type->access<Vec4>(member_object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]),
				Parse::Float(parts[2]),
				Parse::Float(parts[3]));
		}
		break;
		
	case kDataType_String:
		plain_type->access<std::string>(member_object) = text;
		break;
		
	case kDataType_Other:
		Assert(false);
		return false;
	}
	
	return true;
}

//

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
