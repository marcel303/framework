#pragma once

struct ComponentTypeDB;
struct TypeDB;

//

void registerBuiltinTypes(TypeDB & typeDB);
void registerComponentTypes(TypeDB & typeDB, ComponentTypeDB & componentTypeDB);
