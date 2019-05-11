#pragma once

#include <string> // member_totext
#include <typeindex> // todo : remove and replace with opaque type wrapping type index or type hash
#include <vector>

// forward declarations

struct ComponentBase;
struct ComponentJson;
struct ComponentMgrBase;
struct ComponentSet;
struct ComponentTypeBase;

struct Member;
struct Type;
struct TypeDB;

//

void registerBuiltinTypes();

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);
void registerComponentTypes();

bool initComponentMgrs();
void shutComponentMgrs();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

void freeComponentsInComponentSet(ComponentSet & componentSet);
void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase * component);

bool member_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const ComponentJson & j);
bool member_fromjson(const TypeDB & typeDB, const Member * member, void * object, const ComponentJson & j);
bool member_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, ComponentJson & j);
bool member_tojson(const TypeDB & typeDB, const Member * member, const void * object, ComponentJson & j);

bool member_fromtext(const TypeDB & typeDB, const Member * member, void * object, const char * text);
bool member_totext(const TypeDB & typeDB, const Member * member, const void * object, std::string & out_text);

#include "reflection-textio.h"

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
