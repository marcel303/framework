#include "componentType.h"
#include "helpers.h"

#define DEFINE_COMPONENT_TYPES
#include "modelComponent.h"
#include "transformComponent.h"

// todo : remove component mgr globals
extern TransformComponentMgr s_transformComponentMgr;
extern RotateTransformComponentMgr s_rotateTransformComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;

std::vector<ComponentTypeBase*> g_componentTypes;

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr)
{
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
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
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
