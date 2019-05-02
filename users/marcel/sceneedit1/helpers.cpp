#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "Log.h"
#include "reflection.h"
#include "scene.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "rotateTransformComponent.h"
#include "sceneNodeComponent.h"
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
SceneNodeComponentMgr s_sceneNodeComponentMgr;
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
	g_typeDB.addPlain<Vec2>("vec2", kDataType_Float2);
	g_typeDB.addPlain<Vec3>("vec3", kDataType_Float3);
	g_typeDB.addPlain<Vec4>("vec4", kDataType_Float4);
	g_typeDB.addPlain<std::string>("string", kDataType_String);
	g_typeDB.addStructured<AngleAxis>("AngleAxis")
		.add("angle", &AngleAxis::angle)
			.addFlag(new ComponentMemberFlag_EditorType_Angle)
		.add("axis", &AngleAxis::axis)
			.addFlag(new ComponentMemberFlag_EditorType_Axis);
			
	//
	
	g_typeDB.add(std::type_index(typeid(TransformComponent)), new TransformComponentType());
	g_typeDB.addStructured<ParameterDefinition>("parameter")
		.add("type", &ParameterDefinition::type)
		.add("name", &ParameterDefinition::name)
		.add("defaultValue", &ParameterDefinition::defaultValue)
		.add("min", &ParameterDefinition::min)
		.add("max", &ParameterDefinition::max);
}

void registerComponentTypes()
{
	registerComponentType(new CameraComponentType(), &s_cameraComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	registerComponentType(new ParameterComponentType(), &s_parameterComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new SceneNodeComponentType(), &s_sceneNodeComponentMgr);
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
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

void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase * component)
{
	componentSet.remove(component);
	
	auto * componentType = findComponentType(component->typeIndex());
	Assert(componentType != nullptr);
	
	auto * componentMgr = componentType->componentMgr;

	componentMgr->destroyComponent(component);
	Assert(component == nullptr);
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

bool member_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const ComponentJson & j)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				auto * member_interface = static_cast<const Member_VectorInterface*>(member);
				
				auto * vector_type = typeDB.findType(member_interface->vector_type());
				
				if (vector_type == nullptr)
				{
					LOG_ERR("failed to find type for structured type with type name %s", structured_type->typeName);
					result &= false;
				}
				else
				{
					// see if the json contains this member
					
					auto member_json_itr = j.j.find(member->name);
				
					if (member_json_itr != j.j.end())
					{
						auto & member_json = *member_json_itr;
						
						if (member_json.is_array() == false)
						{
							LOG_WRN("json element is not of type array", 0);
						}
						else
						{
							// resize the array
							
							const size_t vector_size = member_json.size();
							
							member_interface->vector_resize(object, vector_size);
							
							// deserialize each element individually
							
							for (size_t i = 0; i < vector_size; ++i)
							{
								void * vector_object = member_interface->vector_access(object, i);
								
								result &= member_fromjson_recursive(typeDB, vector_type, vector_object, member_json.at(i));
							}
						}
					}
				}
			}
			else
			{
				auto * member_scalar = static_cast<const Member_Scalar*>(member);
				
				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				auto * member_object = member_scalar->scalar_access(object);
				
				if (member_type == nullptr)
				{
					LOG_ERR("failed to find type for member %s", member->name);
					result &= false;
				}
				else
				{
					auto member_json_itr = j.j.find(member->name);
					
					if (member_json_itr != j.j.end())
					{
						auto & member_json = *member_json_itr;
						
						result &= member_fromjson_recursive(typeDB, member_type, member_object, member_json);
					}
				}
			}
		}
		
		return result;
	}
	else
	{
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
			
		case kDataType_Float2:
			plain_type->access<Vec2>(object) = j.j.get<Vec2>();
			return true;
			
		case kDataType_Float3:
			plain_type->access<Vec3>(object) = j.j.get<Vec3>();
			return true;
			
		case kDataType_Float4:
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

bool member_fromjson(const TypeDB & typeDB, const Member * member, void * object, const ComponentJson & j)
{
	if (member->isVector) // todo : add support for deserialization of vectors
	{
		LOG_WRN("vector types not supported yet", 0);
		return false;
	}
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type == nullptr)
	{
		LOG_ERR("failed to find type for member %s", member->name);
		return false;
	}
	
	if (member_type->isStructured)
	{
		LOG_ERR("member is a structured type. use member_fromjson_recursive instead", 0);
		return false;
	}
	
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
		
	case kDataType_Float2:
		plain_type->access<Vec2>(member_object) = j.j.value(member->name, Vec2());
		return true;
		
	case kDataType_Float3:
		plain_type->access<Vec3>(member_object) = j.j.value(member->name, Vec3());
		return true;
		
	case kDataType_Float4:
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
}

