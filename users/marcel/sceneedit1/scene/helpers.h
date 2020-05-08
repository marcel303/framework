#pragma once

#include <typeindex>
#include <vector>

// forward declarations

struct ComponentBase;
struct ComponentMgrBase;
struct ComponentSet;
struct ComponentTypeBase;

struct Template;

struct Member;
struct Type;
struct TypeDB;

// -- component type helpers

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);

bool initComponentMgrs();
void shutComponentMgrs();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

template <typename T> T * findComponentType()
{
	return findComponentType(std::type_index(typeid(T)));
}

extern std::vector<ComponentTypeBase*> g_componentTypes;

// -- component set helpers

void freeComponentsInComponentSet(ComponentSet & componentSet);
void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase *& component);

// -- template helpers

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet);
