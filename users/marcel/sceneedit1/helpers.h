#pragma once

#include "reflection.h"
#include <typeindex> // todo : remove and replace with opaque type wrapping type index or type hash
#include <vector>

struct ComponentMgrBase;
struct ComponentSet;
struct ComponentTypeBase;

void registerBuiltinTypes();

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);
void registerComponentTypes();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

void freeComponentsInComponentSet(ComponentSet & componentSet);

extern TypeDB g_typeDB;

extern std::vector<ComponentTypeBase*> g_componentTypes;

//

#include "resource.h" // fixme : move resource helpers somewhere else

extern ResourceDatabase g_resourceDatabase;

template <typename T>
T * findResource(const char * name)
{
	return g_resourceDatabase.find<T>(name);
}