bool member_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, ComponentJson & j)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				auto * member_interface = static_cast<const Member_VectorInterface*>(member);
				
				auto * vector_type = typeDB.findType(member_interface->vector_type());
				
				if (vector_type == nullptr)
				{
					LOG_ERR("failed to find type for type name %s", structured_type->typeName);
					result &= false;
				}
				else
				{
					const size_t vector_size = member_interface->vector_size(object);
					
					for (size_t i = 0; i < vector_size; ++i)
					{
						auto * vector_object = member_interface->vector_access((void*)object, i);
						
						nlohmann::json elem_json;
						ComponentJson elem_json_wrapped(elem_json);
						
						result &= member_tojson_recursive(typeDB, vector_type, vector_object, elem_json_wrapped);
						
						j.j[member->name].push_back(elem_json);
					}
				}
			}
			else
			{
				auto * member_scalar = static_cast<const Member_Scalar*>(member);
				
				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				auto * member_object = member_scalar->scalar_access(object);
				
				if (member_type == nullptr)
				{
					LOG_ERR("failed to find type for member %s", member->name);
					result &= false;
				}
				else
				{
					auto & member_json = j.j[member->name];
					
					ComponentJson member_json_wrapped(member_json);
					
					result &= member_tojson_recursive(typeDB, member_type, member_object, member_json_wrapped);
				}
			}
		}
		
		return result;
	}
	else
	{
		auto * plain_type = static_cast<const PlainType*>(type);

		switch (plain_type->dataType)
		{
		case kDataType_Bool:
			j.j = plain_type->access<bool>(object);
			return true;
			
		case kDataType_Int:
			j.j = plain_type->access<int>(object);
			return true;
			
		case kDataType_Float:
			j.j = plain_type->access<float>(object);
			return true;
			
		case kDataType_Float2:
			j.j = plain_type->access<Vec2>(object);
			return true;
			
		case kDataType_Float3:
			j.j = plain_type->access<Vec3>(object);
			return true;
			
		case kDataType_Float4:
			j.j = plain_type->access<Vec4>(object);
			return true;
			
		case kDataType_String:
			j.j = plain_type->access<std::string>(object);
			return true;
			
		case kDataType_Other:
			Assert(false);
			break;
		}
	}
	
	return false;
}

bool member_tojson(const TypeDB & typeDB, const Member * member, const void * object, ComponentJson & j)
{
	if (member->isVector) // todo : add support for serialization of vectors
	{
		LOG_WRN("vector types not supported yet", 0);
		return false;
	}
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type == nullptr)
	{
		LOG_ERR("failed to find type for member %s", member->name);
		return false;
	}
	
	if (member_type->isStructured)
	{
		LOG_ERR("member '%s' is a structured type. use member_tojson_recursive instead", member->name);
		return false;
	}
	
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
		
	case kDataType_Float2:
		j.j[member->name] = plain_type->access<Vec2>(member_object);
		return true;
		
	case kDataType_Float3:
		j.j[member->name] = plain_type->access<Vec3>(member_object);
		return true;
		
	case kDataType_Float4:
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
}

#else

bool member_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const ComponentJson & j)
{
	return false;
}

bool member_fromjson(const TypeDB & typeDB, const Member * member, void * object, const ComponentJson & j)
{
	return false;
}

bool member_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, ComponentJson & j)
{
	return false;
}

bool member_tojson(const TypeDB & typeDB, const Member * member, const void * object, ComponentJson & j)
{
	return false;
}

#endif

// member <-> text serialization

// todo : remove. belongs to -textio
bool member_fromtext(const TypeDB & typeDB, const Member * member, void * object, const char * text)
{
	if (member->isVector) // todo : add support for deserialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type == nullptr)
	{
		LOG_ERR("failed to find type for member %s", member->name);
		return false;
	}
	
	if (member_type->isStructured) // todo : add support for deserialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);
	
	return plain_type_fromtext(plain_type, member_object, text);
}


// todo : remove. belongs to -textio
bool member_totext(const TypeDB & typeDB, const Member * member, const void * object, std::string & out_text)
{
	if (member->isVector) // todo : add support for serialization of vectors
		return false;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(object);
	
	if (member_type == nullptr)
	{
		LOG_ERR("failed to find type for member %s", member->name);
		return false;
	}
	
	if (member_type->isStructured) // todo : add support for serialization of structured types
		return false;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);
	
	const int text_size = 1024;
	char text[text_size];

	if (plain_type_totext(plain_type, member_object, text, text_size) == false)
		return false;
	
	out_text = text;
	
	return true;
}

//

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
