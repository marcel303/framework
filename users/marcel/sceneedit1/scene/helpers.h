#pragma once

#include <typeindex>
#include <vector>

// forward declarations

struct ComponentBase;
struct ComponentMgrBase;
struct ComponentSet;
struct ComponentTypeBase;

struct Member;
struct Type;
struct TypeDB;

//

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);

bool initComponentMgrs();
void shutComponentMgrs();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

void freeComponentsInComponentSet(ComponentSet & componentSet);
void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase *& component);

#include "reflection-textio.h"

extern TypeDB g_typeDB;

extern std::vector<ComponentTypeBase*> g_componentTypes;
