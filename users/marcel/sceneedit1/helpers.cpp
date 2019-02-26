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

std::vector<ComponentTypeBase*> g_componentTypes;

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr)
{
	Assert(componentType->typeName.empty() == false);
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
		if (componentType->typeName == typeName)
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

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
