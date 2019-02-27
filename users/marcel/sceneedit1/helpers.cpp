#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "Log.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "transformComponent.h"
#include "vfxgraphComponent.h"

#include <algorithm>
#include <string.h>

// todo : remove component mgr globals
CameraComponentMgr s_cameraComponentMgr;
extern TransformComponentMgr s_transformComponentMgr;
RotateTransformComponentMgr s_rotateTransformComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;
ParameterComponentMgr s_parameterComponentMgr;
//VfxgraphComponentMgr s_vfxgraphComponentMgr;

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
	g_typeDB.addStructured<AngleAxis>("AngleAxis")
		.add("angle", &AngleAxis::angle)
			.addFlag(new ComponentMemberFlag_EditorType_Angle)
		.add("axis", &AngleAxis::axis)
			.addFlag(new ComponentMemberFlag_EditorType_Axis);
}

void registerComponentTypes()
{
	registerComponentType(new CameraComponentType(), &s_cameraComponentMgr);
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	registerComponentType(new ParameterComponentType(), &s_parameterComponentMgr);
	//registerComponentType(new VfxgraphComponentType(), &s_vfxgraphComponentMgr);
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

// member <-> json serialization

#if ENABLE_COMPONENT_JSON

#include "componentJson.h"

static void to_json(nlohmann::json & j, const Vec2 & v)
{
	j = { v[0], v[1] };
}

static void to_json(nlohmann::json & j, const Vec3 & v)
{
	j = { v[0], v[1], v[2] };
}

static void to_json(nlohmann::json & j, const Vec4 & v)
{
	j = { v[0], v[1], v[2], v[3] };
}

static void from_json(const nlohmann::json & j, Vec2 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
}

static void from_json(const nlohmann::json & j, Vec3 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
}

static void from_json(const nlohmann::json & j, Vec4 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
	v[3] = j[3].get<float>();
}

#endif

#if ENABLE_COMPONENT_JSON
bool member_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const ComponentJson & j, const Member * in_member)
{
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		bool result = true;
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				//result &= false; // todo : support vector types
				//Assert(false);
				LOG_ERR("todo : support vector types in member_fromjson_recursive!", 0);
			}
			else
			{
				auto * member_scalar = static_cast<const Member_Scalar*>(member);
				
				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				auto * member_object = member_scalar->scalar_access(object);
				
				auto member_json_itr = j.j.find(member->name);
				
				if (member_json_itr != j.j.end())
				{
					auto & member_json = *member_json_itr;
					
					result &= member_fromjson_recursive(typeDB, member_type, member_object, member_json, member);
				}
			}
		}
		
		return true;
	}
	else
	{
		Assert(in_member != nullptr);
		
		auto * plain_type = static_cast<const PlainType*>(type);
		
		switch (plain_type->dataType)
		{
		case kDataType_Bool:
			plain_type->access<bool>(object) = j.j.get<bool>();
			return true;
			
		case kDataType_Int:
			plain_type->access<int>(object) = j.j.get<int>();
			return true;
			
		case kDataType_Float:
			plain_type->access<float>(object) = j.j.get<float>();
			return true;
			
		case kDataType_Vec2:
			plain_type->access<Vec2>(object) = j.j.get<Vec2>();
			return true;
			
		case kDataType_Vec3:
			plain_type->access<Vec3>(object) = j.j.get<Vec3>();
			return true;
			
		case kDataType_Vec4:
			plain_type->access<Vec4>(object) = j.j.get<Vec4>();
			return true;
			
		case kDataType_String:
			plain_type->access<std::string>(object) = j.j.get<std::string>();
			return true;
			
		case kDataType_Other:
			Assert(false);
			break;
		}
	}
	
	return false;
}
#endif

