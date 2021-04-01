#include "componentType.h"
#include "componentTypeDB.h"

#include <algorithm> // std::sort
#include <string.h> // strcmp

ComponentTypeDB g_componentTypeDB;

void ComponentTypeDB::registerComponentType(ComponentTypeBase * componentType)
{
	Assert(componentType->typeName != nullptr);
	Assert(componentType->componentMgr != nullptr);

	componentTypes.push_back(componentType);
	
	std::sort(
		componentTypes.begin(),
		componentTypes.end(),
		[](const ComponentTypeBase * r1, const ComponentTypeBase * r2)
		{
			return r1->tickPriority < r2->tickPriority;
		}
	);
}

bool ComponentTypeDB::initComponentMgrs()
{
	bool result = true;
	
	for (auto * componentType : componentTypes)
	{
		result &= componentType->componentMgr->init();
	}
	
	return result;
}

void ComponentTypeDB::shutComponentMgrs()
{
	for (auto * componentType : componentTypes)
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

ComponentTypeBase * ComponentTypeDB::findComponentType(const char * typeName) const
{
	return ::findComponentType(componentTypes, typeName);
}

ComponentTypeBase * ComponentTypeDB::findComponentType(const std::type_index & typeIndex) const
{
	return ::findComponentType(componentTypes, typeIndex);
}
