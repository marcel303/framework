#pragma once

#include "reflection.h"

//

struct ComponentTypeDB;

//

extern TypeDB g_typeDB;

//

void registerBuiltinTypes(TypeDB & typeDB);
void registerComponentTypes(TypeDB & typeDB, ComponentTypeDB & componentTypeDB);
