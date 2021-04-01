#pragma once

#include "reflection.h"

//

struct ComponentTypeDB;

//

extern TypeDB g_typeDB;

//

void registerBuiltinTypes(TypeDB & typeDB);
void registerComponentTypes(TypeDB & typeDB, ComponentTypeDB & componentTypeDB);

//

#include "resource.h" // fixme : move resource helpers somewhere else

extern ResourceDatabase g_resourceDatabase;

template <typename T>
T * findResource(const char * name)
{
	return g_resourceDatabase.find<T>(name);
}
