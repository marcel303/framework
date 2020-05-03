#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "Log.h"
#include "reflection.h"
#include "scene.h"

#define DEFINE_COMPONENT_TYPES
#include "components/cameraComponent.h"
#include "components/lightComponent.h"
#include "components/modelComponent.h"
#include "components/parameterComponent.h"
#include "components/rotateTransformComponent.h"
#include "components/sceneNodeComponent.h"
#include "components/transformComponent.h"
#include "components/vfxgraphComponent.h"

#include <algorithm>
#include <string.h>

// todo : remove component mgr globals
CameraComponentMgr s_cameraComponentMgr;
LightComponentMgr s_lightComponentMgr;
ModelComponentMgr s_modelComponentMgr;
ParameterComponentMgr s_parameterComponentMgr;
RotateTransformComponentMgr s_rotateTransformComponentMgr;
SceneNodeComponentMgr s_sceneNodeComponentMgr;
TransformComponentMgr s_transformComponentMgr;
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
			.addFlag(new ComponentMemberFlag_EditorType_AngleDegrees)
		.add("axis", &AngleAxis::axis)
			.addFlag(new ComponentMemberFlag_EditorType_OrientationVector);
	
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
	registerComponentType(new LightComponentType(), &s_lightComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	registerComponentType(new ParameterComponentType(), &s_parameterComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new SceneNodeComponentType(), &s_sceneNodeComponentMgr);
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	//registerComponentType(new VfxgraphComponentType(), &s_vfxgraphComponentMgr);
}

bool initComponentMgrs()
{
	bool result = true;
	
	for (auto * componentType : g_componentTypes)
	{
		result &= componentType->componentMgr->init();
	}
	
	return result;
}

void shutComponentMgrs()
{
	for (auto * componentType : g_componentTypes)
	{
		componentType->componentMgr->shut();
	}
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

//

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
