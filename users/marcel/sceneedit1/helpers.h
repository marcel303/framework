#pragma once

#include <typeindex> // todo : remove and replace with opaque type wrapping type index or type hash
#include <vector>

struct ComponentMgrBase;
struct ComponentTypeBase;

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);
void registerComponentTypes();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

extern std::vector<ComponentTypeBase*> g_componentTypes;
