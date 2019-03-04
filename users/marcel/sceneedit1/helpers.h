#pragma once

#include "reflection.h"
#include <string> // member_totext
#include <typeindex> // todo : remove and replace with opaque type wrapping type index or type hash
#include <vector>

struct ComponentJson;
struct ComponentMgrBase;
struct ComponentSet;
struct ComponentTypeBase;

struct LineReader;
struct LineWriter;

void registerBuiltinTypes();

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr);
void registerComponentTypes();

ComponentTypeBase * findComponentType(const char * typeName);
ComponentTypeBase * findComponentType(const std::type_index & typeIndex);

void freeComponentsInComponentSet(ComponentSet & componentSet);

bool member_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const ComponentJson & j);
bool member_fromjson(const TypeDB & typeDB, const Member * member, void * object, const ComponentJson & j);
bool member_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, ComponentJson & j);
bool member_tojson(const TypeDB & typeDB, const Member * member, const void * object, ComponentJson & j);

bool object_fromtext(const TypeDB & typeDB, const PlainType * plain_type, void * object, const char * text);
bool member_fromtext(const TypeDB & typeDB, const Member * member, void * object, const char * text);
bool member_totext(const TypeDB & typeDB, const Member * member, const void * object, std::string & out_text);

bool member_tolines_recursive(const TypeDB & typeDB, const StructuredType * structured_type, const void * object, const Member * member, LineWriter & line_writer, const int currentIndent);

bool object_fromlines_recursive(
	const TypeDB & typeDB, const Type * type, void * object,
	LineReader & line_reader);
bool member_fromlines_recursive(
	const TypeDB & typeDB, const Member * member, void * object,
	LineReader & line_reader);

bool object_tolines_recursive(
	const TypeDB & typeDB, const Type * type, const void * object,
	LineWriter & line_Writer, const int currentIndent);
bool member_tolines_recursive(
	const TypeDB & typeDB, const Member * member, const void * object,
	LineWriter & line_Writer, const int currentIndent);

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
