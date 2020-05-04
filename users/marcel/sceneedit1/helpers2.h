#pragma once

void registerBuiltinTypes();
void registerComponentTypes();

//

#include "resource.h" // fixme : move resource helpers somewhere else

extern ResourceDatabase g_resourceDatabase;

template <typename T>
T * findResource(const char * name)
{
	return g_resourceDatabase.find<T>(name);
}