bool member_fromjson(const TypeDB & typeDB, const Member * member, void * object, const ComponentJson & j)
{
#if ENABLE_COMPONENT_JSON
	if (member->isVector) // todo : add support for deserialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type->isStructured) // todo : add support for deserialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);
	
	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		plain_type->access<bool>(member_object) = j.j.value(member->name, false);
		return true;
		
	case kDataType_Int:
		plain_type->access<int>(member_object) = j.j.value(member->name, 0);
		return true;
		
	case kDataType_Float:
		plain_type->access<float>(member_object) = j.j.value(member->name, 0.f);
		return true;
		
	case kDataType_Vec2:
		plain_type->access<Vec2>(member_object) = j.j.value(member->name, Vec2());
		return true;
		
	case kDataType_Vec3:
		plain_type->access<Vec3>(member_object) = j.j.value(member->name, Vec3());
		return true;
		
	case kDataType_Vec4:
		plain_type->access<Vec4>(member_object) = j.j.value(member->name, Vec4());
		return true;
		
	case kDataType_String:
		plain_type->access<std::string>(member_object) = j.j.value(member->name, std::string());
		return true;
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
#else
	return false;
#endif
}

bool member_tojson(const TypeDB & typeDB, const Member * member, const void * object, ComponentJson & j)
{
#if ENABLE_COMPONENT_JSON
	if (member->isVector) // todo : add support for serialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type->isStructured) // todo : add support for serialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);

	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		j.j[member->name] = plain_type->access<bool>(member_object);
		return true;
		
	case kDataType_Int:
		j.j[member->name] = plain_type->access<int>(member_object);
		return true;
		
	case kDataType_Float:
		j.j[member->name] = plain_type->access<float>(member_object);
		return true;
		
	case kDataType_Vec2:
		j.j[member->name] = plain_type->access<Vec2>(member_object);
		return true;
		
	case kDataType_Vec3:
		j.j[member->name] = plain_type->access<Vec3>(member_object);
		return true;
		
	case kDataType_Vec4:
		j.j[member->name] = plain_type->access<Vec4>(member_object);
		return true;
		
	case kDataType_String:
		j.j[member->name] = plain_type->access<std::string>(member_object);
		return true;
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
#else
	return false;
#endif
}

// member <-> text serialization

#include "Parse.h"
#include "StringEx.h"

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
		return true;
		
	case kDataType_Int:
		plain_type->access<int>(member_object) = Parse::Int32(text);
		return true;
		
	case kDataType_Float:
		plain_type->access<float>(member_object) = Parse::Float(text);
		return true;
		
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
		return true;
		
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
		return true;
		
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
		return true;
		
	case kDataType_String:
		plain_type->access<std::string>(member_object) = text;
		return true;
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

bool member_totext(const TypeDB & typeDB, const Member * member, const void * object, std::string & out_text)
{
	if (member->isVector) // todo : add support for serialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type->isStructured) // todo : add support for serialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);

	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		out_text = plain_type->access<bool>(member_object) ? "true" : "false";
		return true;
		
	case kDataType_Int:
		out_text = String::FormatC("%d", plain_type->access<int>(member_object));
		return true;
		
	case kDataType_Float:
		// todo : need a better float to string conversion function
		out_text = String::FormatC("%f", plain_type->access<float>(member_object));
		return true;
		
	case kDataType_Vec2:
		{
			// todo : need a better float to string conversion function
		
			auto & value = plain_type->access<Vec2>(member_object);
			
			out_text = String::FormatC("%f %f",
				value[0],
				value[1]);
		}
		return true;
		
	case kDataType_Vec3:
		{
			// todo : need a better float to string conversion function
		
			auto & value = plain_type->access<Vec3>(member_object);
			
			out_text = String::FormatC("%f %f %f",
				value[0],
				value[1],
				value[2]);
		}
		return true;
		
	case kDataType_Vec4:
		{
			// todo : need a better float to string conversion function
		
			auto & value = plain_type->access<Vec4>(member_object);
			
			out_text = String::FormatC("%f %f %f %f",
				value[0],
				value[1],
				value[2],
				value[3]);
		}
		return true;
		
	case kDataType_String:
		out_text = plain_type->access<std::string>(member_object);
		return true;
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

//

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
