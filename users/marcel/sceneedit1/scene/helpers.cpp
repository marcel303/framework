#include "componentType.h"
#include "helpers.h"

#include <algorithm> // std::sort
#include <string.h> // strcmp

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

//

#include "template.h"

#include "lineReader.h"
#include "reflection-textio.h"

#include "Log.h"

#include <string.h> // strcmp

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = findComponentType(component_template.typeName.c_str());
		
		if (componentType == nullptr)
		{
			LOG_ERR("unknown component type: %s", component_template.typeName.c_str());
			return false;
		}
		
		ComponentBase * component = componentType->componentMgr->createComponent(componentSet.id);
		
		for (auto & property_template : component_template.properties)
		{
			Member * member = nullptr;
			
			for (auto * member_itr = componentType->members_head; member_itr != nullptr; member_itr = member_itr->next)
				if (strcmp(member_itr->name, property_template.name.c_str()) == 0)
					member = member_itr;
			
			if (member == nullptr)
			{
				LOG_ERR("unknown property: %s", property_template.name.c_str());
				componentType->componentMgr->destroyComponent(componentSet.id);
				return false;
			}
			
			LineReader lineReader(property_template.value_lines, 0, 0);
			
			if (member_fromlines_recursive(typeDB, member, component, lineReader) == false)
			{
				LOG_ERR("failed to deserialize property from text: property=%s, lines=", property_template.name.c_str());
				for (auto & line : property_template.value_lines)
					LOG_ERR("%s", line.c_str());
				componentType->componentMgr->destroyComponent(componentSet.id);
				return false;
			}
		}
		
		componentSet.add(component);
	}
	
	return true;
}
