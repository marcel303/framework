#include "componentType.h"
#include "helpers.h"

#include <algorithm> // std::sort

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

		componentMgr->destroyComponent(componentSet.id);
		component = nullptr;
	}

	componentSet.head = nullptr;
}

void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase *& component)
{
	componentSet.remove(component);
	
	auto * componentType = findComponentType(component->typeIndex());
	Assert(componentType != nullptr);
	
	auto * componentMgr = componentType->componentMgr;
	Assert(componentMgr != nullptr);

	componentMgr->destroyComponent(componentSet.id);
	component = nullptr;
}
